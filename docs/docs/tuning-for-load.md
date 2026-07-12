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

## The socket layer's TIME_WAIT cap

The conntrack chapter trimmed TIME_WAIT's *flow entry* — netfilter's memory that a closed connection existed. The socket layer keeps a TIME_WAIT of its own, one layer down, with the same failure shape and its own ceiling.

When the server closes a TCP connection first — and a busy HTTPS service closing idle keep-alive connections does this thousands of times a second — the kernel holds the socket in **TIME_WAIT** for 60 seconds (a compile-time constant) so a straggler packet from the old connection can't be mistaken for part of a new one on the same address-and-port pair. `net.ipv4.tcp_max_tw_buckets` caps how many sockets may wait like this at once. At the cap the kernel doesn't queue, slow down, or say anything to the application: each further close just destroys the socket instantly, skipping the protection TIME_WAIT exists to provide. There's no single **Linux kernel default** — it's computed at boot from the RAM-scaled TCP hash table, landing at **65,536** on a 16 GB **Ubuntu Server 24.04** box. **OpenWrt** has no position on this one, for the same reason it doesn't raise socket buffers: a router forwards, it doesn't terminate, so it never accumulates TIME_WAIT at all. We run **1,048,576**.

This is one wall you can catch red-handed, because saturation has an unmistakable signature. A healthy TIME_WAIT set *floats* — its size wanders with traffic. A saturated one reads exactly the sysctl's value, continuously:

```bash
# A timewait count sitting *exactly* at tcp_max_tw_buckets, around the
# clock, is a set pinned to its cap — not a busy coincidence
ss -s

# And every increment here is one close that skipped its protection
nstat -az TcpExtTCPTimeWaitOverflow
```

Measured here before the fix: `timewait 65536` around the clock — never a socket more — and **5.7 billion** overflows accumulated over ten days, 400–740 million a day, which is to say essentially every close the server made. Nothing else on the box hinted at it. Raised, the set climbed past 350,000 within the first minute, settling toward its natural ~500,000 at this churn — and the overflow rate went to exactly zero.

```
net.ipv4.tcp_max_tw_buckets = 1048576
```

The memory story is the conntrack chapter's again: each TIME_WAIT socket is a small on-demand slab object, ~256 bytes, gone 60 seconds later — about 130 MB floating at this churn, bounded near 256 MiB if the raised cap ever filled. And the distinction between the two TIME_WAITs bears spelling out, because they're easy to conflate: `nf_conntrack_tcp_timeout_time_wait` trims how long netfilter's *flow entry* lingers after the close; `tcp_max_tw_buckets` caps the socket layer's *own set*. Tuning one does nothing for the other.

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

## The NIC receive ring

The section above followed a packet from the NIC up through the backlog queue to a socket buffer. One layer sits below all of that, in the hardware itself, and it hosts the only truly unrecoverable drop on the whole path.

