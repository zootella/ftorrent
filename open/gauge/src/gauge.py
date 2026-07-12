#!/usr/bin/env python3
"""The open.ftorrent.com gauge.

A long-running, single-purpose process. Once at the top of each UTC minute it
scrapes Prometheus metrics from the three Aquatic tracker containers and health
from the DHT node, reads all four containers' memory from cgroup files, and
writes page.json — the data file the Vue frontend renders. It also maintains
private working state (the ring buffer, the 90-day history, and the
circuit-breaker cool-down file). No HTTP server, no listening ports; it only
writes files.

It stays resident but holds almost nothing between ticks: ring, days, and
breaker state are reloaded from disk every minute. Standard library only.
"""

import json
import math
import os
import re
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

# The gauge's working directory. In Docker, this is /gauge (a bind mount to
# /opt/open.ftorrent.com/data on the host). Locally it defaults to the page
# workspace, so page.json lands where the Vite dev server can pick it up.
#
# Inside this directory:
#   public/page.json  — the public output, served by the reverse proxy
#   ring.json         — private working state, never served
#   breaker.json      — private cool-down state, read by the host's breaker script
GAUGE_DIR = Path(os.environ.get("GAUGE_DIR", "../page"))
PUBLIC_DIR = GAUGE_DIR / "public"
RING_PATH = GAUGE_DIR / "ring.json"
DAYS_PATH = GAUGE_DIR / "days.json"
BREAKER_PATH = GAUGE_DIR / "breaker.json"

# The internet reachability probe file. A cron job on the host pings an
# external IP every minute and touches this file on success. The gauge checks
# its mtime — if it's stale, the server can't reach the internet and this
# minute doesn't count as up. See probe.py and the README.
PROBE_PATH = GAUGE_DIR / "probe"
PROBE_MAX_AGE_MS = 90_000  # 90 seconds — absorbs jitter between cron and gauge

# Where container cgroup files live. In Docker, /sys/fs/cgroup is mounted at
# /host-cgroup (read-only). Locally this path won't exist, so memory stats are
# skipped gracefully.
CGROUP_DIR = Path(os.environ.get("CGROUP_DIR", "/host-cgroup"))
CGROUP_SLICE = CGROUP_DIR / "system.slice"

# ---------------------------------------------------------------------------
# Container identification
# ---------------------------------------------------------------------------

# We identify the four containers by their memory ceiling (memory.max from
# cgroup), which is deliberately unique per container.
#
# This is a concession: the cgroup tree only exposes container IDs, not names.
# We chose not to mount the Docker socket (too much privilege) or pass IDs as
# environment variables (they change on every restart). Matching by ceiling
# works because the limits are intentionally different and documented in the
# guide. The three Aquatic trackers get the same generous ~2 GiB ceiling for
# tracker state, told apart by 1 MiB offsets; the DHT node (qBittorrent, not
# Aquatic) sits far below at 512 MiB, so its ceiling is unique on its own.
#
# Each entry is (key, ceiling-in-bytes).
MEMORY_TARGETS = [
	("udp", 2001 * 1024 * 1024),   # 2001 MiB
	("http", 2002 * 1024 * 1024),  # 2002 MiB
	("ws", 2003 * 1024 * 1024),    # 2003 MiB
	("dht", 512 * 1024 * 1024),    # 512 MiB — DHT node, well clear of the trackers
]

# Prometheus endpoints for the three Aquatic containers. These hostnames
# resolve via Docker's internal DNS on the ftorrent-open network. Locally they
# won't resolve, so scraping is skipped gracefully.
#
# Each entry is (key, url, response-type).
PROMETHEUS_ENDPOINTS = [
	("udp", "http://ftorrent-open-udp-1:9000/metrics", "announce"),
	("http", "http://ftorrent-open-http-1:9000/metrics", "announce"),
	("ws", "http://ftorrent-open-ws-1:9000/metrics", "offer"),
]

# ---------------------------------------------------------------------------
# Cool-down circuit breakers
# ---------------------------------------------------------------------------

