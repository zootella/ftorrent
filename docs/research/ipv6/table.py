#!/usr/bin/env python3
import csv
import json
import re
import sys
from pathlib import Path

# This is the second of three scripts. It never touches the network: it reads what fetch.py wrote into the data folder and turns it into the numbers the report is about, written to data/table.json for chart.py and to two CSV files for anyone who would rather open a spreadsheet. Run it with no arguments: python3 table.py.
#
# For every month that has a full set of Cloudflare pulls, it builds the joint table: three kinds of request (automated, open, closed) by two address families, six cells that together are all of the month's requests. Open means the owner holds root: Windows, macOS, and Linux. Closed means the vendor does: iOS, Android, ChromeOS, and smart TVs. The split is by operating system because root is a property of the operating system, and device type (desktop, mobile) is computed beside it as a check. Each cell is a category's width times the IPv6 rate within it, and because the width and the rate come from different queries, the script also checks the arithmetic against the totals Cloudflare reports directly and records the residuals rather than hiding them.
#
# It also reduces the four cross-check sources to monthly series on the same calendar: Google's and APNIC's user-weighted IPv6 figures, which check the IPv6 share, and StatCounter's and Wikimedia's platform shares, which check the open-versus-closed split. Every percentage in the output is a percentage of what its name says, and the names say it: a share of all requests, a share of human requests, or a rate within one category.

folder = Path(__file__).resolve().parent/'data'#where fetch.py put everything
open_systems = ['WINDOWS', 'MACOSX', 'LINUX']#operating systems whose owner holds root
closed_systems = ['IOS', 'ANDROID', 'CHROMEOS', 'SMART_TV']#operating systems whose vendor holds root
statcounter_open = ['Windows', 'OS X', 'macOS', 'Linux']#StatCounter's column names for the same open systems; OS X and macOS are the same system before and after Apple renamed it

def read_json(name):#one file from the data folder as parsed JSON, or false when it isn't there; a month with a pull missing is skipped, not a crash
	path = folder/name
	if not path.exists(): return False
	return json.loads(path.read_text())

def read_summary(name):#the summary_0 object from one Cloudflare pull, with every share as a float, or false when the file is missing or the API answered empty, which it does for months before May 2023
	d = read_json(name)
	if not d or not d.get('result', {}).get('summary_0'): return False
	return {key: float(value) for key, value in d['result']['summary_0'].items()}

def weighted_rate(shares, rates, members):#the IPv6 rate across several operating systems together: each member's share times its rate, summed, over the members' total share. The share of the group itself comes back too, since every caller wants both
	total = sum(shares[m] for m in members)
	if total <= 0: raise ValueError('no share for ' + ', '.join(members))
	return sum(shares[m]*rates[m] for m in members)/total, total

