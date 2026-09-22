---
title: Roadmap in Reverse
description: A rear-view-mirror roadmap — the road behind ftorrent, told as a growing record of epics already completed.
---

# Roadmap in Reverse

A roadmap is a plan drawn at altitude: a high-level account of where a product is headed and roughly in what order it intends to get there. In Scrum and the agile tradition more broadly it isn't one of the ceremonial artifacts — not a backlog, not a sprint commitment — but the planning layer that sits above them, the bridge between a vision that rarely changes and a backlog that changes constantly. A team reads its roadmap and points the same direction: engineers lay groundwork for what's coming, stakeholders learn what to expect and when, and everyone gains a shared answer to why _this_ is being built before _that_. Modern roadmaps lean toward themes and horizons — now, next, later — rather than hard dates, precisely because the further out you look the fuzzier the road becomes. At its best a roadmap is a forecast offered in good faith, not a contract; it aligns people, frames trade-offs, and tells the story of a project moving deliberately toward something.

This project is going to do the opposite, and name it honestly while doing so. ftorrent is non-commercial, built by human and AI contributors as time and energy allow, with no quarter to hit and no one owed a delivery date — so a forward-looking roadmap would be a promise we have no standing to make. Instead we keep a roadmap in reverse: not the windshield but the rear-view mirror, a record of the road already behind us. Each entry is an epic completed, an initiative carried across the line, written down only once it's actually done. The conceit is tongue-in-cheek and we know it, but the inversion keeps most of what makes a roadmap worth having — it still chronicles a sustained, lengthy course of significant work, still shows direction, still gives contributors something to point at with some pride — while quietly reversing the usual hazards. It cannot over-promise, cannot slip, and cannot fall out of date, because it predicts nothing. It is, for once, a plan guaranteed to be correct: a roadmap whose every milestone has already been reached.

## 2010-Feb-25: First commit

The project's [first commit](https://github.com/zootella/ftorrent/commit/02a7a275b98f145dbd59c631c8b1f1086dd558f1), named simply `start`, laid down a bare Win32 scaffold on this date.

## 2010-Jul-14: Secured ftorrent.com

Registered the ftorrent.com domain, choosing the project's name, and the products' brand and mark.
The domain was available in a search across the ASCII alphabet,
and nods to the market leader, without the Greek and Latin ambiguity around _µ_ or _u_.
We make our residency in the `.com` TLD, the foundational namespace of the web,
resonant with the turn-of-the-millennium energy of the dot-com bubble.
So what does the _f_ stand for?
Something practical: Fast. Free.
Or more poetic: the virtues Frictionless; Fearless.
A storied name like _Formula One_, or Fuji 🗻.
Or foundational: Frontier. Firmament.
We leave it unspecified.

## 2012-Oct-22: Native Windows client

