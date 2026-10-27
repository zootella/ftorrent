import os
import json
import time
import signal
import socket
import threading
import http.server
import ipaddress
import libtorrent as lt

# The DHT bootstrap node at dht.ftorrent.com, and this file is its whole program. It runs one libtorrent session with the mainline DHT on and nothing else: no torrents, no peers, no port mapping. libtorrent answers the DHT's messages on its own threads, and this loop does the three things libtorrent leaves to the program around it: it asks each routing table how many nodes it holds, it answers that on an internal port for the gauge, and it saves the DHT's state so a restart picks up where the last run left off.
#
# libtorrent runs one DHT node, with a routing table of its own, from each address it listens on that reaches beyond the machine. Inside the container that's one address in each family, so the session runs two tables, one on IPv4 and one on IPv6. libtorrent's own counter, dht.dht_nodes, adds them together, which is all a program built on it can usually report; this one asks each table for its count instead, the way the desktop client's Connection tab does, so the dashboard can show IPv4 and IPv6 apart.
#
# The node has no settings file. Everything it's told is below, and the one thing it writes is its state, in the config volume.

port = 51420#the DHT's UDP port, published by the host on both families; the guide says why this number
health_port = 8080#the internal port the gauge reads each table's count from, never published to the host
config = os.environ.get('DHT_CONFIG', '/config')#the one writable folder, a volume, so the state outlasts the container
state_path = os.path.join(config, 'dht.state')
count_every = 10#seconds between asking each table for its count; the gauge reads once a minute
save_every = 10*60#seconds between saves while running, so a crash or a kill loses at most this much; a stop always saves

bootstrap_nodes = [#where a cold start learns its first nodes: the same list the desktop client's engine starts from, in desktop/engine/engine.py, less dht.ftorrent.com, which is this node, and a node can't learn the network from itself
	'dht.libtorrent.org:25401',#run by libtorrent's author, and libtorrent's own default
	'dht.transmissionbt.com:6881',#run by the Transmission project
]

settings = {
	'listen_interfaces': f'0.0.0.0:{port},[::]:{port}',#every address in both families at one port, each with a UDP socket for the DHT; the TCP sockets beside them take nothing, since the host publishes only UDP
	'enable_dht': True,
	'dht_bootstrap_nodes': ','.join(bootstrap_nodes),
	'enable_lsd': False,#local service discovery multicasts torrents to the local network, and this node has neither
	'enable_upnp': False,#the host publishes the port, so nothing here asks a router for anything
	'enable_natpmp': False,
	'alert_mask': lt.alert_category.dht | lt.alert_category.error,#the DHT's alerts, which carry each table's count, and errors; the others aren't wanted
}

flag = lt.save_state_flags_t.save_dht_state#the part of the session's state this node keeps: each DHT node's ID and the nodes it knew

def say(line):#one line to the container's log; the node logs when it starts and stops, and nothing about the messages it answers
	print(f'dht: {line}', flush=True)

def load():#the session's starting point: the saved state when there is one, so each table starts from the nodes it knew, or a cold start from the bootstrap nodes
	params = lt.session_params()
	try:
		with open(state_path, 'rb') as file:
			saved = file.read()
		params = lt.read_session_params(saved, flag)
		say(f'loaded {len(saved)} bytes of saved state, a warm start')
	except FileNotFoundError:
		say('no saved state, a cold start from the bootstrap nodes')
	except Exception as e:#a state file that won't read is set aside rather than stopping the node, which starts cold, and the next save replaces it
		say(f'saved state unreadable, a cold start: {type(e).__name__}: {e}')
	params.settings = settings
	return params

