#!/usr/bin/env python3
import json
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

# This is the first of three scripts, and the only one that touches the network. It pulls web traffic data from five public sources into the data folder beside it, exactly as each source served it, and writes a manifest saying what came from where and when. The second script, table.py, reads only what this one wrote and never the network, so a reader can re-run the arithmetic against our frozen pulls and get our exact numbers, or against a fresh pull and get their own. The third, chart.py, draws what table.py computed.
#
# Four of the five sources need nothing but an HTTPS connection: Google's IPv6 adoption file, APNIC Labs' bulk IPv6 measurement, StatCounter's platform and operating system shares, and Wikimedia's pageview counts. Each of those pulls carries its whole history in one file, so they are fetched once per run regardless of how many months are asked for. The fifth, Cloudflare Radar, answers one question per request, so it is pulled month by month, and it needs a free API token, which this script reads from the RADAR_TOKEN environment variable, or from a .env file beside it holding a RADAR_TOKEN= line. When neither is set the script still pulls the other four and prints how to make the token.
#
# The arguments are one month, python3 fetch.py 2026-09, or a range of months, python3 fetch.py 2023-05 2026-09. A Cloudflare file already in the data folder is kept rather than fetched again, so extending the series by a month later costs only that month's requests; delete a file to force it fresh. Every pull is one small function with the URL written out in full, so the list of what this report reads is the list of functions below. Nothing here interprets the data; that is table.py's job.

months = []#the calendar months this run covers, as YYYY-MM strings in order, set in main from the command line
folder = Path(__file__).resolve().parent/'data'#where every pull lands
environment_path = Path(__file__).resolve().parent/'.env'#where the Cloudflare API token lives when it isn't in the environment: a RADAR_TOKEN= line in a .env file that is never committed
user_agent = 'ftorrent-report (https://github.com/zootella/ftorrent)'#who we say we are; Wikimedia requires a descriptive one and the rest deserve it
manifest = {}#one entry per file in the data folder, written to data/fetched.json at the end: url, when, bytes, status; entries for files kept from an earlier run are carried over so their dates stay true

def check_month(s):#the month must look like 2026-09; anything else is a mistake in the command line, caught here
	if not re.fullmatch(r'\d{4}-(0[1-9]|1[0-2])', s): raise ValueError('expected a month like 2026-09, got: ' + str(s))

def month_days(m):#the first and last calendar day of a month, as 2026-09-01 and 2026-09-30
	year, number = int(m[:4]), int(m[5:])
	if number == 12: next_first = datetime(year + 1, 1, 1)
	else: next_first = datetime(year, number + 1, 1)
	return f'{m}-01', datetime.fromordinal(next_first.toordinal() - 1).strftime('%Y-%m-%d')

def months_between(first, last):#every month from first through last, inclusive, so a range on the command line becomes a list to loop over
	found = []
	year, number = int(first[:4]), int(first[5:])
	while f'{year}-{number:02d}' <= last:
		found.append(f'{year}-{number:02d}')
		number += 1
		if number > 12: year, number = year + 1, 1
	return found

def radar_token():#the Cloudflare API token from the environment, or from the .env file beside this script, or blank when there is none
	if os.environ.get('RADAR_TOKEN', '').strip(): return os.environ['RADAR_TOKEN'].strip()
	if not environment_path.exists(): return ''
	for line in environment_path.read_text().splitlines():
		key, _, value = line.partition('=')
		if key.strip() == 'RADAR_TOKEN': return value.strip().strip('"').strip("'")
	return ''

def fetch(url, headers={}):#the one place this script talks to the network. Returns a result object, never throws: a refused or failed request is an ordinary outcome here, recorded and moved past
	request = urllib.request.Request(url, headers={'User-Agent': user_agent, **headers})
	try:
		with urllib.request.urlopen(request, timeout=60) as response:
			return {'success': True, 'status': response.status, 'text': response.read().decode('utf-8')}
	except urllib.error.HTTPError as e:
		return {'success': False, 'status': e.code, 'text': e.read().decode('utf-8', 'replace')[:2000]}
	except Exception as e:
		return {'success': False, 'status': 0, 'text': str(e)}

