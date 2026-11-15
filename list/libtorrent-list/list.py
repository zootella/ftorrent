import os
import ssl
import sys
import json
import time
import errno
import socket
import secrets
import sqlite3
import tempfile
import traceback
import concurrent.futures
import http.client
import urllib.parse
import urllib.request
import libtorrent as lt
from zootella.ergo import Size, Time, has_tick, tick_to_iso, tick_to_text#ergo's units, so a duration or a size says what it is, and its tick, a moment as milliseconds since 1970, with a check for one from outside and two ways to write one

# The tracker checker at list.ftorrent.com, and this file is its whole program.
#
# Every hour it announces once to each public BitTorrent and WebTorrent tracker it knows of, and records what came back. Two containers run it, built from this folder, one on a network with only IPv4 and one on a network with only IPv6, so libtorrent has no choice of family for any tracker, WebSocket ones included, and each container's results are its family's. LIST_FAMILY tells the program which family it's in.
#
# The candidates come from public lists other people keep, ngosang's and newTrackon's, fetched every hour, plus a list of our own below: ftorrent's trackers and every public WebSocket tracker we could find, working or long gone. A list that can't be fetched keeps its last good copy, so one site down for an hour doesn't empty the candidates.
#
# Before announcing, the pass looks up every tracker's name itself, all at once, and decides right there each name that doesn't resolve or has no address in this family. The rest each get a torrent of their own, a fresh random info hash that names no real torrent, added one a second to a libtorrent session the container keeps for its life, and each is judged when its own window ends. A tracker's outcome for the hour is how far it got: its name resolved to an address in this family, something answered, an HTTPS or WebSocket tracker's certificate held, and the answer was a tracker's reply. Beside the outcome, a check keeps how many tries the answer took, how long the tracker asked clients to wait before announcing again, and any warning it sent, and once a day each HTTPS and WebSocket tracker's certificate is read for its dates and its issuer. The session sends announces and nothing else: it never connects to the peers a reply lists, runs no DHT, and maps no ports.
#
# A UDP or HTTP reply that lists other peers for a hash no one else has is still up, but it hands out addresses no swarm put there, which a client using the tracker would connect to, so a tracker that does it is left out of trackers.txt until a month passes without it.
#
# A UDP tracker paired with the page.json of the gauge beside it, as ftorrent's own is, gets no announce. Its outcome comes from that page instead: the count of UDP announces the tracker served over this container's family in the gauge's last minute, which are peers reaching it from the internet, the same claim every other outcome makes.
#
# Everything lives in one SQLite file in the data folder the two containers share: every check, every tracker ever named, each upstream list's latest copy, each day's certificates, and a tally of each day that has ended, which the report's longer horizons are summed from instead of every hour again. SQLite lets one writer in at a time, which keeps the containers' writes apart on the rare hour they land together.
#
# The container with LIST_REPORT set also publishes. At the top of each hour, before checking, it writes the hour that just ended: trackers.txt, the trackers up in at least 95% of the hours they were checked over the last month, and trackers.json, every tracker's hours over six horizons, from the last hour to the last year, with its last 180 days and the details of its replies. The files always describe one whole hour, about an hour behind, and never a mix of two. They're a record of uptime; a check of right now is the desktop client's job.

horizons = {#the stretches of time trackers.json totals each tracker's hours over, each ending with the hour it reports, by the lowercase letter it names each with; every hour in one, for each tracker in each family, is exactly one outcome, so a horizon's hours always add up to its length
	'h': Time.HOUR,
	'd': Time.DAY,
	'w': Time.WEEK,
	'm': 30*Time.DAY,#a month, a fixed 30 days, where ergo's Time.MONTH is a twelfth of a year
	'q': 90*Time.DAY,#a quarter
	'y': 365*Time.DAY,
}
criterion_horizon = 'm'#trackers.txt lists a tracker whose uptime over this horizon,
criterion_uptime = 0.95#is at least this, in at least one family: its up hours as a share of the hours with a reading, however few those are so far, so the list fills from the first hour and steadies as a month of records grows,
criterion_extra_peers = 0#and whose replies listed extra peers in no more than this many hours of the same horizon, whatever its uptime

letters = {'up': 'U', 'dns': 'D', 'no_address': 'N', 'timeout': 'T', 'refused': 'R', 'cert': 'C', 'bad_reply': 'B', 'no_reading': 'Z'}#each outcome as one capital letter in trackers.json, in the order it lists them, capitals so they never look like the lowercase horizons they sit inside; no reading is an hour with no result, when list's own monitor wasn't watching or every tracker in a family failed at once, which says nothing about the tracker, and the README's table of outcomes spells out each one

