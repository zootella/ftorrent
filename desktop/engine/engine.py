import os
import sys
import json
import re
import errno
import secrets
import ipaddress
import platform
import tempfile
import libtorrent as lt

# The engine is the process that holds libtorrent, and this file is its whole program. It starts, answers lines of newline-delimited JSON on its pipes, and exits when told to or when its pipe closes. It opens no sockets until it's asked for a session, which the desktop client's Connection tab does today, to see what the network does to a client that listens, joins the DHT, and sits in a swarm; the torrent work arrives later on top of exactly this loop.
#
# The loop has no threads, so the engine does work only when a line arrives, and JavaScript sets the pace. libtorrent works on its own threads whether or not anyone asks, and reports what happens as alerts that wait in a queue inside the session; JavaScript sends poll on an interval, and the engine answers with every alert waiting, each as a line. The queue holds 2,000 alerts by default and drops the rest, so a session nobody polls loses news, which is why JavaScript keeps polling for as long as a session runs.
#
# The engine is the far end of the one road between JavaScript and libtorrent. JavaScript builds each command as a line of JSON, the app's Rust passes it down the engine's stdin without reading it, and every line the engine writes to stdout goes back up the same way, unread, until JavaScript takes it. So this file is where JSON meets libtorrent: the dispatch table at the bottom has one entry per command, each entry translates what it's given into libtorrent's own terms and back, and the table is also the list of everything JavaScript can ask the engine to do, on purpose, rather than a way to call any libtorrent method by name. Using a libtorrent call ftorrent hasn't used before means one new entry here and the JavaScript that sends it, and nothing in between.
#
# Two rules about the pipes, both because the other end is Rust reading lines. Every message is one line of JSON ending in a newline and flushed at once, so nothing sits in a buffer while the app waits. And a line that is not JSON, or JSON that is not a command the engine knows, gets an error line back rather than a crash, so a mistake on one side never takes the other side down.

# The centralized servers the engine reaches on its own, and the one place they are written. Two lists, each in the same order: ours first, then the well-known providers or the servers universal to BitTorrent. The trackers a new torrent starts from are a setting instead, in desktop/src/ftorrent.toml. The Names and Numbers document on docs.ftorrent.com lists every entry and who runs it; a change here is a change there.
centralized_servers = {
	'stun': [#the STUN server WebTorrent uses to learn its own public address for WebRTC; libtorrent takes exactly one, so the first entry is the one used, and the rest are the order to fall back through once the engine can test them, and the choices a settings panel will offer
		'stun.ftorrent.com:3478',#ours
		'stun.cloudflare.com:3478',#Cloudflare's public STUN
		'stun.l.google.com:19302',#Google's public STUN, libtorrent's own default
	],
	'dht': [#the bootstrap nodes a fresh DHT routing table starts from; libtorrent takes them all. The engine doesn't save its routing table yet, so every session starts from these; once it does, the saved table makes them unnecessary after the first start. The DHT bootstrap node starts from the same list less its own entry, in open/dht/dht.py
		'dht.ftorrent.com:51420',#ours
		'dht.libtorrent.org:25401',#run by libtorrent's author, and libtorrent's own default
		'dht.transmissionbt.com:6881',#run by the Transmission project
	],
}

name = ''#the product's name, and
version = ''#the app's version, both of which arrive in the init line because the engine cannot read tauri.conf.json, the one place they're written
paths = {}#where the engine will keep what it keeps: the data folder and the state file, which also arrive in init because the app works them out and the engine never does
folders = []#the download folders, as absolute paths; these arrive later, in a folders line, because they are a setting and JavaScript reads the settings after the engine has started

def client_name():#how the client names itself on the wire: brand first, then lineage, the way a browser's user agent reads, so a narrow column shows the brand and a wide one shows the whole truth
	return f'{name}/{version} libtorrent/{lt.version}'

def fingerprint():#the eight characters at the front of every peer id, like -FF0100-: FF is ftorrent's own client code, unused in every table of codes when we chose it and to be registered in each; the digits are the version, one character per part, the same way libtorrent makes its own -LT2110-
	parts = [int(p) if p.isdigit() else 0 for p in (version.split('.') + ['0', '0', '0'])[:3]]#major, minor, patch, with a missing or odd part reading as zero rather than stopping the engine
	return lt.generate_fingerprint('FF', parts[0], parts[1], parts[2], 0)

