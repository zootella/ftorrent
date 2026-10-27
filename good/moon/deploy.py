#!/usr/bin/env python3
# Deploys moon's services from the files beside this script, each step the same the second time, so running it again after an edit is how a change goes up. coturn: Debian's package, our stun-only configuration at /etc/turnserver.conf, the hardening drop-in beside its unit, then a restart. The Go service: Debian's Go, the source copied to the box and built there into /usr/local/bin/ftorrent-moon, its unit, then a restart. And last the Caddyfile to /etc/caddy and a Caddy reload, which keeps its running config if the new one doesn't parse, so Caddy proxies to a service already up. ssh reads a Host block named moon from ssh.conf beside this script.
import os
import subprocess
import sys

os.chdir(os.path.dirname(os.path.abspath(__file__))) # so it runs from anywhere, and the paths below are this folder's
ssh = ["ssh", "-F", "ssh.conf"]

def run(*command): # runs one command here, and stops with its exit code if it fails
	code = subprocess.run(command).returncode
	if code:
		sys.exit(code)

def moon(script): # runs shell on moon
	run(*ssh, "moon", script)

# coturn: Debian's package, which is a no-op once installed, then our configuration and the drop-in, then a restart, since coturn reads its configuration only at start
moon("DEBIAN_FRONTEND=noninteractive apt-get install -y -q coturn >/dev/null; mkdir -p /etc/systemd/system/coturn.service.d")
run("rsync", "-t", "-e", " ".join(ssh), "turnserver.conf", "moon:/etc/turnserver.conf") # rsync takes the ssh command as one string
run("rsync", "-t", "-e", " ".join(ssh), "coturn.service.d/hardening.conf", "moon:/etc/systemd/system/coturn.service.d/hardening.conf")
moon("chmod 644 /etc/turnserver.conf; systemctl daemon-reload; systemctl restart coturn; systemctl is-active coturn") # the mode first: rsync writes a new root-owned file carrying the mode of the package's own, which only the turnserver group could read, so the turnserver user the unit runs as couldn't open it; the configuration holds no secret, and not every rsync takes --chmod, so the box sets it

# the Go service: Debian's Go, a no-op once installed; the source to the box; a build straight into place, which writes a new file where the running binary was and leaves the running process on the old one until the restart; then its unit
moon("DEBIAN_FRONTEND=noninteractive apt-get install -y -q golang-go >/dev/null; mkdir -p /usr/local/src/ftorrent-moon")
run("rsync", "-rlt", "--delete", "--exclude=.DS_Store", "--delete-excluded", "-e", " ".join(ssh), "go-moon/", "moon:/usr/local/src/ftorrent-moon/") # never Finder's .DS_Store, which macOS writes into any folder a window has shown, and none left on moon either
moon("cd /usr/local/src/ftorrent-moon && go build -trimpath -ldflags='-s -w' -o /usr/local/bin/ftorrent-moon .") # -trimpath and -s -w drop build paths and symbol tables, as the main server's image build does
run("rsync", "-t", "-e", " ".join(ssh), "ftorrent-moon.service", "moon:/etc/systemd/system/ftorrent-moon.service")
moon("systemctl daemon-reload; systemctl enable --quiet ftorrent-moon; systemctl restart ftorrent-moon; systemctl is-active ftorrent-moon")

# Caddy last, so its new config proxies to a service already answering
run("rsync", "-t", "-e", " ".join(ssh), "Caddyfile", "moon:/etc/caddy/Caddyfile")
moon("systemctl reload caddy")
