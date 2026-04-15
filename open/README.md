
_ftorrent/open/README.md [open.ftorrent.com](https://open.ftorrent.com)_

![open.ftorrent.com above Earth](https://open.ftorrent.com/images/open.ftorrent.com.jpg)

_Three guides cover this deployment: **dockerizing Aquatic and configuring the Linux server** (this guide), the [dashboard back end](gauge/README.md), and the [dashboard front end](page/README.md)._

# Running Aquatic in Docker: A Complete Guide to Public BitTorrent and WebTorrent Trackers

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.6
> <br>Created: 2026-Apr
> <br>Last reviewed: 2026-Apr
> <br>[Aquatic](https://github.com/greatest-ape/aquatic): v0.9.0
> <br>Rust: 1.85
> <br>Docker Engine: 29
> <br>Docker Compose: V2
> <br>Linux kernel: 5.8
> <br>Docker moby [Seccomp profile](https://github.com/moby/profiles/blob/main/seccomp/default.json)
> from commit [5bb2d1a 2025-May-28](https://github.com/moby/profiles/blob/fa50b7287199d1c781284d1a34d1395a62e57f1e/seccomp/default.json)
> copied 2026-Apr-3

## Why run a public tracker?

BitTorrent is the internet's most successful peer-to-peer protocol. It moves enormous volumes of data every day — Linux distributions, open datasets, game updates, public domain archives, creative commons media — without depending on any single server. The protocol is decentralized by design, but it relies on a few pieces of shared infrastructure to work. One of the most important is the **tracker**.

A tracker is a lightweight coordination service. It doesn't store or transfer any files. When a BitTorrent client wants to download something, it asks the tracker: "who else has this?" The tracker responds with a list of IP addresses — other people who are currently sharing the same content. The client connects to those peers directly and the transfer happens peer-to-peer. The tracker's job is introductions, nothing more.

Public trackers are open to anyone. They don't require accounts or registration — any BitTorrent client can use them. They answer the question "who else has this?" for any torrent hash presented to them. By existing, they make BitTorrent work better for everyone — more trackers mean more redundancy, faster peer discovery, and a healthier decentralized network.

[WebTorrent](https://webtorrent.io/) extends this to the browser. Traditional BitTorrent clients communicate over TCP and UDP, which browsers can't use directly. WebTorrent clients use WebSocket connections to trackers and WebRTC for peer-to-peer data transfer, making BitTorrent work in any web page without plugins or extensions. A public tracker that supports WebSocket serves both worlds — desktop clients and browser-based ones.

Running a public tracker is a contribution to internet infrastructure. It costs relatively little to operate — a tracker is stateless, transfers no file data, and runs comfortably on modest hardware with a good internet connection. In return, it provides a service that the entire BitTorrent network benefits from, for free, for everyone.

The WebSocket tracker is worth highlighting separately. When a browser-based client connects to a WebSocket tracker, it presents an identifier (an info_hash in BitTorrent terms) and the tracker introduces it to other clients presenting the same identifier. This is peer discovery and WebRTC signaling — and it's useful beyond BitTorrent. Any application that needs browser peers to find each other — peer-to-peer video, collaborative tools, decentralized chat, browser-based games — can use a public WebSocket tracker for that introduction. The identifier becomes a room or channel, and the tracker brokers the WebRTC connection offers. A public tracker that supports WebSocket is infrastructure for the peer-to-peer web broadly, not just for BitTorrent.

This guide walks through setting one up — from choosing the software, to building and hardening the containers, to tuning the server for growth. The goal is a tracker that is fast, secure, and ready to serve the public for years.

## Who this guide is for

You have a Linux server — a dedicated machine, a repurposed desktop, a cloud instance, anything with a public IP and a decent internet connection. Maybe it's already running some things: a game server, a personal website, a media library. You're comfortable enough with the command line to SSH in and edit config files, or you're learning by doing. You might be a seasoned sysadmin, or you might be building your first homelab with help from [Claude Code](https://claude.ai/code). Either way, you want to do something useful with the hardware and bandwidth you have.

If you've already got your Linux server running, you may have also already set up Docker — to run other software in isolated containers, to get predictable deployments that work the same way on any machine, or simply because the project you wanted to run shipped a Dockerfile. If so, your existing Docker setup almost certainly works over IPv4, and you probably haven't configured IPv6 yet. That's fine — this guide covers the IPv6 setup step by step. IPv6 matters for a public tracker because it's fundamentally better for peer-to-peer. IPv4's shortage of addresses means most peers are behind NAT, which makes direct connections harder and sometimes impossible. IPv6 gives every device a globally routable address — no NAT, no port forwarding, no hole punching. A tracker that supports IPv6 helps peers find each other more reliably, which is the whole point.

What you're doing here is contributing public infrastructure. A public BitTorrent and WebTorrent tracker is a small service that benefits the entire peer-to-peer internet — every client that connects gets faster, more reliable peer discovery, for free, because your server is answering the question "who else has this?" The hardware requirements are modest, the bandwidth cost is low, and the operational overhead is minimal once it's running. This guide aims to get you from "I have a server" to "I'm running a public tracker" with every step explained and every choice justified.

## Why Aquatic?

Several open-source tracker implementations exist. The most established is [opentracker](https://erdgeist.org/arts/software/opentracker/), a C implementation that has powered many of the internet's largest public trackers for over a decade, including [opentrackr.org](https://opentrackr.org) — currently the largest public tracker on earth, serving roughly 25 million peers across 10 million torrents.

[Aquatic](https://github.com/greatest-ape/aquatic) is a newer tracker written in Rust. We chose it for this guide for three reasons:

**Protocol coverage.** Aquatic implements all three tracker protocols as separate binaries:

- **aquatic_udp** — the [UDP tracker protocol](https://www.bittorrent.org/beps/bep_0015.html) (BEP 15), the most common protocol used by desktop BitTorrent clients like qBittorrent, Deluge, and Transmission
- **aquatic_http** — the [HTTP tracker protocol](https://www.bittorrent.org/beps/bep_0003.html) (BEP 3), used by clients behind restrictive firewalls and by some web-based clients
- **aquatic_ws** — the WebSocket tracker protocol, used by [WebTorrent](https://webtorrent.io/) clients running in browsers

A public tracker that wants to serve every kind of BitTorrent client — desktop apps, command-line tools, and browsers — needs all three protocols. Aquatic provides them in a single project with a consistent configuration format and shared architecture. opentracker, by contrast, supports UDP and HTTP but not WebSocket, so serving WebTorrent clients would require running a separate project alongside it.

**Performance.** Aquatic is fast. Its author [benchmarks](https://github.com/greatest-ape/aquatic/blob/master/BENCHMARKS.md) it at millions of requests per second on commodity hardware. The largest tracker running Aquatic in production is [explodie.org](https://explodie.org), which handles approximately 163,000 requests per second. That's in the same range as opentrackr.org (~200,000 requests per second), the busiest public tracker in the world, which uses roughly 37% of a 1 Gbps connection on hardware that's over a decade old. Aquatic can match or exceed that on a single modest server.

**Rust.** Aquatic is written in Rust, which provides memory safety guarantees that matter for internet-facing software accepting raw packets from untrusted sources. This doesn't eliminate all security concerns — kernel vulnerabilities, logic bugs, and configuration mistakes are still possible — but it removes entire classes of memory corruption vulnerabilities (buffer overflows, use-after-free, double-free) that have historically been the most common and dangerous attack vectors in network services written in C.

For more about Aquatic, see the [project README](https://github.com/greatest-ape/aquatic/blob/master/README.md) and the individual crate documentation for [aquatic_udp](https://github.com/greatest-ape/aquatic/tree/master/crates/udp), [aquatic_http](https://github.com/greatest-ape/aquatic/tree/master/crates/http), and [aquatic_ws](https://github.com/greatest-ape/aquatic/tree/master/crates/ws).

## What this guide adds

Aquatic's own documentation covers building from source and running on bare metal with systemd. This guide takes a different approach: **running Aquatic inside hardened Docker containers with bridge networking.**

This is a meaningful addition for several reasons:

- **Security isolation.** A tracker accepts raw packets from the entire internet. Containers provide namespace isolation, seccomp filtering, capability dropping, and network isolation that bare metal systemd hardening cannot match. We explain exactly what each layer defends against and why it matters.
- **Reproducibility.** The Dockerfiles in this guide build Aquatic from source in a multi-stage build. The result is a minimal, self-contained image that runs the same way on any Linux server with Docker installed. No Rust toolchain on the host, no dependency management, no version drift.
- **io_uring in Docker.** Aquatic's HTTP and WebSocket trackers use the [glommio](https://github.com/DataDog/glommio) async runtime, which requires Linux io_uring with no fallback. Docker's default seccomp profile blocks io_uring syscalls. This guide provides a minimal custom seccomp profile that accommodates glommio while keeping the rest of the security filter intact — a specific, practical solution to a problem anyone containerizing Aquatic will hit.
- **Kernel tuning for bridge networking.** Docker's bridge networking depends on the kernel's connection tracking (conntrack) system, whose defaults are sized for general-purpose servers. Under heavy tracker load, the defaults cause silent packet drops. This guide provides the sysctl settings to prevent that, with explanations of what each parameter does and why the recommended value was chosen.

The performance tradeoff of containerization deserves to be addressed up front, since it shapes every decision that follows.

### Performance: what containerization costs and why it doesn't matter

Aquatic was designed for bare metal. The project supports CPU pinning for thread-per-core scheduling, ships a build script for native SIMD optimization, and uses io_uring-based async runtimes for maximum throughput. There is no official Dockerfile. The design targets dedicated hardware with 10 Gbps+ links serving millions of requests per second.

Containerization with Docker bridge networking adds two costs:

1. **Packet traversal overhead.** Each packet crosses a virtual ethernet pair and bridge, adding ~5-15 microseconds of latency and reducing maximum small-packet UDP throughput to roughly 80-90% of bare metal.
2. **Connection tracking overhead.** Docker's DNAT rules create a conntrack entry for every packet. This is solvable with kernel tuning (covered below) but is an additional moving part that bare metal avoids.

On paper, this sounds significant. In practice, it isn't — because **the network link, not the CPU or container overhead, is the bottleneck.**

Consider a typical deployment: a mid-range 6-core desktop CPU, 16 GB of RAM, a 1 Gbps internet connection — nothing exotic, the kind of hardware you might already have. On a 1 Gbps link, the connection saturates at roughly 300,000-500,000 tracker responses per second, depending on response size. The largest public tracker on earth handles approximately 200,000 requests per second. Docker bridge networking on this modest hardware can push well beyond that before the 1 Gbps link fills up. The CPU has headroom after Docker's overhead, before the network does.

Put differently: a server on a 1 Gbps connection, running Aquatic in Docker with bridge networking, can grow to handle as much traffic as the busiest public trackers today without any of these choices being the bottleneck. The ceiling is the internet connection, not the containerization.

The security benefits, on the other hand, are immediate regardless of traffic level. A single announce from an untrusted client hits the same security surface whether the tracker serves a hundred peers or a hundred million. We containerize for security and accept a performance cost that is, at any realistic scale, unmeasurable.

## Prerequisites

### Linux is required

This guide requires a **Linux server**. This is not a preference or a Docker convention — it's a hard requirement imposed by Aquatic's dependencies.

The chain: aquatic_http and aquatic_ws use [glommio](https://github.com/DataDog/glommio) as their async runtime. glommio is built entirely on [io_uring](https://en.wikipedia.org/wiki/Io_uring), a Linux kernel interface for asynchronous I/O. io_uring exists only in the Linux kernel (introduced in 5.1, mature in 5.8+). It does not exist in Darwin (macOS), the Windows NT kernel, or the BSDs. There is no epoll fallback, no kqueue fallback, no abstraction layer. glommio is a compile-time crate dependency in Aquatic, not a feature flag — there is no way to build aquatic_http or aquatic_ws without it.

aquatic_udp is different. It uses [mio](https://github.com/tokio-rs/mio), which is cross-platform (epoll on Linux, kqueue on macOS, IOCP on Windows). The UDP tracker alone could run on other platforms. But two of the three binaries are Linux-only, which makes Aquatic as a whole — and this guide — Linux-only.

If your first instinct is to run the tracker on a Mac Mini or a Windows server you already have: it won't work for the HTTP and WebSocket protocols. You need a Linux machine — a dedicated server, a repurposed desktop, a cloud instance (AWS EC2, DigitalOcean, Linode, Hetzner, etc.), or any other machine running a modern Linux kernel. The hardware requirements are modest — see the performance discussion above.

### Other requirements

- **Linux kernel 5.8 or later.** Ubuntu Server 22.04+ and Debian 12+ both satisfy this. Any modern Linux distribution with a recent kernel will work.
- **Docker Engine** (not Docker Desktop). Docker Compose V2 (the `docker compose` plugin) for the example compose file.
- A **public IP address** (IPv4, IPv6, or both — dual-stack is recommended so all clients can reach the tracker) with the relevant tracker ports open in your firewall.
- Basic familiarity with Docker, the Linux command line, and editing configuration files.

## Architecture decisions

Two decisions shape the deployment: whether to run Aquatic in Docker containers or directly on the host, and if containerized, what Docker networking mode to use.

### Docker containers vs. bare metal

The case for bare metal is simplicity and raw performance. Aquatic was designed for it — systemd unit files, built-in privilege dropping, CPU pinning, native SIMD. If you trust your host hardening and want the absolute maximum throughput, bare metal is a valid choice and Aquatic's own documentation covers it well.

The case for Docker is security isolation. A tracker accepts raw packets from the entire internet. The UDP tracker takes unauthenticated UDP from any source with no TLS and no HTTP layer in front of it. If the process is compromised, what matters is what it can reach.

On bare metal, even with systemd hardening, a compromised process can make outbound network connections (to the internet, to other devices on the local network, to the router), see other processes on the host, and potentially access the host filesystem through policy misconfiguration.

In a hardened Docker container, a compromised process hits a dead end:

- **No outbound network.** With `DOCKER-USER` iptables rules blocking container-initiated traffic, the container can only respond to inbound connections. No exfiltration, no lateral movement, no command-and-control callbacks.
- **No visibility.** PID namespace isolation means the process sees only itself. Mount namespace isolation with a read-only filesystem means it sees only its own immutable image.
- **No escalation.** All Linux capabilities are dropped. The no-new-privileges flag is set. Even if an attacker gets UID 0 inside the container, there is nothing to do with it.
- **Resource limits.** cgroup limits on memory, CPU, and PIDs prevent a compromised or buggy process from exhausting host resources.
- **Layered defense.** Escaping requires defeating the seccomp filter AND the namespace isolation AND the capability restrictions AND the firewall rules — independent barriers.

As discussed above, the performance cost of containerization is unmeasurable at any scale a 1 Gbps connection can carry. This guide uses Docker.

### Bridge networking vs. host networking

Docker offers two relevant networking modes. **Bridge networking** (the default) gives each container its own network namespace with a virtual ethernet pair. Docker creates DNAT rules to forward published ports, and all forwarded traffic passes through the kernel's connection tracking (conntrack) system. **Host networking** (`--net=host`) shares the host's network namespace directly — no NAT, no conntrack, no virtual interfaces, and network performance identical to bare metal.

Host networking is simpler and faster. Bridge networking is meaningfully more secure, for the same reason containers are more secure than bare metal: **network namespace isolation is what makes outbound blocking work.**

With bridge networking, container traffic traverses the FORWARD chain in iptables, where `DOCKER-USER` rules can drop container-initiated outbound packets. The container is network-isolated — it can receive forwarded inbound connections and respond to them, but it cannot initiate connections to anything.

With host networking, the container shares the host's network stack entirely. There is no FORWARD chain involvement, no way to distinguish container traffic from host traffic, and no network isolation. A compromised container has the same network access as any host process.

The performance cost of bridge networking — the ~10-20% packet traversal overhead and the conntrack table management — is real but solvable. The conntrack issue is handled by kernel tuning (next section). The packet traversal overhead, as established above, doesn't move the needle at realistic scale. This guide uses bridge networking.

### Ports and announce URLs

When the tracker is running, torrent creators add announce URLs to their torrent files or magnet links. These URLs are how BitTorrent clients find the tracker. For a tracker at `open.ftorrent.com`, the announce URLs are:

- `udp://open.ftorrent.com:443/announce` — UDP tracker
- `https://open.ftorrent.com/announce` — HTTP tracker
- `wss://open.ftorrent.com` — WebSocket tracker

The HTTP and WebSocket URLs use port 443 implicitly (it's the default for HTTPS and WSS). The UDP URL specifies port 443 explicitly because there is no default port for the UDP tracker protocol. All three protocols share a single domain name, and the two TCP services (HTTP and WebSocket) share a single port through a reverse proxy that routes by path and protocol.

**Choosing the external UDP port.** The traditional BitTorrent tracker port is **6969**. It's well-known, easy to remember, and works fine on unrestricted networks. The downside is that it's trivially identified and blocked — network filters at universities, corporations, and public WiFi hotspots often block it by number.

This guide uses **443/udp** instead. UDP traffic on port 443 is indistinguishable at the network layer from [QUIC](https://en.wikipedia.org/wiki/QUIC) (HTTP/3), the protocol modern browsers use for fast encrypted web connections. Restrictive networks cannot block UDP/443 without breaking HTTP/3, which makes it the most reachable port for a public tracker that wants to serve clients everywhere. The trade-off: if your server also runs QUIC/HTTP/3, there's a protocol conflict on the same port. For a tracker-focused server, this isn't an issue.

You can use any port you like. 6969 is the simplest choice and works well on open networks. 443 maximizes reachability on restricted networks.

**Internal container ports.** The containers listen on unprivileged ports above 1024 (8443 for UDP, 8081 for HTTP, 8082 for WS) because they run as nobody with all Linux capabilities dropped — binding to ports below 1024 would require `CAP_NET_BIND_SERVICE`, which we deliberately don't grant. Docker's port mapping translates between the external port (443) and the internal port (8443). The internal port numbers don't matter to clients; only the external ports appear in announce URLs.

**TCP port sharing.** The HTTP tracker, WebSocket tracker, and homepage all share external port 443/tcp through a reverse proxy (nginx). nginx terminates TLS and routes requests based on their content: paths starting with `/announce` or `/scrape` go to the HTTP tracker, requests with a WebSocket `Upgrade` header go to the WebSocket tracker, and everything else goes to the homepage. This means a single domain and port serves all three services — clients don't need to know about the internal routing.

**Publishing and visibility.** The UDP container is published on all interfaces (`0.0.0.0:443:8443/udp`) because UDP traffic goes directly from the internet to the container — there's no reverse proxy for UDP. The HTTP and WebSocket containers are published on **localhost only** (`127.0.0.1:8081:8081/tcp`, `127.0.0.1:8082:8082/tcp`) because they should only be reachable through the reverse proxy, not directly from the internet. This ensures all TCP traffic goes through TLS termination and nginx's routing logic.

## The traffic path

Before configuring anything, it helps to see how a packet travels from a BitTorrent client on the internet down to the container that will respond to it. Each step in the path needs to be configured correctly, or traffic gets dropped silently.

```
                     internet
                        │
                        ▼
              router / edge firewall
                        │
                        ▼
              Linux host firewall
                        │
                ┌───────┴───────┐
                │               │
          TCP 80/443        UDP 443
                │               │
                ▼               ▼
              nginx       Docker DNAT
           (TLS, routing)  (kernel-level,
                │          bypasses nginx)
                │               │
                └───────┬───────┘
                        ▼
                  containers
        (aquatic_udp, aquatic_http, aquatic_ws)
```

The **router or edge firewall** is where a client's packet first reaches the deployment. Behind a home or office router, this means port-forwarding rules for IPv4 and allow rules for IPv6. On a cloud instance with a direct public IP, it's the provider's firewall or security group.

The **Linux host firewall** (iptables and ip6tables) controls which inbound packets reach processes on the host. nginx runs as a host process and listens on ports 80 and 443, so those need INPUT allow rules. UDP tracker traffic does not go through INPUT at all — Docker's DNAT handles it in PREROUTING, which runs before INPUT and sends the packet straight to the container.

**nginx** terminates TLS for HTTPS and WSS, handles IPv4 and IPv6 dual-stack, and routes TCP requests by path (`/announce`, `/scrape`) and by WebSocket upgrade header to the right container over localhost. UDP tracker traffic never touches nginx.

**Docker** sits underneath, providing the network namespace where the containers live. Two Docker layers matter: the daemon config (`/etc/docker/daemon.json` with `ipv6`, `ip6tables`, and `userland-proxy: false`) and the compose network (with `enable_ipv6: true`, pinned subnets, and DOCKER-USER firewall rules that isolate the containers from the outside).

The **containers** are the Aquatic binaries themselves, hardened as described later in the guide — running as nobody, read-only filesystem, all capabilities dropped, custom seccomp profile for io_uring.

The rest of the guide configures each of these in order. Some are already covered above (the Docker daemon and network, the containers); others are covered in the sections that follow (host firewall, nginx).

## Server preparation

The configuration in this section lives outside the containers — it's applied to the host server by the administrator. These settings should be applied before the tracker starts.

### Router / edge firewall

If the server is on a home or office connection behind a router, the router needs to route external traffic to the server. If the server has a direct public IP (cloud VPS, bare metal hosting), skip this subsection — the provider's firewall or network security group handles this step instead.

**IPv4 port forwarding (DNAT).** For each of the tracker's public ports, add a DNAT rule forwarding from the router's WAN interface to the server's LAN address:

| External port | Protocol | Purpose |
|---|---|---|
| 80 | TCP | HTTP (ACME challenges, redirect to HTTPS) |
| 443 | TCP | HTTPS (HTTP tracker, WebSocket tracker, homepage) |
| 443 | UDP | UDP tracker |
| 22 (or your chosen port) | TCP | SSH access for administration |

The exact configuration depends on the router. Consumer routers usually call this "port forwarding" in the admin interface. Enterprise routers and firewalls use "destination NAT" or similar terminology.

**IPv6 inbound allow rules.** IPv6 does not use NAT — every device on the LAN has its own globally routable address. Instead of DNAT, the router firewall needs allow rules permitting inbound connections to the server's IPv6 address on the same ports:

- TCP 80
- TCP 443
- UDP 443
- SSH port

On routers with dynamically changing prefixes, the allow rules can often be targeted by the server's stable interface identifier (the lower 64 bits of its IPv6 address) so the rules survive when the prefix rotates.

### Host firewall

On the server itself, iptables and ip6tables control which inbound packets reach processes running on the host. The goal is to allow only the ports that nginx and SSH listen on.

**INPUT allow rules:**

```bash
# SSH
iptables  -A INPUT -p tcp --dport 22 -j ACCEPT
ip6tables -A INPUT -p tcp --dport 22 -j ACCEPT

# HTTP and HTTPS (nginx)
iptables  -A INPUT -p tcp --dport 80  -j ACCEPT
iptables  -A INPUT -p tcp --dport 443 -j ACCEPT
ip6tables -A INPUT -p tcp --dport 80  -j ACCEPT
ip6tables -A INPUT -p tcp --dport 443 -j ACCEPT

# Established/related (responses to outbound connections)
iptables  -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
ip6tables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT

# Loopback
iptables  -A INPUT -i lo -j ACCEPT
ip6tables -A INPUT -i lo -j ACCEPT

# ICMPv6 neighbor discovery (required — IPv6 breaks without it)
ip6tables -A INPUT -p icmpv6 -j ACCEPT
```

**Default policies:**

```bash
iptables  -P INPUT DROP
ip6tables -P INPUT DROP
iptables  -P FORWARD ACCEPT   # Docker manages the FORWARD chain
ip6tables -P FORWARD ACCEPT
iptables  -P OUTPUT ACCEPT
ip6tables -P OUTPUT ACCEPT
```

**The UDP tracker does not need an INPUT rule.** This might be surprising, but it's correct. Docker's DNAT for published ports happens in the PREROUTING chain, which runs before INPUT. When a UDP packet arrives on port 443, Docker rewrites the destination to the container's internal address and the packet moves to the FORWARD chain (which Docker manages), never touching INPUT. INPUT only sees packets destined for host processes like nginx and sshd. Adding a UDP allow rule anyway is harmless — it just never matches.

**ICMPv6 is required for IPv6 to work at all.** Unlike IPv4's ARP, IPv6 uses ICMPv6 for neighbor discovery and path MTU discovery. Blocking ICMPv6 breaks the entire IPv6 stack. This is a common mistake when people write strict firewall rules.

Persist the rules with `iptables-persistent`:

```bash
sudo apt install iptables-persistent
sudo netfilter-persistent save
```

### Docker daemon configuration

Most Docker installations are IPv4-only out of the box. Docker's default is `ipv6: false`, and most tutorials and existing setups don't change it. For the tracker to serve IPv6 clients — which the compose file's `enable_ipv6: true` network requires — the Docker daemon itself needs IPv6 enabled first.

There's also a default Docker behavior that will silently break any BitTorrent tracker: the **userland proxy**. By default, Docker publishes ports through a userland proxy process that rewrites the source IP on every packet to the Docker bridge gateway address (e.g., `172.30.0.1`). The tracker container sees this gateway IP instead of the client's real IP. For a BitTorrent tracker that builds peer lists from source IPs, this is fatal — every peer appears to come from the same address, and the swarm is useless.

Both issues are fixed in `/etc/docker/daemon.json`:

```json
{
  "ipv6": true,
  "ip6tables": true,
  "fixed-cidr-v6": "fd00:cafe:1::/64",
  "userland-proxy": false
}
```

What each setting does:

- **`ipv6`** — enables IPv6 on Docker's default bridge network. This is a prerequisite for IPv6 on any Docker compose network.
- **`ip6tables`** — lets Docker manage ip6tables rules for IPv6 port publishing. Without this, Docker won't create the ip6tables DNAT rules that route IPv6 traffic to containers, and inbound IPv6 packets are silently dropped.
- **`fixed-cidr-v6`** — assigns a ULA subnet to Docker's default bridge. The subnet `fd00:cafe:1::/64` is an example — any ULA address (`fd00::/8` range) works as long as it doesn't conflict with the compose network's subnet (which uses `fd00:cafe:2::/64` in our example). This subnet is internal plumbing for the default bridge; it doesn't appear in any client-facing configuration.
- **`userland-proxy`** — setting this to `false` forces Docker to use kernel-level iptables/ip6tables DNAT for port publishing instead of the userland proxy. This preserves the original source IP on every packet, which is essential for a tracker that needs to see real client addresses.

If you already have a `daemon.json` with other settings, merge these four fields into it rather than replacing the file.

After editing, restart Docker for the changes to take effect:

```bash
sudo systemctl restart docker
```

Note: if Docker was already running containers, they will be stopped and restarted. Plan accordingly.

### Kernel tuning for bridge networking

Bridge networking with conntrack works fine at low traffic. Under heavy load, the default kernel parameters create silent failures — packets vanish with no errors and no logs.

All settings go in a file like `/etc/sysctl.d/90-tracker.conf` and are applied with `sysctl --system`. They persist across reboots — with one caveat covered below.

### Connection tracking table

Docker's DNAT rewrites the destination address on every inbound packet. The kernel records each rewrite in the conntrack table so it can reverse it on the response. Each unique client (source IP + source port) creates one entry. When the table fills, all new connections are silently dropped.

```
# Maximum conntrack entries. Default is 65536-262144.
# A busy tracker has hundreds of thousands of unique clients.
# Each entry uses ~300 bytes of kernel memory.
# 1 million entries ≈ 300 MB.
net.netfilter.nf_conntrack_max = 1048576
```

### Connection tracking timeouts

Conntrack entries persist for a timeout period after the last packet. The defaults are designed for long-lived connections, not tracker request-reply exchanges that complete in milliseconds.

```
# Timeout for UDP streams (bidirectional). Default: 120 seconds.
# A tracker request-reply completes in milliseconds. 15 seconds is generous.
# Reducing this from 120s to 15s cuts steady-state table size by ~8x.
net.netfilter.nf_conntrack_udp_timeout_stream = 15

# Timeout for one-way UDP flows. Default: 30 seconds.
net.netfilter.nf_conntrack_udp_timeout = 10
```

### UDP socket buffers

The kernel buffers incoming packets in per-socket receive buffers. When the buffer is full, new packets are silently dropped. The application sees nothing — the only evidence is a counter in `/proc/net/snmp`.

```
# Maximum socket buffer size any socket can request. Default: ~208 KB.
# At high packet rates, 208 KB buffers roughly 1 millisecond of traffic.
# Any scheduling jitter — CPU spike, cgroup throttle — causes drops.
net.core.rmem_max = 26214400
net.core.wmem_max = 26214400

# Default socket buffer size. Safety net for applications that don't
# explicitly set their own.
net.core.rmem_default = 1048576
net.core.wmem_default = 1048576
```

### NIC receive backlog

When the NIC receives packets faster than the kernel can process them in softirq, they queue in a per-CPU backlog. Packets dropped here are the hardest to diagnose — they die before reaching any socket or application counter.

```
# Per-CPU backlog queue length. Default: 1000.
# Docker's veth pair adds processing overhead per packet,
# making bursts more likely to overflow this queue.
net.core.netdev_max_backlog = 10000
```

### The complete sysctl block

```
# /etc/sysctl.d/90-tracker.conf
# Kernel tuning for BitTorrent tracker behind Docker bridge networking.

# Conntrack table size and timeouts
net.netfilter.nf_conntrack_max = 1048576
net.netfilter.nf_conntrack_udp_timeout = 10
net.netfilter.nf_conntrack_udp_timeout_stream = 15

# UDP socket buffers
net.core.rmem_max = 26214400
net.core.wmem_max = 26214400
net.core.rmem_default = 1048576
net.core.wmem_default = 1048576

# NIC receive backlog
net.core.netdev_max_backlog = 10000
```

### Pre-loading the conntrack module

The `net.core.*` settings above take effect at boot and survive reboots without any extra steps. The `net.netfilter.nf_conntrack_*` settings do not — they are silently skipped.

The reason: the `nf_conntrack` kernel module isn't loaded during early boot when systemd applies sysctl configs. The `net.netfilter.nf_conntrack.*` keys don't exist in `/proc/sys/` until the module is loaded, and `sysctl --system` silently skips keys that don't exist. Docker loads `nf_conntrack` later when it creates DNAT rules, but by then sysctl has already run.

The fix is to pre-load the module before sysctl runs:

```bash
echo "nf_conntrack" | sudo tee /etc/modules-load.d/conntrack.conf
```

This tells systemd to load `nf_conntrack` during early boot, before sysctl configs are applied. After creating this file, the conntrack settings will survive reboots. You can verify after a reboot:

```bash
cat /proc/sys/net/netfilter/nf_conntrack_max
# Should show 1048576, not the default 65536 or 262144
```

### Monitoring

These commands verify the tuning is working and catch problems before they become outages. The `conntrack` command-line tool may need to be installed first (`apt install conntrack` on Debian/Ubuntu).

```bash
# Conntrack table usage — count should stay well below max
cat /proc/sys/net/netfilter/nf_conntrack_count
cat /proc/sys/net/netfilter/nf_conntrack_max

# Conntrack drops — should be 0
conntrack -S

# UDP socket buffer drops — look for RcvbufErrors, SndbufErrors
cat /proc/net/snmp | grep Udp:

# Softirq drops per CPU — second column should be 0
cat /proc/net/softnet_stat

# Kernel log — when the conntrack table overflows, the kernel logs:
#   "nf_conntrack: table full, dropping packet"
# This is often how the problem is discovered before anyone thinks
# to check conntrack -S.
dmesg | grep conntrack
```

## Container configuration

The configuration in this section lives inside the containers and the Docker Compose file — it ships as code in this repository. The Dockerfiles, TOML configs, seccomp profile, and compose file together define the complete container setup.

### Security constraints

Every tracker container runs under these constraints. These are not suggestions — they are the security model. If Aquatic can't work under one of them, that's documented as a finding with a specific accommodation, not a reason to relax the model.

| Constraint | Setting | Purpose |
|---|---|---|
| Non-root user | `user: "65534:65534"` | Process runs as nobody, not root |
| Read-only filesystem | `read_only: true` | Container filesystem is immutable |
| No capabilities | `cap_drop: [ALL]` | No Linux capabilities whatsoever |
| No privilege escalation | `security_opt: [no-new-privileges:true]` | Cannot exec setuid binaries |
| No outbound network | `DOCKER-USER` iptables rules | Container can only respond to inbound |

### io_uring: a necessary accommodation for HTTP and WebSocket

Aquatic's three binaries use different async runtimes:

| Binary | Runtime | io_uring required? |
|---|---|---|
| aquatic_udp | mio (epoll) | No — built without the io_uring feature |
| aquatic_http | glommio | Yes — glommio is a compile-time crate dependency, not a feature flag; there is no way to build aquatic_http without it |
| aquatic_ws | glommio | Yes — same as HTTP |

Docker's default seccomp profile blocks the `io_uring_setup`, `io_uring_enter`, and `io_uring_register` syscalls. The HTTP and WebSocket containers need a custom seccomp profile that extends Docker's default with just those three syscalls. This accommodation is not optional — without it, the containers will crash immediately at startup when glommio tries to initialize io_uring. The custom profile is a precise, well-understood security delta — not a general relaxation of the seccomp filter.

The UDP container is deliberately built without io_uring. It uses mio/epoll, which works under Docker's default seccomp profile with no changes. Since the UDP tracker is the most exposed service (raw UDP from the internet, no TLS, no HTTP layer), it gets the smallest possible kernel attack surface.

The HTTP and WebSocket containers also need a raised memlock ulimit for io_uring buffer registration:

```yaml
ulimits:
  memlock:
    soft: 65536000
    hard: 65536000
```

### Resource limits

Docker's cgroup limits cap the resources each container can consume. These are ceilings, not reservations — a fresh tracker with zero peers uses a few megabytes. The limits define how far each container can grow before hitting a wall.

**Memory.** Aquatic holds all tracker state in memory — every peer address, every torrent hash, every connection record. The amount of memory a tracker needs scales directly with the number of peers it serves. A rough estimate is 50-100 bytes per peer entry. At one million peers, that's 50-100 MB. At 25 million peers (opentrackr scale), that's 1.25-2.5 GB.

The UDP tracker carries the vast majority of traffic — roughly 80-90% of announces come over UDP, since it's the default protocol for desktop BitTorrent clients. The HTTP and WebSocket trackers serve a much smaller fraction of total peers.

The memlock ulimit for the HTTP and WebSocket containers (65 MB for io_uring buffer registration) counts against the container's memory limit. This is a fixed overhead, not a scaling parameter.

These limits are ceilings, not reservations. Linux does not set aside the memory when the container starts — the container uses what it needs, growing from a few megabytes with zero peers toward the limit as traffic increases. Unused ceiling costs nothing. The kernel uses free physical memory as page cache (filesystem cache for disk reads), and reclaims it instantly and silently when an application needs it. There is no degradation threshold, no swap thrashing — just a smooth, linear tradeoff between page cache and application memory.

On a server with 16 GB of RAM running other services alongside the tracker (game servers, static web hosting, file serving), dedicating 3.5 GB in total ceilings to the three tracker containers is comfortable. The actual memory used at low traffic will be a fraction of that. The recommended limits give the tracker room to grow to top-tier scale without hitting a memory ceiling:

| Container | Memory limit | Rationale |
|---|---|---|
| aquatic_udp | 2 GB | Carries ~80-90% of traditional BitTorrent traffic; needs room for millions of peer entries |
| aquatic_http | 512 MB | Less traffic than UDP; 65 MB used by memlock, rest for tracker state |
| aquatic_ws | 1 GB | WebTorrent is growing and the tracker also serves as general WebRTC signaling infrastructure, but browser-based peer-to-peer is much smaller than desktop BitTorrent today; 65 MB used by memlock |

**CPU and PIDs.** These can start with modest limits and be adjusted based on monitoring. Aquatic uses a small, fixed number of threads — typically one socket worker and one swarm worker per binary, plus a few housekeeping threads. A PID limit of 64 per container provides headroom beyond what Aquatic needs while still capping runaway process creation.

### Container summary

| Container | Protocol | Internal Port | External Port | Runtime | Seccomp | Memory |
|---|---|---|---|---|---|---|
| aquatic_udp | UDP | 8443 | 443/udp | mio/epoll | Docker default | 2 GB |
| aquatic_http | HTTP | 8081 | 443/tcp (via nginx) | glommio/io_uring | Custom (+ 3 syscalls) | 512 MB |
| aquatic_ws | WebSocket | 8082 | 443/tcp (via nginx) | glommio/io_uring | Custom (+ 3 syscalls) | 1 GB |

## Building the containers

All files referenced in this section are in the [`open/docker/`](docker/) directory of this repository. The directory contains:

- Three Dockerfiles (one per tracker protocol)
- Three TOML configuration files (one per tracker)
- One custom seccomp profile (for the HTTP and WebSocket containers)
- One example Docker Compose file

### The Dockerfiles

Each Dockerfile follows the same pattern: a multi-stage build that compiles Aquatic from source in a Rust builder stage, then copies the resulting binary into a minimal Debian runtime image.

```dockerfile
# Stage 1: Build — full Rust toolchain, compiles from source
FROM rust:1.85-bookworm AS builder
# ... install cmake, clone Aquatic, cargo build ...

# Stage 2: Runtime — minimal Debian, just the binary and config
FROM debian:bookworm-slim
COPY --from=builder /build/aquatic/target/release/aquatic_udp /usr/local/bin/
COPY aquatic-udp.toml /etc/aquatic/config.toml
USER 65534:65534
ENTRYPOINT ["aquatic_udp"]
CMD ["-c", "/etc/aquatic/config.toml"]
```

The runtime image contains only the compiled binary, the config file, and a minimal Debian userspace. No Rust toolchain, no build tools, no source code. The builder stage is discarded after the binary is extracted.

**Why both versions are pinned.** The Dockerfiles pin both the Rust compiler version (`rust:1.85-bookworm`) and the Aquatic release (`v0.9.0`). This is standard practice for reproducible builds — it means the Dockerfile produces the same binary regardless of when you build it.

The Rust pin is also necessary for a specific reason: Aquatic v0.9.0 depends on an older version of the `metrics` crate that uses a lifetime pattern the Rust compiler accepted at the time but rejects in newer versions (1.93+). Building with the latest `rust:bookworm` image will fail with a compile error in the `metrics` crate. Pinning to Rust 1.85 avoids this. When a new Aquatic version is released with updated dependencies, the Rust pin can be updated or removed. See [rust-lang/rust#141402](https://github.com/rust-lang/rust/issues/141402) for details.

**Build dependencies.** The only system package installed in the builder stage is `cmake`, which is required by the `mimalloc` allocator (a default Aquatic feature that improves memory allocation performance). The `rust:bookworm` image already includes `gcc`, `g++`, and `make`.

The three Dockerfiles differ only in which binary they build and which config file they copy:

| Dockerfile | Build command | Config file |
|---|---|---|
| `Dockerfile.udp` | `cargo build --release -p aquatic_udp` | `aquatic-udp.toml` |
| `Dockerfile.http` | `cargo build --release -p aquatic_http` | `aquatic-http.toml` |
| `Dockerfile.ws` | `cargo build --release -p aquatic_ws` | `aquatic-ws.toml` |

All three use default Cargo features, which include `prometheus` (metrics endpoint) and `mimalloc` (memory allocator). The `io-uring` feature is not in the default set for any of them — for UDP, this means it uses mio/epoll. For HTTP and WS, io_uring comes through glommio as a crate dependency, not as a Cargo feature flag.

### Configuration files

Each tracker binary takes a TOML configuration file via the `-c` flag. The configs in this repository start from the actual defaults generated by each binary (`aquatic_udp -p`, etc.) and modify only what's needed for container deployment. Every change from the default is commented.

The key modifications across all three configs:

- **Listen port** changed from the default 3000 to the container's designated internal port (8443 for UDP, 8081 for HTTP, 8082 for WS). These are all above 1024 so the container doesn't need `CAP_NET_BIND_SERVICE`.
- **Prometheus metrics endpoint** enabled on port 9000 (default is disabled). Each container has its own network namespace, so there's no port conflict — port 9000 is per-container.
- **Privilege dropping disabled** — Aquatic has built-in support for starting as root, binding ports, and dropping to nobody. This is unnecessary in a container that already runs as nobody (UID 65534) on an unprivileged port.

Additional HTTP-specific changes:

- **Reverse proxy mode enabled** (`runs_behind_reverse_proxy = true`). The HTTP tracker sits behind nginx, which handles TLS and forwards requests. Aquatic needs to read the client's real IP from the `X-Forwarded-For` header rather than seeing nginx's internal IP.

Additional WebSocket-specific changes:

- **HTTP health checks enabled** (`enable_http_health_checks = true`). This makes the container respond to `GET /health` with `200 OK`, which Docker and reverse proxies can use to verify the container is alive. This is incompatible with Aquatic's built-in TLS, but we don't use it — TLS is handled by the reverse proxy.

Additional UDP-specific changes:

- **Statistics printed to stdout** (`print_to_stdout = true`). This makes tracker stats visible in `docker logs`, which is useful for monitoring without Prometheus.

**A note on config field names.** Aquatic uses `deny_unknown_fields` in its config parser — if a field name is wrong, the binary will refuse to start and print a clear error message naming the invalid field. This is useful: if you modify the config and make a typo, you'll know immediately at container startup rather than getting silent misbehavior.

### Custom seccomp profile

The file `seccomp-iouring.json` is Docker's [default seccomp profile](https://github.com/moby/profiles/blob/main/seccomp/default.json) with a single entry appended to the `syscalls` array:

```json
{
    "comment": "Allow io_uring syscalls for glommio ...",
    "names": [
        "io_uring_setup",
        "io_uring_enter",
        "io_uring_register"
    ],
    "action": "SCMP_ACT_ALLOW",
    "includes": {
        "minKernel": "5.8"
    }
}
```

This is the complete security delta between our profile and Docker's default: three syscalls, gated on kernel 5.8+. Everything else in the profile — the default deny action, the architecture map, every other syscall rule — is identical to Docker's default.

Without this profile, the HTTP and WebSocket containers crash immediately at startup. glommio's first action is to call `io_uring_setup` to create an io_uring instance, and the default seccomp filter blocks it. The error message from glommio is characteristically colorful: *"Failed to register a probe. The most likely reason is that your kernel witnessed Romulus killing Remus (too old!!)"* — which actually means the seccomp filter blocked the syscall, not that the kernel is too old.

The UDP container does not need this profile — it uses mio/epoll, which only uses syscalls already in Docker's default allowlist.

### Docker Compose

The example `docker-compose.yml` brings all three containers together with the security constraints and resource limits discussed above. A few elements beyond the hardening settings deserve explanation:

**IPv6 network.** The compose file defines a named network with `enable_ipv6: true` and a ULA IPv6 subnet (`fd00:cafe:2::/64`). This is necessary for the UDP container specifically. The HTTP and WebSocket containers sit behind a reverse proxy that handles IPv6 on the outside, but the UDP container publishes directly to the internet — no reverse proxy. For IPv6 clients to reach it, Docker must create ip6tables DNAT rules, and Docker only creates those rules if the container has an IPv6 address on its Docker network. Without `enable_ipv6`, IPv6 UDP packets arrive at the host and are silently dropped — no error, no log, no indication anything is wrong. The IPv4 subnet is also pinned (`172.30.0.0/24`) so it's predictable if you add `DOCKER-USER` firewall rules that reference it by address.

**tmpfs at /tmp.** All three containers use `read_only: true` (immutable filesystem) and mount a `tmpfs` at `/tmp`. Aquatic does not appear to write temp files in normal operation, but glommio or the Rust runtime may need a writable temp directory under some conditions. The tmpfs mount costs nothing in memory unless something writes to it, and prevents a hard-to-diagnose read-only filesystem error if the application does write.

**Prometheus metrics access.** The tracker configs enable Prometheus metrics on port 9000 inside each container, but the compose file does not publish this port to the host. This is intentional — metrics should not be exposed to the internet. To access metrics, either query from another container on the same Docker network (e.g., a stats dashboard container), or temporarily publish on localhost for debugging: `127.0.0.1:9001:9000/tcp` (binding to localhost only, not `0.0.0.0`).

```yaml
name: ftorrent-open                  # Explicit project name

networks:
  tracker:
    name: ftorrent-open                # External Docker network name
    enable_ipv6: true
    ipam:
      config:
        - subnet: 172.30.0.0/24
        - subnet: fd00:cafe:2::/64

services:
  aquatic-udp:
    container_name: ftorrent-open-udp-1
    # ... build ...
    networks: [tracker]
    ports: ["443:8443/udp"]          # All interfaces — direct internet access
    user: "65534:65534"
    read_only: true
    tmpfs: [/tmp]
    cap_drop: [ALL]
    security_opt: [no-new-privileges:true]

  aquatic-http:
    container_name: ftorrent-open-http-1
    # ... build ...
    networks: [tracker]
    ports: ["127.0.0.1:8081:8081"]   # Localhost only — nginx proxies to here
    user: "65534:65534"
    read_only: true
    tmpfs: [/tmp]
    cap_drop: [ALL]
    security_opt:
      - no-new-privileges:true
      - seccomp=seccomp-iouring.json
    ulimits:
      memlock: { soft: 65536000, hard: 65536000 }

  aquatic-ws:
    container_name: ftorrent-open-ws-1
    # Same as HTTP, with 1g memory limit
    ports: ["127.0.0.1:8082:8082"]   # Localhost only — nginx proxies to here
```

The service names (`aquatic-udp`, `aquatic-http`, `aquatic-ws`) describe what each container runs — the Aquatic binary for that protocol. Explicit `container_name:` entries give the running containers the names `ftorrent-open-udp-1`, `ftorrent-open-http-1`, `ftorrent-open-ws-1` that appear in `docker ps`. Without the explicit names, Docker Compose would auto-generate names from the project and service (`ftorrent-open-aquatic-udp-1`), which is correct but longer.

Image names are auto-generated by Docker Compose from the project and service: `ftorrent-open-aquatic-udp`, `ftorrent-open-aquatic-http`, `ftorrent-open-aquatic-ws`. These only appear in `docker images` — they don't need to be cleaner than they are.

Note how the UDP container publishes on all interfaces (direct internet access), while HTTP and WS publish on localhost only (reachable only through the reverse proxy). The UDP container uses Docker's default seccomp (no `seccomp=` line), while HTTP and WS specify the custom profile and the memlock ulimit.

## Building and testing locally

You can build and test the containers on any machine with Docker installed — including a macOS development machine with Docker Desktop, if you have one. This is useful as a sanity check before deploying to a production server.

### Build the images

From the `open/docker/` directory:

```bash
docker build -f Dockerfile.udp  -t ftorrent-open-aquatic-udp  .
docker build -f Dockerfile.http -t ftorrent-open-aquatic-http .
docker build -f Dockerfile.ws   -t ftorrent-open-aquatic-ws   .
```

These tag names match what `docker compose build` auto-generates from the project name (`ftorrent-open`) and the service name (`aquatic-udp`): `ftorrent-open` + `-` + `aquatic-udp` = `ftorrent-open-aquatic-udp`. Using the same names for manual and compose builds means `docker compose up -d` can reuse an image you already built manually, instead of rebuilding from scratch.

Each build compiles Aquatic from source inside Docker, so the first build takes a few minutes (downloading the Rust toolchain, compiling dependencies). Subsequent builds with only config changes are fast — Docker caches the compilation layers.

The build produces an image for whatever architecture your Docker host runs. On an x86_64 Linux server, you get an x86_64 binary. On an ARM64 machine (Apple Silicon Mac with Docker Desktop, Raspberry Pi, AWS Graviton), you get an ARM64 binary. The Dockerfiles are architecture-agnostic.

### Test the UDP tracker

The UDP tracker uses mio/epoll and has no special requirements:

```bash
docker run --rm -d --name test-udp -p 6969:8443/udp ftorrent-open-aquatic-udp
docker logs test-udp
```

(We use 6969 as the host port here for convenience — any available port works for local testing. In production, the compose file maps 443→8443.)

You should see statistics output showing zero peers and zero requests — the tracker is running and waiting for connections. Ctrl+C or `docker stop test-udp` to stop.

### Test the HTTP and WebSocket trackers

The HTTP and WebSocket trackers require the custom seccomp profile and the memlock ulimit:

```bash
# HTTP tracker
docker run --rm -d --name test-http -p 8081:8081 \
  --security-opt seccomp=seccomp-iouring.json \
  --ulimit memlock=65536000:65536000 \
  ftorrent-open-aquatic-http

# WebSocket tracker
docker run --rm -d --name test-ws -p 8082:8082 \
  --security-opt seccomp=seccomp-iouring.json \
  --ulimit memlock=65536000:65536000 \
  ftorrent-open-aquatic-ws
```

**Without the seccomp profile,** these containers will crash immediately with glommio's io_uring probe error. This is the expected behavior with Docker's default profile, and the reason the custom profile exists.

**You may see a warning** in the logs about buffer registration:

```
glommio::sys::uring: Error: registering buffers in the poll ring. SkippingOs {
    code: 12, kind: OutOfMemory, message: "Cannot allocate memory"
}
```

This is a non-fatal warning — glommio continues without registered buffers, using a slightly less optimal I/O path. It can appear on Docker Desktop (macOS/Windows) and on Docker Engine on Linux, depending on the system's default memlock limits and how the container runtime enforces the ulimit. The tracker works correctly without registered buffers; the optimization is a small reduction in memory copies during I/O. If you want to eliminate the warning, try increasing the memlock ulimit in the compose file.

### Verify the trackers respond

```bash
# HTTP tracker — send an announce request
curl "http://localhost:8081/announce?info_hash=01234567890123456789&peer_id=01234567890123456789&port=6881&uploaded=0&downloaded=0&left=100&compact=1"
# Should return a bencoded response (binary data)

# WebSocket tracker — check the health endpoint
curl http://localhost:8082/health
# Should return: Ok
```

### What local testing tells you (and what it doesn't)

Local testing confirms:

- The Dockerfiles build successfully
- The Rust compilation works with the pinned compiler version
- The TOML configs parse without errors (Aquatic rejects unknown fields immediately)
- The binaries start and listen on the configured ports
- The custom seccomp profile allows glommio to initialize io_uring
- The HTTP tracker responds to announce requests
- The WebSocket tracker responds to health checks

Local testing does not confirm:

- Real-world performance under load
- IPv6 dual-stack behavior
- Source IP preservation through Docker bridge networking (clients seeing real peer IPs vs. Docker bridge IPs)
- Interaction with `DOCKER-USER` iptables rules for outbound blocking
- Container behavior under read-only filesystem and dropped capabilities (Docker Desktop may enforce these differently than Docker Engine)
- Reboot survival with `restart: unless-stopped`

These are verified during production deployment on a real Linux server.

## Container outbound blocking

The container hardening described above — nobody user, read-only filesystem, dropped capabilities, seccomp filtering — protects against what a compromised process can do inside its container. Outbound blocking protects against what it can reach outside.

By default, Docker containers can make outbound connections to anywhere — the internet, the local network, the router, other devices. A compromised tracker container could exfiltrate data, scan the LAN, or call back to an attacker's command-and-control server. Outbound blocking closes this door.

### How DOCKER-USER works

Docker manages its own iptables chains for port publishing (DNAT rules that route inbound traffic to containers). These chains are rebuilt every time Docker starts or a container is created, so any rules you add to them get overwritten.

The `DOCKER-USER` chain is the exception. Docker creates it but never modifies it — it's specifically designed for administrators to add persistent firewall rules that Docker won't overwrite. Traffic that enters a container via Docker's port publishing passes through `DOCKER-USER` before reaching the container.

### The rules

Three rules, applied to both iptables (IPv4) and ip6tables (IPv6). Order matters — they're evaluated top to bottom, and the first match wins. All six commands use `-A` (append), so the order you run them is the order they appear in the chain.

**Rule 1: Allow container-to-container traffic within the Docker network.**

```bash
iptables  -A DOCKER-USER -s 172.30.0.0/24 -d 172.30.0.0/24 -j RETURN
ip6tables -A DOCKER-USER -s fd00:cafe:2::/64 -d fd00:cafe:2::/64 -j RETURN
```

This allows the containers in the tracker's compose project to communicate with each other. The stats dashboard needs to scrape Prometheus metrics from the three tracker containers over the internal Docker network. Without this rule, that traffic is blocked by the DROP rule below.

`RETURN` means "stop processing DOCKER-USER and continue to Docker's own rules." It does not mean "allow unconditionally" — Docker's own chains still apply.

**Rule 2: Allow responses to inbound connections.**

```bash
iptables  -A DOCKER-USER -s 172.30.0.0/24 -m conntrack --ctstate ESTABLISHED,RELATED -j RETURN
ip6tables -A DOCKER-USER -s fd00:cafe:2::/64 -m conntrack --ctstate ESTABLISHED,RELATED -j RETURN
```

When an external client sends a packet to the tracker, the tracker needs to send a response back. The `ESTABLISHED,RELATED` conntrack state matches packets that are part of an existing connection — i.e., responses to traffic that someone else initiated. Without this rule, the tracker could receive requests but never respond.

**Rule 3: Drop everything else.**

```bash
iptables  -A DOCKER-USER -s 172.30.0.0/24 -j DROP
ip6tables -A DOCKER-USER -s fd00:cafe:2::/64 -j DROP
```

Any packet from the container subnet that isn't intra-subnet (rule 1) or a response to inbound traffic (rule 2) is dropped. This blocks all container-initiated outbound connections — to the internet, to the LAN, to the router, to other Docker networks. The container is fully isolated.

Note: `-A` appends rules after any existing rules in the DOCKER-USER chain. On a fresh Docker installation this works correctly — Docker starts the chain with a single RETURN-all rule, and our rules go before it. If you already have custom rules in DOCKER-USER, check the final order with `iptables -L DOCKER-USER -n -v` to make sure the rules are positioned correctly.

### Why both iptables and ip6tables

Most Docker firewall guides only show iptables rules. This works if all your traffic is IPv4, but our UDP tracker accepts IPv6 connections directly through Docker's ip6tables DNAT. If we only wrote IPv4 rules, a compromised container could make outbound IPv6 connections — IPv4 outbound would be blocked, but IPv6 outbound would be wide open.

The rules are identical in logic, just applied to both address families. Every rule has an iptables line and an ip6tables line.

### The security trade-off

Before these rules, each container was completely isolated — it couldn't talk to anything, not even the container next door. After these rules, the containers within the same Docker network (`ftorrent-open`) can reach each other.

This means a compromised tracker container can now probe the other containers on the same network — for example, reaching the Prometheus endpoint on port 9000 of a neighboring container. This is the trade-off we accept in exchange for the stats dashboard being able to scrape metrics.

The trade-off is acceptable because:

- All containers in the group are at the same trust level — all internet-facing, all hardened identically
- The walls to the outside are unchanged — the LAN, the internet, and other Docker networks remain unreachable
- The blast radius expands from one container to five, but the prison walls are the same

If you run other Docker compose projects on the same server (a game server, a web app, a database), those are on different Docker networks with different subnets. The tracker containers can't reach them — the intra-subnet RETURN rule only matches the tracker's subnet.

### Making the rules persistent

The iptables and ip6tables rules above take effect immediately but don't survive a reboot. There are several ways to persist them; the most common on Debian/Ubuntu is the `iptables-persistent` package:

```bash
sudo apt install iptables-persistent
```

During installation, it asks whether to save the current rules. Say yes. After that, the current rules are saved to `/etc/iptables/rules.v4` and `/etc/iptables/rules.v6` and restored at boot.

If you add or change rules later, save them again:

```bash
sudo netfilter-persistent save
```

### Verifying the isolation

After applying the rules, verify that the isolation works as intended:

```bash
# Verify the rules are in place and in the right order
sudo iptables  -L DOCKER-USER -n -v
sudo ip6tables -L DOCKER-USER -n -v

# From inside a tracker container, try to reach the internet — should timeout
docker exec <container> timeout 3 curl -s http://1.1.1.1 || echo "Blocked (good)"

# From inside a tracker container, try to reach the LAN gateway — should timeout
# (replace with your actual LAN gateway address)
docker exec <container> timeout 3 curl -s http://192.168.1.1 || echo "Blocked (good)"
```

Note: the tracker containers are minimal Debian images without curl installed. To run these tests, you can temporarily start a test container on the same network:

```bash
# The network name is set explicitly in the compose file (ftorrent-open).
# Verify with: docker network ls
docker run --rm --network ftorrent-open -it debian:bookworm-slim bash
# Inside the container:
apt update && apt install -y curl
curl -s --max-time 3 http://1.1.1.1 || echo "Blocked (good)"
```

The stats dashboard scraping Prometheus is the positive test — if the dashboard shows live metrics from the tracker containers, intra-subnet communication is working. If the curl tests above timeout, outbound is blocked. Both conditions together confirm the isolation is correctly configured.

## Reverse proxy (nginx)

nginx is the final layer between the internet and the TCP containers. It listens on ports 80 and 443 on the host (both IPv4 and IPv6), terminates TLS, and routes each incoming TCP request to the right container over localhost. UDP traffic never touches nginx — it goes directly from Docker DNAT to the UDP container, as described in "The traffic path" above.

Setting up nginx as a reverse proxy is well-documented ground and any Linux sysadmin or AI coding agent can handle the standard parts. This section focuses on the tracker-specific requirements: path-based routing, WebSocket upgrades, real client IPs, and dual-stack listening.

### The four routing cases

A single nginx server block on port 443 routes incoming requests to different containers based on what they ask for:

| Request | Routed to | How it's matched |
|---|---|---|
| `GET /announce?...` or `GET /scrape?...` | HTTP tracker container (`127.0.0.1:8081`) | Path regex `~ ^/(announce\|scrape)` |
| WebSocket upgrade | WebSocket tracker container (`127.0.0.1:8082`) | `Upgrade: websocket` header |
| Anything else | Homepage/stats container (`127.0.0.1:8080`) | Default location |

A second server block on port 80 serves only two purposes: certbot's HTTP-01 ACME challenges, and a `301` redirect to HTTPS for everything else.

### WebSocket upgrade routing

nginx routes by HTTP header using a `map` directive. This must live at the `http` block level, not inside `server` — nginx refuses to start if it's in the wrong scope. A typical pattern:

```nginx
map $http_upgrade $tracker_backend {
    default      http://127.0.0.1:8080;   # Homepage
    websocket    http://127.0.0.1:8082;   # WebSocket tracker
}
```

The variable `$tracker_backend` resolves to the WebSocket container when the client sends `Upgrade: websocket`, and to the homepage container otherwise. The HTTP tracker is handled separately via a `location ~ ^/(announce|scrape)` block that takes precedence over the default.

Inside the location block that proxies to the WebSocket container, two directives are required:

```nginx
proxy_http_version 1.1;
proxy_set_header Upgrade $http_upgrade;
proxy_set_header Connection "upgrade";
```

**Both are necessary.** nginx defaults to HTTP/1.0 for upstream connections, which doesn't support `Upgrade`. Without `proxy_http_version 1.1`, WebSocket upgrades fail silently — the connection closes after the initial request.

### Real client IPs

The HTTP tracker needs to see real client IPs to build its peer list. Without help, it would see `127.0.0.1` (nginx's local address) for every client. nginx provides the real IP through the `X-Forwarded-For` header:

```nginx
proxy_set_header X-Real-IP $remote_addr;
proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
proxy_set_header X-Forwarded-Proto $scheme;
proxy_set_header Host $host;
```

The Aquatic HTTP config reads this header:

```toml
runs_behind_reverse_proxy = true
reverse_proxy_ip_header_name = "X-Forwarded-For"
reverse_proxy_ip_header_format = "last_address"
```

`last_address` is the secure choice when nginx is the only proxy in the chain — nginx sets the header itself, so untrusted values from the client are ignored. If there were multiple proxies in front of nginx (a CDN, a load balancer), a different format would be needed.

### IPv4 and IPv6 dual-stack

nginx handles dual-stack with two `listen` lines in the same server block:

```nginx
listen 443 ssl;
listen [::]:443 ssl;
```

That's it. nginx accepts IPv4 and IPv6 connections equivalently and proxies them to the same localhost backend. The TCP containers only bind IPv4 on the internal Docker bridge, which is fine — nginx terminates the client connection and initiates a new localhost connection, so the original IP version doesn't need to propagate to the backend. The real client IP (v4 or v6) still reaches the tracker through `X-Forwarded-For`.

Do the same for port 80:

```nginx
listen 80;
listen [::]:80;
```

### UDP does not go through nginx

This deserves to be stated explicitly, because readers sometimes try to proxy UDP through nginx and get stuck. **nginx does not handle the UDP tracker traffic.** nginx is fundamentally an HTTP and TCP server. It has a `stream` module that can do raw TCP/UDP proxying, but it's not used here and doesn't need to be.

UDP tracker traffic on port 443 goes directly from the internet to the UDP container via Docker's DNAT rule (defined in the compose file as `443:8443/udp`). With `userland-proxy: false` in the Docker daemon config, this is kernel-level ip6tables/iptables DNAT — nginx is entirely out of the picture for UDP.

If a reader finds themselves writing `stream { ... }` blocks for the UDP tracker, something is wrong. The UDP path is handled entirely by Docker and the host's routing.

### TLS via certbot

TLS certificates are obtained and renewed by [certbot](https://certbot.eff.org/) using Let's Encrypt and the HTTP-01 challenge. On Debian/Ubuntu, the package is `certbot python3-certbot-nginx`. The `--nginx` plugin integrates with nginx directly — it reads the existing config, determines which domains are served, obtains certificates, and edits the config to reference them. Renewal happens automatically via a systemd timer installed by the package.

One thing to know: certbot's HTTP-01 challenge temporarily places files under `/.well-known/acme-challenge/` on port 80 that must be reachable. The certbot nginx plugin handles this automatically during renewal — the administrator does not need to configure a permanent exception for ACME challenges.

### Putting it together

A minimal sketch (not a complete config — adapt to your needs):

```nginx
# Outside the server blocks, at the http level
map $http_upgrade $tracker_backend {
    default      http://127.0.0.1:8080;
    websocket    http://127.0.0.1:8082;
}

# Port 80 — redirect everything except ACME challenges to HTTPS
server {
    listen 80;
    listen [::]:80;
    server_name open.ftorrent.com;

    location /.well-known/acme-challenge/ {
        root /var/www/certbot;
    }

    location / {
        return 301 https://$host$request_uri;
    }
}

# Port 443 — the main server block
server {
    listen 443 ssl;
    listen [::]:443 ssl;
    server_name open.ftorrent.com;

    # TLS — managed by certbot --nginx
    ssl_certificate     /etc/letsencrypt/live/open.ftorrent.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/open.ftorrent.com/privkey.pem;

    # HTTP tracker — /announce and /scrape
    location ~ ^/(announce|scrape) {
        proxy_pass http://127.0.0.1:8081;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_set_header Host $host;
    }

    # Everything else — routed by Upgrade header via the map
    location / {
        proxy_pass $tracker_backend;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_set_header Host $host;
    }
}
```

This is the shape of the configuration — real deployments will have additional directives (logging paths, SSL protocols, gzip, security headers) that are standard nginx practice and out of scope for this guide.

### Verifying it works

After nginx is configured, reloaded, and the containers are running:

```bash
# HTTP tracker — should return a bencoded response
curl "https://open.ftorrent.com/announce?info_hash=01234567890123456789&peer_id=01234567890123456789&port=6881&uploaded=0&downloaded=0&left=100&compact=1"

# WebSocket tracker — should return "Ok" (the health endpoint)
curl https://open.ftorrent.com/health

# Homepage — should return whatever the homepage container serves
curl https://open.ftorrent.com/
```

A successful deployment returns a bencoded response from `/announce`, `"Ok"` from `/health`, and the homepage HTML from `/`. If any of these fail, check the nginx logs (`/var/log/nginx/error.log`), the container logs (`docker logs <container>`), and confirm that the backend ports in the `map` and `proxy_pass` directives match what the compose file publishes on localhost.

## What's next

The sections above cover the full deployment path — architecture, traffic path, server preparation, container configuration, build and test, outbound blocking, and the reverse proxy. A reader who followed the guide in order now has a running public tracker.

The deployment at [open.ftorrent.com](https://open.ftorrent.com/) adds one more thing on top: a dashboard status page that shows what the tracker is doing. Each minute, a back end Node container writes [page.json](https://open.ftorrent.com/page.json) which a front end Vue page displays and animates.

[The 'gauge' back end](gauge/README.md). 📟 A Node.js container that runs next to the trackers. Once a minute it scrapes each tracker's Prometheus endpoint, reads memory usage from cgroup files, and writes a single `page.json` file that nginx serves as a static file. It has no HTTP server and no listening ports — it only writes files. The guide explains the ring buffer that computes 24-hour totals from a single subtraction, the internet reachability probe that separates "the server was down" from "the internet was down," and how modest hardware supports tens of millions of concurrent peers.

[The 'page' front end](page/README.md). 🌎 A Vue + Vite single-page application. It fetches `page.json` and renders the dashboard: a slowly turning Earth, announce URLs, and six counters whose digits tick in real time from a Poisson process matched to each tracker's recorded rate. The guide covers the scaffolding choices, typography and self-hosted fonts, the NASA Blue Marble textures, and the math behind the rate animation.

Neither piece is required to run a tracker. The three Aquatic containers from the sections above serve BitTorrent and WebTorrent clients on their own. The dashboard exists because a public service is easier to trust when you can see it running, and because the deployment was a good chance to write down how to build one.
