
_ftorrent/open/dht/README.md [open.ftorrent.com](https://open.ftorrent.com)_

_Five guides cover this deployment: [dockerizing Aquatic and configuring the Linux server](../README.md), the [dashboard back end](../gauge/README.md), the [circuit breaker](../breaker/README.md), the [dashboard front end](../page/README.md), and the **DHT bootstrap node** (this guide)._

# Running a Public BitTorrent DHT Bootstrap Node in Docker

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.8
> <br>Created: 2026-Jun
> <br>Last reviewed: 2026-Jun
> <br>[qBittorrent](https://www.qbittorrent.org/): nox (version pending deployment)
> <br>[libtorrent](https://www.libtorrent.org/): 2.x (mainline DHT)
> <br>Docker Engine: 29
> <br>Docker Compose: V2

> **Draft.** The concept and rationale below are settled. The deployment specifics — exact package version, listen port, and the outbound firewall posture — are being finalized against the running node at `dht.ftorrent.com` and will be filled in here as they're confirmed.

## What a DHT bootstrap node is

The [Aquatic tracker](../README.md) this repository deploys answers one question: "who else has this?" A BitTorrent client presents an info hash, and the tracker hands back a list of peers. That's one way clients find each other. The **distributed hash table (DHT)** is another, and it's the one that works when there's no tracker at all.

The DHT ([BEP 5](https://www.bittorrent.org/beps/bep_0005.html), a [Kademlia](https://en.wikipedia.org/wiki/Kademlia) network) spreads peer-discovery information across millions of participating clients instead of concentrating it on a server. Any client can look up "who has info hash X?" by querying its way through the table, and any client can announce "I have X." No central coordinator. This is the mechanism that makes trackerless magnet links work.

But the DHT has a chicken-and-egg problem of its own. A client starting fresh — newly installed, no saved state — knows the protocol but doesn't know a single node to talk to. To join the table, it has to contact *some* node already in it. That first contact is **bootstrapping**, and the nodes that serve it are **bootstrap nodes**.

The whole BitTorrent network leans on a small handful of them, hardcoded into client source code:

| Bootstrap node | Operated by |
|---|---|
| `router.bittorrent.com:6881` | BitTorrent Inc / Rainberry |
| `router.utorrent.com:6881` | BitTorrent Inc |
| `dht.transmissionbt.com:6881` | the Transmission project |
| `dht.libtorrent.org:25401` | Arvid Norberg (libtorrent's author) |

There is nothing protocol-special about these. They are ordinary DHT nodes that happen to have high uptime, well-populated routing tables, and stable domain names — and whose hostnames ended up in client source code. They answer the same `find_node` queries any DHT node answers. A new client sends `find_node` for an ID near its own, gets back a handful of nearby nodes, queries those, and within a few round trips has a populated routing table and is a full participant. The bootstrap node's job is done in the first second and the client never needs it again until the next cold start.

`dht.ftorrent.com` adds one more independent entry point to that short list — another well-connected, stable node any client can bootstrap from, run as public infrastructure alongside the tracker.

## A bootstrap node is not a tracker

This is the distinction that matters most, because the two services look similar from a distance and are easy to conflate.

A **tracker** speaks the BitTorrent tracker protocol — HTTP or UDP announce/scrape ([BEP 3](https://www.bittorrent.org/beps/bep_0003.html), [BEP 15](https://www.bittorrent.org/beps/bep_0015.html)). It is a server. Clients ask it about a specific info hash and it answers from its own peer list.

A **DHT bootstrap node** speaks the Kademlia KRPC protocol — the DHT's own query language, carried over UDP. It is a peer. It doesn't hold a master list of anything; it holds a routing table of *other nodes*, and it helps a newcomer find its place in a network that no one owns.

They solve adjacent problems — both are about peers finding each other — but they are different protocols doing different work, and one cannot stand in for the other. Our deployment runs both, side by side, on the same infrastructure: the Aquatic trackers at `open.ftorrent.com`, and the DHT node at `dht.ftorrent.com`.

A reasonable question is why not bootstrap the DHT *through* a tracker — announce a well-known info hash and get back DHT-capable peers. It would work. But the DHT was designed in part to make trackers unnecessary, so bootstrapping it through a tracker would invert the dependency the design was meant to remove. A dedicated bootstrap node keeps the two systems independent, which is the point of having two systems.

## Why qBittorrent-nox

A bootstrap node is just a long-running, well-connected mainline DHT node, so the software question is really: which client gives us a correct, well-maintained DHT implementation that runs headless and stays up?

[qBittorrent-nox](https://www.qbittorrent.org/) is the "no X" (no graphical interface) build of qBittorrent — the same client, made to run on a server. It's a strong fit here:

- **It's libtorrent underneath.** qBittorrent is a front end over [Rasterbar libtorrent](https://www.libtorrent.org/), the most battle-tested BitTorrent engine in existence and the reference implementation of the mainline DHT. The DHT behavior we're exposing is libtorrent's — the same code that runs `dht.libtorrent.org`, decades of refinement, correct by default.
- **It's packaged and maintained.** qBittorrent-nox ships as a Debian/Ubuntu package with an active upstream and a security-conscious maintainer community. That matters for an internet-facing service we intend to keep running for years — updates arrive through the normal package channel rather than a from-source build pipeline we'd own.
- **It's headless by design.** The `-nox` build expects to run as a daemon, configured by file and managed through a Web UI we don't need to expose. No display, no desktop dependencies.
- **It's a known-good network citizen.** DHT, PEX, uTP, encryption, IPv6 — all on, all standard, all the things that make a node useful to the swarm rather than a dead end.

The trade-off is that qBittorrent-nox is a *full client*, and we want only its DHT participation — not downloading, not seeding, not tracker announces. The deployment configures it down to that role (covered below). A purpose-built bootstrap daemon like libtorrent's own [`dht-bootstrap`](https://github.com/arvidn/dht-bootstrap) is the other end of the spectrum — minimal, single-purpose — and is a reasonable alternative; we chose qBittorrent-nox for the maintenance story and the libtorrent DHT it carries. See [Software Selections](https://docs.ftorrent.com/software-selections#mainline-dht-node) on docs.ftorrent.com for how this fits the rest of the stack.

## The security model is different from the trackers

The [Aquatic guide](../README.md) hardens the tracker containers to a specific shape: run as nobody, read-only filesystem, all capabilities dropped, no-new-privileges, and — critically — **no outbound network at all**. The `DOCKER-USER` rules let the tracker containers respond to inbound packets but never initiate a connection. A tracker only ever answers questions, so that posture costs it nothing and closes off exfiltration and lateral movement entirely.

**A DHT node cannot run under that exact rule, and understanding why is the heart of this deployment.**

A DHT node is a *participant*, not just a responder. To do its job it must reach out: it sends `find_node` and `ping` queries to other nodes to build and refresh its routing table, it re-announces itself, and it follows the table outward as it changes. A node that can only answer inbound packets and never send its own queries would never populate a routing table in the first place — it would be a bootstrap node with nothing to bootstrap anyone into. Outbound UDP to the DHT is the service.

So the container keeps every hardening layer the trackers use **except** the blanket outbound block — and in its place puts a *narrowed* outbound policy: the node may send the DHT traffic it needs (UDP, to the wider internet, for the DHT) while still being denied the kinds of outbound that would signal compromise. The exact rule set — which protocols and ports are permitted outbound, and how tightly the rest is closed — is being finalized against the running node and will be documented here. The principle is fixed even while the specifics settle: **a DHT node is allowed to talk; the hardening makes sure it can only talk DHT.**

Everything else carries over from the tracker model unchanged:

| Constraint | Setting | Carries over? |
|---|---|---|
| Non-root user | `user: "65534:65534"` | Yes |
| Read-only filesystem | `read_only: true` | Yes (with a writable volume for DHT state — see below) |
| No capabilities | `cap_drop: [ALL]` | Yes |
| No privilege escalation | `security_opt: [no-new-privileges:true]` | Yes |
| Outbound network | `DOCKER-USER` rules | **Narrowed, not blocked** — DHT participation requires it |

One more difference from the trackers: a useful bootstrap node benefits from **persisting its routing table** across restarts, the way every desktop client saves its DHT state to disk. A node that reloads a warm routing table on restart is immediately useful again; one that starts cold has to rebuild from the other public bootstrap nodes first. That means a small writable volume for DHT state, rather than the trackers' fully ephemeral filesystem.

## Deployment

> This section will be filled in from the running `dht.ftorrent.com` node. The shape it will take, matching the other guides in this set:
>
> - **DNS and port.** `dht.ftorrent.com` resolves to the server's dual-stack address; the node listens on a single UDP port for KRPC. (Bootstrap nodes commonly use 6881 or a high port like libtorrent's 25401; the deployed value goes here.)
> - **The container.** A minimal image running `qbittorrent-nox`, configured to DHT-only: DHT and Local Service Discovery on, downloading/seeding and tracker announces off, Web UI bound to localhost (or off) and never exposed to the internet.
> - **Configuration.** The `qBittorrent.conf` settings that reduce the client to a pure DHT participant, with each non-default change commented — same convention as the Aquatic TOML configs.
> - **Hardening.** The full constraint table above as compose settings, plus the narrowed `DOCKER-USER` outbound policy that permits DHT traffic and denies the rest.
> - **State volume.** The writable mount for the persisted routing table, owned by `65534:65534`.
> - **Verifying it works.** Bootstrapping a fresh client against `dht.ftorrent.com` and watching its routing table populate, the way the other guides each end with a concrete "confirm it's really working" check.

## Using it

Once it's running, `dht.ftorrent.com` is usable by anyone the same way the existing public bootstrap nodes are — add it to a client's bootstrap list, or include it as a DHT node hint. It is one more independent entry point into the mainline DHT, not controlled by any single client project. That independence is the contribution: more bootstrap diversity makes the DHT's cold-start path more resilient for everyone, the same way another public DNS resolver makes name resolution more resilient. It serves the network.
