---
title: How the Mainline DHT Works
description: How BitTorrent's mainline DHT actually works — its history, the Kademlia concept beneath it, the real implementation, and how it's used.
---

# How the Mainline DHT Works

> **Stub — notes for a detailed document, to be researched and written over later passes (web + source reading).**

**Purpose.** Take a reader who knows software development and networking basics but is new to DHTs entirely, and bring them to an accurate, real-world understanding of how the mainline BitTorrent DHT actually operates — picking up the history and the underlying computer-science concepts along the way. High level, but complete and correct: not a toy model, the real thing, with the real numbers and the reasons behind them.

Four parts — **history, concept, implementation, use.**

## History
- The rendezvous / peer-discovery problem; trackers first, then the push to decentralize away from them.
- The two BitTorrent DHTs: the **mainline** DHT (from the original BitTorrent / µTorrent line) vs. the **Vuze/Azureus** DHT — both Kademlia-based, mutually incompatible; mainline won overwhelmingly.
- [Kademlia](https://en.wikipedia.org/wiki/Kademlia) — Maymounkov & Mazières, 2002 — the academic paper the whole thing is built on. Summarize it and link it.
- The specs: **BEP 5** (mainline DHT), then **BEP 32** (IPv6 / dual-stack), **BEP 44** (arbitrary signed/immutable key-value store), **BEP 51** (infohash indexing).
- Scale today (tens of millions of concurrent nodes — get a current figure).

## Concept (the Kademlia computer science)
- What a distributed hash table is, what problem it solves, and why "distributed" — contrast with a tracker's central table.
- 160-bit node IDs and info hashes sharing one address space.
- The **XOR metric**: distance = XOR of two IDs; why XOR (symmetric, a true metric, unidirectional) makes routing work.
- The routing table of **k-buckets**: buckets by shared-prefix distance, each holding up to *k* contacts, bucket splitting, least-recently-seen eviction.
- The four KRPC queries (bencoded over UDP): `ping`, `find_node`, `get_peers`, `announce_peer`.
- **Iterative lookup**: query *alpha* nodes in parallel, each step lands closer in XOR distance, why it converges and terminates in ~log(N) hops.
- **Write tokens** on `announce_peer` — what they are and how they stop spoofed/forged announces.
- Replication: a value lives on the *k* closest nodes to its key.

## Implementation (the real code and the real numbers)
- The reference implementation we actually run is **libtorrent-rasterbar** (also what qBittorrent and our [dht.ftorrent.com](https://github.com/zootella/ftorrent/blob/master/open/dht/README.md) bootstrap node use).
- Point at the actual source — libtorrent's `kademlia/` directory (`routing_table`, `node`, `dht_tracker`, `rpc_manager`, etc.). Fill in exact file paths + permalinks during research.
- Name the **presets** and, for each, say *why that value* (the trade-off it balances — responsiveness vs. churn vs. routing-table size vs. traffic):
  - *k* (bucket size) — the replication/redundancy factor (commonly 8).
  - *alpha* — lookup parallelism / concurrency (commonly 3).
  - ID space — 160 bits.
  - routing-table bucket **refresh interval**.
  - node liveness: ping cadence, when a contact is considered stale/evicted.
  - peer-storage **expiry** (how long an announced peer lives) and the client **re-announce interval**.
  - **token** validity window.
  - the **target peer count** returned in `get_peers` responses.
  - Gather libtorrent's real defaults for each and the rationale behind them.

## Use
- How a BitTorrent client uses it: trackerless magnet links, `get_peers` on an info hash, the DHT bit in the handshake + the `PORT` message, and the bootstrap priority (saved routing table → peers learned from trackers → hardcoded bootstrap nodes).
- How WebTorrent relates (browser peers lean on trackers/WebRTC; note what the DHT can and can't do for a tab).
- Our piece: `dht.ftorrent.com` as a public bootstrap node — see the [DHT node guide](https://github.com/zootella/ftorrent/blob/master/open/dht/README.md).
- Beyond torrents: BEP 44 turning the DHT into a general signed key-value store; the DHT as reusable rendezvous infrastructure.

**Research to do later:** exact libtorrent file paths + line references, the confirmed real default constants and the reasoning for each, the Kademlia paper specifics, and a current network-size figure.
