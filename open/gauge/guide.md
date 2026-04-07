# The open.ftorrent.com gauge

*Prepared by [Claude Code](https://claude.ai/code) using Opus 4.6*
*Created: 2026-Apr-07 | Last reviewed: 2026-Apr-07*

| Component | Version |
|---|---|
| Node | 22+ |

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