def pull(name, url, headers={}, keep=False):#fetch one URL, save it under name in the data folder, record it in the manifest, and print one line about it. With keep, a file already there is kept and the network is not touched. Returns the result object so a caller can look at what came back
	if keep and (folder/name).exists() and (folder/name).stat().st_size > 0:
		manifest[name] = {**manifest.get(name, {'url': url, 'fetched': 'before this run', 'status': 200, 'success': True}), 'kept': True}
		print(f'have  {(folder/name).stat().st_size:>12,} bytes  {name}')
		return {'success': True, 'status': 200, 'text': (folder/name).read_text()}
	result = fetch(url, headers)
	entry = {'url': url, 'fetched': datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'), 'status': result['status'], 'success': result['success']}
	if result['success']:
		(folder/name).write_text(result['text'])
		entry['bytes'] = len(result['text'].encode('utf-8'))
		print(f'ok    {entry["bytes"]:>12,} bytes  {name}')
	else:
		entry['error'] = result['text'][:300]
		print(f'fail  HTTP {result["status"]:<14}  {name}')
	manifest[name] = entry
	return result

# Google IPv6 statistics. The chart at google.com/intl/en/ipv6/statistics.html loads its data from a plain JavaScript file: one row per day since September 2008, each [year, month, day, total IPv6 %, native IPv6 %], with the month zero-based. It counts users reaching Google, not requests. The world map file is a per-country snapshot as of the fetch date
def pull_google():
	pull('google-adoption.js', 'https://www.google.com/intl/en_ALL/ipv6/statistics/data/adoption.js')
	pull('google-worldmap.js', 'https://www.google.com/intl/en_ALL/ipv6/statistics/data/worldmap.js')

# APNIC Labs IPv6 measurement. A script delivered inside online advertisements tests whether each viewer's device can fetch over IPv6, which samples the general population of web users rather than any one site. The world file is one record per day since October 2013, each carrying the raw sample count beside the capable and preferred percentages, plus 10, 30, 60, and 90 day smoothed versions. XA is APNIC's code for the world; the format is documented at data1.labs.apnic.net/ipv6-data-format.html. Re-use is permitted with attribution
def pull_apnic():
	pull('apnic-world.json', 'https://data1.labs.apnic.net/v6stats/v6region/XA.json')

# StatCounter Global Stats. Page views across more than a million sites carrying StatCounter's tracking code, with known bots excluded, reported monthly since January 2009. The CSV export is the same endpoint the site's own download button calls. Two pulls: platform share (desktop, mobile, tablet) and operating system share. Both run from 2009-01 through the last month asked for, so the whole history is on hand. Licensed CC BY-SA 3.0 with a link back
def pull_statcounter():
	first, last = '2009-01', months[-1]
	common = {'region_hidden': 'ww', 'granularity': 'monthly', 'region': 'Worldwide', 'fromInt': first.replace('-', ''), 'toInt': last.replace('-', ''), 'fromMonthYear': first, 'toMonthYear': last, 'csv': '1'}
	platform = {'device_hidden': 'desktop+mobile+tablet', 'statType_hidden': 'comparison', 'statType': 'Comparison', **common}
	pull('statcounter-platform-worldwide-monthly.csv', 'https://gs.statcounter.com/chart.php?' + urllib.parse.urlencode(platform))
	operating_system = {'device_hidden': 'desktop+mobile+tablet+console', 'statType_hidden': 'os_combined', 'statType': 'Operating System', **common}
	pull('statcounter-os-worldwide-monthly.csv', 'https://gs.statcounter.com/chart.php?' + urllib.parse.urlencode(operating_system))

# Wikimedia pageviews. Monthly page views across every Wikimedia project since July 2015, from the public REST API, split by access method (desktop, mobile web, mobile app) and by agent type (user, spider, automated). The automated class starts in April 2020; before that, automated traffic sits inside user. The API rate-limits bursts, so the sixteen pulls are spaced apart and a refused one is retried once after a longer wait. Data is CC0
def pull_wikimedia():
	end = month_days(months[-1])[1].replace('-', '') + '00'#the API wants YYYYMMDDHH, and the end is inclusive
	for access in ['all-access', 'desktop', 'mobile-web', 'mobile-app']:
		for agent in ['all-agents', 'user', 'spider', 'automated']:
			name = f'wikimedia-monthly-{access}-{agent}.json'
			url = f'https://wikimedia.org/api/rest_v1/metrics/pageviews/aggregate/all-projects/{access}/{agent}/monthly/2015070100/{end}'
			result = pull(name, url)
			if not result['success'] and result['status'] == 429:#too many requests; wait it out and try once more
				time.sleep(15)
				pull(name, url)
			time.sleep(2)

# Cloudflare Radar. The share of HTTP requests reaching Cloudflare's network, by whichever dimension the path names, filtered by any of the others. This is the only source that reports IP version, operating system, device type, and bot class together, so it is the source for the report's joint table. Bot class is Cloudflare's bot score: 1 through 29 is likely automated, 30 and above likely human. Dates are absolute, the month's first midnight to its last second, and the API floors them to 15 minutes; the exact range it used comes back in meta.dateRange and is kept with each pull. Data from the API is CC BY-NC 4.0.
#
# Each month gets the same eighteen pulls, in two groups. The table group is what table.py needs: bot class shares of all requests; operating system shares and device type shares within each bot class; IP version shares within each bot class, within each operating system among human requests, and within each device type among human requests. The last month in the run gets a third, verification group, which settles things the docs leave unstated by asking the API and keeping the answers: every operating system's IP version shares with no bot class filter (compared against the filtered pulls, this shows that omitting the filter means all requests), and operating system shares among IPv6 requests alone and IPv4 alone (which the table arithmetic must reproduce). Three more facts were learned the same way while building this and are now simply relied on: the operating system filter takes one value per request, so a column spanning several operating systems is a weighted sum of separate pulls; Radar's HTTP summaries hold data from May 2023 onward, the API refusing dates before 2021-12-27 and answering empty before May 2023; and a time series request may span at most 374 days
def pull_cloudflare():
	token = radar_token()
	if not token:
		print('skip  Cloudflare Radar: no RADAR_TOKEN in the environment or in ' + str(environment_path))
		print('      make one at dash.cloudflare.com: My Profile, API Tokens, Create Token, Create Custom Token, permission Account / Radar / Read, then put RADAR_TOKEN=the-token on a line of that .env file')
		return
	headers = {'Authorization': 'Bearer ' + token}
	base = 'https://api.cloudflare.com/client/v4/radar/http/'
	operating_systems = ['WINDOWS', 'MACOSX', 'LINUX', 'IOS', 'ANDROID', 'CHROMEOS', 'SMART_TV']
	device_types = ['DESKTOP', 'MOBILE', 'OTHER']

	def summary(dimension, when, **filters):#one summary pull for one month, with the file named by month, dimension, and filters so the data folder reads as a list of what was asked. A file already there is kept. A short pause after each real request keeps a run of hundreds polite
		name = f'cloudflare-{when}-{dimension.lower()}' + ''.join('-' + v.lower() for v in filters.values()) + '.json'
		first, last = month_days(when)
		query = {'dateStart': first + 'T00:00:00Z', 'dateEnd': last + 'T23:59:59Z', 'format': 'JSON', **filters}
		fresh = not (folder/name).exists()
		result = pull(name, base + 'summary/' + dimension + '?' + urllib.parse.urlencode(query), headers, keep=True)
		if fresh: time.sleep(0.4)
		return result

	for when in months:
		summary('BOT_CLASS', when)
		summary('IP_VERSION', when)
		for bot_class in ['LIKELY_HUMAN', 'LIKELY_AUTOMATED']:
			summary('OS', when, botClass=bot_class)
			summary('DEVICE_TYPE', when, botClass=bot_class)
			summary('IP_VERSION', when, botClass=bot_class)
		for operating_system in operating_systems:
			summary('IP_VERSION', when, botClass='LIKELY_HUMAN', os=operating_system)
		for device_type in device_types:
			summary('IP_VERSION', when, botClass='LIKELY_HUMAN', deviceType=device_type)

	# the verification group, for the last month only
	when = months[-1]
	for operating_system in operating_systems:
		summary('IP_VERSION', when, os=operating_system)
	for ip_version in ['IPv6', 'IPv4']:
		summary('OS', when, botClass='LIKELY_HUMAN', ipVersion=ip_version)

def main():
	global months, manifest
	arguments = sys.argv[1:]
	if not arguments or len(arguments) > 2: raise ValueError('usage: python3 fetch.py 2026-09, or python3 fetch.py 2023-05 2026-09')
	for a in arguments: check_month(a)
	months = months_between(arguments[0], arguments[-1])
	if not months: raise ValueError('the range is backwards: ' + ' '.join(arguments))
	folder.mkdir(exist_ok=True)
	if (folder/'fetched.json').exists(): manifest = {name: entry for name, entry in json.loads((folder/'fetched.json').read_text()).items() if (folder/name).exists()}#the earlier manifest carries over, minus entries for files deleted since, so the manifest only ever lists what the folder holds
	print(f'fetching {months[0]} through {months[-1]}, {len(months)} months, into {folder}')
	pull_google()
	pull_apnic()
	pull_statcounter()
	pull_wikimedia()
	pull_cloudflare()
	(folder/'fetched.json').write_text(json.dumps(manifest, indent='\t') + '\n')
	failed = [name for name, entry in manifest.items() if not entry['success']]
	kept = [name for name, entry in manifest.items() if entry.get('kept')]
	print(f'done: {len(manifest) - len(failed) - len(kept)} pulled, {len(kept)} kept from before, {len(failed)} failed, manifest in data/fetched.json')
	if failed: print('failed: ' + ', '.join(failed))

if __name__ == '__main__':
	try: main()
	except Exception as e:
		print('fetch.py stopped: ' + repr(e), file=sys.stderr)
		sys.exit(1)
