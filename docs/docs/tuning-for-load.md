---
title: Tuning for Load
description: How to configure Ubuntu Server and Docker to carry a huge number of short-lived UDP and HTTPS connections — what each kernel default limits, how load finds that wall, and what to change.
---

# Tuning for Load

Most of a Linux server's network defaults are sized for a familiar world: a few hundred clients holding connections open for seconds or minutes, exchanging plenty of data over each. A service that fields a _huge number of short-lived connections_ — millions of distinct clients a day, each opening a connection, trading a few packets, and vanishing in milliseconds — lives in a different world, and walks into ceilings the defaults never anticipated. The packets don't bounce off those ceilings loudly; they vanish, silently, with nothing in your application's logs to say why.

The workload behind this guide is a public BitTorrent tracker — tens of millions of UDP and HTTPS announces a day on one modest server ([what that actually costs, measured](/tracker-load)) — but nothing below is specific to BitTorrent. The same walls are hit by anything with the same shape: a DNS resolver, a STUN/TURN server, a game backend, an ad exchange. Each section takes one piece of Linux, shows the default that becomes a wall under this load, what breaks when you reach it (and why you'd never notice), what we change, and why the box is fine afterward. The aim is that you finish understanding the mechanism, not just holding a number to paste.

Where a setting is just a number, we give three: the **Linux kernel default** — or, where there isn't a single one, the value on **Ubuntu Server 24.04** — the default on **OpenWrt**, and the value we use. Watch the OpenWrt column. OpenWrt is Linux built to _be a router_, and a router's whole life is tracking and forwarding a flood of connections; so on the settings that matter most here, its maintainers have already moved in the direction we're about to. Much of this guide is simply giving a general-purpose server a router's posture.

## The connection-tracking table

When a packet crosses Docker's bridge network, the kernel has to remember it. Docker publishes a container's port by rewriting each inbound packet's destination address to the container (DNAT — its own section, below), and to route the reply back it must reverse that rewrite — which means remembering, for every flow, that the connection exists and how to translate it. That memory is the **connection-tracking table**, `conntrack`: a hash table the kernel keeps in its own memory, one entry per flow. Netfilter, the kernel's NAT and firewall machinery, creates an entry the first time it sees a flow and consults it on every packet after.

The table has a hard ceiling, `nf_conntrack_max`. There's no single Linux default for it — the kernel computes one at boot from the machine's RAM, sizing a hash of `nf_conntrack_buckets` and setting max to four times that. On **Ubuntu Server 24.04** with 16 GB of RAM that lands at **262,144** entries over **65,536** buckets. A router expects to track far more, and **OpenWrt** dropped its old fixed cap years ago to scale the table up from device RAM — the first sign of where we're headed: the router OS already assumes a flood, where the server OS assumes a workstation.

262,144 sounds generous until you count unique clients. A tracker sees hundreds of thousands of distinct address-and-port pairs an hour, each a new flow, each an entry. When the table fills, the kernel does the only thing it can — it **drops every new flow** and logs `nf_conntrack: table full, dropping packet` to the kernel ring buffer. Established connections keep working; new clients simply can't get in, the worst possible group to turn away, and your application logs stay silent. The only trace is that `dmesg` line and a climbing count in `conntrack -S`.

There's a quieter failure in the buckets. The table is a hash table, and `nf_conntrack_buckets` is how many buckets entries hash into. Raise `max` without raising buckets and you don't overflow — you pile more entries into the same number of buckets, lengthening the chains the kernel walks on _every_ packet. The table keeps working; it just gets slower, per packet, across the board, for a reason that never surfaces as an error. So we raise both together — in `/etc/sysctl.d/90-tracker.conf`:

```
net.netfilter.nf_conntrack_max = 2097152
net.netfilter.nf_conntrack_buckets = 524288
```

524,288 is a quarter of max, holding the kernel's natural 4:1 ratio and keeping chains short.

Sizing the table up is half of it; getting entries _out_ sooner is the other half. An entry lingers for a timeout after its last packet, and the defaults assume long-lived connections. A closed TCP connection sits in `TIME_WAIT` for **120 seconds** — and a tracker's announce churn makes those closed-connection entries the table's single biggest occupant, so we cut it to **30**. The `established` timeout is the real landmine: **432,000 seconds, five days**, so one announce connection that goes quiet without closing squats a slot for most of a week. We bound it to **7,440** (~2 hours):

```
net.netfilter.nf_conntrack_tcp_timeout_time_wait = 30
net.netfilter.nf_conntrack_tcp_timeout_established = 7440
```

UDP we mostly leave alone. `nf_conntrack_udp_timeout` is already the kernel's 30 seconds; we keep the bidirectional timeout a conservative 180 (a touch over the kernel's 120) and let the large table absorb the residency rather than shorten _every_ UDP flow on the box:

```
net.netfilter.nf_conntrack_udp_timeout = 30
net.netfilter.nf_conntrack_udp_timeout_stream = 180
```

The aggressive tightening the tracker's own ports want is done surgically instead, so it never touches co-resident UDP like DNS or NTP. Linux lets you give specific flows a different conntrack timeout than the global one: define a named timeout policy with `nfct` (from `conntrack-tools`), then attach it to matching traffic with a rule in the `raw` table's `PREROUTING` chain — which runs _before_ conntrack creates the entry, so the policy is stamped on the flow as it's born.

```
# a policy: 1s for an unanswered flow, 2s once it's two-way
nfct add timeout udp4 inet udp unreplied 1 replied 2

# stamp it on the tracker's UDP port, on the public interface
iptables -t raw -I PREROUTING -i eth0 -p udp --dport 443 -j CT --timeout udp4
```

Now an announce to that port gets a 1–2 second residency — far tighter than any global value would be safe — while everything else on the box keeps the conservative 30/180 default. (`eth0` is whatever your public interface is; for IPv6 you repeat this with `ip6tables` and a _separate_ policy name, since one `nfct` policy can't be referenced from both.) The technique is port-agnostic — point it at whatever ports carry your short-lived flood. These are runtime objects, not config files, so making them survive a reboot is its own small piece of boot-time wiring.

What does two million entries cost? Here it helps to know _where_ the memory lives. A conntrack entry is a **kernel slab object** — about 256 bytes, allocated from the kernel's own allocator, not from any process's heap, and not swappable. Two things follow. It's allocated on demand, so the table costs only what's currently live: an idle box uses almost nothing, and a full two-million-entry table tops out near 512 MiB. And because it's kernel memory, it never appears as a process's RSS and can't be paged out under pressure — it's memory you spend deliberately. The buckets are different: the hash index is allocated once at boot, a flat ~4 MiB for 524,288 buckets, present whether the table is full or empty. Cheap insurance against slow lookups.

One footgun is specific to these keys. The `nf_conntrack` module isn't loaded early in boot, and `sysctl` silently skips keys that don't exist yet — so on a fresh boot your `nf_conntrack.*` settings are quietly ignored and the table reverts to its small default, with no error anywhere. Pre-load the module so the settings actually stick:

```bash
echo nf_conntrack | sudo tee /etc/modules-load.d/conntrack.conf
```

## The receive path: socket buffers and the backlog queue

Before the application ever sees a packet, the kernel has to hold it somewhere, and two queues sit on that path. When the NIC hands a packet up, it lands first in a **per-CPU backlog queue** while the kernel's softirq routine catches up; once routed to a socket, it waits in that socket's **receive buffer** until the application reads it. Both are fixed-size, and both drop silently when full.

The socket-buffer ceiling, `net.core.rmem_max`, is a flat kernel constant — **212,992 bytes, about 208 KB** — on Ubuntu and OpenWrt alike. This is one place OpenWrt _doesn't_ follow us, and the reason is itself a lesson: a router _forwards_ packets, it doesn't terminate the sockets that need deep receive buffers, so it has no reason to raise this. The backlog queue, `net.core.netdev_max_backlog`, defaults to **1000** packets, again on both.

208 KB isn't much headroom. While the application keeps draining the socket it's plenty, but let it stall for a moment — a CPU spike, a cgroup throttle, a pause — and a burst arriving in that gap overflows the buffer, and the kernel drops what won't fit. The application never knows; the only evidence is `RcvbufErrors` climbing in `/proc/net/snmp`. The backlog queue fails the same way one layer earlier: a burst arriving faster than softirq can drain — likelier here, since Docker's veth pair adds per-packet overhead — overflows the 1000-deep queue, and those packets die _before reaching any socket at all_, recorded only in the second column of `/proc/net/softnet_stat`. These are the hardest drops to find precisely because nothing in your service notices them.

We give both real slack:

```
net.core.rmem_max = 26214400
net.core.wmem_max = 26214400
net.core.rmem_default = 1048576
net.core.wmem_default = 1048576
net.core.netdev_max_backlog = 10000
```

A 25 MiB ceiling and a 10,000-deep backlog. One subtlety: raising the ceiling only helps if the application actually requests the larger buffer — `rmem_max` is a limit on what a socket _may_ ask for, and a socket gets what it asks for, up to that. So this pairs with the app asking: our UDP tracker requests an 8 MB receive buffer, which the kernel now grants instead of clamping to 208 KB. Raise the ceiling without the app asking, or the reverse, and nothing changes. This memory, too, is the kernel's, charged per socket as packets actually queue — a 25 MiB ceiling doesn't reserve 25 MiB, it caps what one socket may hold during a burst, and most sit far below it.

## Docker's bridge: keep NAT in the kernel

When you publish a container's port, Docker has two ways to get outside packets to it. By default it runs a **userland proxy** — an actual process, `docker-proxy`, that listens on the published port, accepts each connection, and copies bytes between the outside and the container. The alternative is to let the kernel do the work: an iptables/nftables DNAT rule in the `PREROUTING` chain rewrites each packet's destination to the container and forwards it, with no userspace process in the path. This isn't a kernel number with a distro default; it's a Docker daemon setting, `userland-proxy`, and it ships **on**.

The proxy has two problems at scale, and the second is fatal in a way specific to what a tracker is. The first is cost: every packet is copied through a userspace process — a context switch and a memory copy per packet, in the hottest path on the box — overhead the in-kernel path simply doesn't pay. The second: the userland proxy opens the connection to the container _itself_, from the bridge gateway, so the container sees every client as coming from one address, `172.x.x.1`, not the real client. A tracker builds its entire answer — the peer list — out of client IP addresses. Hand it one address for everyone and the peer list is nonsense and the swarm collapses, silently, with nothing in the logs to explain it.

Set it off, in `/etc/docker/daemon.json`:

```json
{ "userland-proxy": false }
```

Docker then publishes ports with in-kernel DNAT: the rewrite happens in `PREROUTING`, the real source address is preserved all the way to the container, and the per-packet userspace hop is gone. This is also what makes the conntrack table from the first section load-bearing — kernel DNAT is exactly what creates those entries — so the two settings are a pair. Nothing is lost in the switch; the userland proxy survives as a default only for backward compatibility with old kernels and hairpin-routing corners that don't arise here.

## Running an io_uring service in a hardened container

Newer high-throughput services use **io_uring**, the kernel's modern asynchronous I/O interface. Instead of a syscall per operation, the application and kernel share a pair of ring buffers in memory — the app posts requests to one, the kernel posts completions to the other — and batch I/O with far fewer crossings into the kernel. It's how our HTTP and WebSocket trackers reach their throughput. Two things about running it inside a hardened container need attention, and one of them fails in a nasty, delayed way.

io_uring registers its buffers as **locked memory** — pinned so the kernel can't page it out, since it's shared with the kernel's I/O path. How much a process may lock is governed by a ulimit, `RLIMIT_MEMLOCK`, and the default a container inherits is small (historically 64 KiB; 8 MiB on recent systemd). Separately, Docker's default seccomp profile — its syscall allowlist — doesn't permit the `io_uring_*` syscalls at all. (No OpenWrt angle here; this is about containerizing an application, not routing.)

The seccomp block is loud and easy: the runtime can't initialize io_uring and the container crashes at startup. The memlock limit is the dangerous one, because it bites _later_. A fixed limit can be ample at startup and low load, then fail under sustained traffic, because io_uring **grows its ring as load rises**, registering more locked memory as it goes. When that growth hits the ceiling mid-flight the kernel refuses it and the worker exits — a crash-loop, under load, on a container that may be using only a third of its actual RAM. The wall isn't memory; it's the lock limit.

So: extend the seccomp profile to allow the three `io_uring_*` syscalls, and set `memlock` to unlimited rather than any fixed number — in the container's compose entry:

```yaml
ulimits:
  memlock: { soft: -1, hard: -1 }
```

Unlimited is the standard for io_uring runtimes (ScyllaDB and seastar ship it the same way), and it's safe here for a precise reason worth holding onto: this is the one place the two kinds of memory limit pull apart. `RLIMIT_MEMLOCK` governs how much memory may be _locked_; the container's **cgroup memory limit** governs _total_ RAM. Pinning the first to a number below the second is exactly what crashed the worker. "Unlimited memlock" doesn't mean unlimited memory — it means the ring may grow as far as the cgroup allows, and the cgroup stays the real, single ceiling. Remove the lower wall and the failure mode is gone.

## TLS termination at the reverse proxy

The HTTPS half of the load arrives encrypted, and someone has to perform the TLS handshake. We don't make the tracker do it: a **reverse proxy** sits in front, terminates TLS, and forwards plaintext to the tracker over loopback. The proxy is where the HTTPS connection volume is actually absorbed — which makes its behavior, not the tracker's, the thing that decides whether the HTTPS side scales.

The thing to understand about HTTPS at this scale is that the expensive part isn't moving data — it's the **handshake**, the asymmetric crypto at the start of each _new_ connection, which costs far more CPU than the tiny announce that follows. So the cost of HTTPS is dominated by new connections, not requests: a client that reuses one connection for ten announces pays one handshake, not ten. On modest hardware the handshake budget, not bandwidth, is what caps the HTTPS side. Two consequences follow.

First, keep connections alive and multiplex them. A proxy that holds connections open (and speaks HTTP/2) amortizes each handshake across many requests on the same connection — the single biggest lever for the handshake budget. We terminate TLS at **HAProxy**, with Caddy behind it for the static dashboard and lego for certificates. (It began as nginx; we moved to HAProxy for its runtime admin socket, which lets per-service state change on the live process without a config reload — the full comparison is in the [circuit-breaker guide](https://github.com/zootella/ftorrent/blob/master/open/breaker/README.md).)

Second, don't let logging become the bottleneck. A proxy that writes one access-log line per request will, at tens of millions of HTTPS announces a day, write tens of millions of lines — gigabytes — daily, and fill the disk faster than anything else the box does. Suppress access logging on the announce and scrape happy path (in nginx, `access_log off;` inside those locations; other proxies have an equivalent), and keep it only for errors and the paths you actually watch.

With TLS terminated, kept alive, and multiplexed at the proxy, the tracker behind it speaks cheap plaintext over loopback, and the box spends its one scarce resource — handshake CPU — only on genuinely new connections.

## Where these settings live

The kernel settings here ship as a ready-to-use, fully commented file, [`open/90-tracker.conf`](https://github.com/zootella/ftorrent/blob/master/open/90-tracker.conf) — drop it in `/etc/sysctl.d/` and apply with `sudo sysctl --system`. The Docker daemon, seccomp, and reverse-proxy pieces, in deployment context, are in the [Aquatic guide](https://github.com/zootella/ftorrent/blob/master/open/README.md). And [Tracker Load](/tracker-load) measures what all of this actually costs in production: 77 million requests in a day, zero dropped packets, under 40 watts.