family = 6 if os.environ.get('LIST_FAMILY') == '6' else 4#which family this container checks, set in the compose file
reporting = os.environ.get('LIST_REPORT') == '1'#whether this container also publishes, which the IPv4 one does
folder = os.environ.get('LIST_DIR', os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data'))#the data folder both containers share, a volume; when run by hand, the data folder beside this program's folder, where the compose file's ./data is
database_path = os.path.join(folder, 'list.sqlite')
public = os.path.join(folder, 'public')#what the web server serves at list.ftorrent.com
pool_threads = 32#how many names the pass looks up, or certificates it reads, at once, so a slow one delays only itself
lookup_limit = 30*Time.SECOND#how long a name may take before it counts as not resolving, the same 30 seconds libtorrent gives a tracker
start_every = 1*Time.SECOND#the pass starts one tracker's announce each second, so each is checked on its own, the way a client announces a torrent to its few trackers, rather than behind hundreds of others
listen_shortest = 50*Time.SECOND#each tracker is listened to at least this long after its own announce, time for two complete tries: libtorrent gives up on a lost UDP packet after 10 seconds and tries again at 39.5, which has until 49.5, and retries a failed HTTP announce at 16.5, which has 30 seconds,
listen_longest = 2*Time.MINUTE#and past that until it has answered or failed, but no longer than this, the stop for one libtorrent never reports on. A pass takes a second for each tracker it announces to, plus this at most, so even 3000 trackers end inside their hour
fetch_limit = 30*Time.SECOND#how long fetching one list may take
certificate_limit = 10*Time.SECOND#how long reading one tracker's certificate may take to connect, and again to finish the handshake
strip_days = 180#how many days trackers.json carries for each tracker and family, one character a day, which the page draws as a calendar
user_agent = f'ftorrent-list/0.1.0 libtorrent/{lt.version}'#how list introduces itself to trackers and to the lists it fetches, so an operator who sees it can find this project

site = 'https://list.ftorrent.com'#where the report is published, which trackers.json names so a copy of it says where the live one is

upstream = [#where the candidates come from, which trackers.json repeats at its top so a reader sees what was checked: each source's name, the page it lives at, its code, and either the lists fetched from it every hour or the trackers it names directly. Tracking the Trackers on docs.ftorrent.com says who keeps the lists and how each one checks
	{
		'name': 'ngosang',
		'home': 'https://github.com/ngosang/trackerslist',
		'code': 'https://github.com/ngosang/trackerslist',#the list is its own repository, so its page and its code are the same
		'lists': [
			'https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all.txt',
			'https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_ws.txt',
		],
	},
	{
		'name': 'newtrackon',
		'home': 'https://newtrackon.com',
		'code': 'https://github.com/CorralPeltzer/newTrackon',
		'lists': [
			'https://newtrackon.com/api/all',
		],
	},
	{
		'name': 'ftorrent',
		'home': site,
		'code': 'https://github.com/zootella/ftorrent/tree/master/list',
		'trackers': [#a list of our own, written here rather than fetched, in normalize()'s spelling: ftorrent's own trackers, and every public WebSocket tracker we could find that is or was in use, from ngosang's WebSocket lists across their history since 2017, the defaults WebTorrent's create-torrent has shipped since its version 3, and P2P Media Loader's. A tracker long gone stays, so the record shows it, and one that comes back is noticed
			'udp://open.ftorrent.com:443/announce',
			'https://open.ftorrent.com/announce',
			'wss://open.ftorrent.com',
			'wss://tracker.webtorrent.dev',#create-torrent's default today
			'wss://tracker.openwebtorrent.com',
			'wss://tracker.files.fm:7073/announce',
			'wss://tracker.novage.com.ua',#P2P Media Loader's default
			'wss://tracker.btorrent.xyz',
			'wss://tracker.webtorrent.io',#WebTorrent's first default
			'wss://tracker.fastcast.nz',
			'wss://tracker.sloppyta.co/announce',
			'wss://tracker.magnetoo.io/announce',
			'wss://tracker.ghostchu-services.top/announce',
			'wss://tracker.dnlab.net/announce',
			'wss://tracker.lab.vvc.niif.hu/announce',
			'wss://tracker.iamhansen.xyz/announce',
			'wss://ltrackr.iamhansen.xyz/announce',
			'wss://qot.abiir.top/announce',
			'wss://ws.peer.ooo/announce',
			'wss://hub.bugout.link/announce',
			'wss://track.file.pizza/announce',
			'wss://spacetradersapi-chatbox.herokuapp.com/announce',
			'wss://open.tube/tracker/socket',#PeerTube instances, which ngosang listed for a time
			'wss://peertube.cpy.re/tracker/socket',
			'wss://peertube.live/tracker/socket',
			'wss://tube.privacytools.io/tracker/socket',
			'wss://video.blender.org/tracker/socket',
			'ws://tracker.files.fm:7072/announce',#and plain WebSocket ones, which a page served over HTTPS can't reach, but other clients can
			'ws://tracker.sloppyta.co/announce',
			'ws://hub.bugout.link/announce',
			'ws://track.file.pizza/announce',
			'ws://tracker.ghostchu-services.top/announce',
			'ws://tracker.lab.vvc.niif.hu/announce',
			'ws://tracker.btsync.cf:2710/announce',
			'ws://tracker.btsync.cf:6969/announce',
			'ws://citrus.carapax.net:6969/announce',
			'ws://npserver.intranet.pw:4201/announce',
			'ws://passpulse.net:8000/announce',
			'ws://tracker.darli.net:6611/announce',
			'ws://tracker.merded.xyz:8000/announce',
			'ws://tracker.nighthawk.pw:2052/announce',
			'ws://tracker.nighthawk.pw:4201/announce',
			'ws://tracker.xn--vzyr4p.top/announce',
		],
	},
]
named_by = {list_url: source['name'] for source in upstream for list_url in source.get('lists', [])}#each fetched list's URL to the name of its source
curated = [tracker for source in upstream for tracker in source.get('trackers', [])]#the trackers a source names directly, checked every hour whatever the fetched lists say
downstream = [f'{site}/trackers.txt', f'{site}/trackers.json']#what this publishes from them

gauges = {#a UDP tracker whose outcome comes from its gauge's page.json rather than an announce: up when the tracker served UDP announces over this container's family in the gauge's last minute, a timeout when it served none; pair your own tracker with your own gauge's page the same way
	'udp://open.ftorrent.com:443/announce': 'https://open.ftorrent.com/page.json',
}

schema = '''
create table if not exists trackers (
	id integer primary key,
	url text unique not null,          -- the announce URL, in the one spelling normalize() gives it
	since integer not null             -- the tick of the first hour it was a candidate
);
create table if not exists upstream (
	list_url text not null,            -- the URL of the list that names it
	url text not null,
	primary key (list_url, url)
);
create table if not exists checks (
	hour integer not null,             -- the tick at the top of the hour the check belongs to
	family integer not null,           -- 4 or 6
	tracker integer not null references trackers (id),
	outcome text not null,             -- up, or how far it got: dns, no_address, timeout, refused, cert, or bad_reply
	message text not null,             -- libtorrent's words for what went wrong, blank when up
	extra_peers integer not null,      -- peers a UDP or HTTP reply listed beyond the one it may, for a hash no one else has
	tries integer,                     -- how many announces libtorrent sent in the tracker's window, 0 when the name lookup decided first; empty on checks from before version 2
	interval integer,                  -- how long the tracker asked clients to wait before announcing again, in milliseconds, when it replied
	warning text not null default '',  -- the tracker's own warning, when it sent one
	primary key (hour, family, tracker)
);
create table if not exists passes (
	hour integer not null,             -- the tick at the top of the hour the pass belongs to
	family integer not null,
	began integer not null,            -- the tick the pass began
	ended integer not null,            -- and the tick it ended
	primary key (hour, family)
);
create table if not exists daily (
	day integer not null,              -- the tick at the start of the UTC day
	family integer not null,
	tracker integer not null references trackers (id),
	outcome text not null,             -- an outcome, or no_reading for the hours every tracker in the family failed
	hours integer not null,            -- how many of the day's hours had this outcome
	retried integer not null,          -- of those hours, the up ones that needed a second try
	primary key (tracker, family, day, outcome)
);
create table if not exists certificates (
	day integer not null,              -- the tick at the start of the UTC day it was read, which happens once a day
	family integer not null,
	tracker integer not null references trackers (id),
	issued integer,                    -- the tick the certificate became valid, when it could be read
	expires integer,                   -- and the tick it stops being valid
	issuer text not null,              -- who signed it, its organization and name
	error text not null,               -- what went wrong when it couldn't be read, blank when it could
	primary key (day, family, tracker)
);
'''
schema_version = 3#the shape schema gives a new database, which the file records in SQLite's PRAGMA user_version, an integer set aside for the program's own use; a file at an older version is brought forward by the steps below when the program opens it
migrations = {#the steps that bring a database to each version, run in order; a step never changes once it has run on the server, and the next change is a new step. Version 1 is the shape from 2026-11-10, before the file recorded its version
	2: [#2026-11-12: each check records how many tries it took, the interval the tracker asked for, and any warning it sent; the daily tally gains retried hours, and since it's a cache of the checks, it's dropped and rebuilds itself
		'alter table checks add column tries integer',
		'alter table checks add column interval integer',
		"alter table checks add column warning text not null default ''",
		'drop table daily',
	],
	3: [#2026-11-13: the daily tally names an hour with no result no_reading rather than asleep, which sounded like a fact about the tracker; it's a cache of the checks, so it's dropped and rebuilds itself, and if exists lets a file at version 1, whose daily step 2 already dropped, come forward in one go
		'drop table if exists daily',
	],
}

def say(line):#one line to the container's log, tagged with the family so the two containers' logs read apart
	print(f'list{family}: {line}', flush=True)

def tick():#now, as a tick, ergo's moment: milliseconds since 1970
	return int(time.time()*Time.SECOND)

def clock():#a reading of the monotonic clock, in milliseconds, for timing a pass without the wall clock jumping under it
	return time.monotonic()*Time.SECOND

def top(t):#the tick at the top of the hour a tick falls in
	return t - t % Time.HOUR

def at_once(work, items, limit):#run work on every item at once, on a pool of threads, and after limit hand back each item's future, done or not; one still out stays out, and its thread finishes on its own
	pool = concurrent.futures.ThreadPoolExecutor(pool_threads)
	futures = {item: pool.submit(work, item) for item in items}
	concurrent.futures.wait(futures.values(), timeout=limit/Time.SECOND)#concurrent.futures counts in seconds
	pool.shutdown(wait=False, cancel_futures=True)
	return futures

# candidates

def normalize(text):#one spelling for each announce URL, so a tracker two lists name differently is one candidate: lowercase, without the port its scheme implies; None for anything that isn't an address of a kind we check
	try:
		parts = urllib.parse.urlsplit(text.strip())
		port = parts.port#reading it checks it, and an impossible port raises
	except ValueError:
		return None
	scheme = parts.scheme.lower()
	if scheme not in ('udp', 'http', 'https', 'ws', 'wss') or not parts.hostname: return None
	host = f'[{parts.hostname}]' if ':' in parts.hostname else parts.hostname#lowercase already; an IPv6 literal takes its brackets back
	if port is not None and not (scheme in ('https', 'wss') and port == 443) and not (scheme in ('http', 'ws') and port == 80): host += f':{port}'
	query = f'?{parts.query}' if parts.query else ''
	return f'{scheme}://{host}{parts.path}{query}'

def fetch(url):#the tracker URLs one source names, or None when it can't be had this hour. A site that's down, slow, or garbled is an ordinary answer here, so this is where those exceptions stop
	request = urllib.request.Request(url, headers={'User-Agent': user_agent})
	try:
		with urllib.request.urlopen(request, timeout=fetch_limit/Time.SECOND) as response:#urllib counts in seconds
			text = response.read(4*Size.MB).decode('utf-8', 'replace')#a few hundred lines at most, so four megabytes means something's wrong, and reading stops there
	except (OSError, ValueError, http.client.HTTPException) as e:
		say(f'kept the last copy of {url}, since fetching it said: {e}')
		return None
	return [tracker for tracker in (normalize(line) for line in text.splitlines()) if tracker]

def gather(database, stamp):#this hour's candidates: each list fetched replaces its last copy, a list that couldn't be fetched keeps its last one, and the trackers named here are always in
	for list_url in named_by:
		found = fetch(list_url)
		if found is None: continue
		with database:#one transaction, so the other container never reads a list half replaced
			database.execute('delete from upstream where list_url = ?', (list_url,))
			database.executemany('insert or ignore into upstream (list_url, url) values (?, ?)', [(list_url, tracker) for tracker in found])
	candidates = {row[0] for row in database.execute('select url from upstream')} | set(curated)
	with database:
		database.executemany('insert or ignore into trackers (url, since) values (?, ?)', [(tracker, stamp) for tracker in candidates])
	return sorted(candidates)

# checks

def result(outcome, message='', extra_peers=0, tries=0, interval=None, warning=''):#one tracker's check for the hour, in the order the checks table keeps it after its hour, family, and tracker; a check decided without an announce, by the name lookup or a gauge, takes the defaults for what only an announce can show
	return (outcome, message, extra_peers, tries, interval, warning)

def local_address():#the address this container sends from in its family: connecting a UDP socket sends nothing, and the system picks the route, the way the desktop client's net_local asks; None when the family has no route
	probe = '2001:db8::1' if family == 6 else '192.0.2.1'#addresses set aside for documentation, which name no one
	try:
		with socket.socket(socket.AF_INET6 if family == 6 else socket.AF_INET, socket.SOCK_DGRAM) as s:
			s.connect((probe, 9))
			return s.getsockname()[0]
	except OSError:#a container whose network has no route in this family
		return None

def lookup(candidates):#look up every candidate's name in Python, all at once, and decide the trackers that can't be announced to from this family, as {tracker URL: result} for each name that doesn't resolve or has no address in this family. libtorrent looks names up one at a time, on a single thread, so a tracker left to it can wait in that line past its 30 seconds and read as a timeout; looked up here, one slow name delays only itself, and the server's DNS cache is warm for the names libtorrent asks again
	want = socket.AF_INET6 if family == 6 else socket.AF_INET
	name_of = {tracker: urllib.parse.urlsplit(tracker).hostname for tracker in candidates}
	def families(name):#the address families a name has, from both kinds of record, whatever this container can reach
		return {entry[0] for entry in socket.getaddrinfo(name, None)}
	asked = at_once(families, set(name_of.values()), lookup_limit)
	decided = {}
	for tracker, name in name_of.items():
		answer = asked[name]
		if answer.cancelled() or not answer.done():
			decided[tracker] = result('dns', f'no answer from DNS in {lookup_limit//Time.SECOND} seconds')
			continue
		try:
			found = answer.result()
		except (OSError, UnicodeError) as e:#a name that doesn't resolve, or can't be written as one; an ordinary answer here, so this is where those exceptions stop
			decided[tracker] = result('dns', str(e))
			continue
		if want not in found: decided[tracker] = result('no_address', f'no IPv{family} address')
	return decided

depth = {'dns': 0, 'no_address': 0, 'timeout': 1, 'refused': 2, 'cert': 3, 'bad_reply': 4}#how far each failure shows a tracker got; when a tracker fails more than one way in a pass, the furthest is the one recorded, so a server that resets a connection and then fails its certificate is recorded as a certificate problem, since something did answer
hangups = {errno.ECONNREFUSED, errno.ECONNRESET, errno.EPIPE}#the system's errors that mean something at the address answered and turned us away, by the numbers this system gives them

def classify(alert):#how far a failed announce got, and the words to keep for it, from the error libtorrent reports: its category says what kind of thing went wrong, and its value which thing
	error = alert.error
	category, value = error.category().name(), error.value()
	reason = alert.failure_reason()
	if reason == '<none>': reason = ''#libtorrent's words for a tracker that gave no reason
	if category == 'http': return ('bad_reply', f'HTTP {alert.status_code}')#a web server answered with an error status, often a proxy like Cloudflare whose server behind it is gone
	if reason: return ('bad_reply', reason)#the tracker's own words for refusing
	message = error.message()
	if category == 'asio.netdb': return ('dns', message)#the name doesn't resolve
	if category == 'libtorrent' and value == 202: return ('dns', message)#a name in another script, which libtorrent refuses by default, since it can be made to look like a different name
	if category == 'libtorrent' and value == 180: return ('no_address', message)#the name resolves, but to no address in this family
	if category == 'asio.ssl.stream': return ('refused', message)#the encrypted connection was cut off partway
	if category == 'asio.ssl': return ('cert', message)#expired, for another name, or signed by no one trusted
	if category == 'system' and value in hangups: return ('refused', message)
	if category == 'system' or (category == 'libtorrent' and value == 36): return ('timeout', message)#nothing there to answer: timed out, or no route to it
	return ('bad_reply', message)#something answered, and what it said wasn't a tracker's reply

def judge(url, peers, failures):#one tracker's outcome for the hour, as (outcome, message, extra peers): up when it replied, or else the furthest of its failures; peers is how many its reply listed, and None when it never replied
	if peers is not None:#None rather than falsy, since a reply that lists no one, zero peers, is the honest one
		extra = 0
		if peers > 1 and not url.startswith(('ws://', 'wss://')): extra = peers - 1#one peer is allowed, since some trackers list the announcer itself; a WebSocket tracker relays offers rather than listing peers, so only UDP and HTTP replies carry a count to read
		return ('up', '', extra)
	if not failures: return ('timeout', 'no answer', 0)#silent for the whole window, with not even an error to report
	outcome, message = max(failures, key=lambda failure: depth[failure[0]])#max keeps the first of equals, so this is the earliest of the furthest
	return (outcome, message, 0)

sessions = {}#the one libtorrent session, kept for the life of the container, by the address it listens on

def session_for(local):#the libtorrent session the passes announce from, made the first time and kept after, the way a client keeps one. Closing a session waits out every connection still open, and a dead WebSocket tracker can leave one that takes minutes to give up, so a pass that made and closed its own session ran minutes long; kept open, whatever a dead tracker leaves behind finishes in the background
	if local not in sessions:
		host = f'[{local}]' if ':' in local else local
		sessions[local] = lt.session({
			'listen_interfaces': f'{host}:{49152 + secrets.randbelow(16384)}',#the one address, at a port picked when the container starts from the range IANA sets aside for this, 49152 through 65535; announces carry the port, and nothing ever connects to it, since nothing comes into the container
			'enable_dht': False,#only announces: no DHT, no local discovery, no port mapping
			'enable_lsd': False,
			'enable_upnp': False,
			'enable_natpmp': False,
			#and no connections to peers. The tracker's reply decides the outcome, and the peers a reply lists for a hash no one else has are only addresses the tracker chose to hand out, so connecting would send BitTorrent handshakes from this server to whoever is there, for a torrent that doesn't exist and nothing the check needs
			'connection_speed': 0,#the connection attempts libtorrent makes each second to the peers a reply lists, to fetch the torrent a hash names; 0 makes none, and the peers still land in each torrent's list, where the extra peers count reads them
			'torrent_connect_boost': 0,#and the burst of connections it makes to a new torrent's first peers, outside that schedule
			'webtorrent_stun_server': '',#a WebSocket announce carries WebRTC offers, which would otherwise ask a STUN server for this container's public address; the check only needs the tracker's reply
			'user_agent': user_agent,
			'alert_mask': lt.alert_category.tracker | lt.alert_category.error,
			'alert_queue_size': 100000,#libtorrent tells a pass everything through a queue the loop in check() empties four times a second, and drops what arrives once it's full, so a reply it dropped would read as a tracker that never answered; with announces a second apart the queue holds a few dozen at most, and 100000 costs nothing until it's used
		})
	return sessions[local]

def check(candidates, local):#announce to each candidate from local, this container's own address, one a second, and judge each when its own window ends, as {tracker URL: result}
	session = session_for(local)
	tracker_of = {}#each torrent's info hash, in the hex libtorrent prints, to its tracker
	replied = set()#the trackers that sent a reply
	failures = {}#tracker to every (outcome, message) its errors showed
	tries = {}#tracker to how many announces libtorrent sent it in its window
	replied_at = {}#tracker to when its reply arrived, by the pass's clock
	warning = {}#tracker to its own warning, when it sent one
	results = {}
	silent = 0#how many trackers libtorrent never said a word about
	with tempfile.TemporaryDirectory() as scratch:#libtorrent wants somewhere to save a torrent's files, and these torrents never have any
		waiting = list(candidates)#the trackers not yet announced to, in the order they start
		live = {}#tracker to its torrent and when its announce started
		next_start = clock()
		while waiting or live:
			while waiting and clock() >= next_start:#start the next announce when its second comes
				tracker = waiting.pop(0)
				info_hash = lt.sha1_hash(secrets.token_bytes(20))#a hash no real torrent has, so no one else announces it
				params = lt.add_torrent_params()
				params.info_hashes = lt.info_hash_t(info_hash)
				params.trackers = [tracker]
				params.save_path = scratch
				params.flags &= ~lt.torrent_flags.auto_managed#active the moment it's added, rather than waiting on libtorrent's queue
				params.flags &= ~lt.torrent_flags.paused
				tracker_of[str(info_hash)] = tracker
				live[tracker] = (session.add_torrent(params), clock())
				next_start += start_every

			session.wait_for_alert(Time.SECOND//4)#libtorrent takes milliseconds here, and returns early as soon as an alert arrives
			for alert in session.pop_alerts():
				if isinstance(alert, lt.alerts_dropped_alert): say('libtorrent dropped alerts it had no room to queue, so some trackers this hour may read as silent')
				if not isinstance(alert, (lt.tracker_announce_alert, lt.tracker_reply_alert, lt.tracker_error_alert, lt.tracker_warning_alert)): continue
				tracker = tracker_of.get(str(alert.handle.info_hashes().v1))
				if tracker not in live: continue#an alert about something other than this pass's torrents, or about one already judged, like the goodbye its removal sends
				if isinstance(alert, lt.tracker_announce_alert):
					tries[tracker] = tries.get(tracker, 0) + 1
				elif isinstance(alert, lt.tracker_warning_alert):
					warning[tracker] = alert.message().split('warning: ', 1)[-1]#the tracker's own words, after libtorrent's prefix naming the torrent, the tracker, and this container's address
				elif isinstance(alert, lt.tracker_reply_alert):
					replied.add(tracker)
					replied_at[tracker] = clock()
				else:
					failures.setdefault(tracker, []).append(classify(alert))#libtorrent retries on its own, so a tracker can fail more than one way in its window

			for tracker, (handle, began) in list(live.items()):#judge each tracker whose own window has ended
				waited = clock() - began
				heard = tracker in replied or tracker in failures
				if not ((waited > listen_shortest and heard) or waited > listen_longest): continue
				peers, interval = None, None
				if tracker in replied:
					status = handle.status()#one call to libtorrent for two things, which waits on its network thread, so the pass makes it once per tracker, when judging, never while replies are arriving
					peers = status.list_peers#the peers libtorrent kept from its reply, which leaves out an address of nothing or a port of zero that some trackers pad a reply with; the reply's own count, num_peers, includes those
					interval = round((status.next_announce.total_seconds()*Time.SECOND + clock() - replied_at[tracker])/Time.SECOND)*Time.SECOND#how long the tracker asked clients to wait: the time left until libtorrent would announce again, plus the time since the reply, to the nearest second, which is all a reply carries
				if not heard: silent += 1
				outcome, message, extra = judge(tracker, peers, failures.get(tracker))
				results[tracker] = result(outcome, message, extra, tries.get(tracker, 0), interval, warning.get(tracker, ''))
				session.remove_torrent(handle)#which tells a tracker that answered that the announcer is leaving, so the random hash doesn't linger in its memory; the session sends it in the background, and the next pass sets its alerts aside
				del live[tracker]
	if silent > 0: say(f'{silent} trackers went {listen_longest//Time.MINUTE} minutes with no word from libtorrent')
	return results

def read_gauge(url):#a paired UDP tracker's result for the hour from its gauge's page.json, with no announce sent, or None when the page can't speak for this hour. A page that can't be fetched or read is an ordinary answer here, so this is where those exceptions stop
	request = urllib.request.Request(url, headers={'User-Agent': user_agent})
	try:
		with urllib.request.urlopen(request, timeout=fetch_limit/Time.SECOND) as response:
			page = json.loads(response.read(1*Size.MB))#a few kilobytes, so a megabyte means something's wrong, and reading stops there
		reading = page['epoch']#when the gauge last read the trackers, which should be a tick
		served = page['servedMinute']#what each tracker served in the minute before that reading, by protocol and family, like udp4
		count = int(served[f'udp{family}'])
		silent = not any(int(n) for n in served.values())
	except (OSError, ValueError, KeyError, TypeError, AttributeError, http.client.HTTPException) as e:#the fetch failing, or a page that isn't the shape the gauge writes
		say(f'no outcome from {url}, since reading it said: {e}')
		return None
	if not has_tick(reading):#a number from outside, so ask before treating it as a tick
		say(f'no outcome from {url}, since its time, {reading!r}, is not a tick')
		return None
	if abs(tick() - reading) > 3*Time.MINUTE:#the gauge writes every minute, and one that's stopped leaves its last numbers in place
		say(f'no outcome from {url}, since its numbers are from {tick_to_text(reading)}')
		return None
	if silent:#every count zero is a gauge with nothing to count from, as in the minute after it starts, which says nothing about the tracker
		say(f'no outcome from {url}, since it counted nothing at all')
		return None
	if count > 0: return result('up')
	return result('timeout', f'page.json counted no UDP announces over IPv{family} in its last minute')#what a client sees of a UDP tracker that's down, or one the circuit breaker has paused

def read_certificates(database, stamp, reachable):#once a day, the certificate of each HTTPS and WebSocket tracker this family can reach: when it was issued, when it expires, and who signed it, read over a connection of our own and checked the way a client checks it
	today = stamp - stamp % Time.DAY
	ids = dict(database.execute('select url, id from trackers'))
	done = {row[0] for row in database.execute('select tracker from certificates where day = ? and family = ?', (today, family))}
	secure = [tracker for tracker in reachable if tracker.startswith(('https://', 'wss://')) and ids[tracker] not in done]
	if not secure: return
	def read(tracker):
		parts = urllib.parse.urlsplit(tracker)
		with socket.create_connection((parts.hostname, parts.port or 443), timeout=certificate_limit/Time.SECOND) as connection:#socket counts in seconds, and gives each step this long
			with ssl.create_default_context().wrap_socket(connection, server_hostname=parts.hostname) as secured:
				certificate = secured.getpeercert()
		issuer = dict(pair[0] for pair in certificate['issuer'])#like {'organizationName': "Let's Encrypt", 'commonName': 'R11'}
		name = issuer.get('organizationName', '')
		if 'commonName' in issuer: name = f"{name} ({issuer['commonName']})".strip()#like Let's Encrypt (R11)
		return (ssl.cert_time_to_seconds(certificate['notBefore'])*Time.SECOND, ssl.cert_time_to_seconds(certificate['notAfter'])*Time.SECOND, name, '')#ssl counts in seconds
	asked = at_once(read, secure, 2*certificate_limit)#time to connect, and then to finish the handshake
	rows = []
	for tracker, answer in asked.items():
		if answer.cancelled() or not answer.done(): continue#no row, so the next pass tries again
		try:
			rows.append((today, family, ids[tracker], *answer.result()))
		except (OSError, ValueError, KeyError) as e:#a certificate that doesn't verify, or a connection that fails; an ordinary answer here, so this is where those exceptions stop
			rows.append((today, family, ids[tracker], None, None, '', str(e)))
	with database:
		database.executemany('insert or replace into certificates (day, family, tracker, issued, expires, issuer, error) values (?, ?, ?, ?, ?, ?, ?)', rows)

def run(database, stamp):#one pass: gather the candidates, check them all, and record what came back under this hour, and how long the pass took
	began = tick()
	local = local_address()
	if local is None:
		say('no route in this family, so nothing was checked this hour')
		return
	candidates = gather(database, stamp)
	paired = {tracker: read_gauge(gauges[tracker]) for tracker in candidates if tracker in gauges}#read first, near the top of the hour, ahead of the pass
	announced = [tracker for tracker in candidates if tracker not in gauges]
	results = lookup(announced)#the trackers this family can't reach, decided before announcing
	reachable = [tracker for tracker in announced if tracker not in results]
	read_certificates(database, stamp, reachable)
	results.update(check(reachable, local))
	results.update({tracker: reading for tracker, reading in paired.items() if reading})#a page that couldn't speak records nothing for its tracker, which counts against no one
	ids = dict(database.execute('select url, id from trackers'))
	with database:
		database.executemany('insert or replace into checks (hour, family, tracker, outcome, message, extra_peers, tries, interval, warning) values (?, ?, ?, ?, ?, ?, ?, ?, ?)', [(stamp, family, ids[tracker], *values) for tracker, values in results.items()])
		database.execute('insert or replace into passes (hour, family, began, ended) values (?, ?, ?, ?)', (stamp, family, began, tick()))
	up = sum(1 for outcome, *_ in results.values() if outcome == 'up')
	say(f'checked {len(results)} trackers in {(tick() - began)//Time.SECOND} seconds, and {up} were up')

# report

void_hours = '''
	with void as (select hour, family from checks where hour between ? and ? group by hour, family having sum(outcome = 'up') = 0)
'''#an hour in which every tracker in a family failed, so the trouble was at this end and list's monitor had no reading, which the queries below count as no reading for every tracker in that family. It takes a first and last hour, and reads the checks in the order of their key, hour first, so it sorts nothing

counted = void_hours + '''
	select tracker, family, case when (hour, family) in (select hour, family from void) then 'no_reading' else outcome end, count(*), coalesce(sum(outcome = 'up' and tries > 1), 0)
	from checks where hour between ? and ?
	group by 1, 2, 3'''#each tracker's hours of each outcome from a first hour through a last, void hours as no reading, and how many of its up hours needed a second try, which is none on checks from before tries were recorded; it takes the two hours twice, once for the void hours and once for the checks

def tally(database, stamp):#fold each day that ended before the stamp's day into daily, once. Totaling a year of hours every hour would sort millions of rows; a tally is one row per tracker, family, day, and outcome, and a finished day never changes
	today = stamp - stamp % Time.DAY
	last = database.execute('select max(day) from daily').fetchone()[0]
	if last is None:#nothing tallied yet, so start from the first hour ever checked
		first = database.execute('select min(hour) from checks').fetchone()[0]
		if first is None: return
		d = first - first % Time.DAY
	else:
		d = last + Time.DAY
	while d < today:
		with database:#a day at a time, each its own transaction
			database.execute('insert into daily (day, tracker, family, outcome, hours, retried) select ?, * from (' + counted + ')', (d, *(d, d + Time.DAY - Time.HOUR)*2))#the day itself, then its first and last hours, twice
		d += Time.DAY

def totals(database, stamp):#for each tracker and family, its hours of each outcome over every horizon, as {(tracker, family): {horizon: {outcome: hours}}}, leaving out the hours it has no record of, which publish counts as no reading, and beside them, the up hours that needed a second try, as {(tracker, family): {horizon: hours}}. A horizon's whole days come from the tally, and its two partial ones, its first day and today, from the checks
	today = stamp - stamp % Time.DAY
	totals_of = {}
	retried_of = {}
	def add(key, horizon, outcome, hours, retried):
		counts = totals_of.setdefault(key, {}).setdefault(horizon, {})
		counts[outcome] = counts.get(outcome, 0) + hours
		retried_in = retried_of.setdefault(key, {})
		retried_in[horizon] = retried_in.get(horizon, 0) + retried
	def add_checks(horizon, first, last):#each tracker's hours from first through last, from the checks themselves
		for tracker, f, outcome, hours, retried in database.execute(counted, (first, last)*2):
			add((tracker, f), horizon, outcome, hours, retried)

	whole = {}#each horizon longer than a day, to the first and last of the whole days it takes from the tally
	for horizon, length in horizons.items():
		first = stamp - length + Time.HOUR#the horizon's first hour
		if length <= Time.DAY:
			add_checks(horizon, first, stamp)
			continue
		start = first - first % Time.DAY#the day that first hour falls in
		add_checks(horizon, first, start + Time.DAY - Time.HOUR)#the rest of that first day
		add_checks(horizon, today, stamp)#and today so far
		whole[horizon] = (start + Time.DAY, today - Time.DAY)
	earliest = min((begin for begin, _ in whole.values()), default=today)#with no horizon longer than a day, the tally isn't needed
	for tracker, f, d, outcome, hours, retried in database.execute('select tracker, family, day, outcome, hours, retried from daily where day between ? and ?', (earliest, today - Time.DAY)):
		for horizon, (begin, end) in whole.items():
			if begin <= d <= end: add((tracker, f), horizon, outcome, hours, retried)
	return totals_of, retried_of

def strips(database, stamp):#for each tracker and family, its last strip_days days as one character each, oldest first, with today so far last: U when every hour with a reading was up, a digit when some were, how many tenths of them, 0 through 9, the letter of the outcome most of its hours had when none was up, like T or D, and Z when no hour had a reading; so a day's character is the same letter, and the page the same color, as the hours it sums
	today = stamp - stamp % Time.DAY
	first = today - (strip_days - 1)*Time.DAY
	counts = {}#(tracker, family) to {day: {outcome: hours}}
	def add(key, d, outcome, hours):
		hours_of = counts.setdefault(key, {}).setdefault(d, {})
		hours_of[outcome] = hours_of.get(outcome, 0) + hours
	for tracker, f, d, outcome, hours in database.execute('select tracker, family, day, outcome, hours from daily where day between ? and ?', (first, today - Time.DAY)):
		add((tracker, f), d, outcome, hours)
	for tracker, f, outcome, hours, _ in database.execute(counted, (today, stamp)*2):
		add((tracker, f), today, outcome, hours)
	def character(hours_of):
		checked = sum(hours for outcome, hours in hours_of.items() if outcome != 'no_reading')
		if checked == 0: return 'Z'
		up = hours_of.get('up', 0)
		if up == checked: return 'U'
		if up > 0: return str(up*10//checked)#a day with any hour down reads 9 at most, never U, and one with any hour up reads a digit, never a failure
		failed = {outcome: hours for outcome, hours in hours_of.items() if outcome not in ('up', 'no_reading')}
		return letters[max(failed, key=lambda outcome: (failed[outcome], depth[outcome]))]#of two outcomes with as many hours, the one that shows the tracker got further
	return {key: ''.join(character(by_day.get(first + i*Time.DAY, {})) for i in range(strip_days)) for key, by_day in counts.items()}

def uptime(hours):#a horizon's up hours as a share of the hours with a reading, from its {letter: hours}, or 0 when no hour had one
	awake = sum(hours.values()) - hours.get('Z', 0)
	return hours.get('U', 0)/awake if awake > 0 else 0

def write(name, text):#one public file, written beside its place and renamed over it, so a reader always gets a whole file, this hour's or the last
	temporary = os.path.join(folder, f'{name}.tmp')
	with open(temporary, 'w') as file:
		file.write(text)
	os.replace(temporary, os.path.join(public, name))

def publish(database, stamp):#write the given hour's report from what both containers recorded; an hour with nothing recorded writes nothing, leaving the last report in place
	rows = database.execute('select trackers.id, trackers.url, trackers.since, checks.family, checks.outcome from checks join trackers on trackers.id = checks.tracker where checks.hour = ?', (stamp,)).fetchall()
	if not rows:
		say(f'nothing was recorded in the hour {tick_to_text(stamp)}, so the last report stays')
		return
	tally(database, stamp)
	totals_of, retried_of = totals(database, stamp)
	days_of = strips(database, stamp)
	url_of = {tracker: url for tracker, url, _, _, _ in rows}
	span = (stamp - horizons[criterion_horizon] + Time.HOUR, stamp)#the first and last hours of the criterion's horizon, which the extra peers and the details look back over

	named = {}#tracker URL to the names of the sources upstream that name it now
	for source in upstream:
		for tracker in source.get('trackers', []): named.setdefault(tracker, set()).add(source['name'])
	for list_url, url in database.execute('select list_url, url from upstream'):
		named.setdefault(url, set()).add(named_by.get(list_url, list_url))

	extra = {}#tracker id to {family: (hours, most)}: how many hours over the criterion's horizon its reply listed peers for a hash no one else has, and the most it listed at once
	for tracker, f, hours, most in database.execute('select tracker, family, count(*), max(extra_peers) from checks where hour between ? and ? and extra_peers > 0 group by 1, 2', span):
		if tracker in url_of: extra.setdefault(tracker, {})[f] = (hours, most)#only the trackers in this hour's report
	extra_peers = []#the same, in the report's shape, by URL
	for tracker in sorted(extra, key=url_of.get):
		extra_peers.append({'url': url_of[tracker], **{f'ipv{f}': {'hours': hours, 'most': most} for f, (hours, most) in sorted(extra[tracker].items())}})

	intervals = {(tracker, f): value for tracker, f, value, _ in database.execute('select tracker, family, interval, max(hour) from checks where hour between ? and ? and interval is not null group by 1, 2', span)}#the latest interval each tracker asked for in each family, since SQLite gives the other columns from the row with the max()
	warnings = {(tracker, f): value for tracker, f, value, _ in database.execute("select tracker, family, warning, max(hour) from checks where hour between ? and ? and warning != '' group by 1, 2", span)}#and its latest warning
	certificates = {}#tracker id to its certificate as last read in the last week, in either family
	for tracker, issued, expires, issuer, error, _ in database.execute('select tracker, issued, expires, issuer, error, max(day) from certificates where day >= ? group by tracker', (stamp - Time.WEEK,)):
		if error: certificates[tracker] = {'error': error}
		else: certificates[tracker] = {'issued': tick_to_iso(issued)[:10], 'expires': tick_to_iso(expires)[:10], 'issuer': issuer}#UTC dates, like 2026-11-12

	passes = {}#how each family's pass went, a few numbers repeated at the top for a quick look at the hour
	for f in (4, 6):
		outcomes = [row[4] for row in rows if row[3] == f]
		up = outcomes.count('up')
		timing = database.execute('select began, ended from passes where hour = ? and family = ?', (stamp, f)).fetchone()
		passes[f'ipv{f}'] = {
			'checked': up > 0,#whether the hour's results in this family mean anything: false when its container recorded nothing or every tracker failed, and either way the hour counts against no tracker
			'trackers': len(outcomes),
			'up': up,
			'seconds': (timing[1] - timing[0])//Time.SECOND if timing else 0,#how long the pass took, or 0 when its container recorded nothing
		}

	def column(key):#one tracker's hours in one family over every horizon, in letters, with the hours it has no record of counted as no reading, so each horizon adds up to its length
		hours_of = {}
		for horizon, length in horizons.items():
			counts = dict(totals_of.get(key, {}).get(horizon, {}))
			counts['no_reading'] = counts.get('no_reading', 0) + length//Time.HOUR - sum(counts.values())
			hours_of[horizon] = {letter: counts[outcome] for outcome, letter in letters.items() if counts.get(outcome, 0) > 0}
		return hours_of

	def details(tracker):#what's known about one tracker beyond its hours, for a reader looking at it closely, leaving out anything empty
		found = {}
		for f in (4, 6):
			about = {}
			retried = {horizon: hours for horizon, hours in retried_of.get((tracker, f), {}).items() if hours > 0}
			if retried: about['retried'] = retried
			if (tracker, f) in intervals: about['interval'] = intervals[(tracker, f)]
			if (tracker, f) in warnings: about['warning'] = warnings[(tracker, f)]
			if about: found[f'ipv{f}'] = about
		if tracker in certificates: found['certificate'] = certificates[tracker]
		return found

	entries = {}#tracker id to its entry in the report, and whether it meets the criterion
	for tracker, url, since, f, outcome in rows:
		if tracker in entries: continue#a row for each family, and the entry covers both
		columns = {f'ipv{f}': column((tracker, f)) for f in (4, 6)}
		up_enough = any(uptime(hours_of[criterion_horizon]) >= criterion_uptime for hours_of in columns.values())#up often enough across the hours it was checked, however few, in at least one family
		honest = sum(hours for hours, _ in extra.get(tracker, {}).values()) <= criterion_extra_peers#and listing no peers that can't exist
		entry = {'url': url, 'since': since, 'upstream': sorted(named.get(url, set())), **columns, 'days': {f'ipv{f}': days_of.get((tracker, f), 'Z'*strip_days) for f in (4, 6)}}
		found = details(tracker)
		if found: entry['details'] = found
		entries[tracker] = (entry, up_enough and honest)

	def standing(entry):#how a tracker ranks: its better family's uptime over the criterion's horizon, then its URL, so the most dependable come first
		return (-max(uptime(entry[key][criterion_horizon]) for key in ('ipv4', 'ipv6')), entry['url'])
	trackers = sorted((entry for entry, meets in entries.values() if meets), key=standing)#the trackers that meet the criterion, the ones trackers.txt lists
	cut = sorted((entry for entry, meets in entries.values() if not meets), key=standing)#and the ones that didn't make the cut

	write('trackers.txt', ''.join(f'{entry["url"]}\n\n' for entry in trackers))#one URL per line with a blank line between, the format ngosang's lists are in and the tools that read them expect
	report = {
		'title': 'list.ftorrent.com uptime record of the public BitTorrent and WebTorrent trackers, fresh each hour',#for anyone reading the file by hand, the way open.ftorrent.com's page.json begins
		'about': site,#the page this file is the data behind
		'hour': stamp,
		'when': tick_to_iso(stamp),#the same hour, readable, like 2026-11-09T18:00:00.000Z
		'upstream': upstream,
		'downstream': downstream,
		'passes': passes,
		'criterion': {'horizon': criterion_horizon, 'uptime': criterion_uptime, 'extra_peers': criterion_extra_peers, 'candidates': len(entries), 'listed': len(trackers)},#the test a tracker passes to be listed in trackers.txt, how many were checked, and how many passed
		'trackers': trackers,
		'cut': cut,
		'extra_peers': extra_peers,#the trackers whose replies listed peers that can't exist, which is why one with a good uptime can be in cut
	}
	write('trackers.json', json.dumps(report, separators=(',', ':')) + '\n')#compact, since it carries hundreds of trackers
	say(f'published the hour {tick_to_text(stamp)}: {len(trackers)} of {len(entries)} trackers listed')

# main

def open_database():#the shared SQLite file, made on first use and brought forward to schema_version
	os.makedirs(public, exist_ok=True)
	database = sqlite3.connect(database_path, timeout=1*Time.MINUTE/Time.SECOND)#a writer that meets the other container's lock waits its turn, up to a minute, rather than failing; sqlite3 counts in seconds
	end = clock() + 1*Time.MINUTE
	while database.execute('pragma journal_mode').fetchone()[0] != 'wal':#readers and a writer at once, and a crash mid-write leaves the file whole; a file made by the other container is already in this mode, so only a new one switches
		try:
			database.execute('pragma journal_mode = wal')
		except sqlite3.OperationalError:#the switch needs the file to itself, which SQLite won't wait for while the other container, starting at the same moment, holds it; an ordinary answer, so try again for up to a minute
			if clock() > end: raise
			time.sleep(1/2)#time.sleep counts in seconds
	database.execute('pragma temp_store = memory')#the container's filesystem is read-only, so any sorting stays in memory

	database.execute('begin immediate')#the write lock, so of two containers starting together, one brings the file forward and the other waits, then finds it done
	version = database.execute('pragma user_version').fetchone()[0]
	if not database.execute("select count(*) from sqlite_master where type = 'table' and name = 'checks'").fetchone()[0]: version = schema_version#a new file, which the schema below makes whole at the current shape
	elif version == 0: version = 1#a file from before the database recorded its version
	if version > schema_version:#an older program never writes into a newer file
		database.rollback()
		say(f'the database is at version {version}, newer than the {schema_version} this program knows, so it stops here')
		raise SystemExit(1)
	for step in range(version + 1, schema_version + 1):
		say(f'bringing the database from version {step - 1} to {step}')
		for statement in migrations[step]: database.execute(statement)
	database.execute(f'pragma user_version = {schema_version}')
	database.commit()
	database.executescript(schema)#every table that doesn't exist yet, at the current shape
	return database

def main():
	database = open_database()
	if sys.argv[1:] == ['now']:#by hand: check this hour now, publish it if this copy reports, and stop, for trying the program without waiting for an hour to turn
		stamp = top(tick())
		run(database, stamp)
		if reporting: publish(database, stamp)
		return
	if sys.argv[1:] == ['publish']:#by hand: write the hour before this one again, from what's already recorded, and stop, so a change to the report's shape shows without waiting for the next hour or checking anything again
		publish(database, top(tick()) - Time.HOUR)
		return
	say(f'started; checking at the top of each hour{", and publishing the hour before" if reporting else ""}')
	while True:
		stamp = top(tick()) + Time.HOUR#the next hour
		time.sleep(max(0, (stamp - tick())/Time.SECOND))#sleep counts in seconds
		try:
			if reporting: publish(database, stamp - Time.HOUR)#the hour that just ended, which both containers finished recording about an hour ago
			run(database, stamp)
		except Exception:#noqa: the top gate, where anything that escaped is a bug to report; the next hour runs regardless
			say(f'this hour failed: {traceback.format_exc()}')

main()