# If a service reaches its threshold of requests served in 24 hours, we turn
# it off for 24 hours to cool it down. The per-service thresholds are in
# SERVICE_BREAKERS below.

# How long a tripped service stays cooled. After this many ms have passed since
# the trip, the wall clock crosses the service's startOn timestamp and the
# service may run again — release is implicit, no separate release code.
COOL_DURATION = 24 * 60 * 60 * 1000

# ---------------------------------------------------------------------------
# Ring buffer — 1,440 slots, one per minute of the day
# ---------------------------------------------------------------------------

# ring.json is an object with a "minutes" array of 1,440 slots. Each slot is
# either None (the gauge didn't run that minute) or a dict with:
#   day     — day number (now_ms // 86_400_000)
#   served  — cumulative counter snapshot: {udp4, udp6, http4, http6, ws4, ws6}
#
# Slot 0 is 00:00 UTC, slot 1439 is 23:59 UTC.
#
# To count downtime: slots at or before the current minute should have today's
# day number. Slots after the current minute should have yesterday's day
# number. Anything else (None or older) is a missed minute.
#
# To get 24-hour served totals: walk backward from the current slot N,
# extending a candidate oldest slot P back one step at a time as long as each
# older slot's counters are all <= the current P's counters. A counter that's
# higher at an older slot than at the current P is the reset signature of a
# container restart between them — stop there, so P stays in the same era as N.
# Report N - P per metric. If no older slot qualified (first tick after a
# restart, or a freshly wiped ring), P stays equal to N and N - P is zero.
# Every reported number is proven growth between two different ring slots.

MINUTES_PER_DAY = 1440
SECONDS_PER_DAY = 86_400
MS_PER_DAY = 86_400_000
MS_PER_MINUTE = 60_000

# The six counter keys stored in each ring slot. Each of the six services
# (three protocols x two IP versions) is metered independently against the
# breaker, so one face of the tracker can cool while its sibling keeps serving
# — nudging clients toward IPv6 and toward UDP under load.
SERVED_KEYS = ["udp4", "udp6", "http4", "http6", "ws4", "ws6"]

# Per-service breaker thresholds, set from measured capacity. On this
# deployment a UDP announce costs ~1 CPU core plus ~23 Mbps of response
# bandwidth per billion announces per day — nearly free, and bandwidth-bound
# long before it is CPU-bound. An HTTPS announce costs ~150-170x as much,
# dominated by TLS handshakes and per-connection overhead — the resource
# that actually limits scale on modest hardware; WS rides the same TLS
# budget, so all four TCP cells share one ceiling. Against budgets of ~70%
# of CPU and ~300 Mbps of transmit, HTTPS fills the box at ~140-150M
# announces a day and UDP at ~13B: the TCP ceiling sits at ~0.6x that fill
# point, and UDP's at ~3x its current daily pace, an order of magnitude
# under its fill. Within each protocol, v4 and v6 get the same full value
# on purpose — v6 carries a small fraction of the traffic, so under real
# load a v4 cell trips first while its v6 sibling keeps serving, nudging
# clients toward IPv6.
SERVICE_BREAKERS = {
	"udp4": 1_500_000_000,
	"udp6": 1_500_000_000,
	"http4": 80_000_000,
	"http6": 80_000_000,
	"ws4": 80_000_000,
	"ws6": 80_000_000,
}

# ---------------------------------------------------------------------------
# Days — 90-day uptime history
# ---------------------------------------------------------------------------

# days.json is a sparse object keyed by day number (string keys, as JSON
# requires). Values are up-minutes (0-1440). Only days when the gauge actually
# ran have entries. Missing keys mean the server was fully down that day — no
# process was running to write a record, and absence of evidence is treated as
# downtime.

HISTORY_DAYS = 90

# Launch day — days before this are excluded from uptime stats. The page shows
# them as "Before launch" and "Launch day" in the bar chart, and excludes the
# launch day itself from the percentage as partial data. Remove this once the
# 90-day window has fully passed launch day.
LAUNCH_DAY = 20551  # 2026 Apr 08


