---
title: Tracker Load
description: What a public BitTorrent and WebTorrent tracker actually costs to run — measured CPU, memory, bandwidth, connection tracking, and power at 77 million requests a day.
---

# Tracker Load

Measured 2026-Jun-4, the [open.ftorrent.com](https://open.ftorrent.com/) deployment served **77 million** requests, moving **10.1 Mbit/s** inbound and **18.5 Mbit/s** outbound, running at a 1-minute load average of **1.50** across its 28 hardware threads (~95% idle), holding its entire working set in **0.96 GB** of RAM, keeping a connection-tracking table peaking at **92,950** entries, drawing **40 watts** at the wall, and dropping zero packets. What follows is where each number comes from.

A public tracker's job sounds heavy — answer the entire internet's question, "who else has this?" — but the work per request is tiny, and the bill at the end of the day is astonishingly small. This is a measured account of what one real deployment costs while serving it.

The numbers come from the server's own [collectd](https://www.collectd.org/) history — one-minute samples read over rolling 7-day and 24-hour windows — plus a live pull of the [`page.json`](https://open.ftorrent.com/page.json) the gauge writes, all taken on 2026-Jun-4. They reflect a clean steady-state window: more than 24 hours since the last reboot, and not a single circuit-breaker cooldown in the previous seven days. Ordinary, uninterrupted serving, not a degraded or throttled period. Every figure is at this one demand — a tracker's load scales with its popularity — so read each as "at this traffic, the box does this," and scale to yours.

## Measured load and utilization

### Requests

On 2026-Jun-4, [open.ftorrent.com](https://open.ftorrent.com/) answered 77 million requests in 24 hours. Its [`page.json`](https://open.ftorrent.com/page.json) breaks them down by protocol and IP family:

| | IPv4 | IPv6 | Total | Share |
|---|---:|---:|---:|---:|
| UDP | 46,413,272 | 11,458,015 | 57,871,287 | 75.1% |
| HTTPS | 14,207,544 | 4,998,966 | 19,206,510 | 24.9% |
| **Total** | **60,620,816** | **16,456,981** | **77,077,797** | |
| Share | 78.6% | 21.4% | | |

**UDP carries three-quarters of the traffic**, and by design it is by far the leaner: a UDP announce is about four small packets — a quick connection handshake and the announce itself, a tiny question and a tiny answer, well under a kilobyte. An **HTTPS announce** answers the same question but wraps it in a TCP handshake, a TLS handshake, and HTTP framing — ten packets or so when the TLS session resumes, closer to twenty when it is cold and the server sends its certificate. Several times the packets for the same small answer, the cost of an encrypted, private exchange.

Total bytes divided by total requests gives a blended average of 1.5 KB inbound and 2.5–3 KB outbound per request — UDP and HTTPS, payload and overhead, all in one figure.

Unfortunately, **only 21.4% of requests arrive over IPv6**: even in 2026 the majority is still IPv4, where NAT makes direct peer connections harder, while IPv6's globally routable addresses are simply better for peer-to-peer.

### Bandwidth

What the tracker moves in each direction (collectd `if_octets`, converted to Mbit/s):

| Direction | 7-day average | 7-day peak |
|---|---:|---:|
| Inbound (rx) | 10.1 Mbit/s | 59.3 Mbit/s |
| Outbound (tx) | 18.5 Mbit/s | 32.1 Mbit/s |

Outbound runs nearly **twice inbound** — a peer-list response is larger than the announce that asks for it. Even the week's single busiest minute touched just 59.3 Mbit/s — 6% of a gigabit link. The tracker is sipping bandwidth.

### Processor

Against the box's 28 hardware threads, the 1-minute load average was **1.50** over seven days and **1.67** over the last day, with a peak of **3.92**. That peak is about 14% of total CPU capacity; the average is closer to 5% — the processor is roughly **95% idle** even while serving 77 million requests a day. 🍹⛱️

Just as important as how little is that the work is **spread, with no single-core bottleneck**. The eight performance-core primary threads run busiest at about 30–41% (the single busiest core ~41%); their hyperthread siblings sit at 5–11%; the twelve efficiency cores at 8–23%. Network-interrupt handling (softirq) is distributed across *all* cores at about 0.2–2.3% each rather than concentrated on one. That distribution is the [Receive Side Scaling](https://www.intel.com/content/www/us/en/support/articles/000006703/ethernet-products.html) effect of the multi-queue NIC: on a single-queue card, every packet's interrupt would land on one core, and that core would become the ceiling long before the others broke a sweat.

### Power

At this load the CPU package reports about **4.8 W** (Intel RAPL). The whole system, measured at the wall on the UPS the server is the sole load on, draws **36–40 watts, usually 39 or 40**. The CPU is only ~4.8 W of that; the remaining ~35 W is the rest of the platform — memory, NVMe storage, the dual-10GbE NIC's PHYs, fans, and power-supply conversion loss.

That is the headline of the whole report: a complete public tracker — processor, memory, a dual-port 10-gigabit NIC, storage, the entire Linux stack — answering 77 million requests a day on **less than a 60-watt incandescent light bulb**. 💡

### Memory

The entire service lives in about a gigabyte. Whole-system memory used was **~0.96 GB average / 1.03 GB peak**.

Per tracker, the container's own cgroup memory (the kernel's live byte count, from [`page.json`](https://open.ftorrent.com/page.json)):

| Tracker | Bytes | ≈ |
|---|---:|---:|
| aquatic UDP | 227,086,336 | 216.6 MiB |
| aquatic HTTP | 89,366,528 | 85.2 MiB |

The two host-side proxies are similarly small in resident memory: HAProxy about 145 MB, Caddy about 40 MB. Every footprint is **flat over 24 hours — average essentially equal to peak**, not climbing. Aquatic holds its peer table as a hash map of IP:port entries keyed by info hash and evicts expired peers on a timer, so memory stabilizes at a level set by *concurrent* peers, not by the total requests ever served.

Normalized against the day's traffic, the tracker footprints work out to about **3.742 MiB per million daily UDP requests** and **4.437 MiB per million daily HTTPS requests** — HTTPS keeps more state per request, consistent with its heavier exchange. (These ratios are a steady-state comparison, not a growth law: memory follows concurrent peers, which the announce interval and `max_peer_age` govern.)

### Connection tracking

Docker's bridge networking creates a connection-tracking entry for every inbound flow, and an undersized table silently drops packets under tracker load. Sized correctly, it's a non-issue here. Table occupancy averaged about **58,300** over seven days and peaked around **92,950**; against the tuned ceiling of **2,097,152** entries, that peak is only **~4.4% of the table**.

And the receive path is never overrun: the NIC's `rx_missed_errors` — packets the kernel never even sees, the one truly unrecoverable loss — were **zero across the entire seven days**, average, peak, and minimum alike.

### The Linux stack

None of this runs on anything exotic. The host is **Ubuntu Server 24.04 LTS** on bare metal, with mainstream, boring components:

- **[HAProxy](https://www.haproxy.org/) 2.8.16** — TLS termination and SNI/path/Upgrade routing
- **[Caddy](https://caddyserver.com/) 2.6.2** — static dashboard serving, on a unix socket behind HAProxy
- **Docker** — the three hardened tracker containers (run as nobody, read-only filesystem, all capabilities dropped, no-new-privileges)
- **dnsmasq**, **iptables/ip6tables** (host firewall plus `DOCKER-USER` container egress isolation), **[lego](https://go-acme.github.io/lego/)** (Let's Encrypt certificates), and **collectd** (the metrics behind most of the numbers above)

The only kernel tuning that matters for tracker load: the conntrack table sized to **2,097,152** entries (524,288 hash buckets) with timeouts adjusted for tracker flows, the `fq_codel` default qdisc, raised UDP socket buffers and NIC receive backlog, and IPv4/IPv6 forwarding enabled. All of it is documented in the [Aquatic guide](https://github.com/zootella/ftorrent/blob/master/open/README.md).

## Hardware

These costs are what they are because of two things: the request rate above, and the machine underneath. The machine is modeled on a single modest box, bare metal — nothing purpose-built or server-class.

The processor is an **Intel Core i7-14700** (Raptor Lake Refresh): 20 cores and 28 threads — eight hyperthreaded performance cores plus twelve efficiency cores — with 33 MB of L3 cache, a peak turbo of 5.4 GHz, and a 65 W base power. A mainstream desktop chip. The whole service uses about a gigabyte of memory.

The network card is a dual-port 10-Gigabit **Intel X550-T2**. Its value to a tracker isn't the 10-gigabit wire speed — the load above uses a few percent of a single gigabit — but its queues: up to 28 receive and 28 transmit hardware queues per port with [Receive Side Scaling](https://www.intel.com/content/www/us/en/support/articles/000006703/ethernet-products.html), which hashes incoming connections across the queues so packet processing spreads over all 20 cores instead of piling onto one. That is what keeps the softirq load distributed in the CPU figures above; a single-queue card would funnel every interrupt onto one core and cap throughput there.

It bears repeating that this is comfortable headroom, not a requirement. The measured load barely touches the machine, and a far more modest one would carry it without strain — the capable hardware buys years of growth, not a higher bar to entry.

## Back of the envelope: Scaling to everyone on Earth ✉️ 🌏

To close, a projection — arithmetic scaled from the measured numbers above, not a measurement. Raise the load until the tracker answers one request a day for every person alive: all **8,250,000,000** of us. What each part would then carry:

- **Requests:** **8.25 billion** a day — one for every person on Earth — in the same 75/25 UDP/HTTPS mix.
- **Bandwidth:** **21 terabytes** out and **12** in — **33 terabytes** a day.
- **Processor:** still a handful of the 28 threads; its costliest job, the TLS handshake, comes to **452 million** a day after the observed 78% session reuse, which hardware crypto absorbs in a few cores.
- **Power:** **1.9 to 2.9 kilowatt-hours** a day — a couple of times today's draw. (An estimate, riding on the processor.)
- **Memory:** **103 gigabytes** of peer-table state, held at once.
- **Connection tracking:** **9.9 million** concurrent flows.

There's a quiet wonder in the arithmetic: the whole world, introduced to one another, still from one modest machine — the center of a planet-spanning network stays small because peer-to-peer was designed to keep it that way.
