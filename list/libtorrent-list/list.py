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
# Each tracker gets a torrent of its own, a fresh random info hash that names no real torrent, in one libtorrent session listening on the container's one address. libtorrent announces to them all at once, metering its HTTP announces fifty at a time the way it does in any client, and the pass listens until every tracker has answered or failed. Each tracker's outcome for the hour is how far it got: its name resolved to an address in this family, something answered, an HTTPS or WebSocket tracker's certificate held, and the answer was a tracker's reply. A UDP or HTTP reply that lists other peers for a hash no one else has is still up, and the tracker is flagged: what that means isn't known, so it's recorded and published, and decides nothing. The session sends announces and nothing else: it never connects to the peers a reply lists, runs no DHT, and maps no ports.
#
# A UDP tracker paired with the page.json of the gauge beside it, as ftorrent's own is, gets no announce. Its outcome comes from that page instead: the count of UDP announces the tracker served over this container's family in the gauge's last minute, which are peers reaching it from the internet, the same claim every other outcome makes.
#
# Everything lives in one SQLite file in the data folder the two containers share: every check, every tracker ever named, each upstream list's latest copy, and a tally of each day that has ended, which the report's longer horizons are summed from instead of every hour again. SQLite lets one writer in at a time, which keeps the containers' writes apart on the rare hour they land together.
#
# The container with LIST_REPORT set also publishes. At the top of each hour, before checking, it writes the hour that just ended: trackers.txt, the trackers up in at least 95% of the hours they were checked over the last month, and trackers.json, every tracker's hours over six horizons, from the last hour to the last year. The files always describe one whole hour, about an hour behind, and never a mix of two. They're a record of uptime; a check of right now is the desktop client's job.

horizons = {#the stretches of time trackers.json totals each tracker's hours over, each ending with the hour it reports; every hour in one, for each tracker in each family, is exactly one outcome, so a horizon's hours always add up to its length
	'hour': Time.HOUR,
	'day': Time.DAY,
	'week': Time.WEEK,
	'month': 30*Time.DAY,#a fixed 30 days, where ergo's Time.MONTH is a twelfth of a year
	'quarter': 90*Time.DAY,
	'year': 365*Time.DAY,
}
criterion_horizon = 'month'#trackers.txt lists a tracker whose uptime over this horizon,
criterion_uptime = 0.95#is at least this, in at least one family: its up hours as a share of the hours it wasn't asleep, however few those are so far, so the list fills from the first hour and steadies as a month of records grows

letters = {'up': 'u', 'dns': 'd', 'no_address': 'n', 'timeout': 't', 'refused': 'r', 'cert': 'c', 'bad_reply': 'b', 'asleep': 'z'}#each outcome as one letter in trackers.json, in the order it lists them; asleep is an hour with no result, when list's own monitor wasn't watching or every tracker in a family failed at once, and the README's glossary spells out each one

family = 6 if os.environ.get('LIST_FAMILY') == '6' else 4#which family this container checks, set in the compose file
reporting = os.environ.get('LIST_REPORT') == '1'#whether this container also publishes, which the IPv4 one does
folder = os.environ.get('LIST_DIR', os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'data'))#the data folder both containers share, a volume; when run by hand, the data folder beside this program's folder, where the compose file's ./data is
database_path = os.path.join(folder, 'list.sqlite')
public = os.path.join(folder, 'public')#what the web server serves at list.ftorrent.com
listen_shortest = 45*Time.SECOND#a pass listens at least this long, past libtorrent's 30 seconds to give up on a tracker, so its retries can show how far a failing tracker gets
listen_longest = 15*Time.MINUTE#and past that, until every tracker has answered or failed, since a tracker queued behind dead ones gets its turn late, and cutting it off would record it as silent; this is only the stop for a pass that can't finish, well inside the hour
fetch_limit = 30*Time.SECOND#how long fetching one list may take
user_agent = f'ftorrent-list/0.1.0 libtorrent/{lt.version}'#how list introduces itself to trackers and to the lists it fetches, so an operator who sees it can find this project

