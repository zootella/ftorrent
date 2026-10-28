#!/usr/bin/env python3
# Deploys moon's services from the files beside this script, each step the same the second time, so running it again after an edit is how a change goes up. coturn: Debian's package, our stun-only configuration at /etc/turnserver.conf, the hardening drop-in beside its unit, then a restart. The Go service: Debian's Go, the source copied to the box and built there into /usr/local/bin/ftorrent-moon, its unit, then a restart. And last the Caddyfile to /etc/caddy and a Caddy reload, which keeps its running config if the new one doesn't parse, so Caddy proxies to a service already up. ssh reads a Host block named moon from ssh.conf beside this script.
import os#the folder this script is in
import subprocess#running ssh and rsync
import sys#exiting with a failed command's code

os.chdir(os.path.dirname(os.path.abspath(__file__)))#so it runs from anywhere, and the paths below are this folder's
ssh = ['ssh', '-F', 'ssh.conf']#ssh with our settings file in place of ~/.ssh/config, which names the box moon

def run(*command):#runs one command here, its words as separate arguments, and stops the whole script with its exit code if it fails
	code = subprocess.run(command).returncode#runs it with its output straight to this terminal, and waits
	if code != 0:
		sys.exit(code)

def moon(script):#runs shell on moon, as root, through the ssh settings above
	run(*ssh, 'moon', script)#*ssh spreads the list into separate arguments, before the host and the command

#coturn: Debian's package, which is a no-op once installed, then our configuration and the drop-in, then a restart, since coturn reads its configuration only at start
moon('DEBIAN_FRONTEND=noninteractive apt-get install -y -q coturn >/dev/null; mkdir -p /etc/systemd/system/coturn.service.d')#noninteractive so the package asks nothing, -y to say yes, -q and the redirect to keep its output out of the way
run('rsync', '-t', '-e', ' '.join(ssh), 'turnserver.conf', 'moon:/etc/turnserver.conf')#-t carries the file's modified time, so a second run sees an unchanged file and skips it; -e is the ssh command, which rsync takes as one string
run('rsync', '-t', '-e', ' '.join(ssh), 'coturn.service.d/hardening.conf', 'moon:/etc/systemd/system/coturn.service.d/hardening.conf')
moon('chmod 644 /etc/turnserver.conf; systemctl daemon-reload; systemctl restart coturn; systemctl is-active coturn')#the mode first: rsync writes a new root-owned file carrying the mode of the package's own, which only the turnserver group could read, so the turnserver user the unit runs as couldn't open it; the configuration holds no secret, and not every rsync takes --chmod, so the box sets it. daemon-reload makes systemd read the drop-in, and is-active prints active once coturn is up

#the Go service: Debian's Go, a no-op once installed; the source to the box; a build straight into place, which writes a new file where the running binary was and leaves the running process on the old one until the restart; then its unit
moon('DEBIAN_FRONTEND=noninteractive apt-get install -y -q golang-go >/dev/null; mkdir -p /usr/local/src/ftorrent-moon')
run('rsync', '-rlt', '--delete', '--exclude=.DS_Store', '--delete-excluded', '-e', ' '.join(ssh), 'go-moon/', 'moon:/usr/local/src/ftorrent-moon/')#-r for the whole folder, -l to copy links as links, -t for the times; --delete removes what's no longer here, and never Finder's .DS_Store, which macOS writes into any folder a window has shown, with --delete-excluded taking any already there off moon too. The slash after go-moon/ copies the folder's contents rather than the folder itself
moon("cd /usr/local/src/ftorrent-moon && go build -trimpath -ldflags='-s -w' -o /usr/local/bin/ftorrent-moon .")#-trimpath and -s -w drop build paths and symbol tables, as the main server's image build does; . builds the package in this folder
run('rsync', '-t', '-e', ' '.join(ssh), 'ftorrent-moon.service', 'moon:/etc/systemd/system/ftorrent-moon.service')
moon('systemctl daemon-reload; systemctl enable --quiet ftorrent-moon; systemctl restart ftorrent-moon; systemctl is-active ftorrent-moon')#enable starts it at every boot from here on, and restart now, onto the new binary

#Caddy last, so its new config proxies to a service already answering
run('rsync', '-t', '-e', ' '.join(ssh), 'Caddyfile', 'moon:/etc/caddy/Caddyfile')
moon('systemctl reload caddy')#reload rather than restart: Caddy reads the new file while it keeps serving, and keeps the old config if the new one has an error