def session_settings():#the settings the engine will hand libtorrent when it creates its session; the lists above flow into libtorrent's own keys here and nowhere else
	return {
		'webtorrent_stun_server': centralized_servers['stun'][0],
		'dht_bootstrap_nodes': ','.join(centralized_servers['dht']),
		'user_agent': client_name(),#the HTTP User-Agent trackers and web seeds see
		'handshake_client_version': client_name(),#the free-text name in the extension handshake, which libtorrent-based clients show verbatim in their peer lists; libtorrent would fall back to user_agent for this, and setting it says so out loud
		'peer_fingerprint': fingerprint(),#the client code and version at the front of the peer id, which clients reading only the peer id look up in a table; the DHT is separate, where libtorrent writes its own LT and version into every message from no setting at all
	}

alert_mask = lt.alert_category.error | lt.alert_category.status | lt.alert_category.port_mapping | lt.alert_category.port_mapping_log | lt.alert_category.dht | lt.alert_category.peer | lt.alert_category.connect | lt.alert_category.tracker#the kinds of alert the session posts: errors, status changes, port mapping, the DHT, peers, connections, and trackers, and of libtorrent's logs only the port mappers', a few lines for each request a mapper sends, since a router that never answers posts no other alert; the rest would be many lines a second

session = None#the one libtorrent session, once JavaScript asks for one, and None again once it's stopped
check_session = None#the session the Trackers tab checks trackers from, made at its first check and kept until the engine exits; closing a libtorrent session waits out every connection still open, minutes behind a dead WebSocket tracker, this loop reads no other line while it waits, and libtorrent's Python binding has no abort that would close one in the background
check_listen = ''#the addresses and port the checking session listens on, as libtorrent's listen_interfaces says them

def ready():#what the engine reports about itself once it is up, and the first thing the app asks for
	return {
		'event': 'ready',
		'libtorrent': lt.version,#the engine's own version string, like 2.1.1.0
		'webtorrent': 'webtorrent_stun_server' in lt.default_settings(),#true when this build of libtorrent was compiled with WebTorrent, which is what adds these settings
		'python': platform.python_version(),
		'frozen': bool(getattr(sys, 'frozen', False)),#true inside the folder pyinstaller made, false when run from a checkout
		'client': client_name(),#the name peers and trackers will see
		'fingerprint': fingerprint(),#and the front of the peer id they will see
		'centralized_servers': centralized_servers,#so the app can show them, and so anyone running the engine by hand sees the same list the document publishes
		'paths': paths,#the paths init carried, sent back unchanged, so the app can see they arrived; the engine will use them once its session keeps state
	}

out = sys.stdout.buffer#bytes rather than text, so the newline is exactly one byte on every platform and nothing translates it

def emit(message):#one line up the road
	out.write((json.dumps(message, separators=(',', ':')) + '\n').encode('utf-8'))
	out.flush()#every line at once; the app is waiting on it

def command_init(message):#the app says which version it is and where the engine keeps what it keeps, once, before anything else
	global name, version, paths
	name = str(message.get('name', ''))
	version = str(message.get('version', ''))
	paths = message.get('paths', {}) if isinstance(message.get('paths'), dict) else {}
	emit(ready())

def command_folders(message):#the download folders JavaScript holds the locks for, as absolute paths
	global folders
	listed = message.get('folders')
	if not isinstance(listed, list): listed = []#anything but a list names no folders, rather than stopping the engine
	folders = [f for f in listed if isinstance(f, str)]#and an entry that isn't text is passed over the same way
	emit({'event': 'folders', 'folders': folders})#sent back as kept, so JavaScript can see they arrived and notice if one was passed over; the engine will use them once its session downloads

