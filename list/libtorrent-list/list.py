import os
import sys
import json
import time
import errno
import socket
import secrets
import sqlite3
import tempfile
import traceback
import http.client
import urllib.parse
import urllib.request
import libtorrent as lt
from zootella.ergo import Size, Time, has_tick, tick_to_iso, tick_to_text#ergo's units, so a duration or a size says what it is, and its tick, a moment as milliseconds since 1970, with a check for one from outside and two ways to write one

# The tracker checker at list.ftorrent.com, and this file is its whole program.
#
# Every hour it announces once to each public BitTorrent and WebTorrent tracker it knows of, and records what came back. Two containers run it, built from this folder, one on a network with only IPv4 and one on a network with only IPv6, so libtorrent has no choice of family for any tracker, WebSocket ones included, and each container's results are its family's. LIST_FAMILY tells the program which family it's in.
#
# The candidates come from public lists other people keep, fetched every hour: ngosang's list of all trackers and its list of WebSocket ones, and newTrackon's full list, plus ftorrent's own three. A list that can't be fetched keeps its last good copy, so one site down for an hour doesn't empty the candidates.
#
# Each tracker gets a torrent of its own, a fresh random info hash that names no real torrent, in one libtorrent session listening on the container's one address. libtorrent announces to them all at once, metering its HTTP announces fifty at a time the way it does in any client, and each tracker's outcome for the hour is how far it got: its name resolved to an address in this family, something answered, an HTTPS or WebSocket tracker's certificate held, and the answer was a tracker's reply. A UDP or HTTP reply that lists other peers for a hash no one else has is still good, and flagged: what that means isn't known, so it's recorded and published, and decides nothing. The session sends announces and nothing else: it never connects to the peers a reply lists, runs no DHT, and maps no ports.
#
# A UDP tracker paired with the page.json of the gauge beside it, as ftorrent's own is, gets no announce. Its outcome comes from that page instead: the count of UDP announces the tracker served over this container's family in the gauge's last minute, which are peers reaching it from the internet, the same claim every other outcome makes.
#
# Everything lives in one SQLite file in the data folder the two containers share: every check, every tracker ever named, and each list's latest copy. SQLite lets one writer in at a time, which keeps the containers' writes apart on the rare hour they land together.
#
# The container with LIST_REPORT set also publishes. At the top of each hour, before checking, it writes the hour that just ended: trackers.txt, the trackers that have been dependable for the last month, and trackers.json, every tracker's outcome that hour and its history by day. The files always describe one whole hour, about an hour behind, and never a mix of two. They're a record of uptime; a check of right now is the desktop client's job.

listing_window = 30*Time.DAY#trackers.txt lists a tracker that's been a candidate for this whole stretch,
listing_share = 0.95#and was good for at least this share of the hours it was checked in it, in at least one family
history = 18*Time.MONTH#how far back trackers.json gives each tracker's days

letters = {'good': 'g', 'dns': 'd', 'no_address': 'n', 'timeout': 't', 'refused': 'r', 'cert': 'c', 'bad_reply': 'b', 'void': 'v'}#each outcome as one letter in trackers.json, which carries hundreds of trackers and months of days; the README's glossary spells each one out, and x counts the hours a good reply listed extra peers

