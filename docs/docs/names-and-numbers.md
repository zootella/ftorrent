---
title: Names and Numbers
description: Where the ftorrent desktop client connects on its own, and what it says about itself when it gets there — every server it reaches, every field it sends, who runs what, and where each is set.
---

# Names and Numbers

A peer-to-peer client still needs a few servers, and it introduces itself wherever it goes. This page is the audit of both. The first half lists every server the desktop client reaches without the user naming it: who runs each one, why it is on the list, and where in the code it is written. The second half lists what the client says about itself on the way: the names it carries and every field a tracker or a peer receives. If a server is not on this page, the client does not contact it on its own; if a field is not on this page, the client does not send it.

Three things are true throughout. Our own service comes first in every list, because we run public infrastructure and use it ourselves. Beside it stand either a well-known provider or the servers the BitTorrent world already treats as universal. And nothing is hidden: no analytics, no telemetry, no crash reporting, no fonts or scripts fetched from anyone. What the user adds, the trackers named in a torrent, the peers a swarm introduces, their own router, is theirs and is not listed here.

## Where the client connects

- One table in the engine's program, `desktop/engine/engine.py`, holds the three lists below, with a comment on every entry.
- The engine hands them to libtorrent when it creates its session, through libtorrent's own settings, and reports the whole table in its `ready` line, so the app can show it and anyone running the engine by hand sees the same list this page publishes.
- The two that belong to the installer and the updater rather than the engine are set in `desktop/src-tauri/tauri.conf.json`.

The engine creates no session yet, so today none of these is contacted. The table is written ahead of the code that uses it, on purpose, so the list is decided in the open before the first connection is made.

### STUN

WebTorrent connects browser peers over WebRTC, and WebRTC begins by asking a STUN server what public address the connection will appear from. libtorrent takes exactly one STUN server, so the first entry is the one it uses; the others are the order the engine will fall back through once it can test them, and the choices a settings page will offer.

| Server | Run by | Why it is here |
|---|---|---|
| `stun.ftorrent.com:3478` | ftorrent, on our own infrastructure | ours; we eat our own cooking |
| `stun.cloudflare.com:3478` | Cloudflare | a well-known provider with a public STUN service |
| `stun.l.google.com:19302` | Google | libtorrent's own default, kept as the last resort |

A STUN request carries no torrent and no content; it asks a server to reflect an address back.

### DHT bootstrap

A client with an empty routing table has to learn one node from somewhere, and the convention since BEP 5 is a short list of well-known nodes with high uptime. libtorrent takes the whole list. After the first start the client saves its routing table, and the bootstrap nodes are not needed again.

| Node | Run by | Why it is here |
|---|---|---|
| `dht.ftorrent.com:51420` | ftorrent | ours; a bootstrap node run as public infrastructure |
| `dht.libtorrent.org:25401` | libtorrent's author | libtorrent's own default |
| `dht.transmissionbt.com:6881` | the Transmission project | an open-source client project's node, long established |

Two nodes most clients also list are not here: `router.bittorrent.com` and `router.utorrent.com`, run by Rainberry, the company that was BitTorrent, Inc. and has been owned by the Tron Foundation since 2018. We prefer operators whose incentives sit with the network's users, and when we checked, neither answered.

### Trackers

These are added to a torrent that ftorrent creates, so a new torrent is reachable from its first minute. Whether they are also added to torrents the user brings is a decision for the add-torrent story, and this page will say what was decided.

| Tracker | Run by | Why it is here |
|---|---|---|
| `udp://open.ftorrent.com:443/announce` | ftorrent | ours, on all three protocols |
| `https://open.ftorrent.com/announce` | ftorrent | |
| `wss://open.ftorrent.com` | ftorrent | |
| `wss://tracker.webtorrent.dev` | the WebTorrent project | the reference implementation's own tracker, on Aquatic like ours |
| `wss://tracker.openwebtorrent.com` | OpenWebTorrent | the longest-running WebTorrent tracker, the default in the browser clients |
| `udp://tracker.opentrackr.org:1337/announce` | not publicly named | the most widely used open tracker, in nearly every client's list |

Four candidates did not make the list. `wss://tracker.btorrent.xyz` and `udp://explodie.org` did not answer when we checked, nor did `exodus.desync.com` or `tracker.openbittorrent.com`; `tracker.torrent.eu.org` and `open.stealth.si` answered well but their operators are not named anywhere we could find, and one such entry is enough.

### Inherited, and the platform's

**The Windows installer.** On a Windows machine without the WebView2 runtime, Tauri's installer fetches Microsoft's bootstrapper for it from Microsoft at install time, which is Tauri's default and is set by `webviewInstallMode` in the Tauri config. Windows 11 and updated Windows 10 already carry the runtime, so most installs fetch nothing. An offline mode that bundles the runtime exists and would make the installer far larger.

**The operating system's own.** Names are resolved by whatever DNS the user's system is configured to use; the client never chooses a resolver. HTTPS trackers are checked against the system's trust roots. The web view that draws the window is the platform's, WebKit on macOS and Linux and WebView2 on Windows, and it updates the way the platform updates it.

### Planned

The update check, when it lands, asks `ftorrent.com` for a version manifest on launch and once a day, and applies nothing without a click; a setting turns it off entirely, and the planning document describes it. Nothing else is planned to reach a server on its own.

## What the client says about itself

Everything in this half is a libtorrent setting or a protocol field, set in the same engine table as the lists above, and the engine reports the name it will use in its `ready` line.

