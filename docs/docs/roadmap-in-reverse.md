---
title: Roadmap in Reverse
description: A rear-view-mirror roadmap — the road behind ftorrent, told as a growing record of epics already completed.
---

# Roadmap in Reverse

A roadmap is a plan drawn at altitude: a high-level account of where a product is headed and roughly in what order it intends to get there. In Scrum and the agile tradition more broadly it isn't one of the ceremonial artifacts — not a backlog, not a sprint commitment — but the planning layer that sits above them, the bridge between a vision that rarely changes and a backlog that changes constantly. A team reads its roadmap and points the same direction: engineers lay groundwork for what's coming, stakeholders learn what to expect and when, and everyone gains a shared answer to why _this_ is being built before _that_. Modern roadmaps lean toward themes and horizons — now, next, later — rather than hard dates, precisely because the further out you look the fuzzier the road becomes. At its best a roadmap is a forecast offered in good faith, not a contract; it aligns people, frames trade-offs, and tells the story of a project moving deliberately toward something.

This project is going to do the opposite, and name it honestly while doing so. ftorrent is non-commercial, built by human and AI contributors as time and energy allow, with no quarter to hit and no one owed a delivery date — so a forward-looking roadmap would be a promise we have no standing to make. Instead we keep a roadmap in reverse: not the windshield but the rear-view mirror, a record of the road already behind us. Each entry is an epic completed, an initiative carried across the line, written down only once it's actually done. The conceit is tongue-in-cheek and we know it, but the inversion keeps most of what makes a roadmap worth having — it still chronicles a sustained, lengthy course of significant work, still shows direction, still gives contributors something to point at with some pride — while quietly reversing the usual hazards. It cannot over-promise, cannot slip, and cannot fall out of date, because it predicts nothing. It is, for once, a plan guaranteed to be correct: a roadmap whose every milestone has already been reached.

## 2010-Jul-14: Registered ftorrent.com domain

Secured ftorrent.com, choosing the project's name, and the products' brand and mark.
The domain was available in a search across the ASCII alphabet,
and nods to the market leader, without the Greek and Latin ambiguity around _µ_ or _u_.
We make our residency in the `.com` TLD, the foundational namespace of the web,
resonant with the turn-of-the-millennium energy of the dot-com bubble.
So what does the _f_ stand for? 
Fast, or Free?
Frictionless? Fearless?
Or perhaps _Formula One_, or Fuji 🗻.
We leave it unspecified.

## 2012-Oct-22: Native Windows client

The first ftorrent was a desktop program — a BitTorrent client for Windows, written in C++ on Rasterbar libtorrent.
It was built directly against the Win32 API in the [Petzold](https://www.charlespetzold.com/pw5/ProgWinEditions.html) tradition: hand-written window procedures, dialog resource scripts, and icons drawn by hand, down to the red, yellow, and green stage lights.
No interface framework sat between the code and the operating system.
More than four hundred commits across 2010 to 2012 carried it from a bare "start" to a working application.
The source still lives in this repository, in the `classic` workspace.

<div style="text-align: center; color: var(--vp-c-text-3); margin: 3.5rem 0;">— 🏜️ <em>thirteen years pass</em> 🌴 —</div>

## 2026-Apr-08: Public trackers at open.ftorrent.com

In April 2026 the project returned to the live web as public infrastructure: a BitTorrent and WebTorrent tracker, open to the decentralized internet.
The engine is [Aquatic](https://github.com/greatest-ape/aquatic), a tracker written in Rust, chosen for its speed — millions of requests per second on modest hardware — and because it speaks all three tracker protocols, including the WebSocket one that serves browser-based [WebTorrent](https://webtorrent.io/) and WebRTC peers.
It runs in hardened Docker containers — read-only, unprivileged, every Linux capability dropped — territory Aquatic's own documentation leaves to the operator.
The network footprint was kept deliberately quiet: the UDP tracker answers on port 443, indistinguishable from QUIC and unblockable without also breaking HTTP/3, rather than the conspicuous 6969 of tradition; the HTTP and WebSocket trackers share the standard 443 behind a reverse proxy; and the status page sits on the same subdomain, one domain and one certificate routed by path.
That status page is its own small work — a dashboard showing NASA's [Blue Marble](https://science.nasa.gov/earth/earth-observatory/blue-marble-next-generation/base-topography-bathymetry/) Earth turning in real time at one rotation per day, lit so the center of the globe is wherever the sun stands right now, the whole-planet view astronauts call the overview effect.
Below it, a ninety-day uptime history on a graphing-calculator LCD panel, and a quote of the day.
Two pieces of quiet math animate the page: the traffic counters advance from a Poisson process matched to each tracker's measured rate, so the digits move the way real arrivals do, and the daily quote is chosen by a [Squirrel3](https://www.youtube.com/watch?v=LWFzPP8ZbdU) noise hash of the calendar day, so every visitor each UTC day sees the same one with no database or server state.
Out of sight, a gauge container writes the page's data once a minute, and a circuit breaker meters each of the six services on its own, shedding load gracefully when one saturates — the deployment can grow under heavy traffic without ever taking the administrator out of control.
The whole data path stays small by design: the gauge writes a file, the reverse proxy serves it, and nothing runs in the request path but the proxy — a static page backed by honest, self-checking statistics, able to introduce tens of millions of peers from a single modest server.