def compute_month(m):#the joint table for one month from its Cloudflare pulls, or false when any pull for the month is missing or empty
	bot_class = read_summary(f'cloudflare-{m}-bot_class.json')
	all_versions = read_summary(f'cloudflare-{m}-ip_version.json')
	human_versions = read_summary(f'cloudflare-{m}-ip_version-likely_human.json')
	automated_versions = read_summary(f'cloudflare-{m}-ip_version-likely_automated.json')
	systems = read_summary(f'cloudflare-{m}-os-likely_human.json')
	devices = read_summary(f'cloudflare-{m}-device_type-likely_human.json')
	if not all([bot_class, all_versions, human_versions, automated_versions, systems, devices]): return False
	system_rates = {}#IPv6 rate within each operating system, among human requests
	for s in open_systems + closed_systems:
		versions = read_summary(f'cloudflare-{m}-ip_version-likely_human-{s.lower()}.json')
		if not versions: return False
		system_rates[s] = versions['IPv6']
	device_rates = {}#IPv6 rate within each device type, among human requests
	for d in ['desktop', 'mobile', 'other']:
		versions = read_summary(f'cloudflare-{m}-ip_version-likely_human-{d}.json')
		if not versions: return False
		device_rates[d] = versions['IPv6']

	# the widths: human and automated as shares of all requests, then open and closed as shares of human requests, from operating system shares that sum to 100 over the requests Cloudflare could classify
	human, automated = bot_class['human'], bot_class['bot']
	open_rate, open_of_human = weighted_rate(systems, system_rates, open_systems)
	closed_rate, closed_of_human = weighted_rate(systems, system_rates, closed_systems)
	open_of_all = human*open_of_human/100
	closed_of_all = human*closed_of_human/100

	# the six cells, each a share of all requests; IPv6 is width times rate, IPv4 is the rest of the width
	cells = {
		'automated_ipv6': automated*automated_versions['IPv6']/100,
		'open_ipv6': open_of_all*open_rate/100,
		'closed_ipv6': closed_of_all*closed_rate/100,
	}
	cells['automated_ipv4'] = automated - cells['automated_ipv6']
	cells['open_ipv4'] = open_of_all - cells['open_ipv6']
	cells['closed_ipv4'] = closed_of_all - cells['closed_ipv6']

	# the checks: what the table's arithmetic predicts against what Cloudflare reports directly. A residual is direct minus predicted, in points of IPv6 share
	predicted_human = sum(systems[s]*system_rates[s] for s in open_systems + closed_systems)/100#human IPv6 rate from the operating system route; differs from the direct figure by what the unclassified requests contribute
	predicted_by_device = sum(devices[d]*device_rates[d] for d in ['desktop', 'mobile', 'other'])/100#the same from the device type route, which classifies nearly everything
	predicted_all = (human*human_versions['IPv6'] + automated*automated_versions['IPv6'])/100#overall IPv6 share from the bot class route
	meta = read_json(f'cloudflare-{m}-ip_version.json')['result']['meta']
	return {
		'month': m,
		'date_range': meta['dateRange'][0],
		'person': human, 'automated': automated,
		'open_of_person': open_of_human, 'closed_of_person': closed_of_human,
		'open_of_all': open_of_all, 'closed_of_all': closed_of_all,
		'all_ipv6': all_versions['IPv6'], 'person_ipv6': human_versions['IPv6'], 'automated_ipv6': automated_versions['IPv6'],
		'open_ipv6': open_rate, 'closed_ipv6': closed_rate,
		'open_squared': open_of_human*open_of_human/100,#the open share squared: the chance that two requests drawn at random are both on open devices
		'open_ipv6_squared': open_rate*open_rate/100, 'person_ipv6_squared': human_versions['IPv6']*human_versions['IPv6']/100,
		'open_ipv6_of_person': open_of_human*open_rate/100, 'open_ipv6_of_person_squared': (open_of_human*open_rate/100)**2/100,#the one cell as a share of requests, and its square: the chance two random requests from people are both on an open device and from an IPv6 address
		'cells': cells,
		'systems': systems, 'system_ipv6': system_rates,
		'devices': devices, 'device_ipv6': device_rates,
		'desktop_of_person': devices['desktop'],
		'residual_systems': human_versions['IPv6'] - predicted_human,
		'residual_devices': human_versions['IPv6'] - predicted_by_device,
		'residual_bot_class': all_versions['IPv6'] - predicted_all,
	}

def verify_month(m, computed):#the verification group fetch.py pulls for its last month, read back and compared: unfiltered per-operating-system IPv6 against the human-filtered rates, and operating system shares among IPv6 requests against what the table predicts. Returns false when the month has no verification pulls
	unfiltered = {}
	for s in open_systems + closed_systems:
		versions = read_summary(f'cloudflare-{m}-ip_version-{s.lower()}.json')
		if not versions: return False
		unfiltered[s] = versions['IPv6']
	among_ipv6 = read_summary(f'cloudflare-{m}-os-likely_human-ipv6.json')
	if not among_ipv6: return False
	weight = {s: computed['systems'][s]*computed['system_ipv6'][s] for s in open_systems + closed_systems}#each system's IPv6 requests as a share of classified human requests, before normalizing
	predicted = {s: 100*weight[s]/sum(weight.values()) for s in weight}
	return {
		'unfiltered_ipv6': unfiltered,
		'human_ipv6': computed['system_ipv6'],
		'share_among_ipv6_direct': among_ipv6,
		'share_among_ipv6_predicted': predicted,
	}

def reduce_google():#Google's daily rows to monthly means of the total and native IPv6 percentages, keyed by month, since 2008-09
	text = (folder/'google-adoption.js').read_text()
	rows = re.findall(r'\[(\d{4}), (\d+), (\d+), ([\d.]+), ([\d.]+)\]', text)
	by_month = {}
	for year, month, day, total, native in rows:#the file's month is zero-based, so September is 8
		key = f'{year}-{int(month) + 1:02d}'
		by_month.setdefault(key, []).append((float(total), float(native)))
	return {key: {'total': sum(t for t, n in days)/len(days), 'native': sum(n for t, n in days)/len(days), 'days': len(days)} for key, days in by_month.items()}

def reduce_apnic():#APNIC's daily records to monthly figures, keyed by month, since 2013-10: capable and preferred percentages weighted by each day's sample count, and the month's total samples
	records = json.loads((folder/'apnic-world.json').read_text())['data']
	by_month = {}
	for r in records:
		by_month.setdefault(r['date'][:7], []).append(r['raw'])
	reduced = {}
	for key, days in by_month.items():
		samples = sum(d['seen'] for d in days)
		if samples <= 0: continue
		reduced[key] = {
			'capable': sum(d['capable_pc']*d['seen'] for d in days)/samples,
			'preferred': sum(d['preferred_pc']*d['seen'] for d in days)/samples,
			'samples': samples, 'days': len(days),
		}
	return reduced

