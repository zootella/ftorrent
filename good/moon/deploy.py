#!/usr/bin/env python3
# Copies the Caddyfile to /etc/caddy and site/ to /var/www/moon, then reloads Caddy, which keeps its running config if the new one doesn't parse. ssh reads a Host block named moon from ssh.conf beside this script.
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

moon("mkdir -p /var/www/moon")
run("rsync", "-t", "-e", " ".join(ssh), "Caddyfile", "moon:/etc/caddy/Caddyfile") # rsync takes the ssh command as one string
run("rsync", "-rlt", "--delete", "--exclude=.DS_Store", "--delete-excluded", "-e", " ".join(ssh), "site/", "moon:/var/www/moon/") # never Finder's .DS_Store, which macOS writes into any folder a window has shown, and none left on moon either
moon("systemctl reload caddy")
