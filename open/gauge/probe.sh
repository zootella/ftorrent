#!/bin/bash
# Internet reachability probe for the open.ftorrent.com gauge.
# Run via cron every minute: * * * * * /opt/open.ftorrent.com/probe.sh
#
# Pings a random target to test internet connectivity. If the ping
# succeeds, touches the probe file. The gauge reads its mtime each tick.
#
# On failure, retries once against a backup found by XOR 3 — always the
# other provider AND the other protocol. 5s timeout each, 10s worst case.
#
#   Index  Megacorp    IP  Address                XOR 3
#   -----  ----------  --  --------------------   -----
#   0      Cloudflare  v4  1.1.1.1                ↩ 3
#   1      Google      v4  8.8.8.8                ↩ 2
#   2      Cloudflare  v6  2606:4700:4700::1111   ↩ 1
#   3      Google      v6  2001:4860:4860::8888   ↩ 0

TARGETS=(1.1.1.1 8.8.8.8 2606:4700:4700::1111 2001:4860:4860::8888)
PROBE=/opt/open.ftorrent.com/data/probe

probe() {
	local target=$1
	if [[ "$target" == *:* ]]; then
		ping -6 -c 1 -W 5 "$target" > /dev/null 2>&1
	else
		ping -4 -c 1 -W 5 "$target" > /dev/null 2>&1
	fi
}

I=$((RANDOM % 4))
if probe "${TARGETS[$I]}"; then
	touch "$PROBE"
	exit 0
fi

# First ping failed — XOR 3 finds the other provider on the other protocol
probe "${TARGETS[$((I ^ 3))]}" && touch "$PROBE"