def current_day(now):
	return now // MS_PER_DAY


def current_minute(now):
	return (now % MS_PER_DAY) // MS_PER_MINUTE


def load_ring():
	try:
		data = json.loads(RING_PATH.read_text(encoding="utf-8"))
	except (OSError, json.JSONDecodeError):
		data = {}
	if not isinstance(data, dict):
		data = {}
	minutes = data.get("minutes")
	if not isinstance(minutes, list) or len(minutes) != MINUTES_PER_DAY:
		data["minutes"] = [None] * MINUTES_PER_DAY
	return data


# A real ring slot is a dict carrying 'day' and a 'served' dict; an empty slot
# is None. The gauge only ever writes those two shapes, so this guard matters
# only against a corrupt or tampered ring.json: a malformed entry is then
# treated as a missed minute (down) instead of crashing the tick, matching how
# the original tolerated anything that wasn't a proper record.
def _is_slot(slot):
	return isinstance(slot, dict) and "day" in slot and isinstance(slot.get("served"), dict)


def count_downtime(minutes, day, minute):
	downtime = 0
	for i in range(MINUTES_PER_DAY):
		expected_day = day if i <= minute else day - 1
		slot = minutes[i]
		if not _is_slot(slot) or slot["day"] != expected_day:
			downtime += 1
	return downtime


def load_days():
	try:
		data = json.loads(DAYS_PATH.read_text(encoding="utf-8"))
	except (OSError, json.JSONDecodeError):
		return {}
	return data if isinstance(data, dict) else {}


# Finalize completed days: scan the ring for day numbers that are in the past
# and not yet recorded in days.json. Count up-minutes for each and write the
# entry. This runs before the ring slot is written, so stale slots from past
# days haven't been overwritten yet. days.json keys are strings (JSON), so we
# convert the numeric day to str at every access.
def finalize_completed_days(minutes, day, days):
	# Collect distinct past day numbers from the ring
	past_days = {slot["day"] for slot in minutes if _is_slot(slot) and slot["day"] < day}

	changed = False
	for d in past_days:
		if str(d) in days:
			continue  # already finalized
		days[str(d)] = sum(1 for slot in minutes if _is_slot(slot) and slot["day"] == d)
		changed = True

	if changed:
		# Trim days older than 90 days ago
		cutoff = day - HISTORY_DAYS
		for key in list(days.keys()):
			if int(key) < cutoff:
				del days[key]
		write_atomic(DAYS_PATH, json.dumps(days, indent="\t") + "\n")


# Build the 90-day history of per-day downtime minutes. Index 0 is 89 days ago,
# last entry is today. Completed days come from days.json (missing = 1440
# downtime). Today is live: (minutes elapsed) - (up-minutes in ring). Returns a
# list; the caller joins it to a string for page.json and reuses the list to
# compute uptime.
def build_history(minutes, day, minute, days):
	values = []

	# 89 completed days, oldest first
	for i in range(HISTORY_DAYS - 1, 0, -1):
		d = day - i
		up = days.get(str(d), 0)
		values.append(MINUTES_PER_DAY - up)

	# Today: live count from the ring
	up_today = sum(1 for slot in minutes if _is_slot(slot) and slot["day"] == day)
	values.append((minute + 1) - up_today)

	return values


# Mirrors Panel.vue's uptimePercent: percentage of post-launch minutes that
# were up. The launch day itself is excluded as partial data; today contributes
# only the elapsed minutes so far. Returns None when the window has no
# post-launch days yet (launch is today or in the future).
def compute_uptime(history_values, day, minute):
	first_day = day - (len(history_values) - 1)
	launch_index = -1 if LAUNCH_DAY < first_day else LAUNCH_DAY - first_day
	post_launch = history_values[launch_index + 1:]
	if not post_launch:
		return None
	total_minutes = (len(post_launch) - 1) * MINUTES_PER_DAY + (minute + 1)
	total_down = sum(post_launch)
	return ((total_minutes - total_down) / total_minutes) * 100


