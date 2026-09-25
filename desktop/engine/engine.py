#./desktop/engine/engine.py

import sys
import json
import platform
import libtorrent as lt

# The engine is the process that will hold libtorrent, and this file is its whole program for now. It starts, answers a few lines of newline-delimited JSON on its pipes, and exits when told to or when its pipe closes. It opens no sockets and creates no libtorrent session yet. What it proves is the shape of things: the app can carry a frozen Python beside itself, start it, talk to it in both directions, and stop it without leaving a process behind. The torrent work arrives later on top of exactly this loop.
#
# Two rules about the pipes, both because the other end is Rust reading lines. Every message is one line of JSON ending in a newline and flushed at once, so nothing sits in a buffer while the app waits. And a line that is not JSON, or JSON that is not a command the engine knows, gets an error line back rather than a crash, so a mistake on one side never takes the other side down.

# The centralized servers the engine reaches on its own, and the one place they are written. Three lists, each in the same order: ours first, then the well-known providers or the servers universal to BitTorrent. The Names and Numbers document on docs.ftorrent.com is the public record of every entry, who runs it, why it is here, and what answered when we checked; a change here is a change there.
centralized_servers = {
	'stun': [#the STUN server WebTorrent uses to learn its own public address for WebRTC; libtorrent takes exactly one, so the first entry is the one used, and the rest are the order to fall back through once the engine can test them, and the choices a settings page will offer
		'stun.ftorrent.com:3478',#ours
		'stun.cloudflare.com:3478',#Cloudflare's public STUN
		'stun.l.google.com:19302',#Google's public STUN, libtorrent's own default
	],
	'dht': [#the bootstrap nodes a fresh DHT routing table starts from; libtorrent takes them all, and after the first start the saved routing table makes them unnecessary
		'dht.ftorrent.com:51420',#ours
		'dht.libtorrent.org:25401',#run by libtorrent's author, and libtorrent's own default
		'dht.transmissionbt.com:6881',#run by the Transmission project
	],
	'trackers': [#the trackers added to a torrent ftorrent creates, so a new torrent is reachable from the start; whether they are also added to torrents the user adds is decided in the add-torrent story
		'udp://open.ftorrent.com:443/announce',#ours, on all three protocols
		'https://open.ftorrent.com/announce',
		'wss://open.ftorrent.com',
		'wss://tracker.webtorrent.dev',#the WebTorrent project's, Aquatic like ours
		'wss://tracker.openwebtorrent.com',#the longest-running WebTorrent tracker, the default in the browser clients
		'udp://tracker.opentrackr.org:1337/announce',#the most widely used open tracker
	],
}

version = ''#the app's version, which arrives in the init line because the engine cannot read tauri.conf.json, the one place it is written
paths = {}#where the engine will keep what it keeps: the data folder, the state file, and the download folders, which also arrive in init because the app works them out and the engine never does

def client_name():#how the client names itself on the wire: brand first, then lineage, the way a browser's user agent reads, so a narrow column shows the brand and a wide one shows the whole truth
	return f'ftorrent/{version} libtorrent/{lt.version}'

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
		'paths': paths,#the paths init carried, sent back unchanged, so the app can see they arrived; the engine uses them once it has a session
	}

def main():
	out = sys.stdout.buffer#bytes rather than text, so the newline is exactly one byte on every platform and nothing translates it
	def emit(message):
		out.write((json.dumps(message, separators=(',', ':')) + '\n').encode('utf-8'))
		out.flush()#every line at once; the app is waiting on it

	for raw in sys.stdin.buffer:#one line per message; the loop ends when the app closes the pipe, which is how a dying app takes the engine with it
		line = raw.decode('utf-8-sig', 'replace').strip()#utf-8-sig drops a byte-order mark at the start, which Windows PowerShell 5.1 puts ahead of anything it pipes into a program
		if not line: continue
		try:
			message = json.loads(line)
		except ValueError:
			emit({'event': 'error', 'message': 'malformed json', 'line': line[:200]}); continue
		command = message.get('command') if isinstance(message, dict) else None
		if command == 'init':
			global version, paths
			version = str(message.get('version', ''))#the app says which version it is, once, before anything else
			paths = message.get('paths', {}) if isinstance(message.get('paths'), dict) else {}
			emit(ready())
		elif command == 'quit':
			break
		else:
			emit({'event': 'error', 'message': 'unknown command', 'command': command})

if __name__ == '__main__':
	try:
		main()
	except BrokenPipeError:
		pass#the app went away mid-write; there is nobody left to tell, and exiting quietly is the right answer
