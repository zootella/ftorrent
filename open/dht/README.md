
_ftorrent/open/dht/README.md [open.ftorrent.com](https://open.ftorrent.com)_

_Five guides cover this deployment: [dockerizing Aquatic and configuring the Linux server](../README.md), the [dashboard back end](../gauge/README.md), the [circuit breaker](../breaker/README.md), the [dashboard front end](../page/README.md), and the **DHT bootstrap node** (this guide)._

# Running a Public BitTorrent DHT Bootstrap Node in Docker

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.8
> <br>Created: 2026-Jun
> <br>Last reviewed: 2026-Jun
> <br>[qBittorrent-nox](https://www.qbittorrent.org/): from `debian:bookworm-slim` (apt)
> <br>Engine: [libtorrent-rasterbar](https://www.libtorrent.org/) (the mainline DHT)
> <br>DHT port: 51420/udp
> <br>Docker Engine: 29
> <br>Docker Compose: V2

This node runs alongside the [Aquatic tracker](../README.md) on the same server, but it is a separate service answering a separate need. Three files in this directory deploy it: [`Dockerfile`](Dockerfile) (a minimal `debian:bookworm-slim` image with `qbittorrent-nox` from apt), [`qBittorrent.conf`](qBittorrent.conf) (the pure-DHT configuration), and [`compose-service.yml`](compose-service.yml) (the service entry to add to the open deployment's `docker-compose.yml`).

## What a DHT bootstrap node is

The [Aquatic tracker](../README.md) answers one question: "who else has this?" A client presents an info hash and the tracker returns a list of peers. The **distributed hash table (DHT)** answers the same question without a server, by spreading peer-discovery information across millions of participating clients. It is the mechanism behind trackerless magnet links: any client can look up "who has info hash X?" by querying its way through the table ([BEP 5](https://www.bittorrent.org/beps/bep_0005.html), a [Kademlia](https://en.wikipedia.org/wiki/Kademlia) network).

The DHT has a cold-start problem. A fresh client knows the protocol but knows no nodes, so to join the table it must first contact *some* node already in it. That first contact is **bootstrapping**, and the whole network leans on a short list of well-known nodes hardcoded into client source:

| Bootstrap node | Operated by |
|---|---|
| `router.bittorrent.com:6881` | BitTorrent Inc / Rainberry |
| `router.utorrent.com:6881` | BitTorrent Inc |
| `dht.transmissionbt.com:6881` | the Transmission project |
| `dht.libtorrent.org:25401` | Arvid Norberg (libtorrent's author) |

There is nothing protocol-special about these. They are ordinary DHT nodes with high uptime, well-populated routing tables, and stable hostnames. `dht.ftorrent.com` adds one more independent entry point to that list — another well-connected node anyone can bootstrap from, run as public infrastructure and controlled by no single client project.

## A bootstrap node is not a tracker

The two services look similar and are easy to conflate, so the distinction is worth stating. A **tracker** speaks the BitTorrent tracker protocol (HTTP or UDP announce/scrape); it is a server with a peer list. A **DHT bootstrap node** speaks the Kademlia KRPC protocol over UDP; it is a *peer* that holds a routing table of other nodes and helps a newcomer find its place in a network no one owns. They solve adjacent problems with different protocols, and neither substitutes for the other. Our deployment runs both: the Aquatic trackers at `open.ftorrent.com`, and this node at `dht.ftorrent.com`.

## Why qBittorrent-nox

A bootstrap node is just a long-running, well-connected mainline DHT node, so the real question is which software gives a correct DHT implementation that runs headless, stays up for years, and lets us *see* that it's healthy.

[qBittorrent-nox](https://www.qbittorrent.org/) — the headless build of qBittorrent — wins on that last point. Its Web API returns the node's health as a single JSON object from one request: routing-table size, connection status, and transfer counters together. For a node whose entire job is to be a reachable, well-connected DHT participant, "how many nodes are in your routing table?" *is* the health check, and qBittorrent answers it in one call with no custom code. The network-facing code underneath is [libtorrent-rasterbar](https://www.libtorrent.org/), the most mature mainline-DHT implementation in the ecosystem — the right thing to have on the one part of this that faces the open internet. The full comparison against the alternatives (transmission-daemon, mldht, the DHT libraries) is in [Software Selections](https://docs.ftorrent.com/software-selections#mainline-dht-node).

We use it as a **pure DHT citizen**: no torrents are ever added, nothing downloads or seeds, PeX and Local Service Discovery are off. It exists only to participate in the mainline DHT and report its health.

## Bootstrapping and persistence

A fair question: if a fresh DHT node knows no other nodes, how does *ours* get in — and does it start over on every restart? Two mechanisms answer it, and both are already handled, not something we configure.

**Cold start.** On its very first run, with an empty routing table, the node bootstraps the way any client does. libtorrent ships with default bootstrap nodes built in — `dht.libtorrent.org:25401`, `router.bittorrent.com:6881`, `router.utorrent.com:6881` — and contacts them to enter the DHT. The list is part of the engine; there is nothing for us to supply. This is the quiet irony of running a bootstrap node: it bootstraps off the existing public ones to *become* one itself, and every node in the table above was, once, seeded by the others. It is also why the [outbound exception](#the-outbound-exception) is load-bearing — with no outbound UDP, the node can't reach those servers and never enters the network at all.

**Warm restart.** This is where a DHT node parts ways with a tracker. A tracker keeps its peer lists in memory and discards them on restart; clients simply re-announce and refill it within minutes, so a tracker persists nothing of its own. A DHT node is the reverse: its value *is* its routing table and its stable node ID, and it would be a poor citizen to forget them and re-hammer the public bootstrap servers on every restart. So libtorrent does what every good DHT implementation does — it saves its routing table (the known-good nodes and the node ID) on shutdown and reloads it on startup. That state lives in the writable `/config` volume, so a restart rejoins the DHT warm and immediately, right where it left off. The first launch is the only true cold start; every restart after it is a warm one.

## The image

There are three ways to get qBittorrent-nox into a container: build from Debian's package, use the project's official image, or use a community image like linuxserver.io. We build from [`debian:bookworm-slim`](https://hub.docker.com/_/debian) with `qbittorrent-nox` from `apt`, for three reasons:

- **It matches the rest of the deployment.** The Aquatic containers and the gauge all run on `debian:bookworm-slim` / Debian Python — one Debian, one libc to reason about. The official qBittorrent image is Alpine (musl), and a different base for one service buys nothing here.
- **It runs cleanly as nobody under read-only.** We control the Dockerfile, so we set `USER 65534` and point every writable path at a mount, exactly like the trackers. The official and community images expect to manage their own user (PUID/PGID, s6 init) and resist a fixed `65534` + read-only root — the off-the-shelf convenience fights our hardening rather than helping it.
- **Provenance through the distribution.** Debian's security team vets the package for inclusion and backports fixes — the same provenance argument that favors the rest of the stack. And because it's a distribution package, a reader can review exactly what we install on Debian's own pages: the [package entry](https://packages.debian.org/bookworm/qbittorrent-nox) for version, source, maintainer, and changelog, and the [package tracker](https://tracker.debian.org/pkg/qbittorrent) for upload and security history. (qBittorrent's own CI image, built from source with an SBOM, is a fine alternative on provenance grounds; it just doesn't fit the base and hardening as cleanly.)

Because qBittorrent rewrites its own config and persists DHT state, the read-only root filesystem is paired with a writable `/config` bind mount and a `/tmp` tmpfs. The Dockerfile sets `HOME` and the `XDG_*` paths so that config and state land in `/config` and caches in `/tmp` — nothing else on the filesystem is ever written.

## Configuration

The pure-DHT configuration lives in [`qBittorrent.conf`](qBittorrent.conf). qBittorrent rewrites this file with `QSettings`, which strips comments, so the file itself carries no annotations — here is what each setting does and why:

| Setting | Value | Why |
|---|---|---|
| `Session\Port` | `51420` | The DHT's UDP listen port. libtorrent runs a v4 and a v6 instance on the same number, so one port is dual-stack. |
| `Session\DHTEnabled` | `true` | The whole point — participate in the mainline DHT. |
| `Session\PeXEnabled` | `false` | Peer exchange is a torrent-swarm feature; off, since there are no torrents. |
| `Session\LSDEnabled` | `false` | Local Service Discovery announces on the LAN; irrelevant to a public DHT node. |
| `Session\AddTorrentStopped` | `true` | Belt-and-suspenders: were a torrent ever added, it would not start. |
| `Session\DefaultSavePath` | `/tmp` | Points the (unused) save path at the ephemeral tmpfs rather than the config volume. |
| `WebUI\Enabled` | `true` | The Web API is the health surface (next section). |
| `WebUI\Address` / `WebUI\Port` | `*` / `8080` | Listen on all interfaces *inside the container* — reachable on the Docker network, never published to the host. |
| `WebUI\AuthSubnetWhitelistEnabled` | `true` | Let the scraper read the API without credentials… |
| `WebUI\AuthSubnetWhitelist` | `172.30.0.5/32, fd00:cafe:2::5/128` | …but only from the scraper's single address, not the whole subnet — see below. |
| `WebUI\HostHeaderValidation`, `WebUI\CSRFProtection` | `false` | Relaxed for a frictionless internal API call; safe *only* because the whitelist is one trusted host and the Web UI is never published. |

**Why a single host, not the subnet.** qBittorrent's subnet whitelist grants *unauthenticated access to the entire Web API* — not just the read-only health endpoint — to every address it covers. The ftorrent-open subnet holds the internet-facing Aquatic containers, and this is the one container with an outbound-UDP exception; a compromised tracker should not be able to drive this node's API (add a torrent, flip on transfers, change the save path) with no credentials. So the whitelist is scoped to the scraper's single pinned address (`172.30.0.5` / `fd00:cafe:2::5`) rather than the `/24` + `/64`. Set it to wherever your scraper actually runs, and pin that container's address to match — the same way this node's address is pinned for the outbound rule.

Rate limits are intentionally not set: with zero torrents there is no transfer to limit.

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
| Read-only filesystem | `read_only: true`, with a writable `/config` volume and a `/tmp` tmpfs |
| No capabilities | `cap_drop: [ALL]` |
| No privilege escalation | `security_opt: [no-new-privileges:true]` |
| Resource ceilings | `memory: 512m`, `pids: 128` |

### The outbound exception

This is the one place a DHT node cannot match the trackers, and it is the heart of the deployment. The Aquatic containers block **all** outbound traffic with `DOCKER-USER` rules — a tracker only ever answers, so it never needs to initiate a connection. A DHT node is different: it is a *participant*. To populate and refresh its routing table — and to cold-start by querying the public bootstrap nodes — it must **send** queries outward, as new UDP flows. A node that could only reply would never build a routing table, and the "300+ nodes" health check below would never pass.

So the DHT container needs a **narrowed outbound policy** rather than the trackers' blanket block: permit its outbound UDP to the internet (the DHT traffic), while still denying outbound TCP and any reach into the LAN — the kinds of connection a compromised process would use to exfiltrate data or call home. The principle holds even though this container is allowed to talk: **it can only talk DHT.**

> **Deployment note:** the surrounding infrastructure already opens the *inbound* path (51420/udp on both families, straight to the host's `INPUT` and on to the container) and the intra-subnet route the scraper uses. A per-flow conntrack timeout is already attached to udp/51420 as part of the host's table-fill defense, so the same tuning the [Aquatic guide](../README.md) applies to UDP covers this port too. The *outbound* UDP exception for this container — written against its pinned address `172.30.0.6` / `fd00:cafe:2::6` — is the one firewall rule the DHT node adds beyond what the trackers need; it must be in place, or the routing table stays empty.

### The Web UI password

The whitelist above lets the scraper read the API without a password, and the Web UI port is never published to the internet. But the same intra-subnet rule that lets the scraper reach the port also lets any *sibling* container reach it, and a non-whitelisted sibling falls back to password auth. Debian bookworm ships qBittorrent 4.5.x — which predates the random-temporary-password change in 4.6.1 — so its Web UI defaults to the well-known `admin` / `adminadmin`. A compromised sibling on the open network could log in with that default and drive the node's full API, then lean on its outbound-UDP exception.

So set a strong Web UI password before the node goes into service. The Web UI here is headless and unpublished — port 8080 is never exposed, so there's no browser to log into without tunnelling to the container through the host. Two paths suit that; either works:

- **Pre-write it (no default-password window).** qBittorrent stores the password as a PBKDF2 hash under `WebUI\Password_PBKDF2`. Generate that hash for your password and add the line to the seeded `qBittorrent.conf` *before first start*, so the node never boots with `admin` / `adminadmin` live — there's no window at all. The most secure option.
- **Set it through the API after start.** From the whitelisted scraper, POST your password to `/api/v2/app/setPreferences` (the `web_ui_password` field); qBittorrent hashes and persists it. No hash to generate, at the cost of a brief default-password window between first start and the POST — during which the Web UI is still reachable only by on-subnet containers, never the internet.

Either way the password is per-operator and never lives in this public repository, and the scraper keeps working throughout because it authenticates by *being* the whitelisted address, not by the password.

## The health surface

The Web API is how the node's health reaches the dashboard. The [gauge](../gauge/README.md) — the same scraper that meters the Aquatic trackers — polls the node once a minute and folds the result into the open.ftorrent.com dashboard's `page.json` as a `dht` block, shown there as a `DHT nodes` figure. `dht.ftorrent.com` itself is just the bootstrap node's DNS name and serves no web page of its own; a dedicated status page, if ever wanted, would be separate work. The relevant call is:

```
GET http://ftorrent-open-dht-1:8080/api/v2/transfer/info
```

which returns, among other fields:

```json
{
  "connection_status": "connected",
  "dht_nodes": 386,
  "dl_info_speed": 0,
  "up_info_speed": 0
}
```

`dht_nodes` is the routing-table size — the one number that proves the node is a real participant rather than just a listening socket. The scraper reaches the Web UI port over internal Docker DNS (allowed by the same intra-subnet `DOCKER-USER` rule the [gauge](../gauge/README.md) uses to scrape the Aquatic Prometheus endpoints), authenticating automatically because its address is the one host on the whitelist. The port is never published to the host, so nothing on the internet can reach it.

## Deployment

The node assumes the [Aquatic guide](../README.md)'s Docker daemon, network, and kernel tuning are already in place, plus the inbound firewall path for 51420/udp and the outbound exception described above.

1. **Copy this directory to the server** next to the `docker-compose.yml` (the same way the gauge's source is deployed), so the compose `build: ./dht` context resolves.

2. **Create the config directory** and give it to the container's user:

   ```bash
   sudo mkdir -p /opt/open.ftorrent.com/dht-config/qBittorrent
   sudo chown -R 65534:65534 /opt/open.ftorrent.com/dht-config
   ```

3. **Seed the configuration** — place [`qBittorrent.conf`](qBittorrent.conf) where qBittorrent reads it inside the `/config` volume (`XDG_CONFIG_HOME=/config`, so the path is `/config/qBittorrent/`):

   ```bash
   sudo install -o 65534 -g 65534 qBittorrent.conf \
     /opt/open.ftorrent.com/dht-config/qBittorrent/qBittorrent.conf
   ```

4. **Add the service** — copy the block from [`compose-service.yml`](compose-service.yml) into the `services:` section of the open deployment's `docker-compose.yml`, then build and bring it up:

   ```bash
   docker compose up -d --build qbittorrent-dht
   ```

5. **Set a strong Web UI password** — bookworm's qBittorrent defaults to the well-known `admin` / `adminadmin`; see [The Web UI password](#the-web-ui-password) above. The scraper keeps working via the whitelist regardless.

## Verifying it works

A healthy libtorrent node fills its IPv4 routing table fast — expect **300+ nodes within a few minutes** of startup. The simplest check is the dashboard, which proves the whole path at once (node → gauge scrape → `page.json`) and needs nothing installed:

```bash
curl https://open.ftorrent.com/page.json
# the "dht" block: { "nodes": 386, "status": "connected", "uptimeMinutes": ... }
```

To read the node's API directly, go through the gauge — it's the container the whitelist trusts, and like every image here it's `-slim` with no `curl`, so use its Python:

```bash
docker exec ftorrent-open-gauge-1 python3 -c \
  'import urllib.request; print(urllib.request.urlopen("http://ftorrent-open-dht-1:8080/api/v2/transfer/info").read().decode())'
# -> {"connection_status":"connected","dht_nodes":386,...}
```

A `dht_nodes` count climbing into the hundreds, with `connection_status: "connected"`, confirms the node is participating — both the inbound path and the outbound exception are working. If the count stays at or near zero, the outbound rule is the first thing to check.

**IPv6 will be sparser and slower to fill.** The IPv6 DHT is a much smaller network, so the v6 routing table stays thinner and lookups take more hops. That is expected, not a fault — the node is a good citizen on both families; it is simply lonelier on v6.

## Using it

Once it's running, `dht.ftorrent.com` is usable the way the existing public bootstrap nodes are: add it to a client's bootstrap list or include it as a DHT node hint. It is one more independent entry point into the mainline DHT, owned by no single client project. That independence is the contribution — more bootstrap diversity makes the DHT's cold-start path more resilient for everyone, the same way another public DNS resolver makes name resolution more resilient. It serves the network.