# Diff the immediately prior ring slot against the current scrape to get
# per-minute served counts. Returns zeros when the prior slot is missing, stale
# (wrong day for its index — e.g. server was down through that minute), or shows
# a reset signature (any key higher in the prior slot than now, meaning Aquatic
# restarted between them). Same conservative stance as compute_served_24h: never
# report more than a real same-era diff.
def compute_served_1m(current_served, minutes, day, minute):
	result = {key: 0 for key in SERVED_KEYS}

	i = (minute - 1 + MINUTES_PER_DAY) % MINUTES_PER_DAY
	slot = minutes[i]
	if not _is_slot(slot):
		return result
	expected_day = day if i <= minute else day - 1
	if slot["day"] != expected_day:
		return result

	is_reset = any(slot["served"].get(k, 0) > current_served.get(k, 0) for k in SERVED_KEYS)
	if is_reset:
		return result

	for key in SERVED_KEYS:
		result[key] = current_served.get(key, 0) - slot["served"].get(key, 0)
	return result


# Walk backward from the current minute, extending a candidate oldest slot P
# back one step at a time as long as each older slot's counters are all <= the
# current P's counters. A counter higher at an older slot than at the current P
# is the reset signature of a container restart between them — we stop there,
# keeping P in the same era as the current scrape. A slot is only considered if
# its stored day number matches what we'd expect at that index: slots at or
# before the current minute should hold today, slots after should hold
# yesterday; anything else is stale.
def compute_served_24h(current_served, minutes, day, minute):
	# P starts with the current scrape so the loop can uniformly compare older
	# slots against it. If no older slot extends P, the final subtraction yields
	# zero for every key.
	p = {"served": current_served}
	for offset in range(1, MINUTES_PER_DAY):
		i = (minute - offset + MINUTES_PER_DAY) % MINUTES_PER_DAY
		slot = minutes[i]
		if not _is_slot(slot):
			continue
		expected_day = day if i <= minute else day - 1
		if slot["day"] != expected_day:
			continue

		is_reset = any(slot["served"].get(k, 0) > p["served"].get(k, 0) for k in SERVED_KEYS)
		if is_reset:
			break
		p = slot

	return {key: current_served.get(key, 0) - p["served"].get(key, 0) for key in SERVED_KEYS}


# ---------------------------------------------------------------------------
# Cool-down state — when each service may run again
# ---------------------------------------------------------------------------

# breaker.json holds an epoch-ms timestamp per service under "startOn". If
# now > startOn[service], the service should be running. If now < startOn, it
# should be paused. 0 means "no order to be off has ever been issued" (a release
# that expired in 1970, always in the past).
#
# State changes happen here in two ways:
#   tripped:  the gauge sets startOn[service] = now + COOL_DURATION when the
#             service's own rolling 24-hour served exceeds its breaker. The six
#             services are evaluated independently, so e.g. http4 can cool while
#             http6 keeps running — clients on v6 stay served.
#   released: implicit — the wall clock simply crosses startOn[service].
#
# The host's breaker script reads breaker.json, computes desired state per
# service against the current wall clock, and reconciles the reverse proxy +
# iptables. We rewrite breaker.json each tick (whether or not it changed) so the
# host's path-unit reconciles against the fresh wall clock every minute.


def _is_number(value):
	# JSON true/false parse to bool, and bool is an int subclass in Python — so
	# we exclude it explicitly. Matches the JS `typeof x === 'number'` check.
	return isinstance(value, (int, float)) and not isinstance(value, bool)


def fresh_start_on():
	return {k: 0 for k in SERVED_KEYS}


