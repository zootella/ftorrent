
_ftorrent/open/gauge/README.md [open.ftorrent.com](https://open.ftorrent.com)_

_Five guides cover this deployment: [dockerizing Aquatic and configuring the Linux server](../README.md), the **dashboard back end** (this guide), the [circuit breaker](../breaker/README.md), the [dashboard front end](../page/README.md), and the [DHT bootstrap node](../dht/README.md)._

# Public Tracker Dashboard Back End

> Prepared by [Claude Code](https://claude.ai/code) using Opus 5.5
> <br>Created: 2026-Apr
> <br>Last reviewed: 2026-Oct
> <br>Python: 3.12

## What this is and where it fits

The [open.ftorrent.com](https://open.ftorrent.com/) dashboard draws on three parts of the deployment. The [Aquatic guide](../README.md) sets up the tracker containers that serve BitTorrent and WebTorrent clients. The [page guide](../page/README.md) builds the Vue frontend that visitors see. This guide covers the piece in between: the gauge, which reads statistics from the trackers and produces the data file the frontend displays.

The gauge is a Python script that runs in its own Docker container alongside the trackers. Once per minute it scrapes Prometheus metrics from the three Aquatic containers and health from the DHT node, reads all four containers' memory usage from cgroup files, and writes a single JSON file — `page.json` — that the reverse proxy serves as a static file. The Vue frontend fetches `page.json` and renders the dashboard. The gauge has no HTTP server and no listening ports. It only writes files.

## Why it's built this way

**Why a separate container?** The gauge needs access to Prometheus endpoints (inside the Docker network) and cgroup files (on the host). The frontend is static files served by the reverse proxy — it can't make those requests. Putting the data-gathering in a container keeps it inside the security boundary (same hardening as the trackers: nobody user, read-only filesystem, all capabilities dropped) while giving it network access to the things it needs to read.

**Why write a file instead of serving an API?** A JSON file served by the reverse proxy is the simplest possible data path. No application server in the request path, no ports to expose, no process that can be crashed by a malformed HTTP request. The gauge writes, the reverse proxy serves, the frontend reads. Each piece can fail independently — if the gauge crashes, the reverse proxy keeps serving the last good `page.json` until the gauge restarts.

**Why clock-aligned scheduling?** The gauge fires once just after the top of each UTC minute, not on a fixed interval. A fixed 60-second sleep between ticks drifts over time and can skip or double-fire at minute boundaries, which would create false gaps in the ring buffer. Clock alignment means exactly one tick per calendar minute.

## The ring buffer

The gauge maintains a private file `ring.json` with 1,440 slots — one per minute of the day. Each slot records the day number and the current cumulative Prometheus counter values at that moment. This ring is the gauge's memory: it knows what the counters were at any minute in the last 24 hours.

To compute "served in the last 24 hours," the gauge walks backward from the current slot `N`, extending a candidate oldest slot `P` back one minute at a time as long as counters stay monotonic (each older slot's counters are no larger than the current `P`'s). It then reports `N − P` per metric. In the common case — no container restart in the window — `P` ends up at the slot from ~24 hours ago and the math is a full-day delta. Gaps in the ring (the gauge was down for some minutes) don't affect the math; the walk skips over them.

When a tracker container restarts, its Prometheus counter resets to zero. On the next walk, that shows up as the *reset signature*: an older slot with a counter higher than a newer slot. The walk stops at the reset boundary, keeping `P` inside the same era as `N`. The gauge then reports served counts since the restart — smaller than a full 24-hour total, but accurate for that span. And because the walk is joint — a single oldest slot `P` serves all six counters — a restart of any one tracker container re-bases the whole six-cell window: every service's 24-hour total reaccumulates from the restart moment, not just the restarted tracker's. That's conservative in the right direction for the circuit breaker, which briefly sees less than a true 24-hour count, never more. On the very first tick after a restart there's no older post-restart slot yet, so `P` equals `N` and the report is `0` for that minute; by the next tick it shows a one-minute delta, growing as the ring repopulates. Every reported number is the proven growth between two different ring slots.

Downtime is counted by walking the ring and checking each slot against two conditions: the slot must exist with the expected day number (today for slots at or before the current minute, yesterday for slots after), and the internet reachability probe must have been fresh when the slot was written. A minute is only "up" if both conditions were met — the gauge ran and the server could reach the internet. See the probe section below.

The ring forgets a day as its slots are overwritten, so before that happens the gauge counts each finished day's up-minutes into a second private file, `days.json`, keyed by day number and trimmed to the last 90 days. The dashboard's 90-day history comes from there, and a day with no entry — no gauge was running to write one — counts as fully down.

## Internet reachability probe

The gauge tracks whether it ran each minute, but that alone doesn't prove the server was reachable from the internet. The gauge could be ticking along happily while the server's connection to the internet is down and no BitTorrent client can reach the tracker. To report honest downtime, the gauge needs an external signal: was the server actually online?

A cron job on the host handles this. Every minute, `probe.py` pings one of four targets — Cloudflare and Google, each on both IPv4 and IPv6:

| Target | Address |
|---|---|
| Cloudflare IPv4 | `1.1.1.1` |
| Google IPv4 | `8.8.8.8` |
| Cloudflare IPv6 | `2606:4700:4700::1111` |
| Google IPv6 | `2001:4860:4860::8888` |

Each run randomly picks one, spreading the load across providers and protocols — the tracker serves both IPv4 and IPv6 clients, so probing both catches single-stack outages that a v4-only probe would miss. Over time each target gets roughly one ping every four minutes.

If the first ping fails — a dropped packet, a brief routing hiccup, a provider edge momentarily busy — the probe retries once against a backup target that is always the other provider AND the other protocol. The backup index is found by XOR 3, which maps each of the four targets to its diagonal opposite in the 2×2 grid of (provider × protocol):

```
Index   Target            XOR 3   Backup
0       Cloudflare v4     3       Google v6
1       Google v4         2       Cloudflare v6
2       Cloudflare v6     1       Google v4
3       Google v6         0       Cloudflare v4
```

This maximizes independence between the two attempts: different provider, different protocol, different network path. To register as down, both pings must fail within the same minute — that's a real outage, not a fluke. Each ping has a 5-second timeout; worst case (two timeouts) is 10 seconds, well within the 60-second cron interval.

If either ping succeeds, the probe touches a file. If both fail, the file's modification time goes stale. One or two pings, one touch, no output, no logs.

The gauge reads this file's mtime each tick. If the mtime is within the last 90 seconds (not 60 — the extra 30 seconds absorbs scheduling jitter between cron and the gauge's clock-aligned tick), the server had internet connectivity during this minute. That minute counts as "up." If the file doesn't exist or is stale, the minute counts as down regardless of whether the gauge itself ran.

**Why this runs on the host, not in the container.** The gauge container is firewalled — the DOCKER-USER chain blocks all outbound traffic from the tracker network to the internet. This is a deliberate security constraint documented in the [Aquatic guide](../README.md). Punching a hole for the gauge to make outbound pings would weaken that guarantee. Running the probe on the bare-metal host avoids any firewall changes. The host already has unrestricted outbound, and the result is shared with the gauge through the existing bind mount — the probe writes to `/opt/open.ftorrent.com/data/probe` on the host, and the gauge sees it at `/gauge/probe`.

**Why ping someone else's address, not fetch our own page.** Fetching `https://open.ftorrent.com/page.json` from the server would exercise more of the stack — DNS, the reverse proxy, TLS — but from the wrong place. A request from a machine to its own public name can be answered without ever crossing the internet, so it succeeds even when nothing outside can reach the machine. A ping to an address on another network has to go out and come back, and that round trip is the one fact the probe exists to learn.

**What is cron.** Cron is the standard Linux job scheduler. It runs in the background on every Linux system (and macOS) and executes commands on a schedule you define. Each line in the crontab (cron table) specifies when to run and what to run. The format is five time fields followed by a command:

```
# ┌───────────── minute (0-59)
# │ ┌─────────── hour (0-23)
# │ │ ┌───────── day of month (1-31)
# │ │ │ ┌─────── month (1-12)
# │ │ │ │ ┌───── day of week (0-7, 0 and 7 are Sunday)
# │ │ │ │ │
# * * * * * command
```

Five asterisks (`* * * * *`) means "every minute of every hour of every day." That's the schedule for the probe — it runs once per minute, matching the gauge's own tick rate. Note there's no `60` or `60000` anywhere — cron's language is declarative ("run at every minute that matches") not interval-based ("run every N milliseconds"). You describe *when*, not *how often*.

## What page.json contains

```json
{
	"title": "open.ftorrent.com live tracker performance record, fresh each minute",
	"image": "https://open.ftorrent.com/images/open.ftorrent.com.jpg",
	"epoch": 1779269280153,
	"when": "2026-05-20T09:28:00.153Z",
	"day": 20593,
	"minute": 568,
	"memory": { "udp": 129449984, "http": 62971904, "ws": 31092736, "dht": 50049024 },
	"coolDown": { "udp4": false, "udp6": false, "http4": false, "http6": false, "ws4": false, "ws6": false },
	"servedDay":    { "udp4": 3594247, "udp6": 454780, "http4": 3628188, "http6": 744186, "ws4": 0, "ws6": 0 },
	"servedMinute": { "udp4":   43567, "udp6":  10291, "http4":   46195, "http6":  16119, "ws4": 0, "ws6": 0 },
	"servedSecond": { "udp4":      41, "udp6":      5, "http4":      41, "http6":      8, "ws4": 0, "ws6": 0 },
	"uptimePercent": 97.76041140690909,
	"downtimeDay": 1,
	"downtimeDays": "0,0,0,3,1440,1440,0,0,...,0,0",
	"dht": { "nodes4": 401, "nodes6": 341, "status": "connected", "uptimeMinutes": 1234 }
}
```

- **title** and **image** — a human-readable description and the Open Graph card URL, for link previews and for anyone reading the file by hand
- **epoch** — the moment of this tick, in milliseconds since 1970; `day` and `minute` are cut from it
- **when** — the same moment as an ISO 8601 UTC string, to the millisecond
- **day** — current UTC day number (`floor(epoch / 86_400_000)`), used by the frontend to compute dates for the 90-day history bars
- **minute** — current ring slot index (0 = 00:00 UTC, 1439 = 23:59 UTC)
- **memory** — current memory usage in bytes per container, the three Aquatic trackers and the DHT node; the memory section below says how the gauge reads and tells them apart
- **coolDown** — six booleans, one per service (`udp4`, `udp6`, `http4`, `http6`, `ws4`, `ws6`). `true` means that service is currently paused. The [circuit breaker](../breaker/README.md) acts on the same decision, which it reads from the gauge's private `breaker.json`, with 503s at the reverse proxy and DROP rules in iptables; these booleans are for display.
- **servedDay** — 24-hour totals split by protocol and IP version. UDP and HTTP count announce responses; WS counts WebRTC offers relayed (each offer is the tracker brokering a direct connection between two peers).
- **servedMinute** — same six keys, for the most recent one-minute window. Useful for live-rate displays that need a recent number rather than a 24-hour total.
- **servedSecond** — `floor(servedDay / 86_400)` per service: the per-second average rate as a whole number, written here once so every consumer reads the same value. Integer because a fractional request-per-second isn't a meaningful unit, and at typical traffic the numbers are large enough that floor's bias is invisible.
- **uptimePercent** — the share of the last 90 days' minutes that were up, today counting only the minutes so far; the frontend derives the same figure from `downtimeDays`
- **downtimeDay** — minutes in the last 24 hours where the gauge didn't run or the server couldn't reach the internet
- **downtimeDays** — comma-separated downtime-minute values, one per UTC day, most-recent last. Always 90 entries, the 90-day window the dashboard shows. `0` means a fully up day (zero downtime minutes), `1440` means fully down (the entire day). The last entry is today and is the only partial value — it grows through the day as `minute` advances. The frontend draws this as the uptime bar chart.
- **dht** — the DHT bootstrap node's health, read from its health port in one GET (`ftorrent-open-dht-1:8080/`): `nodes4` and `nodes6` (the live nodes in its IPv4 and IPv6 routing tables, each table counted toward the family of the address it runs from), `status` (the gauge's own word: `connected` when the node answered with nodes in its tables, `disconnected` otherwise), and `uptimeMinutes` (a true count of accumulated uninterrupted minutes the node has been observed in the DHT — held in memory, so it resets if the node drops out or the gauge restarts). A bounded health gauge, not a cumulative counter — expect a few hundred nodes on each family, holding roughly steady, with IPv6 the thinner. It sits last in the file, after the tracker data, since it's a separate service. See the [DHT node guide](../dht/README.md).

All fields are always present. All numeric values are 0 or positive integers, except `uptimePercent`, which is a float between 0 and 100.

## How the gauge reads memory

Every Docker container runs inside a Linux **cgroup** (control group) — a kernel subsystem that tracks and limits resources per process group. The kernel maintains a file called `memory.current` for each cgroup: the bytes of memory charged to that group right now. Not a sample, not an average, not what the application thinks it allocated — the kernel's own live count, covering the binary loaded into memory, the heap, thread stacks, io_uring buffers, shared libraries, and allocator overhead alike. The container can't hide memory usage and can't undercount it.

The gauge mounts the host's `/sys/fs/cgroup` read-only at `/host-cgroup` and, once a minute, reads `memory.current` for each container scope under `system.slice`. That raw byte count is what the dashboard's memory column shows.

**Telling the containers apart.** The cgroup tree names each container's directory by its ID, as `docker-<id>.scope`, not by its name, and the ID changes on every restart. Mounting the Docker socket would answer the question but hand the gauge control of Docker itself, and passing the IDs in as environment variables would go stale at the first restart. So the gauge reads each scope's `memory.max` as well, and matches it against ceilings the compose files set deliberately unique: 2001, 2002, and 2003 MiB for the UDP, HTTP, and WebSocket trackers, and 512 MiB for the DHT node. A scope whose ceiling matches none of them, or that has no ceiling at all, isn't one of ours, and the gauge skips it.

For what these numbers come to on a busy public deployment, beside the processor, bandwidth, and connection tracking, see [Tracker Load](https://docs.ftorrent.com/tracker-load).

## Project structure

```
open/gauge/
├── Dockerfile             Container image definition
├── compose-service.yml    Compose service entry (add to your docker-compose.yml)
├── data/                  Local stand-in for the server's data directory, with the sample page.json
├── probe.py               Internet reachability probe (runs on host via cron)
├── README.md              This document
├── requirements.txt       The gauge's one dependency, ergo, pinned by version and hash
└── src/
    └── gauge.py           The gauge script
```

## Development

Run the gauge and the Vite dev server side by side. The gauge writes `page.json` into `data/public/` in this folder (controlled by `GAUGE_DIR`, which defaults to `data/` here and is `/gauge` in the container), and the page workspace's dev server answers `/page.json` from there.

Locally there's no host cron touching the probe, and the gauge only ticks when the probe is fresh — so without it `tick()` treats every minute as offline and writes nothing. Stand in for the cron job with a third terminal that keeps the marker under 90 seconds old.

```bash
# Terminal 1 — the gauge, with ergo from requirements.txt
cd open/gauge && uv run --with-requirements requirements.txt src/gauge.py

# Terminal 2 — the Vite dev server
cd open/page && pnpm dev

# Terminal 3 — stand in for the host's probe cron
cd open/gauge && while true; do touch data/probe; sleep 30; done
```

On a development machine, memory and served counts will be 0, since there are no tracker containers to read. Downtime starts at 1439 and decreases each minute.

`data/` stands in for the server's data directory. It holds a committed sample, `data/public/page.json`, with round numbers and a 90-day history, so the dashboard has something to draw with no gauge running at all. A local gauge run writes over it, and `git restore open/gauge/data/public/page.json` puts it back. The gauge's private files, `ring.json`, `days.json`, and `breaker.json`, land in `data/` beside the probe marker, and git ignores all four. None of it reaches the server: the image copies only `src/`, and the page's build carries no `page.json`.

## Deployment

The gauge follows the same container hardening model as the Aquatic trackers (see the [Aquatic guide](../README.md)). Copy this folder's `Dockerfile`, `requirements.txt`, and `src/` to the server as `/opt/open.ftorrent.com/python-gauge/`, add the service entry from `compose-service.yml` to your docker-compose.yml, and bring it up. The image fetches ergo from PyPI as it builds, the way the tracker images fetch their crates; once running, the container reaches nothing outside its network.

Before the first run, create the data directory with the right ownership:

```bash
sudo mkdir -p /opt/open.ftorrent.com/data/public
sudo chown -R 65534:65534 /opt/open.ftorrent.com/data
```

Install the internet reachability probe. It's a stdlib-only Python script — no virtualenv or pip install, it runs under the host's system `python3`. Copy `probe.py` to the server and make it executable (the shebang lets cron run it directly):

```bash
sudo cp probe.py /opt/open.ftorrent.com/probe.py
sudo chmod +x /opt/open.ftorrent.com/probe.py
```

Add a cron entry for root. This appends to the existing crontab without overwriting anything:

```bash
sudo sh -c '(crontab -l 2>/dev/null; echo "# ftorrent open tracker — internet reachability probe (every minute)"; echo "* * * * * /opt/open.ftorrent.com/probe.py") | crontab -'
```

Wait about 60 seconds, then verify the probe file exists and is being touched:

```bash
stat /opt/open.ftorrent.com/data/probe
# Should show a modification time within the last 90 seconds
```

Verify the gauge container can see it through the bind mount:

```bash
docker exec ftorrent-open-gauge-1 stat /gauge/probe
# Same modification time
```

Verify the crontab entry:

```bash
sudo crontab -l
# Should include:
# # ftorrent open tracker — internet reachability probe (every minute)
# * * * * * /opt/open.ftorrent.com/probe.py
```

After `docker compose up -d`, verify with:

```bash
curl https://open.ftorrent.com/page.json
```

The `minute` field should match the current UTC minute. Memory fields should be non-zero. Served fields start at 0 and grow as the ring fills over the first 24 hours.

## Security

The gauge container runs on the `ftorrent-open` network (cafe 2), at the pinned address `172.30.0.5` / `fd00:cafe:2::5`, with the same constraints as the trackers: nobody user, read-only filesystem, all capabilities dropped, no-new-privileges, memory and PID limits. It can reach the Aquatic Prometheus endpoints and the DHT node's health port over internal Docker DNS (allowed by the intra-subnet DOCKER-USER rule) but cannot reach anything outside that network. Its one dependency beyond the standard library is installed at build time from a requirements file that pins both the version and the hash of the exact file, so pip refuses anything else.

The data directory is where the gauge's writes meet the host: the probe, the breaker, and the reverse proxy each act on files inside it, so the design keeps the gauge itself out of reach rather than second-guessing what it writes. It has no listening ports, no route to the internet, and nothing to fetch but the trackers' metrics and the DHT node's health on the internal network, so reaching it would take breaking an internet-facing tracker or the DHT node first. The frontend bundle sits in a separate host directory the reverse proxy serves directly, so not even the gauge's own output can change the HTML, JS, or CSS a browser runs.