family = 6 if os.environ.get('LIST_FAMILY') == '6' else 4#which family this container checks, set in the compose file
reporting = os.environ.get('LIST_REPORT') == '1'#whether this container also publishes, which the IPv4 one does
folder = os.environ.get('LIST_DIR', os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data'))#the data folder both containers share, a volume; when run by hand, the data folder beside this program's folder, where the compose file's ./data is
database_path = os.path.join(folder, 'list.sqlite')
public = os.path.join(folder, 'public')#what the web server serves at list.ftorrent.com
wait = 45*Time.SECOND#how long a pass listens for replies, comfortably longer than libtorrent's 30 seconds to give up on a tracker
fetch_limit = 30*Time.SECOND#how long fetching one list may take
user_agent = f'ftorrent-list/0.1.0 libtorrent/{lt.version}'#how list introduces itself to trackers and to the lists it fetches, so an operator who sees it can find this project

lists = {#where the candidates come from, by the name trackers.json gives each; Tracking the Trackers on docs.ftorrent.com says who keeps them and how each one checks
	'https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all.txt': 'ngosang',
	'https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_ws.txt': 'ngosang',
	'https://newtrackon.com/api/all': 'newtrackon',
}
ours = [#ftorrent's own trackers, checked every hour whatever the lists say
	'udp://open.ftorrent.com:443/announce',
	'https://open.ftorrent.com/announce',
	'wss://open.ftorrent.com',
]
gauges = {#a UDP tracker whose outcome comes from its gauge's page.json rather than an announce: good when the tracker served UDP announces over this container's family in the gauge's last minute, a timeout when it served none; pair your own tracker with your own gauge's page the same way
	'udp://open.ftorrent.com:443/announce': 'https://open.ftorrent.com/page.json',
}

schema = '''
create table if not exists trackers (
	id integer primary key,
	address text unique not null,      -- the announce address, in the one spelling normalize() gives it
	first_seen integer not null        -- the tick of the hour it first became a candidate
);
create table if not exists listings (
	list text not null,                -- the address of the list that named it
	address text not null,
	primary key (list, address)
);
create table if not exists checks (
	hour integer not null,             -- the tick at the top of the hour the check belongs to
	family integer not null,           -- 4 or 6
	tracker integer not null references trackers (id),
	outcome text not null,             -- good, or how far it got: dns, no_address, timeout, refused, cert, or bad_reply
	message text not null,             -- libtorrent's words for what went wrong, blank when good
	extra_peers integer not null,      -- peers a good UDP or HTTP reply listed beyond the one it may, for a hash no one else has
	primary key (hour, family, tracker)
);
'''

def say(line):#one line to the container's log, tagged with the family so the two containers' logs read apart
	print(f'list{family}: {line}', flush=True)

def tick():#now, as a tick, ergo's moment: milliseconds since 1970
	return int(time.time()*Time.SECOND)

def clock():#a reading of the monotonic clock, in milliseconds, for timing a pass without the wall clock jumping under it
	return time.monotonic()*Time.SECOND

def top(t):#the tick at the top of the hour a tick falls in
	return t - t % Time.HOUR

def day(t):#the date of the day a tick falls in, in UTC, like 2026-11-09
	return tick_to_iso(t)[:10]#the date part of ergo's ISO form, like 2026-11-09T07:00:00.000Z

# candidates

def normalize(text):#one spelling for each announce address, so a tracker two lists name differently is one candidate: lowercase, without the port its scheme implies; None for anything that isn't an address of a kind we check
	try:
		parts = urllib.parse.urlsplit(text.strip())
		port = parts.port#reading it checks it, and an impossible port raises
	except ValueError:
		return None
	scheme = parts.scheme.lower()
	if scheme not in ('udp', 'http', 'https', 'wss') or not parts.hostname: return None
	host = f'[{parts.hostname}]' if ':' in parts.hostname else parts.hostname#lowercase already; an IPv6 literal takes its brackets back
	if port is not None and not (scheme in ('https', 'wss') and port == 443) and not (scheme == 'http' and port == 80): host += f':{port}'
	query = f'?{parts.query}' if parts.query else ''
	return f'{scheme}://{host}{parts.path}{query}'

def fetch(url):#one list's addresses, or None when it can't be had this hour. A site that's down, slow, or garbled is an ordinary answer here, so this is where those exceptions stop
	request = urllib.request.Request(url, headers={'User-Agent': user_agent})
	try:
		with urllib.request.urlopen(request, timeout=fetch_limit/Time.SECOND) as response:#urllib counts in seconds
			text = response.read(4*Size.MB).decode('utf-8', 'replace')#a few hundred lines at most, so four megabytes means something's wrong, and reading stops there
	except (OSError, ValueError, http.client.HTTPException) as e:
		say(f'kept the last copy of {url}, since fetching it said: {e}')
		return None
	return [address for address in (normalize(line) for line in text.splitlines()) if address]

def gather(database, stamp):#this hour's candidates: each list fetched replaces its last copy, a list that couldn't be fetched keeps its last one, and ftorrent's own trackers are always in
	for url in lists:
		found = fetch(url)
		if found is None: continue
		with database:#one transaction, so the other container never reads a list half replaced
			database.execute('delete from listings where list = ?', (url,))
			database.executemany('insert or ignore into listings (list, address) values (?, ?)', [(url, address) for address in found])
	candidates = {row[0] for row in database.execute('select address from listings')} | set(ours)
	with database:
		database.executemany('insert or ignore into trackers (address, first_seen) values (?, ?)', [(address, stamp) for address in candidates])
	return sorted(candidates)

# checks

def local_address():#the address this container sends from in its family: connecting a UDP socket sends nothing, and the system picks the route, the way the desktop client's net_local asks; None when the family has no route
	probe = '2001:db8::1' if family == 6 else '192.0.2.1'#addresses set aside for documentation, which name no one
	try:
		with socket.socket(socket.AF_INET6 if family == 6 else socket.AF_INET, socket.SOCK_DGRAM) as s:
			s.connect((probe, 9))
			return s.getsockname()[0]
	except OSError:#a container whose network has no route in this family
		return None

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

def judge(address, peers, failures):#one tracker's outcome for the hour, as (outcome, message, extra peers): good when it replied, or else the furthest of its failures; peers is how many its reply listed, and None when it never replied
	if peers is not None:#None rather than falsy, since a reply that lists no one, zero peers, is the honest one
		extra = 0
		if peers > 1 and not address.startswith('wss://'): extra = peers - 1#one peer is allowed, since some trackers list the announcer itself; a WebSocket tracker relays offers rather than listing peers, so only UDP and HTTP replies carry a count to read
		return ('good', '', extra)
	if not failures: return ('timeout', 'no answer', 0)#silent for the whole pass, with not even an error to report
	outcome, message = max(failures, key=lambda failure: depth[failure[0]])#max keeps the first of equals, so this is the earliest of the furthest
	return (outcome, message, 0)

def check(candidates, local):#announce once to every candidate from local, this container's own address, and judge what came back, as {tracker address: (outcome, message, extra peers)}
	host = f'[{local}]' if ':' in local else local
	session = lt.session({
		'listen_interfaces': f'{host}:{49152 + secrets.randbelow(16384)}',#the one address, at a port picked fresh each pass from the range IANA sets aside for this, 49152 through 65535; announces carry the port, and nothing ever connects to it, since nothing comes into the container
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
	})
	tracker_of = {}#each torrent's info hash, in the hex libtorrent prints, to its tracker
	replied = set()#the trackers that sent a reply
	failures = {}#tracker to every (outcome, message) its errors showed this pass
	with tempfile.TemporaryDirectory() as scratch:#libtorrent wants somewhere to save a torrent's files, and these torrents never have any
		started = clock()
		handles = []
		for tracker in candidates:
			info_hash = lt.sha1_hash(secrets.token_bytes(20))#a hash no real torrent has, so no one else announces it
			params = lt.add_torrent_params()
			params.info_hashes = lt.info_hash_t(info_hash)
			params.trackers = [tracker]
			params.save_path = scratch
			params.flags &= ~lt.torrent_flags.auto_managed#every torrent active at once, rather than libtorrent's queue starting a few at a time
			params.flags &= ~lt.torrent_flags.paused
			handles.append(session.add_torrent(params))
			tracker_of[str(info_hash)] = tracker
		while clock() < started + wait:
			session.wait_for_alert(Time.SECOND//2)#libtorrent takes milliseconds here, and returns early as soon as an alert arrives
			for alert in session.pop_alerts():
				if not isinstance(alert, (lt.tracker_reply_alert, lt.tracker_error_alert)): continue
				tracker = tracker_of.get(str(alert.handle.info_hashes().v1))
				if tracker is None: continue#an alert about something other than our torrents
				if isinstance(alert, lt.tracker_reply_alert):
					replied.add(tracker)
				else:
					failures.setdefault(tracker, []).append(classify(alert))#libtorrent retries on its own, so a tracker can fail more than one way in a pass
		peers = {tracker_of[str(handle.info_hashes().v1)]: handle.status().list_peers for handle in handles}#the peers libtorrent kept from each reply, which leaves out an address of nothing or a port of zero that some trackers pad a reply with; the reply's own count, num_peers, includes those
		for handle in handles: session.remove_torrent(handle)#which tells each tracker that answered that the announcer is leaving, so the random hashes don't linger in its memory
		end = clock() + 3*Time.SECOND#a moment for those goodbyes to go out before the session closes
		while clock() < end:
			session.wait_for_alert(Time.SECOND//2)
			session.pop_alerts()
	return {tracker: judge(tracker, peers[tracker] if tracker in replied else None, failures.get(tracker)) for tracker in candidates}

def read_gauge(url):#a paired UDP tracker's outcome for the hour from its gauge's page.json, as (outcome, message, extra peers), or None when the page can't speak for this hour. A page that can't be fetched or read is an ordinary answer here, so this is where those exceptions stop
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
	if count > 0: return ('good', '', 0)
	return ('timeout', f'page.json counted no UDP announces over IPv{family} in its last minute', 0)#what a client sees of a UDP tracker that's down, or one the circuit breaker has paused

def run(database, stamp):#one pass: gather the candidates, check them all, and record what came back under this hour
	local = local_address()
	if local is None:
		say('no route in this family, so nothing was checked this hour')
		return
	candidates = gather(database, stamp)
	paired = {tracker: read_gauge(gauges[tracker]) for tracker in candidates if tracker in gauges}#read first, near the top of the hour, ahead of the pass
	results = check([tracker for tracker in candidates if tracker not in gauges], local)
	results.update({tracker: outcome for tracker, outcome in paired.items() if outcome})#a page that couldn't speak records nothing for its tracker, which counts against no one
	ids = dict(database.execute('select address, id from trackers'))
	with database:
		database.executemany('insert or replace into checks (hour, family, tracker, outcome, message, extra_peers) values (?, ?, ?, ?, ?, ?)', [(stamp, family, ids[tracker], outcome, message, extra) for tracker, (outcome, message, extra) in results.items()])
	good = sum(1 for outcome, _, _ in results.values() if outcome == 'good')
	say(f'checked {len(results)} trackers, and {good} were good')

# report

void_hours = '''
	with void as (select hour, family from checks where hour > ? and hour <= ? group by hour, family having sum(outcome = 'good') = 0)
'''#an hour in which every tracker in a family failed is void for that family, since the trouble was at this end; the queries below leave it out of every share and show it as void in the days

def shares(database, stamp):#for each tracker and family, the share of hours it was good among the hours it was checked over the listing window, void hours left out
	rows = database.execute(void_hours + '''
		select tracker, family, sum(outcome = 'good'), count(*) from checks
		where hour > ? and hour <= ? and (hour, family) not in (select hour, family from void)
		group by tracker, family''', (stamp - listing_window, stamp)*2)#the window twice: once for the void hours, once for the shares
	return {(tracker, f): good/checked for tracker, f, good, checked in rows}

def days(database, stamp):#for each tracker and family, its history over the stretch trackers.json covers: runs of days, each as [first date, how many days, {outcome: hours}], with identical days run together and outcomes with no hours left out
	rows = database.execute(void_hours + '''
		select tracker, family, hour - hour % ?,
			case when (hour, family) in (select hour, family from void) then 'void' else outcome end,
			count(*), sum(extra_peers > 0)
		from checks where hour > ? and hour <= ?
		group by 1, 2, 3, 4 order by 1, 2, 3''', (stamp - history, stamp, Time.DAY, stamp - history, stamp))#the stretch for the void hours, the length of a day to find each hour's, and the stretch again for the checks
	counts = {}#(tracker, family) to {day tick: {outcome: hours}}
	for tracker, f, d, outcome, hours, flagged in rows:
		hours_of = counts.setdefault((tracker, f), {}).setdefault(d, {})
		hours_of[letters[outcome]] = hours
		if flagged > 0: hours_of['x'] = hours_of.get('x', 0) + flagged#hours whose good reply listed peers that shouldn't exist, beside the outcomes rather than among them, since those hours were good
	runs = {}
	for key, by_day in counts.items():
		run_list = []
		previous = None
		for d in sorted(by_day):
			if run_list and run_list[-1][2] == by_day[d] and d == previous + Time.DAY:#the same counts as the day before, so the run grows
				run_list[-1][1] += 1
			else:
				run_list.append([day(d), 1, by_day[d]])
			previous = d
		runs[key] = run_list
	return runs

def write(name, text):#one public file, written beside its place and renamed over it, so a reader always gets a whole file, this hour's or the last
	temporary = os.path.join(folder, f'{name}.tmp')
	with open(temporary, 'w') as file:
		file.write(text)
	os.replace(temporary, os.path.join(public, name))

def publish(database, stamp):#write the given hour's report from what both containers recorded; an hour with nothing recorded writes nothing, leaving the last report in place
	rows = database.execute('select trackers.id, trackers.address, trackers.first_seen, checks.family, checks.outcome, checks.message, checks.extra_peers from checks join trackers on trackers.id = checks.tracker where checks.hour = ?', (stamp,)).fetchall()
	if not rows:
		say(f'nothing was recorded in the hour {tick_to_text(stamp)}, so the last report stays')
		return
	share = shares(database, stamp)
	history_of = days(database, stamp)
	named = {}#tracker address to the names of the lists that name it now
	for url, address in database.execute('select list, address from listings'):
		named.setdefault(address, set()).add(lists.get(url, url))
	families = {}
	for f in (4, 6):
		outcomes = [row[4] for row in rows if row[3] == f]
		families[f'ipv{f}'] = 'missing' if not outcomes else 'void' if 'good' not in outcomes else 'checked'#missing when that container recorded nothing, void when every tracker failed

	trackers = {}#tracker id to its entry in the report
	for tracker, address, first_seen, f, outcome, message, extra in rows:
		entry = trackers.setdefault(tracker, {'address': address, 'since': first_seen, 'lists': sorted(named.get(address, set()) | ({'ftorrent'} if address in ours else set())), 'listed': False, 'stack': 'neither', 'ipv4': None, 'ipv6': None})
		column = {'outcome': letters[outcome]}
		if message: column['message'] = message
		if extra > 0: column['extra_peers'] = extra
		column['share'] = round(share.get((tracker, f), 0), 4)
		column['days'] = history_of.get((tracker, f), [])
		entry[f'ipv{f}'] = column
		if first_seen <= stamp - listing_window + Time.HOUR and share.get((tracker, f), 0) >= listing_share: entry['listed'] = True#a candidate since the window's first hour, and good often enough across it

	for entry in trackers.values():#which families the tracker's name had an address in this hour, whether or not anything answered there
		addressed = [f for f in (4, 6) if entry[f'ipv{f}'] and entry[f'ipv{f}']['outcome'] not in ('d', 'n')]
		entry['stack'] = 'dual' if len(addressed) == 2 else f'ipv{addressed[0]}' if addressed else 'neither'

	def standing(entry):#how a tracker ranks: its better family's share, then its address, so the most dependable come first
		return (-max((entry[key]['share'] for key in ('ipv4', 'ipv6') if entry[key]), default=0), entry['address'])
	ordered = sorted(trackers.values(), key=standing)
	listed = [entry['address'] for entry in ordered if entry['listed']]

	write('trackers.txt', ''.join(f'{address}\n\n' for address in listed))#one address per line with a blank line between, the format ngosang's lists are in and the tools that read them expect
	write('trackers.json', json.dumps({'hour': stamp, 'families': families, 'listing': {'window': listing_window, 'share': listing_share}, 'trackers': ordered}, separators=(',', ':')) + '\n')#compact, since its days make it the large file
	say(f'published the hour {tick_to_text(stamp)}: {len(listed)} of {len(ordered)} trackers listed')

# main

def open_database():#the shared SQLite file, made on first use
	os.makedirs(public, exist_ok=True)
	database = sqlite3.connect(database_path, timeout=1*Time.MINUTE/Time.SECOND)#a writer that meets the other container's lock waits its turn, up to a minute, rather than failing; sqlite3 counts in seconds
	database.execute('pragma journal_mode = wal')#readers and a writer at once, and a crash mid-write leaves the file whole
	database.execute('pragma temp_store = memory')#the container's filesystem is read-only, so any sorting stays in memory
	database.executescript(schema)
	return database

def main():
	database = open_database()
	if sys.argv[1:] == ['now']:#by hand: check this hour now, publish it if this copy reports, and stop, for trying the program without waiting for an hour to turn
		stamp = top(tick())
		run(database, stamp)
		if reporting: publish(database, stamp)
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
