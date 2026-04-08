
_ftorrent/open/gauge/README.md [open.ftorrent.com](https://open.ftorrent.com)_

# Public Tracker Dashboard Back End

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.6
>
> Created: 2026-Apr-07
>
> Last reviewed: 2026-Apr-07

| Component | Version |
|---|---|
| Node | 22 |

## What this is and where it fits

The [open.ftorrent.com](https://open.ftorrent.com/) deployment has three parts. The [Aquatic guide](../guide.md) sets up the tracker containers that serve BitTorrent and WebTorrent clients. The [page guide](../page/guide.md) builds the Vue frontend that visitors see. This guide covers the piece in between: the gauge, which reads statistics from the trackers and produces the data file the frontend displays.

The gauge is a Node.js script that runs in its own Docker container alongside the trackers. Once per minute it scrapes Prometheus metrics from the three Aquatic containers, reads their memory usage from cgroup files, and writes a single JSON file — `page.json` — that nginx serves as a static file. The Vue frontend fetches `page.json` and renders the dashboard. The gauge has no HTTP server and no listening ports. It only writes files.

## Why it's built this way

**Why a separate container?** The gauge needs access to Prometheus endpoints (inside the Docker network) and cgroup files (on the host). The frontend is static files served by nginx — it can't make those requests. Putting the data-gathering in a container keeps it inside the security boundary (same hardening as the trackers: nobody user, read-only filesystem, all capabilities dropped) while giving it network access to the things it needs to read.

**Why write a file instead of serving an API?** A JSON file served by nginx is the simplest possible data path. No application server in the request path, no ports to expose, no process that can be crashed by a malformed HTTP request. The gauge writes, nginx serves, the frontend reads. Each piece can fail independently — if the gauge crashes, nginx keeps serving the last good `page.json` until the gauge restarts.

**Why clock-aligned scheduling?** The gauge fires once just after the top of each UTC minute, not on a fixed interval. A 60-second `setInterval` drifts over time and can skip or double-fire at minute boundaries, which would create false gaps in the ring buffer. Clock alignment means exactly one tick per calendar minute.

## The ring buffer

The gauge maintains a private file `ring.json` with 1,440 slots — one per minute of the day. Each slot records the day number and the current cumulative Prometheus counter values at that moment. This ring is the gauge's memory: it knows what the counters were at any minute in the last 24 hours.

To compute "served in the last 24 hours," the gauge finds the oldest valid slot and subtracts its counters from the current values. One subtraction, not a sum across 1,440 slots. Gaps in the ring (the gauge was down for an hour) don't affect the math — the two endpoints are still valid, and the subtraction gives the total traffic between them.

If a Prometheus counter dropped (a tracker container restarted and the counter reset to zero), the gauge reports 0 for that metric rather than a negative or fabricated number. Every number in `page.json` is the proven growth between two ring snapshots. The gauge never reports a raw Prometheus counter.

Downtime is counted by walking the ring and checking for missing or stale slots. A slot is expected to have today's day number (or yesterday's, for slots on the far side of midnight). Any gap is a missed minute.

## What page.json contains

```json
{
	"minute": 600,
	"memory": { "udp": 5083136, "http": 27623424, "ws": 33914880 },
	"served": { "udp4": 12345, "udp6": 67, "http4": 890, "http6": 12, "ws4": 34, "ws6": 5 },
	"downtime": 0
}
```

- **minute** — current ring slot index (0 = 00:00 UTC, 1439 = 23:59 UTC)
- **memory** — current memory usage in bytes per Aquatic container, identified by matching their unique cgroup memory ceilings (a concession documented in the source — cgroup paths expose container IDs, not names)
- **served** — 24-hour totals split by protocol and IP version. UDP and HTTP count announce responses. WS counts WebRTC offers relayed (each offer is the tracker brokering a direct connection between two peers).
- **downtime** — minutes in the last 24 hours where the gauge didn't run

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
├── guide.md               This document
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

The gauge follows the same container hardening model as the Aquatic trackers (see the [Aquatic guide](../guide.md)). Copy the source to the server as `/opt/open.ftorrent.com/node-gauge/`, add the service entry from `compose-service.yml` to your docker-compose.yml, and bring it up.

Before the first run, create the data directory with the right ownership:

```bash
sudo mkdir -p /opt/open.ftorrent.com/data/public
sudo chown -R 65534:65534 /opt/open.ftorrent.com/data
```

After `docker compose up -d`, verify with:

```bash
curl https://open.ftorrent.com/page.json
```

The `minute` field should match the current UTC minute. Memory fields should be non-zero. Served fields start at 0 and grow as the ring fills over the first 24 hours.

## Security

The gauge container runs on the `ftorrent-open` network (cafe 2) with the same constraints as the trackers: nobody user, read-only filesystem, all capabilities dropped, no-new-privileges, memory and PID limits. It can reach the Aquatic Prometheus endpoints over internal Docker DNS (allowed by the intra-subnet DOCKER-USER rule) but cannot reach the LAN or internet.

The frontend static files are mounted read-only at `/static`. A compromised gauge can write wrong numbers to `page.json` but cannot inject scripts into the HTML, JS, or CSS that the browser executes. The frontend can only be changed by rsyncing from the development machine.