Before the kernel touches a packet, the NIC places it — by direct memory access, no CPU involved — into a **receive descriptor ring**: a per-queue circle of slots the driver hands to the hardware, each slot pointing at a buffer that can hold one incoming packet. The kernel drains filled slots in softirq and reposts them empty. A microburst that outruns the reposting — even for tens of microseconds — leaves the NIC holding a packet with no free slot, and the hardware discards it. The kernel never saw the packet. No socket counter, no backlog column, no log line above can record it, because nothing above the NIC was ever involved. The only witness is the NIC's own statistics, read with `ethtool -S` — the counter is `rx_no_dma_resources` on Intel's ixgbe driver; other drivers use other names. (No OpenWrt column for this one — it isn't a sysctl at all, which is rather the point: this layer hides below the file every other section of this guide writes to.)

The way the ring fills defeats intuition, and it's worth getting right before sizing it. There is no steady water level. Slots turn over in microseconds, so at a tracker's ordinary rate — tens of thousands of packets a second spread across a multi-queue NIC's many queues — the instantaneous occupancy of any one ring is zero or one slot. The ring runs at ~0% essentially always; then a burst takes one queue's ring to 100% for a flash, and every drop happens inside such a flash. Measured here at the driver default of **512** slots per queue: a few hundred to a few thousand hardware drops a day at ordinary load, 99% of minutes at zero — rising roughly sixfold when UDP announce volume tripled over a weekend, to about 0.0005% of four billion packets a day. Harmless even at the risen level, and trending with growth.

Sizing is about time, not capacity. A deeper ring buys the kernel longer to come back from a stall: against a worst-case burst concentrated on one queue, 512 slots absorb roughly 340 microseconds of drain stall, and **4,096** — where we run — absorbs about 2.7 milliseconds, comfortably past the scheduling transients that cause stalls. A burst that outruns 4,096 slots isn't a burst anymore; it's sustained arrival faster than drain, which no ring depth fixes. And unlike conntrack's pay-for-what's-live table, ring memory is **preallocated and held**: about 2 KB per slot, idle or saturated, so deepening 512 → 4,096 takes each queue from ~1 MB to ~8 MB — times every queue on the card, which lands a modern many-queue NIC in the low hundreds of megabytes, held around the clock (and the hardware maximum, 32,768 slots per queue, would be eight times that again). That's why you size to the burst class you actually see, not to what the card will take.

The live change is one command, with one operational surprise (`enp1s0` here is whatever predictable name your interface has):

```bash
ethtool -G enp1s0 rx 4096
```

Applying it **reinitializes the receive path** — we measured a carrier flap that dropped the box's DHCPv4 lease for about 30 seconds before the ISP re-leased the same address to the same MAC. Plan for a brief WAN pause, not an invisible change.

And `ethtool -G` doesn't survive a reboot — which brings up a trap nastier than the setting itself. On a systemd-networkd system, the native persistence is a `.link` file. But **udev applies only the first matching `.link` file**: a custom one doesn't merge with the stock `99-default.link`, it replaces it entirely — including `NamePolicy`, the setting that gives NICs their predictable names at boot. Write a minimal `.link` with just a match and `RxBufferSize`, and on the next boot the port comes up as `eth0` — and everything keyed to the predictable name (the `.network` file, firewall rules, monitoring) silently fails. On a headless box, that's a machine that boots and never comes back onto the network. The working shape replicates the stock file's policy lines verbatim and adds the one new setting:

```
# /etc/systemd/network/10-tracker-nic.link
[Match]
PermanentMACAddress=aa:bb:cc:dd:ee:ff

[Link]
NamePolicy=keep kernel database onboard slot path
AlternativeNamesPolicy=database onboard slot path
MACAddressPolicy=persistent
RxBufferSize=4096
```

Match on the permanent MAC — `ethtool -P` prints it — so the file follows the physical port rather than a name that might change. And before trusting a boot to it, one command proves both halves with no reboot: `ID_NET_LINK_FILE` must name your custom file (it matched), and `ID_NET_NAME` must show the predictable name (the name policy survived). Both right, or stop and fix:

```bash
udevadm test-builtin net_setup_link /sys/class/net/enp1s0 2>&1 | grep -E "ID_NET_LINK_FILE=|ID_NET_NAME="
```

## The softirq budget, and the counter that cries wolf

The receive-path section sent you to `/proc/net/softnet_stat` to check the second column for drops. Sit with that file on any busy box and the **third** column — `time_squeeze` — will climb while you watch, and it looks alarming. This section exists mostly so you don't tune a healthy machine — and because this one setting is the clearest window in the whole file into how Linux actually runs the network stack.

Start with what happens when a packet arrives. The NIC writes it into the receive ring and raises an interrupt. The interrupt handler does almost nothing — acknowledges the hardware, masks further interrupts from that queue, and schedules the real work. That work runs moments later in **softirq** context on the same core: a kernel loop pulls packets off the ring and carries each one up the entire stack — ethernet, IP, conntrack and Docker's DNAT, UDP, into a socket's receive buffer — then returns to the ring for the next. Nearly everything this guide has described happens inside that loop. It *is* the network stack, executing.

A loop like that, holding a CPU during a flood, would happily hold it forever — so the kernel gives it a quantum, exactly the way the scheduler gives a process a timeslice. One softirq pass may process `netdev_budget` packets (**Linux default 300**) or run for `netdev_budget_usecs` (**default 2,000 µs**), whichever comes first; then it must stop — even with packets still queued — hand the CPU back, and let the leftover work resume in the next pass, often on `ksoftirqd`, a kernel thread that runs at normal priority precisely so user programs can compete with it. That forced stop with work remaining is one tick of `time_squeeze`. A squeeze is a **yield, not a drop**: nothing is lost, only deferred — and since arrival doesn't pause while the loop is away, this setting and the NIC ring above are siblings: the budget decides how often the kernel steps away from the conveyor belt, and the ring decides how much can pile up while it's away.

So what does raising the budget cost, if not memory? Nothing is *consumed* at all — it's a ceiling, not an allocation. A pass that finds 40 packets queued processes 40 and exits, identical down to the instruction under either ceiling; the setting only exists, behaviorally, in the moments when more packets than the budget are waiting on one core. When it does engage, the currency is the **worst-case latency of everything else on that core**: during a burst, the packet loop may now hold the CPU for up to 8 ms before yielding instead of 2, and a user-space thread waiting for that particular core waits accordingly longer. The total work is conserved either way — every packet gets processed, in fewer long turns or more short turns — and fewer turns is marginally *cheaper*, since each yield-and-resume pays a little overhead. The processor runs as hot as the traffic makes it, not as hot as the sysctl allows.

Which explains why the kernel default is low. 300 packets and 2,000 µs are tuned for the general-purpose machine — the laptop, the desktop, the app server — where a human or a latency-sensitive process lives on the other side of that core, and packet processing should never make the machine feel sticky. The default optimizes for the responsiveness of everything that *isn't* networking. A box whose job is packets can afford longer turns — and with a multi-queue NIC spreading receive queues across the many cores of any modern processor, an occasional 8 ms hold on one of them is unmeasurable.

Measured here: drops read **zero across 10.3 days** at tens of millions of requests a day, while `time_squeeze` ticked along at ~1,500 a day — the budget engaging about once a minute, harmlessly. We doubled it anyway, free insurance:

```
net.core.netdev_budget = 600
net.core.netdev_budget_usecs = 8000
```

But the order of operations is the teaching point: **check the drops column before touching anything**. A climbing `time_squeeze` beside zero drops is a busy box doing normal bookkeeping, not a wall.

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

The kernel settings here ship as a ready-to-use, fully commented file, [`open/90-tracker.conf`](https://github.com/zootella/ftorrent/blob/master/open/90-tracker.conf) — drop it in `/etc/sysctl.d/` and apply with `sudo sysctl --system`. The one exception is the NIC receive ring, which is not a sysctl at all: `ethtool -G` for a one-off, a `.link` file for persistence, as its section shows. The Docker daemon, seccomp, and reverse-proxy pieces, in deployment context, are in the [Aquatic guide](https://github.com/zootella/ftorrent/blob/master/open/README.md). And [Tracker Load](/tracker-load) measures what all of this actually costs in production: 77 million requests in a day, zero dropped packets, under 40 watts.