def load_breaker():
	try:
		data = json.loads(BREAKER_PATH.read_text(encoding="utf-8"))
	except (OSError, json.JSONDecodeError):
		data = {}
	if not isinstance(data, dict):
		data = {}
	start_on = data.get("startOn")
	if not isinstance(start_on, dict):
		data["startOn"] = fresh_start_on()
	else:
		# Defensive: ensure every service has a numeric value so downstream
		# comparisons (now < startOn[service]) always make sense.
		for k in SERVED_KEYS:
			if not _is_number(start_on.get(k)):
				start_on[k] = 0
	return data


# Trip any service whose 24-hour served exceeds its breaker. A service already
# cooling (now < startOn[service]) is left alone, so the trip moment isn't
# bumped forward minute after minute while traffic remains over threshold — the
# original release time is sacred. Each service is evaluated on its own, so
# http4 can be cooling while http6 keeps serving.
def tick_breaker(start_on, served_24h, now):
	for key in SERVED_KEYS:
		if now < start_on[key]:
			continue
		if served_24h[key] > SERVICE_BREAKERS[key]:
			start_on[key] = now + COOL_DURATION


# Per-service booleans for page.json's display. cool_down[service] is True when
# startOn[service] is in the future, meaning that service is currently paused.
def expand_cool_down(start_on, now):
	return {k: start_on[k] > now for k in SERVED_KEYS}


# ---------------------------------------------------------------------------
# Prometheus scraping
# ---------------------------------------------------------------------------

# Parse Prometheus text format into a flat dict of metric values.
def parse_prometheus(text):
	metrics = {}
	for line in text.split("\n"):
		if line.startswith("#") or line.strip() == "":
			continue
		match = re.match(r"^(\S+)\s+(\S+)$", line)
		if match:
			metrics[match.group(1)] = float(match.group(2))
	return metrics


# Extract counts by ip_version for a given response type. Returns (v4, v6),
# always both present. The counters are integer-valued; we cast to int so the
# JSON output stays integers (page.json's documented contract), matching the
# old implementation.
def extract_by_type(metrics, response_type):
	v4 = v6 = 0
	for key, value in metrics.items():
		if "aquatic_responses_total" not in key:
			continue
		if f'type="{response_type}"' not in key:
			continue
		if 'ip_version="4"' in key:
			v4 = int(value)
		if 'ip_version="6"' in key:
			v6 = int(value)
	return v4, v6


# Scrape all three Prometheus endpoints. Returns cumulative counters as a flat
# dict: {udp4, udp6, http4, http6, ws4, ws6}, all six keys always present,
# defaulting to 0.
def scrape_prometheus():
	result = {key: 0 for key in SERVED_KEYS}

	for key, url, response_type in PROMETHEUS_ENDPOINTS:
		try:
			with urllib.request.urlopen(url, timeout=5) as response:
				text = response.read().decode("utf-8")
		except (urllib.error.URLError, OSError):
			# Endpoint unreachable (local development or container down)
			continue
		metrics = parse_prometheus(text)
		v4, v6 = extract_by_type(metrics, response_type)
		result[f"{key}4"] = v4
		result[f"{key}6"] = v6
	return result


# ---------------------------------------------------------------------------
# DHT bootstrap node
# ---------------------------------------------------------------------------

# The DHT node (qBittorrent-nox) exposes its health on its Web API: one GET
# returns the routing-table size and connection state. The hostname resolves
# via Docker's internal DNS on the ftorrent-open network, and the node's WebUI
# whitelist trusts this scraper's address, so no credentials are needed.
# Locally it won't resolve, so the scrape is skipped gracefully.
DHT_ENDPOINT = "http://ftorrent-open-dht-1:8080/api/v2/transfer/info"


# Scrape the DHT node's Web API. Returns (nodes, status): routing-table size as
# an int and qBittorrent's connection_status string. On any failure
# (unreachable, bad JSON, missing fields) returns (0, "disconnected"), so the
# tick never crashes and page.json always carries the keys.
def scrape_dht():
	try:
		with urllib.request.urlopen(DHT_ENDPOINT, timeout=5) as response:
			data = json.loads(response.read().decode("utf-8"))
	except (urllib.error.URLError, OSError, json.JSONDecodeError):
		return 0, "disconnected"
	if not isinstance(data, dict):
		return 0, "disconnected"
	nodes = data.get("dht_nodes", 0)
	status = data.get("connection_status", "disconnected")
	return (
		int(nodes) if _is_number(nodes) else 0,
		status if isinstance(status, str) else "disconnected",
	)