def command_session(message):#start the session, listening on every address in both families at the port JavaScript chose, with the DHT on and the router left alone; a session already running is replaced
	global session
	port = message.get('port')
	if not isinstance(port, int) or not 0 < port < 65536:
		emit({'event': 'error', 'message': 'a session needs a port from 1 to 65535', 'command': 'session'}); return
	session = None#a session already running goes first, so the new one can have the port
	settings = session_settings()
	settings.update({
		'listen_interfaces': f'0.0.0.0:{port},[::]:{port}',#every address of each family at one port, each with a TCP socket and a UDP one, the UDP one carrying uTP, the DHT, and UDP trackers
		'enable_dht': True,
		'announce_to_all_tiers': message.get('announce') != 'first',#announce to one tracker in each of a torrent's tiers at once, which with one tracker to a tier, the way lists are written, is every tracker, as qBittorrent, Transmission, and µTorrent do; libtorrent on its own announces only to the first tier that answers, keeping the rest as backups, the way BEP 12 describes. trackers.announce in the settings says which, and anything but first is all
		'enable_upnp': False,#libtorrent asks the router to map the port by default, and these two stay off until JavaScript asks for mapping, so it sees what the network does before and after
		'enable_natpmp': False,
		'upnp_lease_duration': 60,#and when it does, each mapping lasts a minute and is renewed at three quarters of that, so a mapping this session leaves behind, if the engine dies holding one, is gone from the router a minute later; libtorrent's own is an hour
		'natpmp_lease_duration': 60,#the same for the NAT-PMP mapper, which speaks PCP too
		'enable_lsd': False,#local service discovery, which multicasts each torrent's info-hash to find peers on the same network, off while the only session is the Connection tab's, which measures whether strangers on the internet get in; with a socket on every address the session hears its own announcements and dials itself. It goes back on for the session that downloads, where the two mappers stay off, since ftorrent maps the port with its own code
		'optimistic_disk_retry': 2**31 - 1,#how many seconds before libtorrent takes an auto-managed torrent out of upload mode, in case the disk error it assumes put it there has cleared, ten minutes by default; then it downloads, and a session left running fetched a whole torrent and seeded it. This is sixty-eight years, so the torrent stays in upload mode, while keeping libtorrent's usual start, in which auto-management resumes the torrent and its DHT lookups begin within seconds; adding it unmanaged and unpaused instead left it without a DHT lookup for thirteen minutes. It goes back to ten minutes for the session that downloads, with local service discovery
		'alert_mask': alert_mask,
	})
	session = lt.session(settings)
	emit({'event': 'session', 'port': port})

def command_mapping(message):#turn libtorrent's two port mappers on or off in the running session, both together; on, each asks the router to forward the session's port, and off, each deletes what it mapped
	if session is None:
		emit({'event': 'error', 'message': 'no session to map the port of', 'command': 'mapping'}); return
	on = message.get('on') is True
	session.apply_settings({'enable_upnp': on, 'enable_natpmp': on})#libtorrent starts or stops the mappers on every socket it listens on, and a mapper stopped sends its deletes on the way out
	emit({'event': 'mapping', 'on': on})

def command_poll(message):#every alert that has arrived since the last poll, each as a line, oldest first; with no session there's nothing to say, which is the answer to a poll that crossed a stop on the road
	if session is None: return
	for alert in session.pop_alerts():
		try:
			emit(alert_event(alert))
		except Exception as e:#a field this build of libtorrent spells differently, sent up as what it is rather than lost
			emit({'event': 'alert', 'type': alert.what(), 'message': alert.message(), 'fault': f'{type(e).__name__}: {e}'})

def command_stats(message):#ask libtorrent for its counters, which arrive as an alert on a later poll
	if session is None: return
	session.post_session_stats()

def command_tables(message):#each DHT node the session runs, one for each address it runs the DHT from, each with a routing table of its own: its address and node ID now, and a request to it for its live nodes, which arrive as alerts on a later poll; libtorrent's own post_dht_stats describes only one table, the one with the most buckets, so this asks every table instead
	if session is None: return
	flag = lt.save_state_flags_t.save_dht_state
	state = lt.write_session_params(session.session_state(flag), flag)#the DHT's state as it would be saved, which is the one place this binding gives each node's address and ID; reading session_state's dht_state.nids directly fails, since the binding registers no Python type for it
	tables = []
	for entry in state.get(b'dht state', {}).get(b'node-id', []):#each entry is the node's 20-byte ID followed by its local address, 4 bytes for IPv4 and 16 for IPv6
		tables.append({'address': str(ipaddress.ip_address(entry[20:])), 'id': entry[:20].hex()})
		session.dht_live_nodes(lt.sha1_hash(entry[:20]))
	emit({'event': 'tables', 'tables': tables})

