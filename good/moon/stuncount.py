#!/usr/bin/env python3
# Appends one row an hour to /var/log/stuncount.csv: the packets and bytes each of the six STUN counting rules in the host firewall has seen, read from iptables and ip6tables by the comment on each rule. The rules sit first in INPUT and OUTPUT and count without deciding anything; rules.v4 says what each counts. A counter restarts at zero when the box reboots, so a row lower than the one before marks a reboot, and a rule missing from its chain reads -1. The main server writes a file of the same shape, so the two read side by side. stuncount.timer runs this on the hour, as root, which reading the firewall needs.
import csv#writing rows the way spreadsheets read them
import datetime#the hour each row is for
import os#whether the file exists yet
import subprocess#running iptables and ip6tables

path = '/var/log/stuncount.csv'#the one file this writes, growing a row an hour
rules = [('iptables', 'INPUT', 'stun4-in'), ('iptables', 'OUTPUT', 'stun4-out'), ('iptables', 'INPUT', 'stun4-excess'), ('ip6tables', 'INPUT', 'stun6-in'), ('ip6tables', 'OUTPUT', 'stun6-out'), ('ip6tables', 'INPUT', 'stun6-excess')]#each counting rule as the tool that lists its family, the chain it's in, and its comment, in the order the columns go

def count(tool, chain, name):#the packets and bytes of the rule commented name in chain, or -1 and -1 when no rule there carries that comment
	listing = subprocess.run([tool, '-nvxL', chain], capture_output=True, text=True).stdout#-L lists the chain, -v adds the counters, -x prints them exactly rather than as 12K, and -n skips name lookups; capture_output and text hand back what it printed as a string
	for line in listing.splitlines():
		if f'/* {name} */' in line:#iptables prints a rule's comment at the end of its line between /* and */
			fields = line.split()#the line's columns, split on runs of spaces
			return int(fields[0]), int(fields[1])#packets and bytes are the first two columns whatever else the rule carries, and a rule with no target leaves that column blank rather than shifting these
	return -1, -1

row = [datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%MZ')]#the hour, in UTC, which is the box's clock too
for tool, chain, name in rules:
	row.extend(count(tool, chain, name))#two columns for each rule, its packets and then its bytes
new = not os.path.exists(path)#a file that isn't there yet gets a header row first
with open(path, 'a', newline='') as f:#'a' appends, and newline='' leaves the line endings to the csv module, which writes them itself
	writer = csv.writer(f)
	if new:
		writer.writerow(['time'] + [f'{name}-{kind}' for _, _, name in rules for kind in ('packets', 'bytes')])#a header once, like stun4-in-packets, stun4-in-bytes, and on through the six
	writer.writerow(row)