# How many minutes the node has been continuously in the DHT — a true count of
# accumulated uninterrupted minutes, held in memory rather than on disk. "In
# the DHT" means a non-empty routing table (nodes > 0). The count resets to 0
# whenever the node drops out (0 nodes / unreachable), and also whenever the
# gauge itself restarts, since either is a break in uninterrupted observation.
_dht_session_start = None  # epoch ms the current session began, or None


def dht_uptime(nodes, now):
	global _dht_session_start
	if nodes > 0:
		if _dht_session_start is None:
			_dht_session_start = now
	else:
		_dht_session_start = None
	return 0 if _dht_session_start is None else (now - _dht_session_start) // MS_PER_MINUTE


# ---------------------------------------------------------------------------
# Memory reading
# ---------------------------------------------------------------------------

# Returns {udp, http, ws, dht} with memory in bytes, all keys always present.
def read_container_memory():
	result = {"udp": 0, "http": 0, "ws": 0, "dht": 0}
	try:
		entries = os.listdir(CGROUP_SLICE)
	except OSError:
		# cgroup path doesn't exist (local development)
		return result

	for name in entries:
		if not name.startswith("docker-") or not name.endswith(".scope"):
			continue

		scope_dir = CGROUP_SLICE / name
		try:
			current = int((scope_dir / "memory.current").read_text(encoding="utf-8"))
			# memory.max is the literal "max" for an unlimited cgroup; int()
			# raises ValueError on it, which we treat like an unreadable file —
			# that container simply isn't one of ours.
			mem_max = int((scope_dir / "memory.max").read_text(encoding="utf-8"))
		except (OSError, ValueError):
			continue  # skip unreadable cgroup files / unlimited ceilings

		key = next((k for k, ceiling in MEMORY_TARGETS if ceiling == mem_max), None)
		if key is not None:
			result[key] = current
	return result


# ---------------------------------------------------------------------------
# Atomic file writing
# ---------------------------------------------------------------------------

def write_atomic(path, data):
	tmp = path.with_name(path.name + ".tmp")
	tmp.write_text(data, encoding="utf-8")
	os.replace(tmp, path)


# ---------------------------------------------------------------------------
# Probe check
# ---------------------------------------------------------------------------

# Returns True if the probe file was touched within the last 90 seconds. False
# if the file doesn't exist or is stale (local dev, or ISP down). st_mtime is in
# seconds; we convert to ms to compare against the ms-based clock reading.
def probe_is_fresh(now):
	try:
		mtime_ms = PROBE_PATH.stat().st_mtime * 1000
	except OSError:
		return False
	return (now - mtime_ms) < PROBE_MAX_AGE_MS


