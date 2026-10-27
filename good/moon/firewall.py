#!/usr/bin/env python3
# Installs moon's host firewall: installs iptables-persistent, which loads /etc/iptables/rules.v4 and rules.v6 at every boot, copies the two files there, loads them now, and installs the hourly read of the STUN counters, stuncount.py under a systemd timer. The package asks at install whether to save the live tables into those files; the script answers no, so the files stay the source. A rule that shut out ssh would be hard to undo from here, so before loading, the script starts a timer on the box that reopens INPUT after three minutes; a fresh ssh connection after the load proves the rules admit ssh, and stops the timer. ssh reads ssh.conf beside this script, as deploy.py does.
import os
import subprocess
import sys

os.chdir(os.path.dirname(os.path.abspath(__file__))) # so it runs from anywhere, and the paths below are this folder's
ssh = ["ssh", "-F", "ssh.conf"]

def run(*command): # runs one command here, and stops with its exit code if it fails
	code = subprocess.run(command).returncode
	if code:
		sys.exit(code)

def moon(script): # runs shell on moon, one command per line or semicolon
	run(*ssh, "moon", script)

# the timer first, replacing one a previous run left; stopping a timer that isn't there complains, and that's fine
moon("systemctl stop firewall-revert.timer 2>/dev/null; systemd-run --quiet --collect --on-active=3min --unit=firewall-revert sh -c 'iptables -P INPUT ACCEPT; ip6tables -P INPUT ACCEPT'")

# the package, with its two install questions answered no ahead of time, so it never writes the live tables over the files
moon(r"""
printf '%s\n' 'iptables-persistent iptables-persistent/autosave_v4 boolean false' 'iptables-persistent iptables-persistent/autosave_v6 boolean false' | debconf-set-selections
DEBIAN_FRONTEND=noninteractive apt-get install -y -q iptables-persistent >/dev/null
mkdir -p /etc/iptables
""")

run("rsync", "-t", "-e", " ".join(ssh), "rules.v4", "rules.v6", "moon:/etc/iptables/") # rsync takes the ssh command as one string

moon("iptables-restore < /etc/iptables/rules.v4; ip6tables-restore < /etc/iptables/rules.v6")

# the hourly read of the STUN counters: the script, and the unit and timer that run it
run("rsync", "-tp", "-e", " ".join(ssh), "stuncount.py", "moon:/usr/local/bin/stuncount.py") # -p keeps it executable
run("rsync", "-t", "-e", " ".join(ssh), "stuncount.service", "stuncount.timer", "moon:/etc/systemd/system/")
moon("systemctl daemon-reload; systemctl enable --quiet --now stuncount.timer")

# a fresh connection proves ssh still passes the new rules; it stops the timer and shows INPUT as loaded
code = subprocess.run([*ssh, "-o", "ConnectTimeout=15", "moon", "systemctl stop firewall-revert.timer; iptables -S INPUT; ip6tables -S INPUT; systemctl list-timers stuncount.timer --no-pager"]).returncode
if code:
	sys.exit("ssh did not connect; the timer reopens INPUT in three minutes")