def command_read(message):#the torrent file at path, read without joining its swarm: its name, info-hashes, size, and files, so the panel can show what was chosen
	path = message.get('path')
	try:
		info = lt.torrent_info(path)
	except Exception as e:#not a file, not a torrent, or not readable, each an ordinary answer
		emit({'event': 'error', 'message': f'not a torrent file: {e}', 'command': 'read', 'path': path}); return
	emit({'event': 'read', 'path': path, **describe(info)})

def command_add(message):#join the swarm of the torrent file at path in upload mode, which announces and takes connections from peers but downloads nothing
	if session is None:
		emit({'event': 'error', 'message': 'no session to add a torrent to', 'command': 'add'}); return
	path = message.get('path')
	try:
		info = lt.torrent_info(path)
		params = lt.add_torrent_params()
		params.ti = info
		params.save_path = tempfile.gettempdir()#upload mode writes nothing, but libtorrent wants a place it could have
		params.flags |= lt.torrent_flags.upload_mode#held there by the session's optimistic_disk_retry, which says why
		session.add_torrent(params)
	except Exception as e:
		emit({'event': 'error', 'message': f'could not add the torrent: {e}', 'command': 'add', 'path': path}); return
	emit({'event': 'added', 'path': path, **describe(info)})

def command_stop(message):#tear the session down, its torrents, its sockets, and the DHT; letting go of the last reference destroys it, which waits while libtorrent closes its sockets and tells the trackers it's leaving
	global session
	if session is None: return
	session = None
	emit({'event': 'stopped'})

def command_check_session(message):#make the checking session, listening on these addresses at this port, or point the one already made at them, so a check announces from each family's own address and from no other, loopback and link-local among them; it only announces, with no DHT, no connections to peers, and no port mapping, which are all session-wide
	global check_session, check_listen
	addresses, port = message.get('addresses'), message.get('port')
	if not isinstance(addresses, list) or not all(isinstance(a, str) and a for a in addresses) or not addresses or not isinstance(port, int) or not 0 < port < 65536:
		emit({'event': 'error', 'message': 'a checking session needs addresses and a port from 1 to 65535', 'command': 'check_session'}); return
	listen = ','.join(f'[{a}]:{port}' if ':' in a else f'{a}:{port}' for a in addresses)
	if check_session is None:
		settings = session_settings()
		settings.update({
			'listen_interfaces': listen,
			'enable_dht': False,
			'enable_lsd': False,
			'enable_upnp': False,
			'enable_natpmp': False,
			'connection_speed': 0,#no connection attempts to the peers a reply lists; the reply decides the check, and the peers stay in each torrent's list unconnected
			'torrent_connect_boost': 0,#and none of the burst libtorrent makes to a new torrent's first peers
			'webtorrent_stun_server': '',#a WebSocket announce would otherwise ask a STUN server for this machine's public address, which the check doesn't need
			'tracker_completion_timeout': 15,#seconds an HTTP announce may take, 30 by default; a UDP one gives up after tracker_receive_timeout's 10
			'alert_mask': lt.alert_category.tracker | lt.alert_category.error,
			'alert_queue_size': 10000,#a check announces to a few hundred trackers, each with a few alerts, and the panel polls ten times a second
		})
		check_session = lt.session(settings)
	elif listen != check_listen:
		check_session.apply_settings({'listen_interfaces': listen})#the network changed since the last check; libtorrent reopens its sockets in place, with no new session and so no wait
	check_listen = listen
	emit({'event': 'check_session', 'listen': listen})

def command_check_add(message):#announce a fresh random hash to one tracker from the checking session, a hash no real torrent has, so no one else is announcing it; answers the hash, which every alert about it carries
	tracker = message.get('tracker')
	if check_session is None or not isinstance(tracker, str):
		emit({'event': 'error', 'message': 'a check needs its session and a tracker', 'command': 'check_add'}); return
	info_hash = lt.sha1_hash(secrets.token_bytes(20))
	params = lt.add_torrent_params()
	params.info_hashes = lt.info_hash_t(info_hash)
	params.trackers = [tracker]
	params.save_path = tempfile.gettempdir()#libtorrent wants a place for a torrent's files, and these torrents never have any
	params.flags &= ~lt.torrent_flags.auto_managed#active the moment it's added, rather than waiting on libtorrent's queue
	params.flags &= ~lt.torrent_flags.paused
	check_session.add_torrent(params)
	emit({'event': 'check_added', 'tracker': tracker, 'hash': str(info_hash)})

