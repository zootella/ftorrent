#!/usr/bin/env python3
# Installs moon's host firewall: installs iptables-persistent, which loads /etc/iptables/rules.v4 and rules.v6 at every boot, copies the two files there, loads them now, turns on IPv4 forwarding for the network namespaces those rules translate, and installs the hourly read of the STUN counters, stuncount.py under a systemd timer. The package asks at install whether to save the live tables into those files; the script answers no, so the files stay the source. A rule that shut out ssh would be hard to undo from here, so before loading, the script starts a timer on the box that reopens INPUT after three minutes; a fresh ssh connection after the load proves the rules admit ssh, and stops the timer. ssh reads ssh.conf beside this script, as deploy.py does.
import os#the folder this script is in
import subprocess#running ssh and rsync
import sys#exiting with a failed command's code

os.chdir(os.path.dirname(os.path.abspath(__file__)))#so it runs from anywhere, and the paths below are this folder's
ssh = ['ssh', '-F', 'ssh.conf']#ssh with our settings file in place of ~/.ssh/config, which names the box moon

def run(*command):#runs one command here, its words as separate arguments, and stops the whole script with its exit code if it fails
	code = subprocess.run(command).returncode#runs it with its output straight to this terminal, and waits
	if code != 0:
		sys.exit(code)

def moon(script):#runs shell on moon, as root, one command per line or semicolon
	run(*ssh, 'moon', script)#*ssh spreads the list into separate arguments, before the host and the command

#the timer first, replacing one a previous run left; stopping a timer that isn't there complains, and that's fine
moon("systemctl stop firewall-revert.timer 2>/dev/null; systemd-run --quiet --collect --on-active=3min --unit=firewall-revert sh -c 'iptables -P INPUT ACCEPT; ip6tables -P INPUT ACCEPT'")#systemd-run makes a one-off timer named firewall-revert that fires in three minutes and is cleaned away after; what it runs sets INPUT's policy, the fate of a packet no rule accepts, back to accept in both families, which with rules that only ever accept lets everything in

#the package, with its two install questions answered no ahead of time, so it never writes the live tables over the files
moon(r"""
printf '%s\n' 'iptables-persistent iptables-persistent/autosave_v4 boolean false' 'iptables-persistent iptables-persistent/autosave_v6 boolean false' | debconf-set-selections
DEBIAN_FRONTEND=noninteractive apt-get install -y -q iptables-persistent >/dev/null
mkdir -p /etc/iptables
""")#debconf-set-selections stores answers to a package's install questions before it asks them, here no to saving each family's live rules; r makes the string raw, so the \n reaches printf as written

run('rsync', '-t', '-e', ' '.join(ssh), 'rules.v4', 'rules.v6', 'moon:/etc/iptables/')#-t carries the files' modified times; -e is the ssh command, which rsync takes as one string

moon('iptables-restore < /etc/iptables/rules.v4; ip6tables-restore < /etc/iptables/rules.v6')#each table in each family's file replaced whole, at once: filter in both, and nat in IPv4's

#IPv4 forwarding, once the rules that govern it are loaded: the drop-in where systemd-sysctl reads it at every boot, and read now
run('rsync', '-t', '-e', ' '.join(ssh), 'forwarding.conf', 'moon:/etc/sysctl.d/60-forwarding.conf')
moon('sysctl -q -p /etc/sysctl.d/60-forwarding.conf')#-p reads settings from the file named, and -q keeps it from echoing them

#the hourly read of the STUN counters: the script, and the unit and timer that run it
run('rsync', '-tp', '-e', ' '.join(ssh), 'stuncount.py', 'moon:/usr/local/bin/stuncount.py')#-p carries the file's permissions, which keeps it executable
run('rsync', '-t', '-e', ' '.join(ssh), 'stuncount.service', 'stuncount.timer', 'moon:/etc/systemd/system/')
moon('systemctl daemon-reload; systemctl enable --quiet --now stuncount.timer')#enable the timer at every boot, and --now start it this moment too

#a fresh connection proves ssh still passes the new rules; it stops the timer and shows INPUT as loaded
code = subprocess.run([*ssh, '-o', 'ConnectTimeout=15', 'moon', 'systemctl stop firewall-revert.timer; iptables -S INPUT; iptables -S FORWARD; iptables -t nat -S POSTROUTING; ip6tables -S INPUT; sysctl net.ipv4.ip_forward; systemctl list-timers stuncount.timer --no-pager']).returncode#a new ssh connection, given fifteen seconds to open, which only gets through if the rules admit it; -S prints a chain's rules as the commands that would make them
if code != 0:
	sys.exit('ssh did not connect; the timer reopens INPUT in three minutes')#sys.exit with text prints it and exits with code 1
