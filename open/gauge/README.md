
_ftorrent/open/gauge/README.md [open.ftorrent.com](https://open.ftorrent.com)_

_Three guides cover this deployment: [dockerizing Aquatic and configuring the Linux server](../README.md), the **dashboard back end** (this guide), and the [dashboard front end](../page/README.md)._

# Public Tracker Dashboard Back End

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.6
> <br>Created: 2026-Apr
> <br>Last reviewed: 2026-Apr
> <br>Node: 22

## What this is and where it fits

The [open.ftorrent.com](https://open.ftorrent.com/) deployment has three parts. The [Aquatic guide](../README.md) sets up the tracker containers that serve BitTorrent and WebTorrent clients. The [page guide](../page/README.md) builds the Vue frontend that visitors see. This guide covers the piece in between: the gauge, which reads statistics from the trackers and produces the data file the frontend displays.

The gauge is a Node.js script that runs in its own Docker container alongside the trackers. Once per minute it scrapes Prometheus metrics from the three Aquatic containers, reads their memory usage from cgroup files, and writes a single JSON file — `page.json` — that nginx serves as a static file. The Vue frontend fetches `page.json` and renders the dashboard. The gauge has no HTTP server and no listening ports. It only writes files.

## Why it's built this way

**Why a separate container?** The gauge needs access to Prometheus endpoints (inside the Docker network) and cgroup files (on the host). The frontend is static files served by nginx — it can't make those requests. Putting the data-gathering in a container keeps it inside the security boundary (same hardening as the trackers: nobody user, read-only filesystem, all capabilities dropped) while giving it network access to the things it needs to read.

**Why write a file instead of serving an API?** A JSON file served by nginx is the simplest possible data path. No application server in the request path, no ports to expose, no process that can be crashed by a malformed HTTP request. The gauge writes, nginx serves, the frontend reads. Each piece can fail independently — if the gauge crashes, nginx keeps serving the last good `page.json` until the gauge restarts.

**Why clock-aligned scheduling?** The gauge fires once just after the top of each UTC minute, not on a fixed interval. A 60-second `setInterval` drifts over time and can skip or double-fire at minute boundaries, which would create false gaps in the ring buffer. Clock alignment means exactly one tick per calendar minute.

## The ring buffer

The gauge maintains a private file `ring.json` with 1,440 slots — one per minute of the day. Each slot records the day number and the current cumulative Prometheus counter values at that moment. This ring is the gauge's memory: it knows what the counters were at any minute in the last 24 hours.

To compute "served in the last 24 hours," the gauge walks backward from the current slot `N`, extending a candidate oldest slot `P` back one minute at a time as long as counters stay monotonic (each older slot's counters are no larger than the current `P`'s). It then reports `N − P` per metric. In the common case — no container restart in the window — `P` ends up at the slot from ~24 hours ago and the math is a full-day delta. Gaps in the ring (the gauge was down for some minutes) don't affect the math; the walk skips over them.

When a tracker container restarts, its Prometheus counter resets to zero. On the next walk, that shows up as the *reset signature*: an older slot with a counter higher than a newer slot. The walk stops at the reset boundary, keeping `P` inside the same era as `N`. The gauge then reports served counts since the restart — smaller than a full 24-hour total, but accurate for that span. On the very first tick after a restart there's no older post-restart slot yet, so `P` equals `N` and the report is `0` for that minute; by the next tick it shows a one-minute delta, growing as the ring repopulates. Every reported number is the proven growth between two different ring slots.

Downtime is counted by walking the ring and checking each slot against two conditions: the slot must exist with the expected day number (today for slots at or before the current minute, yesterday for slots after), and the internet reachability probe must have been fresh when the slot was written. A minute is only "up" if both conditions were met — the gauge ran and the server could reach the internet. See the probe section below.

## Internet reachability probe

The gauge tracks whether it ran each minute, but that alone doesn't prove the server was reachable from the internet. The gauge could be ticking along happily while the ISP is down and no BitTorrent client can reach the tracker. To report honest downtime, the gauge needs an external signal: was the server actually online?

A cron job on the host handles this. Every minute, `probe.sh` pings one of four targets — Cloudflare and Google, each on both IPv4 and IPv6:

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

**Why ping an external IP, not curl our own domain.** An earlier design had the probe curl `https://open.ftorrent.com/page.json` from the host, which would test more of the stack (DNS, nginx, TLS). The problem is hairpin NAT: many consumer routers successfully route requests from the LAN to the WAN IP back to the server, even when the ISP link is down. The probe would report "up" when the server was unreachable from the actual internet. Pinging an external IP tests real internet connectivity — if the ISP is down, the ping fails.

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
	"day": 20559,
	"minute": 600,
	"memory": { "udp": 5083136, "http": 27623424, "ws": 33914880 },
	"served": { "udp4": 12345, "udp6": 67, "http4": 890, "http6": 12, "ws4": 34, "ws6": 5 },
	"downtime": 0,
	"history": "0,0,0,3,1440,1440,0,0,...,0,0"
}
```

- **day** — current UTC day number (`Math.floor(Date.now() / 86_400_000)`), used by the frontend to compute dates for the 90-day history bars
- **minute** — current ring slot index (0 = 00:00 UTC, 1439 = 23:59 UTC)
- **memory** — current memory usage in bytes per Aquatic container, identified by matching their unique cgroup memory ceilings (a concession documented in the source — cgroup paths expose container IDs, not names)
- **served** — 24-hour totals split by protocol and IP version. UDP and HTTP count announce responses. WS counts WebRTC offers relayed (each offer is the tracker brokering a direct connection between two peers).
- **downtime** — minutes in the last 24 hours where the gauge didn't run or the server couldn't reach the internet
- **history** — 90 comma-separated downtime-minute values, one per day. First value is 89 days ago, last value is today. `0` means fully up (zero downtime minutes), `1440` means fully down (the entire day). The frontend draws this as the 90-day uptime bar chart and computes the uptime percentage from it.

All fields are always present. All numbers are 0 or positive.

## What the memory numbers mean

Every Docker container runs inside a Linux **cgroup** (control group) — a kernel subsystem that tracks and limits resources per process group. The kernel maintains a file called `memory.current` for each cgroup: the number of bytes of physical RAM assigned to that group right now. Not a sample, not an average, not what the application thinks it allocated — the kernel's own live count of every physical page assigned to the container. The binary loaded into memory, the heap, thread stacks, io_uring buffers, shared libraries, allocator overhead, everything. The container can't hide memory usage and can't undercount it. This is the most honest number available on a Linux system.

Docker exposes cgroup files through the host filesystem at `/sys/fs/cgroup`. The gauge mounts this path read-only and reads `memory.current` from each Aquatic container once per minute. That raw byte count is what appears in the memory column on the dashboard.

**What's in the number.** Each tracker's memory has a fixed component and a variable component. The fixed part is the Rust binary, the runtime's pre-allocated buffers, and — for the HTTP and WebSocket trackers — io_uring's registered memory (allocated at startup by the glommio async runtime, regardless of traffic). The variable part is the **peer table**: a hash map of IP:port entries keyed by torrent info hash. Every announce from a BitTorrent client adds or updates an entry. Each entry is small — roughly 18 to 50 bytes depending on IPv4 vs IPv6 and hash map overhead — but there can be millions. Aquatic periodically evicts expired peers (controlled by `max_peer_age` in the TOML config), so memory stabilizes at a level proportional to concurrent active peers, not total announces ever served.

At idle with near-zero traffic, the three containers use roughly 5 MB (UDP), 27 MB (HTTP), and 34 MB (WebSocket). The HTTP and WS numbers are higher because of the fixed io_uring buffers, not because they're doing more work.

**How far does modest hardware go.** Consider a server with 16 GB of physical RAM — a used enterprise small form factor desktop, the kind that sells for around $50 on eBay (or around $500 new). It runs a handful of services: a reverse proxy, a few web applications, monitoring, the usual. The UDP tracker gets a 2 GB memory ceiling in the compose file. After subtracting the ~5 MB fixed overhead, roughly 1,995 MB is available for the peer table. At a conservative 50 bytes per peer entry, that's approximately **40 million concurrent peers**.

This is peer-to-peer architecture doing what it does best. The tracker's job is small: answer "who else has this?" with a hash map lookup and a UDP response. Microseconds of work per request, a few tens of bytes of state per peer. The expensive part — actually moving the data — happens between the peers themselves, distributed across the network. The coordination service at the center stays tiny because the architecture was designed to keep it that way. One modest server can be a meaningful piece of global infrastructure, because the infrastructure was built so that modest servers are enough.

## Project structure

```
open/gauge/
├── Dockerfile             Container image definition
├── compose-service.yml    Compose service entry (add to your docker-compose.yml)
├── package.json           Workspace metadata and start script
├── probe.sh               Internet reachability probe (runs on host via cron)
├── README.md              This document
└── src/
    └── gauge.js           The gauge script
```

## Development

Run the gauge and the Vite dev server side by side. The gauge writes `page.json` to the page workspace's `public/` directory (controlled by `GAUGE_DIR`, defaults to `../page`), and the dev server picks it up.

```bash
# Terminal 1
cd open/gauge && pnpm start

# Terminal 2
cd open/page && pnpm dev
```

Locally, memory and served counts will be 0 (no cgroups or Prometheus endpoints on macOS). Downtime starts at 1439 and decreases each minute. The gauge also creates `ring.json` in `open/page/` — this is gitignored and should not be committed.

## Deployment

The gauge follows the same container hardening model as the Aquatic trackers (see the [Aquatic guide](../README.md)). Copy the source to the server as `/opt/open.ftorrent.com/node-gauge/`, add the service entry from `compose-service.yml` to your docker-compose.yml, and bring it up.

Before the first run, create the data directory with the right ownership and install the reachability probe:

```bash
sudo mkdir -p /opt/open.ftorrent.com/data/public
sudo chown -R 65534:65534 /opt/open.ftorrent.com/data
```

Install the internet reachability probe. Copy `probe.sh` to the server and make it executable:

```bash
sudo cp probe.sh /opt/open.ftorrent.com/probe.sh
sudo chmod +x /opt/open.ftorrent.com/probe.sh
```

Add a cron entry for root. This appends to the existing crontab without overwriting anything:

```bash
sudo sh -c '(crontab -l 2>/dev/null; echo "# ftorrent open tracker — internet reachability probe (every minute)"; echo "* * * * * /opt/open.ftorrent.com/probe.sh") | crontab -'
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
# * * * * * /opt/open.ftorrent.com/probe.sh
```

After `docker compose up -d`, verify with:

```bash
curl https://open.ftorrent.com/page.json
```

The `minute` field should match the current UTC minute. Memory fields should be non-zero. Served fields start at 0 and grow as the ring fills over the first 24 hours.

## Security

The gauge container runs on the `ftorrent-open` network (cafe 2) with the same constraints as the trackers: nobody user, read-only filesystem, all capabilities dropped, no-new-privileges, memory and PID limits. It can reach the Aquatic Prometheus endpoints over internal Docker DNS (allowed by the intra-subnet DOCKER-USER rule) but cannot reach the LAN or internet.

A compromised gauge can write wrong numbers to `page.json`. It cannot inject scripts into the HTML, JS, or CSS the browser executes — the frontend bundle sits in a separate host directory that nginx serves directly.
