---
title: Centralized Servers
description: Every server the ftorrent desktop client reaches on its own, who runs each one, why it is there, where it is set, and what answered when we checked.
---

# Centralized Servers

A peer-to-peer client still needs a few servers. A STUN server tells a peer its own public address so WebRTC can connect browsers. A DHT bootstrap node is where a fresh client knocks first to find the rest of the network. A tracker answers "who else has this?" for a torrent that names it. Every client has such a list, and most keep it quiet. This page is ours in full: every server the desktop client reaches without the user naming it, who runs it, why it is on the list, and where in the code it is written. If a server is not on this page, the client does not contact it on its own.

Three things are true of every list here. Our own service comes first, because we run public infrastructure and use it ourselves. Beside it stand either a well-known provider or the servers the BitTorrent world already treats as universal. And nothing is hidden: no analytics, no telemetry, no crash reporting, no fonts or scripts fetched from anyone. What the user adds, the trackers named in a torrent, the peers a swarm introduces, their own router, is theirs and is not listed here.

## Where they are set

- One table in the engine's program, `desktop/engine/engine.py`, holds the three lists below with a comment on every entry.
- The engine hands them to libtorrent when it creates its session, through libtorrent's own settings, and reports the whole table in its `ready` line, so the app can show it and anyone running the engine by hand sees the same list this page publishes.
- The two that belong to the installer and the updater rather than the engine are set in `desktop/src-tauri/tauri.conf.json`.

The engine creates no session yet, so today none of these is contacted; the table is written ahead of the code that uses it, on purpose, so the list is decided in the open before the first connection is made.

## STUN

WebTorrent connects browser peers over WebRTC, and WebRTC begins by asking a STUN server what public address the connection will appear from. libtorrent takes exactly one STUN server, so the first entry is the one it uses; the others are the order the engine will fall back through once it can test them, and the choices a settings page will offer.

| Server | Run by | Why it is here |
|---|---|---|
| `stun.ftorrent.com:3478` | ftorrent, on our own infrastructure | ours; we eat our own cooking |
| `stun.cloudflare.com:3478` | Cloudflare | a well-known provider with a public STUN service |
| `stun.l.google.com:19302` | Google | libtorrent's own default, kept as the last resort |

A STUN request carries no torrent and no content; it asks a server to reflect an address back.

## DHT bootstrap

A client with an empty routing table has to learn one node from somewhere, and the convention since BEP 5 is a short list of well-known nodes with high uptime. libtorrent takes the whole list. After the first start the client saves its routing table, and the bootstrap nodes are not needed again.

| Node | Run by | Why it is here |
|---|---|---|
| `dht.ftorrent.com:51420` | ftorrent | ours; a bootstrap node run as public infrastructure |
| `dht.libtorrent.org:25401` | libtorrent's author | libtorrent's own default |
| `dht.transmissionbt.com:6881` | the Transmission project | an open-source client project's node, long established |

Two nodes most clients also list are not here: `router.bittorrent.com` and `router.utorrent.com`, run by Rainberry, the company that was BitTorrent, Inc. and has been owned by the Tron Foundation since 2018. We prefer operators whose incentives sit with the network's users, and when we checked, neither answered.

## Trackers

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

## Inherited, and the platform's

**The client's name on the wire.** A BitTorrent client names itself in three places, and each reaches a different reader. The peer id carries an eight-character code at the front of every connection, and clients that read only that code look it up in a table; ours stays libtorrent's own, `-LT2110-`, so those readers see a current libtorrent, which is exactly what they are talking to, with every capability and convention that implies. The extension handshake carries a free-text name, which libtorrent-based clients show verbatim in their peer lists, and the HTTP user agent goes to trackers and web seeds. Both of those say `ftorrent/0.1.0 libtorrent/2.1.1.0`: brand first, then lineage, the way a browser's user agent reads, so a narrow column shows the name and a wide one the whole truth. The engine builds that string when it starts, from the app's version, which the Rust core sends it in the first line of their conversation, and from the version libtorrent reports about itself, so neither number is written a second time anywhere. A peer-id code of our own is something we may grow into, registered the relationship-first way in the tables the client projects keep; `FT` is already taken by an older client, so it would be another pair of letters.

**The Windows installer.** On a Windows machine without the WebView2 runtime, Tauri's installer fetches Microsoft's bootstrapper for it from Microsoft at install time, which is Tauri's default and is set by `webviewInstallMode` in the Tauri config. Windows 11 and updated Windows 10 already carry the runtime, so most installs fetch nothing. An offline mode that bundles the runtime exists and would make the installer far larger.

**The operating system's own.** Names are resolved by whatever DNS the user's system is configured to use; the client never chooses a resolver. HTTPS trackers are checked against the system's trust roots. The web view that draws the window is the platform's, WebKit on macOS and Linux and WebView2 on Windows, and it updates the way the platform updates it.

## Planned

The update check, when it lands, asks `ftorrent.com` for a version manifest on launch and once a day, and applies nothing without a click; a setting turns it off entirely, and the planning document describes it. Nothing else is planned to reach a server on its own.

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

- **2026-Sep-22.** First record: the three lists as written into the engine, ahead of the session that will use them.
