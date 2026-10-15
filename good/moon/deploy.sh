#!/bin/sh
# Copies the Caddyfile to /etc/caddy and site/ to /var/www/moon, then reloads Caddy, which keeps its running config if the new one doesn't parse. ssh reads ssh.hide.conf beside this script, gitignored: Host moon, its address, User root, and moon's key.
set -eu
cd "$(dirname "$0")"
ssh="ssh -F ssh.hide.conf"
$ssh moon 'mkdir -p /var/www/moon'
rsync -t -e "$ssh" Caddyfile moon:/etc/caddy/Caddyfile
rsync -rlt --delete -e "$ssh" site/ moon:/var/www/moon/
$ssh moon 'systemctl reload caddy'