site = 'https://list.ftorrent.com'#where the report is published, which trackers.json names so a copy of it says where the live one is

upstream = [#where the candidates come from, which trackers.json repeats at its top so a reader sees what was checked: each source's name, the page it lives at, its code, and the lists fetched from it every hour. Tracking the Trackers on docs.ftorrent.com says who keeps them and how each one checks
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
]
named_by = {list_url: source['name'] for source in upstream for list_url in source['lists']}#each list's URL to the name of its source
downstream = [f'{site}/trackers.txt', f'{site}/trackers.json']#what this publishes from them

ours = [#ftorrent's own trackers, checked every hour whatever the lists upstream say
	'udp://open.ftorrent.com:443/announce',
	'https://open.ftorrent.com/announce',
	'wss://open.ftorrent.com',
]
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
	primary key (hour, family, tracker)
);
create table if not exists daily (
	day integer not null,              -- the tick at the start of the UTC day
	family integer not null,
	tracker integer not null references trackers (id),
	outcome text not null,             -- an outcome, or asleep for the hours every tracker in the family failed
	hours integer not null,            -- how many of the day's hours had this outcome
	primary key (tracker, family, day, outcome)
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

# candidates

def normalize(text):#one spelling for each announce URL, so a tracker two lists name differently is one candidate: lowercase, without the port its scheme implies; None for anything that isn't an address of a kind we check
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

def fetch(url):#the tracker URLs one source names, or None when it can't be had this hour. A site that's down, slow, or garbled is an ordinary answer here, so this is where those exceptions stop
	request = urllib.request.Request(url, headers={'User-Agent': user_agent})
	try:
		with urllib.request.urlopen(request, timeout=fetch_limit/Time.SECOND) as response:#urllib counts in seconds
			text = response.read(4*Size.MB).decode('utf-8', 'replace')#a few hundred lines at most, so four megabytes means something's wrong, and reading stops there
	except (OSError, ValueError, http.client.HTTPException) as e:
		say(f'kept the last copy of {url}, since fetching it said: {e}')
		return None
	return [tracker for tracker in (normalize(line) for line in text.splitlines()) if tracker]

def gather(database, stamp):#this hour's candidates: each list fetched replaces its last copy, a list that couldn't be fetched keeps its last one, and ftorrent's own trackers are always in
	for list_url in named_by:
		found = fetch(list_url)
		if found is None: continue
		with database:#one transaction, so the other container never reads a list half replaced
			database.execute('delete from upstream where list_url = ?', (list_url,))
			database.executemany('insert or ignore into upstream (list_url, url) values (?, ?)', [(list_url, tracker) for tracker in found])
	candidates = {row[0] for row in database.execute('select url from upstream')} | set(ours)
	with database:
		database.executemany('insert or ignore into trackers (url, since) values (?, ?)', [(tracker, stamp) for tracker in candidates])
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

def judge(url, peers, failures):#one tracker's outcome for the hour, as (outcome, message, extra peers): up when it replied, or else the furthest of its failures; peers is how many its reply listed, and None when it never replied
	if peers is not None:#None rather than falsy, since a reply that lists no one, zero peers, is the honest one
		extra = 0
		if peers > 1 and not url.startswith('wss://'): extra = peers - 1#one peer is allowed, since some trackers list the announcer itself; a WebSocket tracker relays offers rather than listing peers, so only UDP and HTTP replies carry a count to read
		return ('up', '', extra)
	if not failures: return ('timeout', 'no answer', 0)#silent for the whole pass, with not even an error to report
	outcome, message = max(failures, key=lambda failure: depth[failure[0]])#max keeps the first of equals, so this is the earliest of the furthest
	return (outcome, message, 0)

def check(candidates, local):#announce once to every candidate from local, this container's own address, and judge what came back, as {tracker URL: (outcome, message, extra peers)}
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
		'alert_queue_size': 100000,#libtorrent tells the pass everything through a queue the loop below empties every half second, and drops what arrives once it's full, so a reply it dropped would read as a tracker that never answered; a pass's busiest moment, its opening burst, is about one alert per tracker, and the 2000 libtorrent starts with would fill at seven times today's lists, while 100000 costs nothing until it's used
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
		while clock() < started + listen_longest:
			if clock() > started + listen_shortest and len(replied | failures.keys()) == len(candidates): break#every tracker has answered or failed
			session.wait_for_alert(Time.SECOND//2)#libtorrent takes milliseconds here, and returns early as soon as an alert arrives
			for alert in session.pop_alerts():
				if isinstance(alert, lt.alerts_dropped_alert): say('libtorrent dropped alerts it had no room to queue, so some trackers this hour may read as silent')
				if not isinstance(alert, (lt.tracker_reply_alert, lt.tracker_error_alert)): continue
				tracker = tracker_of.get(str(alert.handle.info_hashes().v1))
				if tracker is None: continue#an alert about something other than our torrents
				if isinstance(alert, lt.tracker_reply_alert):
					replied.add(tracker)
				else:
					failures.setdefault(tracker, []).append(classify(alert))#libtorrent retries on its own, so a tracker can fail more than one way in a pass
		unheard = len(candidates) - len(replied | failures.keys())
		if unheard > 0: say(f'stopped listening after {listen_longest//Time.MINUTE} minutes, with {unheard} trackers still unheard')
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
	if count > 0: return ('up', '', 0)
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
	ids = dict(database.execute('select url, id from trackers'))
	with database:
		database.executemany('insert or replace into checks (hour, family, tracker, outcome, message, extra_peers) values (?, ?, ?, ?, ?, ?)', [(stamp, family, ids[tracker], outcome, message, extra) for tracker, (outcome, message, extra) in results.items()])
	up = sum(1 for outcome, _, _ in results.values() if outcome == 'up')
	say(f'checked {len(results)} trackers, and {up} were up')

# report

void_hours = '''
	with void as (select hour, family from checks where hour between ? and ? group by hour, family having sum(outcome = 'up') = 0)
'''#an hour in which every tracker in a family failed, so the trouble was at this end and list's monitor was as good as asleep; the queries below count it as asleep for every tracker in that family. Each passes its first and last hour, which this reads in the order of the checks' key, hour first, so it sorts nothing

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
			database.execute(void_hours + '''
				insert into daily (day, family, tracker, outcome, hours)
				select ?, family, tracker, case when (hour, family) in (select hour, family from void) then 'asleep' else outcome end, count(*)
				from checks where hour between ? and ?
				group by family, tracker, 4''', (d, d + Time.DAY - Time.HOUR, d, d, d + Time.DAY - Time.HOUR))#the day's first and last hours for the void hours, the day itself, and its hours again for the checks
		d += Time.DAY

def totals(database, stamp):#for each tracker and family, its hours of each outcome over every horizon, as {(tracker, family): {horizon: {outcome: hours}}}, leaving out the hours it has no record of, which publish counts as asleep. A horizon's whole days come from the tally, and its two partial ones, its first day and today, from the checks
	today = stamp - stamp % Time.DAY
	result = {}
	def add(key, horizon, outcome, hours):
		counts = result.setdefault(key, {}).setdefault(horizon, {})
		counts[outcome] = counts.get(outcome, 0) + hours
	def add_checks(horizon, first, last):#each tracker's hours from first through last, from the checks themselves, with void hours as asleep
		for tracker, f, outcome, hours in database.execute(void_hours + '''
			select tracker, family, case when (hour, family) in (select hour, family from void) then 'asleep' else outcome end, count(*)
			from checks where hour between ? and ?
			group by 1, 2, 3''', (first, last)*2):#the two hours twice: once for the void hours, once for the checks
			add((tracker, f), horizon, outcome, hours)

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
	for tracker, f, d, outcome, hours in database.execute('select tracker, family, day, outcome, hours from daily where day between ? and ?', (earliest, today - Time.DAY)):
		for horizon, (begin, end) in whole.items():
			if begin <= d <= end: add((tracker, f), horizon, outcome, hours)
	return result

def uptime(hours):#a horizon's up hours as a share of the hours that weren't asleep, from its {letter: hours}, or 0 when every hour was asleep
	awake = sum(hours.values()) - hours.get('z', 0)
	return hours.get('u', 0)/awake if awake > 0 else 0

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
	totals_of = totals(database, stamp)
	named = {}#tracker URL to the names of the sources upstream that name it now
	for list_url, url in database.execute('select list_url, url from upstream'):
		named.setdefault(url, set()).add(named_by.get(list_url, list_url))
	extra = {row[0] for row in database.execute('select distinct tracker from checks where hour between ? and ? and extra_peers > 0', (stamp - horizons['month'] + Time.HOUR, stamp))}#the trackers whose reply, at least once in the last 30 days, listed peers for a hash no one else has
	checked = {f'ipv{f}': any(row[4] == 'up' for row in rows if row[3] == f) for f in (4, 6)}#whether this hour's results in each family mean anything: true when that family's container recorded the hour and at least one tracker answered, false when it recorded nothing or every tracker failed

	def column(key):#one tracker's hours in one family over every horizon, in letters, with the hours it has no record of counted as asleep, so each horizon adds up to its length
		hours_of = {}
		for horizon, length in horizons.items():
			counts = dict(totals_of.get(key, {}).get(horizon, {}))
			counts['asleep'] = counts.get('asleep', 0) + length//Time.HOUR - sum(counts.values())
			hours_of[horizon] = {letter: counts[outcome] for outcome, letter in letters.items() if counts.get(outcome, 0) > 0}
		return hours_of

	trackers = {}#tracker id to its entry in the report
	for tracker, url, since, f, outcome in rows:
		if tracker in trackers: continue#a row for each family, and the entry covers both
		columns = {f'ipv{f}': column((tracker, f)) for f in (4, 6)}
		trackers[tracker] = {
			'url': url,
			'since': since,
			'upstream': sorted(named.get(url, set())),
			'listed': any(uptime(hours_of[criterion_horizon]) >= criterion_uptime for hours_of in columns.values()),#up often enough across the hours it was checked, however few, in at least one family
			'extra_peers': tracker in extra,
			**columns,
		}

	def standing(entry):#how a tracker ranks: its better family's uptime over the criterion's horizon, then its URL, so the most dependable come first
		return (-max(uptime(entry[key][criterion_horizon]) for key in ('ipv4', 'ipv6')), entry['url'])
	ordered = sorted(trackers.values(), key=standing)
	listed = [entry['url'] for entry in ordered if entry['listed']]

	write('trackers.txt', ''.join(f'{url}\n\n' for url in listed))#one URL per line with a blank line between, the format ngosang's lists are in and the tools that read them expect
	report = {
		'title': 'list.ftorrent.com uptime record of the public BitTorrent and WebTorrent trackers, fresh each hour',#for anyone reading the file by hand, the way open.ftorrent.com's page.json begins
		'about': site,#the page this file is the data behind
		'hour': stamp,
		'when': tick_to_iso(stamp),#the same hour, readable, like 2026-11-09T18:00:00.000Z
		'upstream': upstream,
		'downstream': downstream,
		'checked': checked,
		'criterion': {'horizon': criterion_horizon, 'uptime': criterion_uptime},#the one test a tracker passes to be listed in trackers.txt
		'trackers': ordered,
	}
	write('trackers.json', json.dumps(report, separators=(',', ':')) + '\n')#compact, since it carries hundreds of trackers
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
