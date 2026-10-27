
_ftorrent/open/dht/README.md [open.ftorrent.com](https://open.ftorrent.com)_

_Five guides cover this deployment: [dockerizing Aquatic and configuring the Linux server](../README.md), the [dashboard back end](../gauge/README.md), the [circuit breaker](../breaker/README.md), the [dashboard front end](../page/README.md), and the **DHT bootstrap node** (this guide)._

# Running a Public BitTorrent DHT Bootstrap Node in Docker

> Prepared by [Claude Code](https://claude.ai/code) using Opus 5.5
> <br>Created: 2026-Jun
> <br>Last reviewed: 2026-Oct
> <br>[libtorrent](https://www.libtorrent.org/): 2.1, the maintainers' wheel (the mainline DHT)
> <br>[Python](https://www.python.org/): 3.13, from `python:3.13-slim-trixie`
> <br>DHT port: 51420/udp
> <br>Docker Engine: 29
> <br>Docker Compose: V2

This node runs alongside the [Aquatic tracker](../README.md) on the same server, but it is a separate service answering a separate need. Five files in this directory deploy it: [`dht.py`](dht.py) (the program, libtorrent's DHT and nothing else), [`pyproject.toml`](pyproject.toml) and [`uv.lock`](uv.lock) (libtorrent, pinned by hash), [`Dockerfile`](Dockerfile) (the image), and [`compose-service.yml`](compose-service.yml) (the service entry to add to the open deployment's `docker-compose.yml`).

## What a DHT bootstrap node is

The [Aquatic tracker](../README.md) answers one question: "who else has this?" A client presents an info hash and the tracker returns a list of peers. The **distributed hash table (DHT)** answers the same question without a server, by spreading peer-discovery information across millions of participating clients. It is the mechanism behind trackerless magnet links: any client can look up "who has info hash X?" by querying its way through the table ([BEP 5](https://www.bittorrent.org/beps/bep_0005.html), a [Kademlia](https://en.wikipedia.org/wiki/Kademlia) network).

The DHT has a cold-start problem. A fresh client knows the protocol but knows no nodes, so to join the table it must first contact *some* node already in it. That first contact is **bootstrapping**, and the whole network leans on a short list of well-known nodes written into client source. These are the defaults in the newest releases of libtorrent, qBittorrent, and Transmission, in October 2026:

| Bootstrap node | Operated by | Default in |
|---|---|---|
| `dht.libtorrent.org:25401` | Arvid Norberg (libtorrent's author) | libtorrent 2.1, qBittorrent 5.3 |
| `dht.transmissionbt.com:6881` | the Transmission project | Transmission 4.1, qBittorrent 5.3 |
| `router.bt.ouinet.work:6881` | the Ouinet project | qBittorrent 5.3 |

There is nothing protocol-special about these. They are ordinary DHT nodes with high uptime, well-populated routing tables, and stable hostnames. The lists drift as nodes come and go, and a release keeps the list it shipped with: `router.bittorrent.com:6881`, `router.utorrent.com:6881`, and `dht.aelitis.com:6881` no longer answer, and qBittorrent listed the first through 5.2 and all three through 5.0. A DHT query from a dual-stack connection on 2026-10-27 reached `dht.libtorrent.org` over both families, `dht.transmissionbt.com` over IPv4 only, and `router.bt.ouinet.work`, which has IPv4 addresses alone, at three of its four; the three older names answered nothing. `dht.ftorrent.com` adds one more independent entry point to that list — another well-connected node anyone can bootstrap from, run as public infrastructure and controlled by no single client project.

## A bootstrap node is not a tracker

The two services look similar and are easy to conflate, so the distinction is worth stating. A **tracker** speaks the BitTorrent tracker protocol (HTTP or UDP announce/scrape); it is a server with a peer list. A **DHT bootstrap node** speaks the Kademlia KRPC protocol over UDP; it is a *peer* that holds a routing table of other nodes and helps a newcomer find its place in a network no one owns. They solve adjacent problems with different protocols, and neither substitutes for the other. Our deployment runs both: the Aquatic trackers at `open.ftorrent.com`, and this node at `dht.ftorrent.com`.

## Why libtorrent, directly

A bootstrap node is just a long-running, well-connected mainline DHT node, so the real question is which software gives a correct DHT implementation that runs headless, stays up for years, and lets us *see* that it's healthy.

The DHT itself is [libtorrent](https://www.libtorrent.org/)'s, the most mature mainline-DHT implementation in the ecosystem and the engine inside qBittorrent and Deluge — the right thing to have on the one part of this that faces the open internet. What runs it is a short program of our own, [`dht.py`](dht.py), on libtorrent's Python bindings: one session with the DHT on, no torrents, no peers, and nothing else.

The program exists for the health check. libtorrent runs a DHT node, with a routing table of its own, from each address it listens on, so a server with an address in each family runs two tables, one on IPv4 and one on IPv6. "How many nodes are in your routing table?" *is* the health check for a node whose whole job is to be a well-connected participant, and the honest answer is two numbers. libtorrent's own counter, `dht.dht_nodes`, adds the tables together, and that sum is all a client built on it usually reports: from June to October 2026 this node ran qBittorrent-nox, whose Web API gave the one sum. `dht.py` asks each table for its own count, the way the ftorrent desktop client's Connection tab does, and answers both on an internal port. It also runs on the same libtorrent wheel as the desktop client's engine, so one version, checked once, serves both. The full comparison against the alternatives is in [Software Selections](https://docs.ftorrent.com/software-selections.html#mainline-dht-node).

## Bootstrapping and persistence

A fair question: if a fresh DHT node knows no other nodes, how does *ours* get in — and does it start over on every restart?

**Cold start.** On its very first run, with empty routing tables, the node bootstraps the way any client does, from a list of bootstrap nodes. Its list is the desktop client's, `dht.libtorrent.org:25401` and `dht.transmissionbt.com:6881`, less the client's first entry, `dht.ftorrent.com:51420`, which is this node: a node can't learn the network from itself. One answering node is all a cold start needs, and libtorrent's own default is the first of the two alone. This is the quiet irony of running a bootstrap node: it bootstraps off the existing public ones to *become* one itself, and every node in the table above was, once, seeded by the others. It is also why the [outbound exception](#the-outbound-exception) is load-bearing — with no outbound UDP, the node can't reach those servers and never enters the network at all.

**Warm restart.** This is where a DHT node parts ways with a tracker. A tracker keeps its peer lists in memory and discards them on restart; clients simply re-announce and refill it within minutes, so a tracker persists nothing of its own. A DHT node is the reverse: its value *is* its routing tables and its stable node IDs, and it would be a poor citizen to forget them and re-hammer the public bootstrap servers on every restart. So `dht.py` saves the DHT's state — each node's ID and the nodes it knew — to `/config/dht.state` when the container stops and every ten minutes while it runs, and libtorrent starts from that file the next time. The first launch is the only true cold start; every restart after it is a warm one.

## The image

The [`Dockerfile`](Dockerfile) builds from [`python:3.13-slim-trixie`](https://hub.docker.com/_/python), Docker's official Python image on Debian's current release, in two stages:

- **The wheel, by hash.** The first stage holds [uv](https://docs.astral.sh/uv/), which installs libtorrent 2.1.1 from the maintainers' own wheel on PyPI, checked against the SHA-256 that [`uv.lock`](uv.lock) records for each platform and refused if it differs. The desktop client's engine is frozen against the same wheels, and the [libtorrent Provenance](https://docs.ftorrent.com/libtorrent-provenance.html) record on docs.ftorrent.com says who builds them, how, and what we checked. Python 3.13, because the wheels stop there.
- **Nothing but the program.** The second stage copies the installed environment and `dht.py` onto a fresh copy of the same base, so uv and its downloads never reach the image that runs. The image holds the Python, libtorrent, and one file of ours.
- **Nobody, read-only.** `dht.py` writes one file, its state, in the `/config` volume, so the root filesystem stays read-only with no tmpfs beside it, and the image sets `USER 65534` like the trackers.

The trackers and the gauge build on bookworm; this image is on trixie because it was built new after bookworm moved to Debian's long-term support, and nothing in it depends on which. Both are glibc Debian.

## The program

[`dht.py`](dht.py) hands libtorrent these settings and leaves the rest at libtorrent's defaults:

| Setting | Value | Why |
|---|---|---|
| `listen_interfaces` | `0.0.0.0:51420,[::]:51420` | Every address in both families at one port. In the container that's one address per family beyond loopback, so libtorrent runs two DHT nodes, one on each. The TCP sockets beside the UDP ones take nothing, since the host publishes only UDP. |
| `enable_dht` | `true` | The whole point — participate in the mainline DHT. |
| `dht_bootstrap_nodes` | `dht.libtorrent.org:25401,dht.transmissionbt.com:6881` | Where a cold start learns its first nodes, as above. |
| `enable_lsd` | `false` | Local service discovery multicasts torrents to the local network; this node has neither. |
| `enable_upnp`, `enable_natpmp` | `false` | The host publishes the port, so nothing here asks a router for anything. |
| `alert_mask` | DHT and errors | The DHT's alerts carry each table's count; the rest aren't wanted. |

Among the defaults left alone is the DHT's own upload limit, `dht_upload_rate_limit`, 8,000 bytes a second, past which libtorrent drops incoming queries until the budget refills. It is the same limit the node ran with under qBittorrent, and the first setting to look at if the node ever answers less than it's asked.

The program logs a line when it starts and when it stops, and nothing about the messages it answers.

### Choosing the port

```
    0  ┐ well-known — privileged (needs CAP_NET_BIND_SERVICE)
 1023  ┘
 1024  ┐ registered
 6881  ┤    mainline BitTorrent
 6969  ┤    tracker announce
49151  ┘
49152  ┐ dynamic / ephemeral — blends into background traffic
51413  ┤    Transmission (a fingerprint — avoid)
51420  ┤  ← our choice
65535  ┘ 0xffff
```

The DHT listens on UDP **51420**, and the number is chosen against three considerations.

**It must be ≥ 1024.** The container runs as `nobody` with all capabilities dropped, so it cannot bind a privileged port — anything below 1024 would require `CAP_NET_BIND_SERVICE`, which we don't grant.

**Blend into the noise.** IANA divides the port space into three bands: well-known (**0–1023**), registered (**1024–49151**), and dynamic/ephemeral (**49152–65535**). The dynamic band is the one the operating system draws from for temporary outbound connections, so a long-lived listener up there looks like ordinary background traffic to a firewall or ISP, and no registered service can collide with it. A high port in that range is the quiet choice.

**Don't be a fingerprint.** A few BitTorrent ports are so well known that they identify the software on sight and get profiled or blocked on restrictive networks: **6881** (the original mainline BitTorrent client), **6969** (the conventional tracker-announce port), and **51413** (hardcoded by [Transmission](https://transmissionbt.com/), now effectively a Transmission fingerprint). They all work — each one just announces what it is.

So the recipe is an arbitrary, forgettable number in **49152–65535** that isn't already associated with anything. **51420** fits: high enough to blend in, sitting right next to Transmission's **51413** so it reads as mundane without actually colliding with that fingerprint, and easy to remember — and the hundreds are a wink to Elon 🌿. As with the example domain, this is the concrete value the guide uses — substitute your own and the same reasoning applies: keep it above 1024, ideally in the dynamic range, and clear of the well-known defaults.

## Hardening

Every constraint from the [tracker model](../README.md) carries over unchanged — same user, same read-only root, same dropped capabilities — with one writable mount for state and one deliberate exception to the outbound block:

| Constraint | Setting |
|---|---|
| Non-root user | `user: "65534:65534"` (nobody) |
| Read-only filesystem | `read_only: true`, with a writable `/config` volume |
| No capabilities | `cap_drop: [ALL]` |
| No privilege escalation | `security_opt: [no-new-privileges:true]` |
| Resource ceilings | `memory: 512m`, `pids: 128` |

The ceilings are far above what the node uses, about 20 MB and six threads in testing, and stay where they are because the [gauge](../gauge/README.md) tells the containers apart by their memory ceilings.

### The outbound exception

This is the one place a DHT node cannot match the trackers, and it is the heart of the deployment. The Aquatic containers block **all** outbound traffic with `DOCKER-USER` rules — a tracker only ever answers, so it never needs to initiate a connection. A DHT node is different: it is a *participant*. To populate and refresh its routing tables — and to cold-start by querying the public bootstrap nodes — it must **send** queries outward, as new UDP flows. A node that could only reply would never build a routing table, and the health check below would read zero.

So the DHT container needs a **narrowed outbound policy** rather than the trackers' blanket block: permit its outbound UDP to the internet (the DHT traffic), while still denying outbound TCP and any reach into the LAN — the kinds of connection a compromised process would use to exfiltrate data or call home. The principle holds even though this container is allowed to talk: **it can only talk DHT.**

> **Deployment note:** the surrounding infrastructure already opens the *inbound* path (51420/udp on both families, straight to the host's `INPUT` and on to the container) and the intra-subnet route the gauge uses. A per-flow conntrack timeout is already attached to udp/51420 as part of the host's table-fill defense, so the same tuning the [Aquatic guide](../README.md) applies to UDP covers this port too. The *outbound* UDP exception for this container — written against its pinned address `172.30.0.6` / `fd00:cafe:2::6` — is the one firewall rule the DHT node adds beyond what the trackers need; it must be in place, or the routing tables stay empty.

## The health surface

The health port is how the node's health reaches the dashboard. The [gauge](../gauge/README.md) — the same scraper that meters the Aquatic trackers — polls the node once a minute and folds the result into the open.ftorrent.com dashboard's `page.json` as a `dht` block. `dht.ftorrent.com` itself is just the bootstrap node's DNS name and serves no web page of its own. The call is:

```
GET http://ftorrent-open-dht-1:8080/
```

which answers each routing table, the address it runs from, its node ID, and how many live nodes it holds:

```json
{
  "libtorrent": "2.1.1.0",
  "tables": [
    { "address": "172.30.0.6", "id": "126f5e7c45b6217bf4c2eceb7d73e7894098a6f0", "nodes": 57 },
    { "address": "fd00:cafe:2::6", "id": "47045b4192ad6e609036d9f95077cb9a070265de", "nodes": 0 }
  ]
}
```

`dht.py` asks each table for its count every ten seconds, so the answer is never older than that, and `nodes` is `null` for a table whose first count hasn't arrived. The gauge adds each table to the family of its address and writes `nodes4` and `nodes6`. The addresses are the container's own, inside the Docker network; each node ID is built from the public address the DHT sees it at, the way [BEP 42](https://www.bittorrent.org/beps/bep_0042.html) asks.

The port answers this one GET and nothing else: there is nothing on it to log in to and nothing it can change, so it needs no password and no list of trusted addresses. A sibling container on the same network can read the counts, which is all there is. The gauge reaches it over internal Docker DNS (allowed by the same intra-subnet `DOCKER-USER` rule the gauge uses to scrape the Aquatic Prometheus endpoints), and the port is never published to the host, so nothing on the internet can reach it.

## Deployment

The node assumes the [Aquatic guide](../README.md)'s Docker daemon, network, and kernel tuning are already in place, plus the inbound firewall path for 51420/udp and the outbound exception described above.

1. **Copy this directory to the server** next to the `docker-compose.yml` (the same way the gauge's source is deployed), so the compose `build: ./dht` context resolves.

2. **Create the config directory** and give it to the container's user:

   ```bash
   sudo mkdir -p /opt/open.ftorrent.com/dht-config
   sudo chown -R 65534:65534 /opt/open.ftorrent.com/dht-config
   ```

3. **Add the service** — copy the block from [`compose-service.yml`](compose-service.yml) into the `services:` section of the open deployment's `docker-compose.yml`, then build and bring it up:

   ```bash
   docker compose up -d --build libtorrent-dht
   ```

## Verifying it works

The container's log says how it started:

```bash
docker logs ftorrent-open-dht-1
# dht: no saved state, a cold start from the bootstrap nodes
# dht: libtorrent 2.1.1.0, listening on port 51420, health on port 8080
```

The simplest check is the dashboard, which proves the whole path at once (node → gauge scrape → `page.json`) and needs nothing installed:

```bash
curl https://open.ftorrent.com/page.json
# the "dht" block: { "nodes4": ..., "nodes6": ..., "status": "connected", "uptimeMinutes": ... }
```

To read the node directly, go through the gauge — like every image here it's `-slim` with no `curl`, so use its Python:

```bash
docker exec ftorrent-open-gauge-1 python3 -c \
  'import urllib.request; print(urllib.request.urlopen("http://ftorrent-open-dht-1:8080/").read().decode())'
```

The tables appear within about fifteen seconds of a cold start, once the DHT has bootstrapped, and an IPv4 table passes fifty nodes within the first two minutes and keeps growing for hours. libtorrent's table holds at most 240 nodes in its four buckets for the farthest nodes and 8 in each bucket closer in, and how many of those closer buckets fill depends on how many DHT nodes have IDs near the table's own, so a long-running table levels off in the hundreds. A count climbing past the first few dozen confirms the node is participating — both the inbound path and the outbound exception are working. If a table stays at or near zero, the outbound rule is the first thing to check.

**IPv6 will be sparser and slower to fill.** The IPv6 DHT is a much smaller network, so the v6 routing table stays thinner and lookups take more hops. That is expected, not a fault — the node is a good citizen on both families; it is simply lonelier on v6.

## Updating

The image holds what it was built with until it's built again: the Python and Debian files of the base it pulled, and the libtorrent wheel the lock names. The host's own updates never reach inside a container. Each part updates its own way:

```bash
docker compose build --pull libtorrent-dht
docker compose up -d libtorrent-dht
```

- **Python and the base.** `python:3.13-slim-trixie` is a tag Docker's maintainers refresh with each Python patch release and Debian's fixes, and `--pull` fetches the current one, which rebuilds both stages on top of it. Without `--pull`, Docker reuses the base it already holds.
- **libtorrent.** The wheel changes only when the pin does: a new version in `pyproject.toml` and `uv.lock`, which a rebuild then fetches by its new hash. libtorrent parses every DHT message that reaches 51420/udp from the internet, so a libtorrent release is worth taking promptly; the pin moves together with the desktop client's engine, and the libtorrent Provenance record logs each move.

The routing tables and node IDs are in the `/config` volume, so the new container rejoins the DHT warm, as after any restart.

## Using it

Once it's running, `dht.ftorrent.com` is usable the way the existing public bootstrap nodes are: add it to a client's bootstrap list or include it as a DHT node hint. It is one more independent entry point into the mainline DHT, owned by no single client project. That independence is the contribution — more bootstrap diversity makes the DHT's cold-start path more resilient for everyone, the same way another public DNS resolver makes name resolution more resilient. It serves the network.

### Write it as `host:port`, with no scheme

The address is `dht.ftorrent.com:51420` — a bare host and port, *not* a URL, and deliberately so. This is the formatting counterpart of the [tracker-vs-node distinction](#a-bootstrap-node-is-not-a-tracker) above, and it trips people up because the trackers right beside it on the dashboard *do* carry a scheme:

```
udp://open.ftorrent.com:443/announce     ← UDP tracker
https://open.ftorrent.com/announce       ← HTTP tracker
wss://open.ftorrent.com                  ← WebTorrent (WebSocket) tracker
dht.ftorrent.com:51420                   ← DHT bootstrap node (no scheme)
```

A tracker announce URL carries a scheme — `udp://`, `https://`, `wss://` — because a client reads the protocol to speak *from* that scheme; there are several tracker protocols and it has to know which one. A DHT bootstrap node has no scheme because there is only one DHT wire protocol (Kademlia KRPC over UDP, [BEP 5](http://www.bittorrent.org/beps/bep_0005.html)), so there is nothing to disambiguate — and accordingly **no `dht://` scheme exists**. BEP 5 specifies nodes as plain host-and-port pairs (`nodes = [["<host>", <port>], ...]`), and every public bootstrap node is written the same way — `dht.libtorrent.org:25401`, `dht.transmissionbt.com:6881`, `router.bt.ouinet.work:6881`. So a client's "DHT bootstrap nodes" config field and a `.torrent`'s `nodes` list both want the bare `host:port`; prefixing it with `dht://` (or anything else) is non-standard and won't parse. If you ever see someone "fix" the address by adding a scheme, this is why not to.