The first ftorrent was a desktop program — a BitTorrent client for Windows, written in C++ on Rasterbar libtorrent.
It was built directly against the Win32 API in the [Petzold](https://www.charlespetzold.com/pw5/ProgWinEditions.html) tradition: hand-written window procedures, dialog resource scripts, and icons drawn by hand, down to the red, yellow, and green stage lights.
No interface framework sat between the code and the operating system.
More than four hundred commits across 2010 to 2012 carried it from a bare "start" to a working application.
The source still lives in this repository, in the `classic` workspace.

<div style="text-align: center; color: var(--vp-c-text-3); margin: 3.5rem 0;">— 🏜️ <em>thirteen years pass</em> 🌴 —</div>

## 2026-Mar-31: Monorepo and return

After more than a decade in the shed, the project came back — and rather than a single program, it was rebuilt as a [pnpm](https://pnpm.io/) monorepo: one repository, many workspaces, each a distinct but related tool or guide that can be improved on its own, in the open, shipped early and often.
The original Windows client was preserved in place as the `classic` workspace, a working record of the C++/Win32 era kept rather than discarded.
The `open` workspace holds the public tracker — the hardened Aquatic deployment and the guide that documents it.
And `docs` is this documentation site, a VitePress build at docs.ftorrent.com meant to grow over time into a repository of documentation, essays, brainstorming, planning, and the occasional rant.
More workspaces stand ready as the project widens — a project homepage, an IPv4/IPv6 connectivity diagnostic, a cross-platform desktop client — each a separate piece of one effort: better tools and clearer guides for peer-to-peer and the decentralized web.

## 2026-Apr-08: Dockerized Aquatic

The engine is [Aquatic](https://github.com/greatest-ape/aquatic), a tracker written in Rust.
We surveyed the field — [opentracker](https://erdgeist.org/arts/software/opentracker/), the C implementation behind most of the internet's large public trackers, among them — and chose Aquatic for three reasons: it speaks all three tracker protocols as separate binaries (UDP for desktop clients, HTTP for clients behind restrictive firewalls, and the WebSocket protocol that serves browser-based [WebTorrent](https://webtorrent.io/) and WebRTC peers), it's fast — benchmarked at millions of requests per second on commodity hardware — and Rust's memory safety removes whole classes of vulnerability from software that accepts raw packets from the entire internet.

Aquatic was built for bare metal — CPU pinning, native SIMD, io_uring — and ships no Dockerfile.
We put it in hardened Docker containers anyway, trading a performance cost that is unmeasurable at any scale a 1 Gbps link can carry for real security isolation: each tracker runs read-only, as nobody, with every Linux capability dropped, no-new-privileges set, and outbound traffic blocked at the DOCKER-USER firewall chain, so a compromised process can do nothing but answer the requests it is handed.
Two obstacles stood between Aquatic and a clean containerization, both left to the operator by Aquatic's own documentation.
[glommio](https://github.com/DataDog/glommio), the async runtime under the HTTP and WebSocket trackers, requires io_uring with no fallback, and Docker's default seccomp profile blocks it; we ship a minimal custom profile that adds exactly three syscalls and changes nothing else.
And Docker's bridge networking leans on the kernel's connection-tracking table, whose defaults silently drop packets under a tracker's load; a tuned set of sysctls — a larger conntrack table, shorter UDP timeouts, deeper socket buffers and receive backlog — keeps the path clean under continuous global load.
The network footprint was kept deliberately quiet: the UDP tracker answers on port 443, indistinguishable from QUIC and unblockable without also breaking HTTP/3, rather than the conspicuous 6969 of tradition; the HTTP and WebSocket trackers share the standard 443 behind a reverse proxy; and the whole stack is dual-stack, because IPv6's NAT-free addressing is exactly what peer-to-peer wants.

## 2026-Apr-12: Tracker page

With the trackers answering, the project gave them a face: a status page on the same subdomain, one domain and one certificate routed by path.
At its center, NASA's [Blue Marble](https://science.nasa.gov/earth/earth-observatory/blue-marble-next-generation/base-topography-bathymetry/) — the cloudless, public-domain satellite Earth — turns on a [three.js](https://threejs.org/) sphere at the planet's real rate of one full rotation per day, lit so the center of the globe is always wherever the sun stands at that moment: solar noon, live.
There is no grabbing it, no zoom, no pins or labels; the texture swaps each month so the season on screen is the season outside, and to see the far side you wait twelve hours.
What we're after is the whole-planet view astronauts call the overview effect — an Earth with weight and rigidity, the opposite of a spinnable virtual globe.

Below it sits a graphing-calculator LCD panel: six live traffic counters and a ninety-day uptime history.
The counters are honest about being a simulation.
Rather than stream live counts to every visitor, each one starts from the tracker's real recorded 24-hour total and then advances from a Poisson process matched to that tracker's measured rate, so the digits move — bursts, gaps, the runs and held breaths — the way independent arrivals actually do, and snap back to the recorded number every ten minutes so a long-open tab never drifts.
At the foot of the page, a quote of the day, chosen by a [Squirrel3](https://www.youtube.com/watch?v=LWFzPP8ZbdU) noise hash of the calendar day, so every visitor anywhere on earth sees the same one with no database, cache, or server state — the shared fact everyone agrees on is the date itself.
The quotes come from a curated research pull spanning antiquity to the present — philosophers, inventors, activists, scientists, legal texts, and sacred traditions from every inhabited continent — on the themes the project is built around: free expression, shared knowledge, open networks, and the interconnection of humanity.
The page's type is set in [Monaspace](https://monaspace.githubnext.com/), GitHub's monospaced superfamily — Krypton's sharp, mechanical digits for the LCD counters, Radon's hand-drawn feel for the announce URLs.
The page itself is static files behind the reverse proxy; nothing runs at request time but the proxy handing back pre-rendered HTML and a small JSON file.

## 2026-Apr-30: Gauge and breaker

Two unseen components keep the page honest and the server standing.
The gauge is a small Python container that runs beside the trackers: once a minute it scrapes each tracker's Prometheus metrics, reads memory straight from the kernel's cgroup files, and writes the single [`page.json`](https://open.ftorrent.com/page.json) the page renders.
It has no HTTP server and no open ports — it only writes a file.
A ring buffer of 1,440 slots, one per minute of the day, lets it report a true 24-hour total as the proven difference between two recorded moments, stepping cleanly over gaps and over the counter resets a container restart produces.
And because the gauge cannot tell on its own whether the server is reachable from the outside, a companion probe runs on the host — pinging Cloudflare and Google over both IPv4 and IPv6 each minute — so the uptime figure separates "the gauge was down" from "the internet was down."

The circuit breaker is the deployment's graceful load shedding.
It meters each of the six services independently — three protocols times two IP versions — so a saturated UDP-over-IPv4 trips on its own while UDP-over-IPv6 keeps serving, nudging clients toward whatever path still works rather than refusing everyone equally.
The gauge decides and writes its intent — six timestamps saying when each service may run again; the breaker reads that file and reconciles reality to match, returning fast 503s at the reverse proxy for HTTP and WebSocket and dropping UDP in iptables `raw PREROUTING`, before the kernel even opens a connection-tracking entry.
It keeps no state of its own: every minute it compares what the file asks for against what is actually in effect and converges, idempotently.
The two layers stay independent — the gauge writes a file, the breaker enforces it — and the whole data path stays small by design: a static page backed by honest, self-checking statistics, able to introduce tens of millions of peers from a single modest server.

## 2026-May-23: Software and hardware upgrade

We had been running [nginx](https://nginx.org/) for everything — TLS termination, reverse-proxying to the tracker containers, static hosting, and the breaker's per-service gating — with [certbot](https://certbot.eff.org/) for certificates.
We migrated to [HAProxy](https://www.haproxy.org/) fronting [Caddy](https://caddyserver.com/), with [lego](https://go-acme.github.io/lego/) for ACME.
HAProxy terminates TLS more efficiently, which matters because TLS is the most expensive work the box does — a fresh handshake on every new connection — and that cost is unavoidable even while shedding load, since answering a paused service with a fast 503 still means completing the handshake first.
It also exposes a runtime admin socket that fits the breaker exactly: per-service toggles push to the live process every minute with no config reload.
Caddy serves the static dashboard and [`page.json`](https://open.ftorrent.com/page.json) that HAProxy, not a web server, can't, and lego replaced certbot because HAProxy's own ACME support is still behind an experimental flag.
While we removed nginx from the server, we kept it in the repository and documentation, generalizing the guide into a framework-agnostic pattern with both stacks as worked examples.

On the hardware, we moved from an Intel Core i5-8500 (Coffee Lake, 2018) to an Intel Core i7-14700 (Raptor Lake Refresh), about six years and several microarchitecture generations newer: 6 cores and 6 threads became 20 cores and 28 threads — 8 hyperthreaded performance cores plus 12 efficiency cores — with L3 cache up from 9 MB to 33 MB, peak turbo from 4.1 to 5.4 GHz, and single-box throughput up roughly five- to six-fold.
Yet it's still a mainstream 65 W desktop chip, not a server part — a generational leap, not an exotic one, well within reach of a modest build.
Networking hardware went from a single 1-Gigabit port to a dual-port 10-Gigabit card (Intel X550-T2).
The old single-queue controller that funneled every packet's interrupt onto one core gave way to as many as 28 receive and 28 transmit queues with [Receive Side Scaling](https://www.intel.com/content/www/us/en/support/articles/000006703/ethernet-products.html), which hashes connections across queues so packet processing spreads over all 20 cores instead of piling onto one.
That removes the single-core network ceiling, but it's headroom rather than a requirement: at today's load of about 77 million requests a day the tracker pushes only 10.1 Mbps inbound and 18.5 Mbps outbound — the outbound side under 2% of a single gigabit line — so a plain 1 GbE box is more than enough to fork this setup and run it.

## 2026-Jun-11: DHT bootstrap node

A tracker answers "who else has this?" from a server; the [mainline DHT](https://en.wikipedia.org/wiki/Mainline_DHT) answers the same question from no server at all — a Kademlia network spread across millions of participating clients, the machinery behind every trackerless magnet link. But the DHT has a cold-start problem: a fresh client knows the protocol and not a single peer, so the whole network leans on a short, hardcoded list of well-known **bootstrap nodes**: ordinary high-uptime nodes that simply happen to be where everyone is told to knock first. It is a small and quietly load-bearing list, and ftorrent now adds one more independent entry to it: `dht.ftorrent.com` runs [qBittorrent-nox](https://www.qbittorrent.org/) on [libtorrent](https://www.libtorrent.org/) as a pure DHT citizen — no torrents, no seeding, just a well-connected node that helps newcomers find their place — hardened like the trackers beside it and folded into the same dashboard as a live count of the nodes in its routing table. The contribution isn't the node, it's the diversity: bootstrapping is one of the last places the decentralized web still funnels through a handful of named hosts, and every independent one, owned by no single client project, makes that first step more resilient for everyone. The quiet irony is the best part — a new bootstrap node joins the network the only way anyone can, by bootstrapping off the ones already there, and every node on that list was itself, once, seeded by the others.

## 2026-Jun-18: Home page scaffolded to be modern and private

We scaffolded the home page at the apex of [ftorrent.com](https://ftorrent.com/) choosing [Nuxt](https://nuxt.com/), a modern, full-stack JavaScript framework, and then turned the full stack off. With `ssr: false` set, the build emits a static front-end bundle and nothing else: one `index.html` shell and its hashed JavaScript. Routes resolve in the browser, and the web server's job collapses to one rule any static host can follow — serve a real file if it exists, otherwise hand back the shell and let the page sort out the route. We keep the framework's conveniences — including access to the entire front-end npm ecosystem — and pay for no runtime at all: the server does nothing but deliver static files.

This construction opens a third part of the URL: the fragment, the part after the `#`. You already know it from Wikipedia — the `#History` in a link that jumps you to that section. By [RFC 3986](https://www.rfc-editor.org/rfc/rfc3986#section-3.5), the fragment is the browser's to handle, so the browser splits it off before sending the server its request. That gives the page a place to put a payload — peer-to-peer information, like a magnet link, with uses to grow into: a friendlier landing for it, an inspector that takes it apart, even an in-browser client. Today, _ftorrent.com_ gained a modern web application framework ready and able to give visitors rich peer-to-peer experiences in the page itself, all delivered from a simple and stateless server.

## 2026-Jul-16: Connection checker _good.ftorrent.com_

Most people think about their internet connection only when it breaks — _Is the wifi down? Is the internet slow?_ — and turn to familiar sites to check. Netflix's [fast.com](https://fast.com) reports your speed for streaming. [ipchicken.com](https://ipchicken.com) is the classic for your public IP address. [ipv6test.google.com](https://ipv6test.google.com/) asks _Ready for the future of the internet?_ but then surrenders immediately when the answer is no, giving an IPv4-only connection a green checkmark with the label "no problems detected."

[good.ftorrent.com](https://good.ftorrent.com/) is dedicated to a single, opinionated question: Is this connection **good**? Good, meaning capable of peer-to-peer — ultimately, for digital citizenship. Does this connection let me be **both a speaker and a listener** — reachable directly by another computer, or is it merely able to fetch from a server? That is what the modern internet quietly stopped guaranteeing — the mark of full participation in the network rather than the second-class standing of a connection that can only consume.

Under the hood are three reflectors, each simply handing back the address your request arrived from. `ip.ftorrent.com` answers over whichever family you reach it on, while `ip4.ftorrent.com` and `ip6.ftorrent.com` carry single-family DNS records that force IPv4 or IPv6. The page calls that pair at once, and which of the two answers gives your dual-stack verdict. All three are public primitives in the [ipify](https://www.ipify.org) tradition, open to any caller: a stateless echo that reads your address and hands it straight back. This first version is our foundation, reporting the address families you have. From here we'll add whether your NAT will let a peer punch through, and whether your browser can hold a WebRTC connection.

## 2026-Jul-20: Public STUN server _stun.ftorrent.com_

STUN, from [RFC 8489](https://datatracker.ietf.org/doc/html/rfc8489), is a UDP exchange that tells a client the public IP address and port its request arrived from. We run one at `stun.ftorrent.com` as part of [good.ftorrent.com](https://good.ftorrent.com) — its workspace, guide, and diagnostic page — where our `ip.ftorrent.com` reflector already reports the address but, being TCP, can't give the port or test that UDP gets through. STUN adds both.

The server runs [coturn](https://github.com/coturn/coturn), the reference implementation, on the standard port 3478. Our short configuration file mostly turns things off, with `stun-only` refusing TURN relaying and a handful of `no-` flags closing listeners we don't need. _stun.ftorrent.com_ joins our other public services as a small contribution to the decentralized web, and using it is as simple as adding `stun.ftorrent.com` to your list of STUN servers.

## 2026-Aug-25: Desktop client scaffolded

ftorrent began as a desktop program, and today it has one again. The `desktop` workspace became a running application — [Tauri 2](https://tauri.app/) with a [Vue](https://vuejs.org/) front end over a Rust core — built and launched from fresh clones on both macOS and Windows, with Linux beside them. Where the classic client was written directly against the Win32 API and ran on one operating system, this one is cross-platform from its first commit. It downloads nothing yet, and that is the plan: we are building it from the outside in, so installation, updates, and the window itself come before the engine, which arrives last onto a foundation already settled.

## 2026-Sep-22: libtorrent in the desktop client

We put [libtorrent](https://www.libtorrent.org/) into the desktop client scaffold today, without needing to build it from source. libtorrent 2.1 shipped in July with WebTorrent on by default, and its maintainers publish wheels for every platform we ship from a workflow in libtorrent's own repository, so we know who built each one and how. We pin each platform's wheel by SHA-256 in a lockfile, and every build machine fetches the same bytes and verifies them before it builds.

ftorrent will run libtorrent in a second process, the engine: a small Python program, since libtorrent's only maintained bindings are Python, frozen with its interpreter into a folder the app carries beside itself. The app starts the engine as its own child and talks to it over pipes, one line of JSON per message, rather than over a local network port. A pipe between a parent and its child has no address, so no other process on the machine can reach the conversation. We built and ran it on macOS and on Windows.

Linux ships four packages from a pipeline that runs in Docker on the Mac, with no Linux computer involved. One Dockerfile builds a toolchain image per architecture, arm64 native on Apple Silicon and x86-64 emulated, and a container made from it lives for exactly one build: it takes a read-only copy of the source, freezes the engine on Linux from the same lockfile, compiles the app, and writes a `.deb`, and on x86-64 an `.rpm` from the same compile. A third container wraps that `.deb` into a Flatpak, the one package that installs on SteamOS and Bazzite, whose root filesystems cannot take a `.deb` or an `.rpm`. Then bare Debian 12 and Fedora 40 containers, with nothing installed, unpack each package and run the engine from where the package put it. We build on a Debian 12 base for its glibc, which sets the floor on who can run the result: Debian 12 and 13, Ubuntu 24.04 LTS and 26.04, Mint 22, Fedora 40 and up, and both current generations of Raspberry Pi OS.

The engine creates no session yet. But it holds BitTorrent and WebTorrent together from the start, so when it does, a browser tab and a PC will share one swarm.
