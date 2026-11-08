---
title: Tracker Load
description: What a public BitTorrent and WebTorrent tracker costs to run, measured over one day of 172,779,643 requests, what a UDP announce and an HTTPS announce each cost, and how those costs set the tracker's limits.
---

# Tracker Load

On 2026 November 2, [open.ftorrent.com](https://open.ftorrent.com/) answered **172,779,643** requests, moved **19.5 Mbit/s** inbound and **44.6 Mbit/s** outbound, used **3.0** of its 28 hardware threads (89.3% idle), held the three trackers, the [DHT](/how-the-dht-works) node, and both proxies in **1.1 GB** of memory, kept a connection-tracking table peaking at **113,800** entries, dropped zero packets, and did all that drawing only **40 watts** at the wall.

A public tracker's job sounds heavy: answer the whole internet's question, "who else has this?" But the work per request is tiny, and the bill at the end of the day is small.

The request counts are the rolling 24-hour totals the [gauge](https://github.com/zootella/ftorrent/blob/master/open/gauge/README.md) publishes in [`page.json`](https://open.ftorrent.com/page.json). Everything else comes from the server's [collectd](https://www.collectd.org/) history, one-minute samples over that day and the week ending with it. A tracker's load scales with its popularity, so read each figure as "at this traffic, the machine does this," and scale to yours.

## The machine

The processor is an **Intel Core i7-14700** ([Raptor Lake Refresh](https://en.wikipedia.org/wiki/Raptor_Lake)), a mainstream desktop chip: 20 cores and 28 threads, eight hyperthreaded performance cores and twelve efficiency cores, with a 65 W base power. The per-request costs below are this chip's.

The network card is an **[Intel X550](https://docs.kernel.org/networking/device_drivers/ethernet/intel/ixgbe.html)**, 10-gigabit. The traffic peaked at 7.2% of one gigabit, so the card's speed doesn't matter here. Its receive queues do. With [Receive Side Scaling](https://www.intel.com/content/www/us/en/support/articles/000006703/ethernet-products.html), the card hashes incoming connections across many queues, and every core takes a share of the packets.

The software:

- **[Ubuntu Server](https://ubuntu.com/server) 24.04 LTS**, [Linux](https://www.kernel.org/) 6.8, on bare metal
- **[Docker](https://docs.docker.com/engine/) 29**, running the three trackers of [Aquatic](https://github.com/greatest-ape/aquatic) 0.9.0, for UDP, HTTP, and WebSocket, in hardened containers, beside a [libtorrent](https://www.libtorrent.org/) DHT node
- **[HAProxy](https://www.haproxy.org/) 2.8**, terminating [TLS](https://en.wikipedia.org/wiki/Transport_Layer_Security) and routing, with **[Caddy](https://caddyserver.com/) 2.6** behind it serving the dashboard
- **collectd** for the history behind most of these numbers, and the gauge, which counts the requests

[Tuning for Load](/tuning-for-load) explains, setting by setting, the kernel tuning that lets one machine carry this many short-lived connections: the connection-tracking table, socket buffers, the receive ring, and the TIME_WAIT cap. The [Aquatic guide](https://github.com/zootella/ftorrent/blob/master/open/README.md) covers the deployment.

## The requests

The day's requests, by protocol and address family:

| | IPv4 | IPv6 | Total | Share |
|---|---:|---:|---:|---:|
| UDP | 105,604,612 | 20,952,621 | 126,557,233 | 73.2% |
| HTTPS | 35,969,465 | 9,918,504 | 45,887,969 | 26.6% |
| WebSocket | 314,763 | 19,678 | 334,441 | 0.2% |
| **Total** | **141,888,840** | **30,890,803** | **172,779,643** | |
| Share | 82.1% | 17.9% | | |

UDP carried 73.2% of the requests, 2.76 for every HTTPS announce, and it's the lighter protocol. A [UDP announce](https://www.bittorrent.org/beps/bep_0015.html) is two small packets, a tiny question and a tiny answer, after a two-packet handshake that gives the client a connection ID it can reuse for a minute. An HTTPS announce asks the same question inside a TCP handshake, a TLS handshake, and HTTP framing: ten packets or so when the TLS session resumes, closer to twenty when it starts cold and the server sends its certificate. That's the price of an encrypted, private exchange.

The WebSocket row counts offers relayed between browsers. Each one is the tracker introducing two [WebTorrent](https://webtorrent.io/) clients so they can connect directly. Browser peers are the newest part of the network, and their share is still small.

IPv6 carried 17.9% of the day's requests. Most peers on IPv4 sit behind address translation, which makes direct connections harder. IPv6 gives every device [an address others can reach](/open-address-closed-device), and that's better for peer-to-peer.

## What the day cost

### Processor

On average the machine kept **3.0 of its 28 hardware threads busy**, 10.7% of the processor. The 1-minute load average ran 2.92 over the 24 hours and peaked at 6.67; over the week it averaged 2.99 and peaked at 6.83. The processor sat 89.3% idle while serving 173 million requests. 🍹⛱️

No single core became a bottleneck. The eight performance cores' primary threads ran 10–15% busy, their hyperthread siblings 4–6%, and the twelve efficiency cores 9–19%. [Softirq](/tuning-for-load#the-softirq-budget-and-the-counter-that-cries-wolf), the kernel's handling of network interrupts, ran at 0.4–4.4% on every core instead of piling onto one. That's Receive Side Scaling at work: on a card with a single queue, every packet's interrupt would land on one core, and that core would become the ceiling long before the others broke a sweat.

Where the processor time went:

<div class="breakout">

| | Per announce | Announces a second | Cores |
|---|---:|---:|---:|
| UDP, in aquatic_udp | 0.089 core-ms | 1,465 | 0.13 |
| HTTPS, in HAProxy and aquatic_http | 4.73 core-ms | 531 | 2.51 |
| The kernel and the rest, in no process | | | 0.35 |

</div>

HTTPS's 4.73 core-ms splits into 4.31 at HAProxy and 0.42 at aquatic_http. HAProxy also handles the WebSocket tracker, the dashboard and websites, and the cleartext announces it drops on port 80, so its figure is an upper bound for HTTPS. The 0.35 cores that belong to no process are the kernel's address rewriting, connection tracking, and packet handling, plus Docker and collectd. Most of UDP's path runs there, so UDP's in-process figure understates its full cost.

### Bandwidth and packets

The server averaged **19.5 Mbit/s in and 44.6 Mbit/s out** across the day. The busiest minute of the week ran 29.2 in and 71.7 out, 7.2% of a gigabit link. The tracker is sipping bandwidth. Outbound is more than twice inbound, because a list of peers is bigger than the announce that asks for it.

Packets ran 15,500 a second in and 12,800 out. Per request, UDP and HTTPS together, that's 1.2 KB in over 7.8 packets, and 2.8 KB out over 6.4. These counts cover all of the server's traffic, the DHT node and the websites included, so the per-request figures are upper bounds.

### Memory

The trackers, the DHT node, and both proxies fit in **1.1 GB**. Each container's figure is the kernel's own live count of the memory charged to its cgroup, the number the gauge publishes in `page.json`:

| | Memory |
|---|---:|
| Aquatic UDP | 402.4 MiB |
| Aquatic HTTP | 187.4 MiB |
| Aquatic WebSocket | 110.3 MiB |
| DHT node | 18.1 MiB |

HAProxy held another 341 MB resident, and Caddy 43 MB. Aquatic keeps each torrent's peers in a hash map, and the UDP and HTTP trackers drop a peer that hasn't announced in an hour, twice the half-hour announce interval. So memory grows with the number of peers present at once. Divided by the day's traffic, that's 3.18 MiB per million daily UDP requests and 4.08 MiB per million HTTPS requests.

### Connection tracking

Docker's bridge networking puts every inbound flow through the kernel's connection-tracking table, and a table too small for the load drops new flows without telling the application. With the table sized for this load, occupancy averaged **86,500** entries across the day and peaked at **113,800**. Over the week it averaged 88,600 and peaked at 123,800, 5.9% of the 2,097,152 entries the tuning allows.

The card dropped no packets. `rx_missed_errors`, its count of packets discarded before the kernel ever saw them, read **zero all week**.

### Power

A 10-second reading of Intel's [RAPL](https://www.kernel.org/doc/html/latest/power/powercap/powercap.html) counter put the processor package at **5.5 W**. The whole machine drew **40 W** at the wall. Memory, storage, the network card, fans, and the power supply's own losses make up the other 34.5 W.

That's a complete public tracker — processor, memory, storage, the entire Linux stack — answering 173 million requests a day on the glow of an incandescent reading lamp. 💡

## UDP and HTTPS, side by side

HTTPS is 26.6% of the requests and 95% of the processor time the tracker's own processes use. Per announce:

| | UDP | HTTPS |
|---|---:|---:|
| Share of requests | 73.2% | 26.6% |
| Packets | 2 to 4 | 10 to 20 |
| Processor, in process | 0.089 core-ms | 4.73 core-ms |
| Processor, all in | 0.3 core-ms | 5 core-ms |
| Memory per million daily requests | 3.18 MiB | 4.08 MiB |

That 95% is 2.51 cores against UDP's 0.13. Count most of the unattributed kernel time as UDP's, since its path runs mostly in the kernel, and a UDP announce costs 0.3 core-ms all in. An HTTPS announce costs around sixteen times that.

HAProxy accounts for 4.31 of HTTPS's 4.73 core-ms, and its main job here is TLS. The costly part of TLS is the handshake at the start of each new connection, whose public-key math costs far more than the tiny announce that follows. A client that keeps its connection open pays for one handshake across many announces. So the cost of HTTPS follows new connections more than requests, and the proxy decides how far HTTPS scales.

## Setting the ceilings

The tracker protects itself with a [circuit breaker](https://github.com/zootella/ftorrent/blob/master/open/breaker/README.md). The gauge keeps a rolling 24-hour count for each of six services, three protocols on two address families. When one crosses its ceiling, the breaker pauses that service for a day, and the other five keep serving. [`SERVICE_BREAKERS`](https://github.com/zootella/ftorrent/blob/master/open/gauge/src/gauge.py), in the gauge, sets the ceilings: **600 million** a day for each UDP service, and **200 million** for each HTTPS and WebSocket service.

We set them from the costs above, against one budget: if UDP and HTTPS both sat at their ceilings at the same moment, the machine should use half its processor, 14 of its 28 threads. Taking UDP at its all-in 0.30 core-ms, and rounding HTTPS up to 5.0 for its share of the kernel:

| | Ceiling a day | Per announce | Cores |
|---|---:|---:|---:|
| UDP | 600 million | 0.30 core-ms | 2.1 |
| HTTPS | 200 million | 5.0 core-ms | 11.6 |
| **Together** | | | **13.7 of 14** |

The budget assumes IPv6 and WebSocket stay small, so the IPv4 services reach their ceilings first. At that point bandwidth would be 20% of a gigabit, connection tracking 18% of its table, and TIME_WAIT 15% of its cap. The processor is the resource closest to its limit, and half of it is still free.

We gave each protocol's IPv4 and IPv6 services the same ceiling. IPv6 carries less traffic, so under real load the IPv4 service trips first while IPv6 keeps serving, which nudges clients toward IPv6. WebSocket gets the HTTPS ceiling because it spends the same TLS budget.

Against those ceilings, udp4's 105.6 million was 17.6% of its own and http4's 36.0 million was 18.0%: room for 5.7 and 5.6 times the traffic.

### Aquatic's worker threads

The half-processor budget treats the 28 threads as one pool. That's true of HAProxy, which spreads its TLS work across all of them. It isn't true of Aquatic's part of each announce. Aquatic's configuration sets how many worker threads it runs, and a thread can use only one core, however idle the others are.

With one socket worker, the thread that accepts each connection, parses the announce, and writes the answer, aquatic_http averaged 0.22 cores, 81% of it on that thread: 0.34 core-ms per HTTPS announce. At that cost one thread fills at 2,940 announces a second, 254 million a day across both address families, 5.5 times this traffic. At this mix, with IPv6 at 21.6% of HTTPS, that puts http4 at 199 million, just under its 200 million ceiling. The two limits meet by coincidence. With a little more IPv6 the thread would fill first: the served rate would level off below the ceiling, the breaker would never trip, and requests would slow down and time out at the proxy instead of pausing cleanly.

So the HTTP tracker's configuration sets [`socket_workers`](https://github.com/zootella/ftorrent/blob/master/open/docker/aquatic-http.toml) to 2. Each worker opens its own socket on the same port, and the kernel spreads connections across them. HAProxy opens a fresh connection to the tracker for each request, so the work splits evenly; we measured 50.5% and 49.5%. Two workers move the limit to 503 million HTTPS a day, where the busier one fills, well past the ceilings. The swarm worker, which holds the peer table, does a small share of each announce and is far from filling its core.

The UDP tracker doesn't need a second worker. Its whole cost is 0.089 core-ms per announce, so a single thread would fill at 971 million a day, well past its ceilings at this mix. For both trackers the rule is the same: when a ceiling goes up, add workers to match.

The day used three of the machine's 28 threads and 1.1 GB of memory, and a far more modest machine would carry it. The capable hardware buys years of growth, not a higher bar to entry.
