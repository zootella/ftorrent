#!/usr/bin/env python3
# Internet reachability probe for the open.ftorrent.com gauge.
# Run via cron every minute: * * * * * /opt/open.ftorrent.com/probe.py
#
# Pings a random target to test internet connectivity. If the ping
# succeeds, touches the probe file. The gauge reads its mtime each tick —
# a fresh mtime means the server could reach the internet this minute.
#
# On failure, retries once against a backup found by XOR 3 — always the
# other provider AND the other protocol. Each ping waits up to 5s (-W 5),
# so two attempts is ~10s normally; a 10s-per-call subprocess backstop
# caps a hung ping at ~20s — still well within the 60s cron interval.
#
#   Index  Megacorp    IP  Address                XOR 3
#   -----  ----------  --  --------------------   -----
#   0      Cloudflare  v4  1.1.1.1                ↩ 3
#   1      Google      v4  8.8.8.8                ↩ 2
#   2      Cloudflare  v6  2606:4700:4700::1111   ↩ 1
#   3      Google      v6  2001:4860:4860::8888   ↩ 0
#
# This script has exactly two outcomes and no third one: either everything
# is fine (a target answered, the marker is touched, exit 0) or it isn't
# (nothing answered or the marker couldn't be written, no touch, exit 1).
# Nothing escapes as an unhandled exception — the two operations that talk
# to the OS, the ping and the touch, each have their failures folded into
# "not fine." The gauge already treats a stale or missing marker as down,
# so declining to touch is all we ever need to do to report a problem.
#
# Stdlib only — no pip install, no virtualenv. Runs under the host's
# system python3 straight from cron.

import random  # for picking a random target each run
import subprocess  # for shelling out to the system `ping` binary
from pathlib import Path  # Path objects give us a tidy .touch() method

# The four probe targets: Cloudflare and Google, each on IPv4 and IPv6.
# The tracker serves both IPv4 and IPv6 clients, so probing both protocols
# catches single-stack outages that a v4-only probe would miss.
TARGETS = [
	"1.1.1.1",               # 0  Cloudflare v4
	"8.8.8.8",               # 1  Google     v4
	"2606:4700:4700::1111",  # 2  Cloudflare v6
	"2001:4860:4860::8888",  # 3  Google     v6
]

# The marker file. On the host this lives in the gauge's bind-mounted data
# directory, so the containerized gauge sees the same file at /gauge/probe.
PROBE = Path("/opt/open.ftorrent.com/data/probe")


def ping(target):
	"""Ping `target` once. Returns True on success, False on any failure.

	We shell out to the system `ping` binary rather than open a raw socket
	ourselves: raw ICMP sockets need elevated privilege, and the installed
	`ping` already carries the setuid/capability to do it. Shelling out also
	keeps the exact flags identical to the old bash probe.

	A hung, missing, or unrunnable `ping` is, for our purposes, the same as
	a dropped packet: the reachability check failed. So we catch those cases
	and return False rather than letting them escape — the caller then falls
	through to the backup target, exactly as a normal failed ping would.
	"""
	# An IPv6 address contains colons; an IPv4 address never does. Pick the
	# matching protocol flag so `ping` doesn't have to guess.
	flag = "-6" if ":" in target else "-4"

	try:
		# -c 1  send a single packet    -W 5  wait up to 5 seconds for a reply
		# stdout/stderr are discarded so cron has nothing to email us about.
		# check=False: we read the return code ourselves instead of having
		# subprocess raise on a non-zero exit (a failed ping is expected).
		# timeout=10 is a backstop in case `ping` ignores its own -W deadline.
		done = subprocess.run(
			["ping", flag, "-c", "1", "-W", "5", target],
			stdout=subprocess.DEVNULL,
			stderr=subprocess.DEVNULL,
			timeout=10,
			check=False,
		)
	except (subprocess.TimeoutExpired, OSError):
		# TimeoutExpired: ping wedged past the backstop.
		# OSError: ping binary missing or not runnable (covers FileNotFound,
		# PermissionError, and friends — they're all OSError subclasses).
		return False

	# A process returns 0 on success by convention; ping follows it.
	return done.returncode == 0


def reachable():
	"""True if the internet answered on either the primary or backup target."""
	# Pick one of the four targets at random. Spreading load across providers
	# and protocols means each gets pinged roughly once every four minutes.
	i = random.randrange(4)

	# Try the chosen target first. If it fails — a dropped packet, a brief
	# routing hiccup, a provider edge momentarily busy — retry the diagonal
	# opposite. XOR 3 flips both bits of the index, so it always lands on the
	# other provider AND the other protocol: maximum path independence. To
	# register as down, both pings must fail within the same minute. The `or`
	# short-circuits, so the backup is only pinged when the primary fails.
	return ping(TARGETS[i]) or ping(TARGETS[i ^ 3])


def main():
	"""Return True iff the marker was touched (everything fine), else False."""
	if not reachable():
		return False

	try:
		# Touching the marker updates its mtime to now — the single signal
		# this whole script exists to produce.
		PROBE.touch()
	except OSError:
		# We confirmed the internet is up but couldn't write the marker —
		# the data directory is missing or has the wrong owner, i.e. a broken
		# deploy. We swallow it rather than emailing a traceback every minute:
		# a stale marker already reports "down" to the gauge, and the README's
		# deploy step (stat the marker) is where this condition gets caught
		# once, at install time, instead of once a minute forever.
		return False

	return True


if __name__ == "__main__":
	# Exit 0 = fine (marker fresh), 1 = not fine. Cron keys off output, not
	# the exit code, but a truthful exit code is standard practice and lets a
	# human or wrapper see the result. raise SystemExit is a clean exit, not
	# an unhandled exception — so the "no third outcome" guarantee holds.
	raise SystemExit(0 if main() else 1)