def command_check_remove(message):#forget one checked torrent, by its hash, which tells a tracker that answered that the announcer is leaving
	if check_session is None: return
	handle = check_session.find_torrent(lt.sha1_hash(bytes.fromhex(message['hash'])))#a hash this engine answered with; anything else is a bug in the panel, which the loop below reports as an error line
	if handle.is_valid(): check_session.remove_torrent(handle)

def command_check_poll(message):#every tracker alert from the checking session since the last poll, each as a line, oldest first
	if check_session is None: return
	for alert in check_session.pop_alerts():
		try:
			event = check_event(alert)
		except Exception as e:#an alert about a torrent already removed, or a field this build spells differently, sent up as what it is rather than lost
			event = {'event': 'check_alert', 'kind': 'fault', 'message': alert.message(), 'fault': f'{type(e).__name__}: {e}'}
		if event: emit(event)

check_kinds = {'tracker_announce_alert': 'announce', 'tracker_reply_alert': 'reply', 'tracker_error_alert': 'error', 'tracker_warning_alert': 'warning'}#the tracker alerts a check reads, by libtorrent's name for each

def check_event(alert):#one tracker alert as a line for JavaScript: the torrent's hash and its tracker, the family of the address it went from, libtorrent's own time for it to the microsecond, and what came back, as libtorrent gives it
	kind = type(alert).__name__
	if kind == 'alerts_dropped_alert': return {'event': 'check_alert', 'kind': 'dropped'}
	if kind not in check_kinds: return None
	address = str(alert.local_endpoint[0])
	event = {'event': 'check_alert', 'kind': check_kinds[kind], 'hash': str(alert.handle.info_hashes().v1), 'tracker': alert.tracker_url(), 'family': 6 if ':' in address else 4, 'time': alert.timestamp().timestamp()*1000}#the timestamp as milliseconds since 1970, so a reply's time minus its announce's is how long the tracker took, however late the poll that carried them
	if kind == 'tracker_reply_alert': event['peers'] = alert.num_peers
	elif kind == 'tracker_warning_alert': event['message'] = alert.message().split('warning: ', 1)[-1]#the tracker's own words, after libtorrent's prefix naming the torrent and the tracker
	elif kind == 'tracker_error_alert':
		error = alert.error
		category = error.category().name()
		reason = alert.failure_reason()
		event.update({
			'category': category,#the kind of error, like system, asio.netdb, asio.ssl, or http
			'value': error.value(),#and which one, a number that means something only beside its category
			'name': errno.errorcode.get(error.value(), '') if category == 'system' else '',#a system error's name, like ECONNREFUSED, which this platform's own numbering would hide
			'message': error.message(),
			'reason': '' if reason == '<none>' else reason,#the tracker's own words for refusing, which libtorrent writes as <none> when there are none
			'status': alert.status_code,#the HTTP status, for an HTTP error
		})
	return event

def describe(info):#what the panels show about a torrent: its name, its info-hashes, blank for a version the torrent doesn't have, and its size and files
	hashes = info.info_hashes()
	return {
		'name': info.name(),
		'v1': str(hashes.v1) if hashes.has_v1() else '',#the SHA-1 hash of the info dictionary, the one every BitTorrent client knows a torrent by
		'v2': str(hashes.v2) if hashes.has_v2() else '',#and BitTorrent v2's SHA-256, which a hybrid torrent has as well
		'size': info.total_size(),
		'files': info.num_files(),
	}