### The three names

A BitTorrent client names itself in three places, and each reaches a different reader.

**The peer id** is twenty bytes at the front of every connection, and by the convention of [BEP 20](https://www.bittorrent.org/beps/bep_0020.html) it begins with a two-letter client code and a version, then random bytes. Clients that read only the peer id look the code up in a table. Ours is `-FF0100-`: `FF` is ftorrent's own code, and the digits are the version, one character per part, built by the engine from the version the app sends it, the same way libtorrent makes its own `-LT2110-`. The code is not yet in any table. There is no single registry; a code is registered by adding a line to each table that matters, the relationship-first way, with a shipping client behind the request, and until those lines land, a client that reads only the peer id shows the raw code and version rather than a name. We checked every table below before choosing, and `FF` appeared in none; `FT` belongs to an older client. The tables:

- [BEP 20](https://www.bittorrent.org/beps/bep_0020.html), the convention itself, with the original list
- [the community wiki's peer_id section](https://wiki.theory.org/BitTorrentSpecification#peer_id), the fullest list and the one people check first
- [libtorrent's table](https://github.com/arvidn/libtorrent/blob/RC_2_1/src/identify_client.cpp), which every libtorrent-based client inherits, qBittorrent and Deluge among them
- [Transmission's table](https://github.com/transmission/transmission/blob/main/libtransmission/clients.cc)
- [bittorrent-peerid](https://github.com/webtorrent/bittorrent-peerid), the table the WebTorrent clients use

**The handshake name** is a free-text field in the extension handshake, and libtorrent-based clients show it verbatim in their peer lists, ahead of the peer-id table. **The user agent** is the HTTP header sent to trackers and web seeds. Both say `ftorrent/0.1.0 libtorrent/2.1.1.0`: brand first, then lineage, the way a browser's user agent reads, so a narrow column shows the name and a wide one the whole truth. The engine builds that string when it starts, from the app's version, which the Rust core sends it in the first line of their conversation, and from the version libtorrent reports about itself, so neither number is written a second time anywhere.

### What a tracker receives

An HTTP announce is one request with these fields, and each says something about the sender:

| Field | What it carries |
|---|---|
| `info_hash` | which torrent, as a hash; the tracker never sees a name |
| `peer_id` | the client code and version, then random bytes for this session |
| `port` | where the client listens for peers |
| `uploaded`, `downloaded`, `left`, `corrupt` | how far along this torrent is, in bytes |
| `event` | started, stopped, or completed, when one of those just happened |
| `key` | a random value for this session, so the tracker can match a client whose address changed |
| `numwant` | how many peers the client would like back |
| `compact`, `no_peer_id`, `supportcrypto` | which reply format and features the client accepts |
| `redundant`, `trackerid`, `ip`, `ipv4`, `ipv6` | only when there is something to say: wasted bytes if that reporting is on, the tracker's own id if it gave one, an address only when the client is told to announce one or has one of the other family |

The tracker also sees the address the request came from, and the user agent header above. A UDP announce carries the same facts in one fixed binary packet, with no header and no user agent. A WebSocket announce, the WebTorrent form, carries the same fields as JSON plus one more thing: WebRTC offers, each naming the addresses the peer could be reached at, local ones included, because that is how WebRTC finds a path between two machines. The tracker relays offers and answers between peers and keeps neither. That exchange is inherent to WebTorrent and identical in every browser client.

### What a peer receives

The handshake carries the peer id above. The extension handshake that follows carries the handshake name, the extensions the client supports, its listen port, how many requests it will queue, whether it is only uploading, and `yourip`, the address the other peer appears to have, which is a courtesy every libtorrent-based client extends. On the DHT, a client's node id is twenty bytes derived from its own address and a random value, the BEP 42 rule that keeps ids from being chosen freely, and it is new on every start. Every DHT message also carries a version tag, `v`, and there the client is not ftorrent: libtorrent writes `LT` and its own version into that field itself, from no setting, so DHT nodes see a libtorrent client and nothing more specific.

## What answered when we checked

Checked 2026-Sep-22 from an Apple Silicon Mac, with the wheel the engine is frozen against. Each bootstrap node was tried alone from an empty routing table, and the count is nodes learned within twelve seconds; each tracker was announced to at once for a Creative Commons torrent.

```
dht.ftorrent.com:51420          10 nodes
dht.libtorrent.org:25401         9 nodes
dht.transmissionbt.com:6881      9 nodes
router.bittorrent.com:6881       0, no answer
router.utorrent.com:6881         0, no answer
dht.aelitis.com:6881             0, no answer

udp://open.ftorrent.com:443/announce         replied, 6 peers
https://open.ftorrent.com/announce           replied, 2 peers
wss://open.ftorrent.com                      replied
wss://tracker.webtorrent.dev                 replied
wss://tracker.openwebtorrent.com             replied
udp://tracker.opentrackr.org:1337/announce   replied, 200 peers
```

## Log

- **2026-Sep-22.** First record: the three server lists as written into the engine, ahead of the session that will use them.
- **2026-Sep-22, later the same day.** The page widened from the servers alone to the client's own names and the fields it sends; the handshake name and user agent set to `ftorrent/0.1.0 libtorrent/2.1.1.0`, the peer id left as libtorrent's.
- **2026-Sep-22, later still.** The peer id became ftorrent's own, `-FF0100-`, chosen after checking the four client tables and the community wiki for the code; registration in each is the next step, when the client has users.