def reduce_statcounter():#StatCounter's two CSV files to one monthly record, keyed by month, since 2009-01: the platform shares as published, and an open share built from its operating system columns the same way the Cloudflare table builds its own
	platform = {}
	with open(folder/'statcounter-platform-worldwide-monthly.csv', newline='') as f:
		for row in csv.DictReader(f):
			platform[row['Date']] = {'desktop': float(row['Desktop']), 'mobile': float(row['Mobile']), 'tablet': float(row['Tablet'])}
	with open(folder/'statcounter-os-worldwide-monthly.csv', newline='') as f:
		for row in csv.DictReader(f):
			key = row['Date']
			open_share = sum(float(row[c]) for c in statcounter_open if c in row)
			other = float(row.get('Other', 0))
			platform.setdefault(key, {})
			platform[key].update({'open': open_share, 'closed': 100 - open_share - other, 'other': other})
	return platform

def reduce_wikimedia():#Wikimedia's sixteen monthly series to one record per month, keyed by month, since 2015-07: the desktop share of human page views, the mobile web and mobile app shares, and the automated share of all page views, which is everything not classed as user
	def series(access, agent):
		d = read_json(f'wikimedia-monthly-{access}-{agent}.json')
		if not d: return {}
		return {item['timestamp'][:4] + '-' + item['timestamp'][4:6]: item['views'] for item in d['items']}
	all_user, all_agents = series('all-access', 'user'), series('all-access', 'all-agents')
	desktop_user, web_user, app_user = series('desktop', 'user'), series('mobile-web', 'user'), series('mobile-app', 'user')
	reduced = {}
	for key in all_user:
		if key not in all_agents or all_user[key] <= 0: continue
		reduced[key] = {
			'desktop_of_user': 100*desktop_user.get(key, 0)/all_user[key],
			'mobile_web_of_user': 100*web_user.get(key, 0)/all_user[key],
			'mobile_app_of_user': 100*app_user.get(key, 0)/all_user[key],
			'automated_of_all': 100*(1 - all_user[key]/all_agents[key]),
			'user_views': all_user[key], 'all_views': all_agents[key],
		}
	return reduced

def write_csv(name, rows, columns):#one CSV in the data folder with the named columns, numbers rounded to three places so the file reads as a table rather than a dump of floats
	with open(folder/name, 'w', newline='') as f:
		writer = csv.writer(f)
		writer.writerow(columns)
		for row in rows:
			writer.writerow([round(v, 3) if isinstance(v, float) else v for v in row])