def alert_event(alert):#one alert as a line for JavaScript: its type, libtorrent's own sentence about it, and the fields JavaScript reads, as plain values
	event = {'event': 'alert', 'type': alert.what(), 'message': alert.message()}
	kind = type(alert).__name__
	if kind in ('listen_succeeded_alert', 'listen_failed_alert'):
		event.update({'address': alert.address, 'port': alert.port, 'socket': alert.socket_type.name})
		if kind == 'listen_failed_alert': event['error'] = alert.error.message()
	elif kind == 'external_ip_alert':
		event['address'] = str(alert.external_address)
	elif kind == 'incoming_connection_alert':
		event.update({'address': str(alert.endpoint[0]), 'port': alert.endpoint[1], 'socket': alert.socket_type.name})
	elif kind == 'peer_connect_alert':
		event.update({'address': str(alert.endpoint[0]), 'outgoing': 'outgoing connection' in event['message']})#the binding leaves out libtorrent's direction field, so it's read from libtorrent's own sentence, which says incoming or outgoing
	elif kind == 'dht_outgoing_get_peers_alert':
		event['address'] = str(alert.endpoint[0])#the node the DHT asked
	elif kind in ('tracker_reply_alert', 'dht_reply_alert'):
		event['peers'] = alert.num_peers
	elif kind == 'dht_live_nodes_alert':
		event.update({'id': str(alert.node_id), 'nodes': alert.num_nodes})#which DHT node, by the ID the tables command named it by, and how many live nodes its routing table holds
	elif kind in ('portmap_alert', 'portmap_error_alert', 'portmap_log_alert'):
		event.update({'mapping': int(getattr(alert, 'mapping', -1)), 'transport': alert.map_transport.name, 'local': portmap_local(event['message'])})#which mapping, by libtorrent's index, which mapper, natpmp or upnp, and the local address it mapped from
		if kind == 'portmap_alert': event.update({'protocol': alert.map_protocol.name, 'port': alert.external_port})#tcp or udp, and the external port the router gave
		if kind == 'portmap_error_alert': event['error'] = alert.error.message()#the router's refusal, in PCP's result-code words for either protocol, or UPnP's own
	elif kind == 'session_stats_alert':
		event['values'] = dict(alert.values)#every counter by name, three hundred of them
		del event['message']#which would be the same numbers again, as one long sentence
	elif kind == 'session_stats_header_alert':
		del event['message']#the counters' names, which the values above already carry
	elif kind == 'alerts_dropped_alert':
		event['dropped'] = sum(1 for dropped in alert.dropped_alerts if dropped)
	return event

def portmap_local(message):#the local address a port mapper's alert is about, which the binding leaves out of all three and libtorrent's own sentence names, as local: 192.0.2.7 in a mapping and in brackets in an error or a log line
	found = re.search(r'local: (\S+) external|\[([^\]]*)\]', message)
	return (found.group(1) or found.group(2)) if found else ''

commands = {#every command the engine understands, and the function that carries it out; quit is handled in the loop below, since it ends the loop
	'init': command_init,
	'folders': command_folders,
	'session': command_session,
	'mapping': command_mapping,
	'poll': command_poll,
	'stats': command_stats,
	'tables': command_tables,
	'read': command_read,
	'add': command_add,
	'stop': command_stop,
	'check_session': command_check_session,
	'check_add': command_check_add,
	'check_remove': command_check_remove,
	'check_poll': command_check_poll,
}

def main():
	for raw in sys.stdin.buffer:#one line per message; the loop ends when the app closes the pipe, which is how a dying app takes the engine with it
		line = raw.decode('utf-8-sig', 'replace').strip()#utf-8-sig drops a byte-order mark at the start, which Windows PowerShell 5.1 puts ahead of anything it pipes into a program
		if not line: continue
		try:
			message = json.loads(line)
		except ValueError:
			emit({'event': 'error', 'message': 'malformed json', 'line': line[:200]}); continue
		command = message.get('command') if isinstance(message, dict) else None
		if command == 'quit': break
		handler = commands.get(command)
		if handler:
			try:
				handler(message)
			except Exception as e:#a command that fails in a way nobody foresaw answers with an error line, rather than taking the engine down
				emit({'event': 'error', 'message': f'{type(e).__name__}: {e}', 'command': command})
		else: emit({'event': 'error', 'message': 'unknown command', 'command': command})

if __name__ == '__main__':
	try:
		main()
	except BrokenPipeError:
		pass#the app went away mid-write; there is nobody left to tell, and exiting quietly is the right answer
	os._exit(0)#leave now, at quit or when the pipe closes, rather than let Python tear down the sessions on the way out: destroying a libtorrent session waits out every connection still open, minutes behind a dead WebSocket tracker, while Rust waits on this process before the app can finish quitting. Nothing here is state to save: the system closes every socket with the process, and a port mapping a session leaves behind lapses with its one-minute lease. An engine that holds state to keep saves it on a command of its own before quit, and still leaves this way