# Format an epoch-ms instant as an ISO-8601 UTC string with millisecond
# precision and a trailing "Z", e.g. "2026-04-08T12:34:56.789Z" — matching what
# the old implementation wrote.
def iso_utc(now):
	dt = datetime.fromtimestamp(now // 1000, tz=timezone.utc)
	dt = dt.replace(microsecond=(now % 1000) * 1000)
	return dt.isoformat(timespec="milliseconds").replace("+00:00", "Z")


# ---------------------------------------------------------------------------
# Tick — runs once per minute
# ---------------------------------------------------------------------------

def tick():
	# Capture one clock reading and pass it everywhere downstream so every
	# derived value (probe freshness, day, minute) sees the same instant. This
	# rules out a sub-millisecond straddle of the day or minute boundary
	# producing inconsistent numbers within a tick. Epoch milliseconds, to keep
	# the day/minute arithmetic identical to the original.
	now = int(time.time() * 1000)

	# If the server can't reach the internet, skip this tick entirely. No ring
	# slot is written, so count_downtime counts this minute as down.
	if not probe_is_fresh(now):
		return

	memory = read_container_memory()
	served = scrape_prometheus()
	dht_nodes, dht_status = scrape_dht()
	dht_uptime_minutes = dht_uptime(dht_nodes, now)
	day = current_day(now)
	minute = current_minute(now)

	# Finalize any completed days before writing to the ring, so stale slots
	# from past days haven't been overwritten yet.
	ring = load_ring()
	days = load_days()
	breaker = load_breaker()
	finalize_completed_days(ring["minutes"], day, days)

	# Update the ring with current cumulative counters
	ring["minutes"][minute] = {"day": day, "served": served}
	write_atomic(RING_PATH, json.dumps(ring, separators=(",", ":")) + "\n")

	# Build page.json — all fields always present
	downtime = count_downtime(ring["minutes"], day, minute)
	history_values = build_history(ring["minutes"], day, minute, days)
	history = ",".join(str(v) for v in history_values)
	uptime = compute_uptime(history_values, day, minute)
	served_24h = compute_served_24h(served, ring["minutes"], day, minute)
	served_1m = compute_served_1m(served, ring["minutes"], day, minute)

	# Trip any service whose 24-hour served has crossed its breaker, then write
	# breaker.json (whether or not its content changed — the host's path unit
	# relies on the per-minute heartbeat). Build the per-service coolDown
	# booleans for page.json from the same in-memory state.
	tick_breaker(breaker["startOn"], served_24h, now)
	write_atomic(BREAKER_PATH, json.dumps(breaker, indent="\t") + "\n")
	cool_down = expand_cool_down(breaker["startOn"], now)

	# Per-second rate from the 24-hour total, rounded down to an integer.
	# servedSecond[k] == servedDay[k] // SECONDS_PER_DAY. Whole requests-per-
	# second is a cleaner read than a fractional rate, and the numbers are big
	# enough that integer precision is fine.
	served_sec = {k: served_24h[k] // SECONDS_PER_DAY for k in SERVED_KEYS}

	page = {
		"title": "open.ftorrent.com live tracker performance record, fresh each minute",
		"image": "https://open.ftorrent.com/images/open.ftorrent.com.jpg",
		"epoch": now,
		"when": iso_utc(now),
		"day": day,
		"minute": minute,
		"memory": memory,
		"coolDown": cool_down,
		"servedDay": served_24h,
		"servedMinute": served_1m,
		"servedSecond": served_sec,
		"uptimePercent": uptime,
		"downtimeDay": downtime,
		"downtimeDays": history,
		"dht": {
			"nodes": dht_nodes,
			"status": dht_status,
			"uptimeMinutes": dht_uptime_minutes,
		},
	}
	write_atomic(PUBLIC_DIR / "page.json", json.dumps(page, indent="\t") + "\n")


# ---------------------------------------------------------------------------
# Scheduling — resident, aligned to the top of each minute
# ---------------------------------------------------------------------------

# Sleep until ~100 ms past the top of the next UTC minute. Aligning to the
# clock (rather than sleeping a fixed 60 s) means exactly one tick per calendar
# minute, with no drift that could skip or double a minute boundary.
def sleep_to_next_minute():
	now = time.time() * 1000  # ms
	next_minute = math.ceil(now / MS_PER_MINUTE) * MS_PER_MINUTE
	delay_ms = next_minute - now + 100  # 100 ms past the boundary
	time.sleep(delay_ms / 1000)


def main():
	# Tick once immediately on startup, then align to each minute boundary. Each
	# tick is guarded: an unexpected failure leaves this minute's slot unwritten
	# — which count_downtime already reads as "down" — so we just log one line
	# to stderr (visible in `docker logs`) and carry on to the next minute. One
	# bad tick never takes the gauge down.
	while True:
		try:
			tick()
		except Exception as error:
			print(f"gauge: tick failed: {error!r}", file=sys.stderr)
		sleep_to_next_minute()


if __name__ == "__main__":
	main()