def main():
	if not folder.exists(): raise FileNotFoundError('no data folder; run fetch.py first')
	months = sorted({re.match(r'cloudflare-(\d{4}-\d{2})-', p.name).group(1) for p in folder.glob('cloudflare-*-bot_class.json')})
	cloudflare = {}
	for m in months:
		computed = compute_month(m)
		if computed: cloudflare[m] = computed
		else: print(f'skipping {m}: Cloudflare pulls missing or empty')
	if not cloudflare: raise ValueError('no month has a complete set of Cloudflare pulls')
	latest = max(cloudflare)
	verification = verify_month(latest, cloudflare[latest])

	google, apnic, statcounter, wikimedia = reduce_google(), reduce_apnic(), reduce_statcounter(), reduce_wikimedia()
	table = {'latest': latest, 'months': sorted(cloudflare), 'cloudflare': cloudflare, 'verification': verification or {}, 'google': google, 'apnic': apnic, 'statcounter': statcounter, 'wikimedia': wikimedia}
	(folder/'table.json').write_text(json.dumps(table, indent='\t') + '\n')

	# the monthly joint table as a spreadsheet: one row per month, the widths, the rates, the six cells, the squares, the device route, and the residuals
	columns = ['month', 'person', 'automated', 'open_of_person', 'closed_of_person', 'open_of_all', 'closed_of_all', 'all_ipv6', 'person_ipv6', 'automated_ipv6', 'open_ipv6', 'closed_ipv6', 'open_squared', 'open_ipv6_squared', 'person_ipv6_squared', 'open_ipv6_of_person', 'open_ipv6_of_person_squared', 'automated_ipv4_cell', 'automated_ipv6_cell', 'open_ipv4_cell', 'open_ipv6_cell', 'closed_ipv4_cell', 'closed_ipv6_cell', 'desktop_of_person', 'desktop_ipv6', 'mobile_ipv6', 'residual_systems', 'residual_devices', 'residual_bot_class']
	rows = []
	for m in sorted(cloudflare):
		c = cloudflare[m]
		rows.append([m, c['person'], c['automated'], c['open_of_person'], c['closed_of_person'], c['open_of_all'], c['closed_of_all'], c['all_ipv6'], c['person_ipv6'], c['automated_ipv6'], c['open_ipv6'], c['closed_ipv6'], c['open_squared'], c['open_ipv6_squared'], c['person_ipv6_squared'], c['open_ipv6_of_person'], c['open_ipv6_of_person_squared'], c['cells']['automated_ipv4'], c['cells']['automated_ipv6'], c['cells']['open_ipv4'], c['cells']['open_ipv6'], c['cells']['closed_ipv4'], c['cells']['closed_ipv6'], c['desktop_of_person'], c['device_ipv6']['desktop'], c['device_ipv6']['mobile'], c['residual_systems'], c['residual_devices'], c['residual_bot_class']])
	write_csv('table.csv', rows, columns)

	# the long series as a second spreadsheet: every month any cross-check source covers, blank where a source has nothing for that month
	keys = sorted(set(google) | set(apnic) | set(statcounter) | set(wikimedia) | set(cloudflare))
	columns = ['month', 'google_ipv6_users', 'google_native_ipv6_users', 'apnic_ipv6_capable_users', 'apnic_ipv6_preferred_users', 'apnic_samples', 'statcounter_desktop', 'statcounter_mobile', 'statcounter_tablet', 'statcounter_open', 'statcounter_closed', 'statcounter_other', 'wikimedia_desktop_of_user', 'wikimedia_automated_of_all', 'cloudflare_all_ipv6', 'cloudflare_person_ipv6', 'cloudflare_open_ipv6', 'cloudflare_open_of_person', 'cloudflare_automated']
	rows = []
	for key in keys:
		g, a, s, w, c = google.get(key, {}), apnic.get(key, {}), statcounter.get(key, {}), wikimedia.get(key, {}), cloudflare.get(key, {})
		rows.append([key, g.get('total', ''), g.get('native', ''), a.get('capable', ''), a.get('preferred', ''), a.get('samples', ''), s.get('desktop', ''), s.get('mobile', ''), s.get('tablet', ''), s.get('open', ''), s.get('closed', ''), s.get('other', ''), w.get('desktop_of_user', ''), w.get('automated_of_all', ''), c.get('all_ipv6', ''), c.get('person_ipv6', ''), c.get('open_ipv6', ''), c.get('open_of_person', ''), c.get('automated', '')])
	write_csv('history.csv', rows, columns)

	# say what was built, and show the latest month's table so a run is checkable at a glance
	c = cloudflare[latest]
	print(f'{len(cloudflare)} months of Cloudflare data, {sorted(cloudflare)[0]} through {latest}; google {min(google)} through {max(google)}, apnic {min(apnic)} through {max(apnic)}, statcounter {min(statcounter)} through {max(statcounter)}, wikimedia {min(wikimedia)} through {max(wikimedia)}')
	print(f'wrote data/table.json, data/table.csv, data/history.csv')
	print(f'\n{latest}: shares of all traffic, automated included, with the IPv6 share of each row')
	for name, label in [('automated', 'automated requests'), ('open', 'requests, open device'), ('closed', 'requests, closed device')]:
		width = c[name if name == 'automated' else name + '_of_all']
		print(f'  {label:24} {width:6.2f}%   IPv4 {c["cells"][name + "_ipv4"]:6.2f}   IPv6 {c["cells"][name + "_ipv6"]:6.2f}   rate {c[name + "_ipv6"]:6.2f}%')
	print(f'  {"all traffic":24} 100.00%   IPv6 share of all traffic {c["all_ipv6"]:.2f}%, of requests {c["person_ipv6"]:.2f}%, open-device IPv6 squared {c["open_ipv6_squared"]:.2f}%')
	print(f'  residuals in points of IPv6 share: systems {c["residual_systems"]:+.2f}, devices {c["residual_devices"]:+.2f}, bot class {c["residual_bot_class"]:+.2f}')
	for source, record, fields in [('google', google.get(latest), ['total', 'native']), ('apnic', apnic.get(latest), ['capable', 'preferred']), ('statcounter', statcounter.get(latest), ['desktop', 'mobile', 'open', 'closed']), ('wikimedia', wikimedia.get(latest), ['desktop_of_user', 'automated_of_all'])]:
		if record: print(f'  {source:12} ' + '  '.join(f'{f} {record[f]:.2f}%' for f in fields))

if __name__ == '__main__':
	try: main()
	except Exception as e:
		print('table.py stopped: ' + repr(e), file=sys.stderr)
		sys.exit(1)
