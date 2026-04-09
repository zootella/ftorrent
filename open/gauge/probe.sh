#!/bin/bash
# Internet reachability probe for the open.ftorrent.com gauge.
# Run via cron every minute: * * * * * /opt/open.ftorrent.com/probe.sh
#
# Pings one of two well-known public servers to test whether this server
# can reach the internet. Cloudflare (1.1.1.1) and Google (8.8.8.8) both
# run public DNS resolvers at these addresses — they're always on, globally
# distributed, and designed to respond to anyone. We're not using them for
# DNS here, just pinging them as a quick test of internet connectivity.
# Each run randomly picks one to avoid depending on a single provider.
#
# If the ping succeeds, touches the probe file. The gauge container reads
# this file's mtime to decide whether the server had internet connectivity
# during each minute.
ping -c 1 -W 5 $(shuf -n1 -e 1.1.1.1 8.8.8.8) > /dev/null 2>&1 && touch /opt/open.ftorrent.com/data/probe
