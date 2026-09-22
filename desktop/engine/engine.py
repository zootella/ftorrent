#./desktop/engine/engine.py

import sys
import json
import platform
import libtorrent as lt

# The engine is the process that will hold libtorrent, and this file is its whole program for now. It starts, answers a few lines of newline-delimited JSON on its pipes, and exits when told to or when its pipe closes. It opens no sockets and creates no libtorrent session yet. What it proves is the shape of things: the app can carry a frozen Python beside itself, start it, talk to it in both directions, and stop it without leaving a process behind. The torrent work arrives later on top of exactly this loop.
#
# Two rules about the pipes, both because the other end is Rust reading lines. Every message is one line of JSON ending in a newline and flushed at once, so nothing sits in a buffer while the app waits. And a line that is not JSON, or JSON that is not a command the engine knows, gets an error line back rather than a crash, so a mistake on one side never takes the other side down.

def ready():#what the engine reports about itself once it is up, and the first thing the app asks for
	return {
		'event': 'ready',
		'libtorrent': lt.version,#the engine's own version string, like 2.1.1.0
		'webtorrent': 'webtorrent_stun_server' in lt.default_settings(),#true when this build of libtorrent was compiled with WebTorrent, which is what adds these settings
		'python': platform.python_version(),
		'frozen': bool(getattr(sys, 'frozen', False)),#true inside the folder pyinstaller made, false when run from a checkout
	}

def main():
	out = sys.stdout.buffer#bytes rather than text, so the newline is exactly one byte on every platform and nothing translates it
	def emit(message):
		out.write((json.dumps(message, separators=(',', ':')) + '\n').encode('utf-8'))
		out.flush()#every line at once; the app is waiting on it

	for raw in sys.stdin.buffer:#one line per message; the loop ends when the app closes the pipe, which is how a dying app takes the engine with it
		line = raw.decode('utf-8', 'replace').strip()
		if not line: continue
		try:
			message = json.loads(line)
		except ValueError:
			emit({'event': 'error', 'message': 'malformed json', 'line': line[:200]}); continue
		command = message.get('command') if isinstance(message, dict) else None
		if command == 'init':
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