def save(session):#the DHT's state, written beside the old copy and renamed over it, so a crash mid-write leaves the last good one; a save that fails says why in the log and the node keeps running, since a node without its state is still a node
	data = lt.write_session_params_buf(session.session_state(flag), flag)
	temporary = state_path + '.tmp'
	try:
		with open(temporary, 'wb') as file:
			file.write(data)
			file.flush()
			os.fsync(file.fileno())
		os.replace(temporary, state_path)
	except OSError as e:#most likely a config folder the container's user can't write
		say(f'could not save state: {e}')
		return 0
	return len(data)

def tables(session):#each DHT node the session runs, as (address, ID in hex); the state the session would save is the one place the Python binding gives both
	state = lt.write_session_params(session.session_state(flag), flag)
	found = []
	for entry in state.get(b'dht state', {}).get(b'node-id', []):#each entry is the node's 20-byte ID followed by its local address, 4 bytes for IPv4 and 16 for IPv6
		found.append((str(ipaddress.ip_address(entry[20:])), entry[:20].hex()))
	return found

report = b'{"tables":[]}'#the health answer, rebuilt by the loop below and read by the server's thread; replacing it whole is one step, so a reader never meets half of one

class Health(http.server.BaseHTTPRequestHandler):#GET / answers the report, and that's all it does: it can't change the node, so it needs no password, and the port is never published
	timeout = 5#seconds a client gets to send its request, so one that stalls can't hold the server
	def do_GET(self):
		if self.path != '/':
			self.send_error(404); return
		body = report
		self.send_response(200)
		self.send_header('Content-Type', 'application/json')
		self.send_header('Content-Length', str(len(body)))
		self.end_headers()
		self.wfile.write(body)
	def log_message(self, format, *args):#no access log
		pass

class DualStack(http.server.HTTPServer):#one socket for both families, since Docker's DNS can answer the gauge with either address
	address_family = socket.AF_INET6
	def server_bind(self):
		self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
		super().server_bind()

def serve():#the health port, on a thread of its own; on a network without IPv6, IPv4 alone
	try:
		server = DualStack(('::', health_port), Health)
	except OSError:
		server = http.server.HTTPServer(('0.0.0.0', health_port), Health)
	threading.Thread(target=server.serve_forever, daemon=True).start()

stopping = threading.Event()#set by docker stop's SIGTERM, or by ctrl+c when run by hand

def main():
	signal.signal(signal.SIGTERM, lambda number, frame: stopping.set())#as the container's first process, the program gets no default handling of SIGTERM, so without this a stop would wait out the grace period and kill it unsaved
	signal.signal(signal.SIGINT, lambda number, frame: stopping.set())
	global report
	session = lt.session(load())
	say(f'libtorrent {lt.version}, listening on port {port}, health on port {health_port}')
	serve()
	counts = {}#each table's latest count, by ID
	saved = time.monotonic()
	while not stopping.is_set():
		found = tables(session)
		for address, nid in found:
			session.dht_live_nodes(lt.sha1_hash(bytes.fromhex(nid)))#each table's count arrives as an alert, read below
		until = time.monotonic() + count_every
		while time.monotonic() < until and not stopping.is_set():
			session.wait_for_alert(250)#up to a quarter second, so a stop is noticed within one
			for alert in session.pop_alerts():#every alert is taken, so the queue never fills, and all but the counts are let go unread
				if isinstance(alert, lt.dht_live_nodes_alert):
					counts[str(alert.node_id)] = alert.num_nodes
		counts = {nid: counts[nid] for address, nid in found if nid in counts}#a table whose ID changed or whose address went away is dropped
		report = json.dumps({
			'libtorrent': lt.version,
			'tables': [{'address': address, 'id': nid, 'nodes': counts.get(nid)} for address, nid in found],#nodes is null until a table's first count arrives
		}, separators=(',', ':')).encode('utf-8')
		if time.monotonic() - saved >= save_every:
			save(session)
			saved = time.monotonic()
	written = save(session)
	say(f'stopping, saved {written} bytes of state' if written else 'stopping without saving')
	del session#letting go of the session closes its sockets

if __name__ == '__main__':
	main()
