---
title: Good Service Planning
description: The order of work for good.ftorrent.com and the desktop client's Connection tab, from the page as it stands to a full account of what a connection can do.
---

# Good Service Planning

_Written 2026-10-09 at the start of the good sprint series, as the order of work for good's engineering spike: every measurement built, proven against real networks, and reported plainly in the two places a person meets good. What comes after, the articles, the visual design, the choosing and ordering of what a user sees, sits outside this plan's milestones on purpose. The map comes first, then the two places and the rule between them, then ten milestones of roughly an evening each, every one opening with its tasks as boxes, and last the two stages that follow them. We check the boxes as the work lands, and once a milestone is finished we rewrite the prose beneath its boxes from the plan into an introduction to what now exists: the new parts, how they work, what they're for, how to use them, and what they tell us about a connection._

## World Map

The finished system, every part labeled, with the milestone that brings each part in parentheses. Six places, and the roads between them. The user is whoever has the page open in a browser tab and the desktop client running with its Connection tab showing; while we build, that is us, the first user. Read this first, then watch the milestones load the parts in and connect them.

**By name.** Every domain and subdomain in the system, what runs at each, what it does, and where.

- `good.ftorrent.com` is the page: a Vue app on Vite, static files, served from our main server by Caddy behind HAProxy. It runs in the user's browser tab, fetches the reflectors, asks the STUN servers, meets moon's peers, and shows its verdicts.
- `ip.ftorrent.com` is the HTTP address reflector, go-ip, one static binary behind the proxy on our main server. It answers one line: the address the request came from, read from the last forwarded address.
- `ip4.ftorrent.com` is the same reflector under a name with only an A record, so a browser is forced to reach it over IPv4, which nothing else in a browser can do.
- `ip6.ftorrent.com` is the same reflector under a name with only a AAAA record, forcing IPv6 the same way.
- `stun.ftorrent.com` is coturn in stun-only mode on UDP 3478, both families, on our main server, published by Docker DNAT so it sees the real source and counted by firewall rules. It answers a binding request with the address and port it saw it arrive from.
- `open.ftorrent.com` is the tracker, Aquatic, over UDP, HTTP, and WebSocket, on our main server. It introduces peers, and the engine announces to it when it joins a swarm.
- `moon.ftorrent.com` is the second vantage: a small paid server at another place on the internet, built from cloud-init, the moon to the main server's sun. It runs coturn for a second STUN answer; a Go service behind Caddy for a second reflector, UDP pokes and TCP dials back to whoever asks, and a signaling room; two NAT namespaces we shape, with a pion peer and a UDP peer behind each, to punch through; and a host firewall with counters.
- `ip4.moon.ftorrent.com` and `ip6.moon.ftorrent.com` are moon's reflector under single-family names, one record each, so the page's cross-check can force each family exactly as it does against the main server.
- `stun.cloudflare.com` is Cloudflare's public STUN server: the second STUN answer until moon exists, then the fallback.
- `ipv4only.arpa` is a name defined to have only A records, at the root of the internet's naming. A AAAA answer for it reveals DNS64 and names the NAT64 prefix.
- `docs.ftorrent.com` is the documentation site, where this article lives, on our main server. Not part of good.
- `ftorrent.com` is the home page and the installers, on our main server. Not part of good.

The panel, the Rust commands, and the engine have no name of their own. They run on the user's machine, and the next section places them.

**By place.** The same parts again, laid out where they run.

**The user's page.** A browser tab navigated to `good.ftorrent.com`.

- The browser, running the page: a Vue app on Vite, static files, no server of its own (Good2). It fetches the reflectors over HTTPS, asks STUN servers over UDP through the browser's ICE, meets moon's signaling room over HTTPS, and opens WebRTC data channels to moon's peers (Good8).

**The user's panel.** The desktop client, running with its Connection tab showing, and the machine beneath it.

- The desktop client, ftorrent, with the panel as its Connection tab: ConnectionPage.vue and a module beside it holding the logic, a list of facts above and the running-diagnosis box below, with Copy under both (Good1).
- The client's Rust commands, general and named by operation: net_get and net_local, which exist today and grow a family (Good1); net_stun (Good2); net_interfaces and net_resolve (Good3); net_natpmp and the UPnP command on igd-next (Good5); net_punch (Good8); and process_run, which runs the platform's route, interface, and firewall tools so the panel can parse them (Good3, Good5).
- The engine, Python on libtorrent, one loop and no threads: session, poll, stats, add, and stop (Good4). It listens on both families, joins the DHT, announces to the tracker, and hands every alert and counter to the panel.
- The machine's own parts the panel reads: its interfaces and their addresses, its routing table and gateway, its system resolver, and its firewall, the alf preferences on macOS and netsh on Windows (Good3, Good5, changed with the user's say-so in Good9).

**The user's network.**

- The router: NAT and a firewall over IPv4, a firewall alone over IPv6. It answers NAT-PMP and PCP on port 5351 and UPnP over SSDP and SOAP (read in Good5, asked for mappings and pinholes in Good9).
- Sometimes a carrier's translator above the router, and sometimes DNS64 with a NAT64 translator; the panel detects both (Good3, Good5).

**Our main server, under `ftorrent.com`.**

- `good.ftorrent.com`: the page's static files, served by Caddy behind HAProxy, uploaded by rsync.
- `ip.ftorrent.com`, `ip4.ftorrent.com`, `ip6.ftorrent.com`: the HTTP address reflector, go-ip, one static binary behind the proxy, reading the last forwarded address. ip4 has only an A record and ip6 only a AAAA, which is how a browser is made to use one family.
- `stun.ftorrent.com`: coturn in stun-only mode on UDP 3478, both families, published by Docker DNAT so it sees the real source. Counted by firewall rules from Good6 and guarded by the same rule with a drop target once the counts say what normal looks like.
- `open.ftorrent.com`: the tracker, Aquatic. The engine announces to it when it joins a swarm (Good4).

**moon, at `moon.ftorrent.com`.** A small paid server at another place on the internet, entirely ours, built from cloud-init with its files in `good/moon/` (Good6).

- coturn in stun-only mode on 3478, both families: the second STUN vantage (Good6, used from Good7).
- The Go service behind Caddy over HTTPS, which only ever sends to the address a request came from: the reflector, the UDP poke, and the TCP dial with a BitTorrent handshake (Good6), and the signaling room (Good8), all rate-limited per source. The reflector also answers at `ip4.moon.ftorrent.com` and `ip6.moon.ftorrent.com`, one record each, so a browser can be made to use one family.
- Two network namespaces with source-NAT rules we write, one shaped like a home router and one symmetric (Good8). Behind each, a pion WebRTC peer for the page and a tiny UDP peer for the panel (Good8).
- The host firewall, with the STUN counters and the dry-run guard (Good6).

**Elsewhere.**

- `stun.cloudflare.com`: the second STUN vantage until moon exists, then the fallback (Good2).
- The DHT and the swarm of a Creative Commons torrent: strangers who find the engine through the tracker and the DHT and dial in (Good4).
- `ipv4only.arpa`: a name with only A records, whose AAAA answer reveals DNS64 (Good3).
- Let's Encrypt, for moon's certificate, and the provider, for the box itself (Good6).

**The roads.**

- From the page: HTTPS to the three reflectors and to moon's; UDP binding requests to `stun.ftorrent.com`, Cloudflare's, and moon's; HTTPS to moon's signaling room; WebRTC through the NATs to moon's two peers.
- From the panel: the same HTTPS and STUN roads from sockets it binds itself; UDP to the router on 5351 and multicast to 1900; reads of the machine; HTTPS to moon for pokes, dials, and the punch exchange; UDP straight to moon's UDP peers through the NATs; commands to the engine.
- From the engine: announces to `open.ftorrent.com` and the DHT; listening sockets on both families that moon and strangers dial.
- From moon, toward the user only: UDP pokes and TCP dials to the address a request came from.

```
the user                                              our main server, under ftorrent.com
  the page, in a browser tab ──────── HTTPS ─────────► good.          the page's files
  the panel, in the desktop client ── HTTPS ─────────► ip. ip4. ip6.  the reflector
     rust commands ────────────────── UDP ───────────► stun.          coturn
     the engine ───────────────────── TCP and UDP ───► open.          the tracker
         ▲ dialed by moon and by strangers
         │ through
  the router: NAT, firewall,                          moon, at moon.ftorrent.com
    NAT-PMP and PCP on 5351, UPnP ◄── reads, mappings   coturn          the second STUN
    (a carrier's NAT above it, sometimes)               Go service      reflector, poke, dial, signaling
                                                        two namespaces  cone and symmetric, each with
                                                                        a pion peer and a UDP peer
elsewhere: stun.cloudflare.com, the DHT and the swarm, ipv4only.arpa, Let's Encrypt, the provider
```

**What comes out.** The page's verdicts, the panel's facts and its box, and at the end of every run one self-describing document, each entry carrying its value, its method, and its vantage, labeled proven, inferred, or unknown (Good10).

## The two places, and the rule between them

A person meets good in two places. **The page** is `good.ftorrent.com`: a browser, a sandbox, and whatever can be learned between that sandbox and our servers. **The panel** is the Connection tab in the desktop client: a list of facts at the top, a text box that fills the window, and Copy underneath, so whatever the panel finds can be pasted into a message or an issue. The panel reaches the machine through Rust commands and reaches BitTorrent through the engine, so it can do everything the page can and a great deal the page cannot.

They are separate code. The page is a Vue app on Vite in `good/page/`, scaffolded like `open/page/` rather than on Nuxt, since one page with no routes wants the lighter scaffold, and built and uploaded the way the tracker's dashboard is; the panel is `desktop/src/pages/ConnectionPage.vue` with a module beside the client's other modules holding its logic. The two happen to use the same tools, so they will look alike and we carry ideas across freely, but no file, component, or package is shared between them, and nothing links the two builds. What they share is the servers they call and the contract of each server's answer. An earlier plan had one frontend serving both; it is withdrawn, and it is unlikely to come back. A desktop client built with web tools does not have to be the web page's code, and trying to make it so would make a mess of both. The deeper reason is that the two are opposites in what they ask of their web view: the page exists to use and test everything the browser's own connectivity allows, while the panel's web view is deliberately sandboxed, closed to foreign script, and reaches the network through Rust. And a browser-tier check run inside the client's web view would be testing the wrong browser, since what a user wants to know is whether their browser can be a peer, not whether WebKit inside ftorrent can.

The page stays plain static files with no server behind it, served by Caddy from the rsync target we own, so a new page goes up whenever we run the upload. The panel's Rust grows only general commands, named by operation and ignorant of good, per the lib.rs essay: a STUN binding to a server, the machine's interfaces, a name resolved, a packet sent to the gateway. The meaning of each answer is the panel's. The engine grows one dispatch entry per libtorrent call the panel needs, and the page polls it rather than the engine growing threads.

## How the milestones are ordered, and why

The order follows two instincts. First, the free work comes first: everything that runs on infrastructure already live on our server and on the development machine, because those evenings cost nothing but time and each one teaches something the later ones depend on. Second, moon sits in the middle rather than at either end. Standing up a second server before the clients exist would mean guessing at its API; leaving it to the end would stall the three milestones that need a second address. So moon arrives once the page and the panel have run every check a single vantage allows and know exactly what they want from a second one.

Mutation comes after observation throughout. Nothing changes the router or the firewall until the panel can already measure what the change would do, so every change has a before and an after. Synthesis comes last, once every raw measurement exists to synthesize.

Each evening starts by reading the prior art named in its section, since established tools have already met most of these questions. The best single piece of prior art for the panel as a whole is Tailscale's `netcheck`, which reports UDP, each family, whether the mapping varies, hairpinning, and which port-mapping protocols the router speaks, all in one run; nothing we build should be worse than it at the things it does. For the page the nearest prior art is the browser NAT checkers that ask two STUN servers and compare. For moon's active tests it is Transmission's port test, a server at the project's domain that tries to connect back to the port the client reports.

Every evening ends the same way: smoke test on the development machine, and from a second internet connection whenever the result depends on the network, since one connection teaches one connection's lesson; note what was learned, correct anything the evening proved wrong, check the milestone's boxes here, and once the milestone is finished, rewrite its prose as an introduction to what now exists, that same evening. When an evening changes the page or a service, the public guide in good/README.md is re-read whole and corrected the same night, with its last-reviewed date moved; the first such evening is Good2, which also rewrites the guide's what's-next section against this file and retires its paragraph on the reserved pion slot.

Through Good10 the work gathers raw material, and condensing it comes after. Run every check every time, show every result as it came back, in the box where Copy takes it whole, spot check that each is present and correct, and move on. A verdict is a small reading laid over the results, never a substitute for one: no logic stands one server's answer in for another's or hides which check answered, since a little condensing done early masks what happened and muddles the real condensing at the end. A check that depends on an earlier one's answer is fine where the investigation needs that order. Keep each fact apart from its reading, so a reading can change later without measuring again. When an evening meets a new kind of connection, write its signature into the milestone's section in the three parts the later stage speaks in: the facts, the likely reading, and what it means for the person. When there's room for one more measurement, choose the one whose answer would change what a person is told or can do. And in Good9, keep each router's or firewall's exact answer, since each is a case the finished feature has to handle.

## Good1: The page as it stands, and the panel's first facts

- [x] Good1a: Fetch `ip4.ftorrent.com` and `ip6.ftorrent.com` from the panel through net_get, time each, show both public addresses on the panel, and report a fetch that succeeds but takes seconds as present but unusable.
- [x] Good1b: Give net_local a family, so it binds a v6 socket toward a documentation address and answers the global v6 source address or that there is none.
- [x] Good1c: Make the text box the running diagnosis: a Run button, a header of date, version, and platform, one appended line per check with what it asked, what came back, and how long it took, and Copy taking the facts and the box together.

The page at `good.ftorrent.com` is one static file, as it was before this milestone: it fetches `ip4.ftorrent.com` and `ip6.ftorrent.com` at once and reports which families answered. The Connection tab in the desktop client asks the same questions and more, and it opens empty: nothing leaves the machine until the user presses Check.

A run asks two questions of each family, IPv4 and IPv6, and the table at the top sets the answers side by side: the address the machine sends from, and the address the internet sees.

The first question goes to the operating system. A machine holds several addresses at once: a private IPv4 address its router handed out, one IPv6 address or several, and a link-local address on every interface. For each connection, the system picks one as the source by looking up the route to the destination. The `net_local` command asks for that choice without sending anything: it connects a UDP socket toward an address beyond the local network, and connecting a UDP socket only settles its route and its source address, so the system answers and no packet leaves. The two addresses it connects toward, `192.0.2.1` and `2001:db8::1`, are ones RFC 5737 and RFC 3849 set aside for documentation, so they name no one, and every address past the router gets the same answer, the default route's.

The second question goes to a server. The reflectors at `ip4.ftorrent.com` and `ip6.ftorrent.com` answer with the address the request arrived from, and each name has an address record in one family only, so a request to it can travel over that family and no other. The forcing lives in DNS, which is why the page and the panel can both rely on it: neither can tell its network stack which family to use, and neither can reach a name over a family the name has no address in.

Side by side, the two answers show how the families differ. On IPv4 they differ: the machine holds a private address like `192.168.1.23`, and the router rewrites the source of every packet to the one public address its whole network shares, so a machine can learn its public IPv4 address only by asking something outside. On IPv6 they're usually the same address, since each device holds a globally routable address of its own and nothing translates it. The exception is the temporary address. Every consumer system makes a new IPv6 address for privacy about once a day and keeps the older ones for a while as deprecated, and in our measurement from a dual-stack connection, a Mac sent five of forty fetches from an address it had already deprecated, so the two IPv6 answers can differ with nothing translating between them.

The panel times each check, and we give each four seconds. A fetch over HTTPS is four round trips, one each for the name, the connection, the encryption, and the request, and eighty of them from a dual-stack connection took a median of 190 milliseconds. Four seconds leaves room for a satellite in geostationary orbit, near two and a half seconds a fetch, and for a lost packet sent again. So a check ends one of three ways: an answer, with its time; a failure, with its reason, like no route, which comes back in milliseconds; or no answer in four seconds, which only a path that silently drops what it's sent produces.

Beneath the table, the box keeps every run since the tab opened. Each run starts with when it ran, the client's version, and the platform, then a line per check with what it asked, how long it took, and what came back. Copy takes the table and the box together, so a whole report pastes into a message or an issue:

```
2026-10-17T16:00:00Z, ftorrent 0.1.0 on macOS
route to 192.0.2.1                5 ms  192.168.1.23
GET https://ip4.ftorrent.com/   245 ms  203.0.113.42
route to 2001:db8::1              5 ms  2001:db8:4a2c:100:8d3e:22f1:9b07:5c1e
GET https://ip6.ftorrent.com/   254 ms  2001:db8:4a2c:100:8d3e:22f1:9b07:5c1e
```

What these checks prove is that each family reaches a web server over TCP from this machine, and at which address the world sees it. They say nothing about UDP, which peer connections travel on, nothing about ports, and nothing about whether anyone outside can reach the machine; those are the questions the milestones after this one answer.

## Good2: STUN from both places

- [x] Good2a: Move the page onto Vite and Vue in `good/page/`, mirroring `open/page/`, with an upload script that rsyncs the build to the directory Caddy serves.
- [x] Good2b: Gather server-reflexive candidates on the page from our STUN server: create the peer connection, open a data channel to start gathering, and show the mapped v4 address and port and whether a v6 candidate arrives.
- [x] Good2c: Compare on the page, per family, the address STUN saw with the address the HTTP reflector saw, and name what a disagreement means.
- [x] Good2d: Add Cloudflare's STUN server to the same connection and derive the mapping verdict from the count of reflexive candidates per family, inconclusive when the machine has more than one interface in that family.
- [x] Good2e: Write net_stun in Rust: bind a UDP socket on a given port, send a binding request built by hand, parse the response by hand, and answer the mapped address, port, round trip, response size, or the error.
- [x] Good2f: Build the panel's mapping check: call net_stun against our server over v4 and v6, then against both servers from one local port, derive the mapping verdict, and show it with the HTTP-versus-STUN comparison on the panel.

Both places now learn their public address and port over UDP from `stun.ftorrent.com`, and both ask whether the router keeps one public port for every destination, the question hole punching turns on.

The page is a Vue app on Vite in `good/page/`, scaffolded like `open/page/`, and `pnpm upload` there builds it and ships it through `upload.js` at the repository root, the way every site goes up. On a computer it fills the window, with the facts and the verdict at the top and the box taking the rest of the height, and on a phone the facts stack, each name above its value, and the page scrolls. It runs as it opens: the two reflector fetches the page has made since July, and three WebRTC gatherings, all at once.

A page has no socket of its own, so it asks STUN through the browser, the way the WebRTC project's Trickle ICE sample does: it makes a peer connection naming STUN servers and asks for a data channel, which is what sets the browser gathering, and reads the candidates the browser hands back. A host candidate is a socket on this machine, and every mainstream browser hides its address behind a made-up name ending in `.local`, though not its port. A server-reflexive candidate is the public address and port a STUN server saw that socket's request arrive from, with its local address written `0.0.0.0` or `::` for the same reason. The first gathering names our server alone, and its reflexive candidates fill the table's public address over UDP for each family. An IPv6 reflexive candidate moves the IPv6 claim from "reaches a web server" to "carries UDP," which is the claim a hole punch needs.

The table sets the UDP address beside the one the reflector saw over HTTPS. They should agree, IPv4 by its address and IPv6 by its /64, since a machine sends from more than one address in its network. When they don't, the row gives both addresses, and a sentence under the verdict says what most likely put them apart: over IPv4, a carrier's translator with more than one public address of its own, and in either family a VPN, a proxy, or a relay such as iCloud Private Relay carrying one path and not the other.

The second gathering names Cloudflare's server alone, which shows it answers from here, and the third names ours and Cloudflare's together, so the browser asks both from the same sockets. A router keys each mapping on the inside address and port, so when both servers see the same public port, their two candidates are identical, and the browser, which drops a candidate that repeats one it already has, hands back one, its line naming whichever server answered first. A browser gathers from as many sockets in a family as it likes, one per interface and, in Safari, two or three on one address, and never says how many. But asked by one server, each socket answers once, so the first gathering's count is the number of sockets. When the third gathering's count is the same, every socket kept one public port for both servers: endpoint-independent mapping, the kind a hole punch works through. When it's double, each socket got a new port for the second server: endpoint-dependent, the symmetric case, where it fails. Any other count, or a gathering that heard nothing, reads unknown. Cloudflare's server is the second answer until moon's takes its place in Good7.

The box keeps everything the browser reports, since at this stage what one browser writes and another leaves out is itself a finding. Under each check's summary line, with its host candidates counted, it lists every gathering in full: each candidate line as the browser wrote it, when it arrived, and the server it came from where the browser says; each error a STUN server's silence raised; and the browser's own record of each local candidate, from getStats. The ICE username fragment, the `ufrag` in every line, is a random label each peer connection makes for itself, so it tells which gathering a line belongs to even in a paste cut short. Chrome writes `network-cost 999` on every candidate and `networkType=unknown` in getStats, declining to say whether a network is Wi-Fi or cellular, and Safari's getStats records carry fewer fields than Chrome's.

The panel asks the same questions with no browser in the way. `net_stun` takes a list of servers, a family, a local port, and a time limit, binds one UDP socket on that port, and sends every server a binding request at once, the way Tailscale's netcheck asks its servers, sending each again after half a second and then at doubling intervals until it answers or the time runs out. The request is twenty bytes built by hand, a header with STUN's magic cookie and a random transaction id, and the answer is parsed by hand, matched to its server by that id and read for its XOR-mapped address. That attribute can be decoded wrong and still yield a plausible address, so the test vectors STUN's authors published in RFC 5769 pin the decoding for both families. For each server the command answers the address and port it saw, the round trip, how many times it sent, and the size of the answer in bytes, which set against the request's twenty is the multiplier anyone reflecting traffic off a STUN server would get. Our server answers in 40 bytes over IPv4 and 52 over IPv6, and Cloudflare's in 32 and 44, coturn's answer carrying one more small attribute; with the IP and UDP headers every packet carries, ours comes to about 1.4 times the request on the wire over IPv4 and 1.5 over IPv6.

The panel calls `net_stun` once per family from a new port in the dynamic range each run, so a mapping the router made for the last run can't answer for this one, and the box shows a line for each server. The same public port at both servers is endpoint-independent mapping, with "port kept" beside it when that port is the one the machine chose, since then the router left it alone or nothing translates at all; different ports are endpoint-dependent. Asked from one socket it chose itself, with no counting of sockets, the panel's verdict is the authoritative form of the test. Its table compares our server's UDP address with HTTPS the same way the page's does. The socket is bound to no particular address, so over IPv6 the system picks the source for each packet, and on a Mac it sent STUN from the temporary address it had been using earlier that day while HTTPS went from the current one, the behavior Good1 found, which is why the comparison is of the /64.

Together the checks say whether each family carries UDP from here, at which public address and port, and whether the router keeps one port for every destination. They don't say whether a peer can reach this machine unasked, or how the router filters what arrives from someone it hasn't spoken to; those need a second vantage that sends to the machine, which is Good7's work, and the hole punch itself is Good8's.

We ran both places from a dual-stack connection, and the page in Safari from a phone on cellular data. On the dual-stack connection, both read endpoint-independent in both families, and its router kept the port the machine chose: the panel says so as "port kept," and the page shows it too, each IPv4 host candidate's port matching its reflexive one. On cellular, the carrier rewrites IPv4 ports, sends HTTPS and UDP out from two public IPv4 addresses in the same block, and still keeps one mapping per socket for every destination: in the third gathering every candidate named Cloudflare's server, ours having seen the same address and port and been dropped as a repeat, with no error to say ours went unheard. Over IPv6 on cellular, HTTPS arrived from a different network than UDP, from an address ending in `::8`, the shape of a server's address rather than a phone's, so something carried Safari's HTTPS that didn't carry its WebRTC: a proxy the carrier runs, or a relay such as iCloud Private Relay, which from inside the page look the same.

## Good3: What the machine knows about itself

- [ ] Good3a: Write net_interfaces in Rust to list every interface with every address and its up and kind flags, through if-addrs or the libc and windows crates.
- [ ] Good3b: Classify each address in the panel by the prefix table, and name the interface carrying the default route by matching the net_local answers against the list.
- [ ] Good3c: Read the gateway's address through process_run and each platform's route command, with the parsing in the panel.
- [ ] Good3d: Mark temporary v6 addresses apart from stable ones, reading the flag through process_run from ifconfig, the Windows network shell, or ip, and say which of the machine's addresses the reflector saw.
- [ ] Good3e: Write net_resolve in Rust to resolve a name through the system resolver and answer every address it returns.
- [ ] Good3f: Resolve `ipv4only.arpa` in the panel to expose DNS64 and the NAT64 prefix, and resolve the ip4 and ip6 names to show the single-family records in the box.
- [ ] Good3g: Add the shared-address-space check, counted only on the interface carrying the default route.

The panel alone, and a quiet evening: no server is involved. This is the small, almost purely educational feature, and it is where we finally answer the confusion about long colon-separated addresses.

A net_interfaces command lists every interface with every address on it, with the flags that say whether it is up and which kind of interface it is; the if-addrs crate does this on all three platforms, or the libc and windows crates the client already carries do it directly. The panel classifies each address by a prefix table that is spec rather than convention: link-local, unique local, Teredo, 6to4, documentation, global unicast on the v6 side; the three private ranges, shared address space, link-local, and public on the v4 side. It names the interface that carries the default route by matching the net_local answers against the list, and it reads the gateway's address through process_run and the platform's route command, since Rust has no portable way to ask and the parsing belongs to the panel. From that point the panel can say, for the first time, "this fe80 address is normal and local only, this fd address is private to your network, and the address the world sees you on is none of them." On the v6 side it also marks temporary addresses apart from stable ones, the RFC 8981 addresses every consumer OS makes and replaces about daily, which if-addrs does not expose, so the flag comes from the platform's own tools through process_run beside the gateway lookup: ifconfig on macOS, the network shell on Windows, ip on Linux. With that the panel can say which of the machine's addresses the reflector saw, that it is the temporary one, and that it will be a different one tomorrow, the fact that decides which v6 address a peer could ever be given.

A net_resolve command resolves a name through the system resolver and answers every address it returns. The panel resolves `ipv4only.arpa`, a name that has only A records by definition: a AAAA answer means the network synthesizes v6 for v4-only names, which is DNS64, and the prefix of that answer names the NAT64 translator. That turns a known false positive, a v6-only phone network passing the IPv4 check through its carrier's translator, into a labeled result. The panel also resolves the ip4 and ip6 names and shows the single-family records, so the trick the whole page rests on is visible in the box.

The shared-address-space check belongs here, qualified: an address in 100.64.0.0/10 counts as a carrier-grade NAT signal only on the interface carrying the default route, never on a tunnel interface, because mesh VPNs borrow that range for their virtual interfaces and the people running them are exactly our users.

## Good4: libtorrent wakes up

- [ ] Good4a: Give the engine a session command that creates `lt.session` from session_settings plus the diagnostic's choices: listen on both families at one port, DHT on, UPnP and NAT-PMP off, and the alert mask.
- [ ] Good4b: Give the engine poll and stats commands: poll pops the alerts that arrived and emits each as an event with its type, message, and fields; stats posts session stats and emits the counters as a dictionary.
- [ ] Good4c: Show on the panel the listen results per family and transport, the DHT's bootstrap state and node count, and the session's peer-voted external address.
- [ ] Good4d: Give the engine an add command, and join a well-seeded Creative Commons torrent in upload mode from the panel, counting incoming connections by alert and by the session stats gauge, with a Stop button that tears the session down.

The engine creates its first session, and the panel is the first place in the client that talks to it beyond hello. No download and no seed; the session exists to listen, to join the DHT, and to report what the network does to it.

Give the engine a session command that creates `lt.session` from session_settings plus the diagnostic's own choices: listen interfaces on both families at one port, DHT on, UPnP and NAT-PMP off for now, and an alert mask covering status, port mapping, DHT, peer, connect, and error. The engine stays a single loop with no threads: the panel sends a poll command on an interval, the engine pops the alerts that arrived and emits each as an event with its type, its message, and the fields that matter, and a stats command posts session stats and emits the counters as a dictionary. The panel shows the listen port and whether each socket bound (listen_succeeded_alert and listen_failed_alert, per family and per transport, since each port opens both a TCP and a UDP socket), the DHT's node count and whether bootstrap finished (dht_bootstrap_alert, dht_stats_alert), and the address the session believes is its external one (external_ip_alert, which is the majority vote of what peers and the DHT say they see, a third vantage on the public address beside HTTP and STUN).

Then the first swarm. An add command puts the session into a well-seeded Creative Commons torrent in upload mode, the flag that announces and accepts connections without downloading a byte, so strangers find us through the tracker and the DHT and dial in. The panel counts them: incoming_connection_alert for each, with its socket type, and the session stats gauge that says whether any incoming connection has ever arrived, which is the fact behind the little connectivity icon in every mainstream client. Without a port mapping the expected count on a home NAT is zero, and seeing zero, with everything else working, is the lesson of the evening. A Stop button tears the session down. Names of alerts and counters are confirmed against the libtorrent build the engine freezes, not this file, with session_stats_metrics listing every counter the build has.

This is also where the operating system first notices ftorrent listening. The macOS firewall ships turned off, so on most Macs nothing appears. With it on, its default of allowing signed software doesn't cover ftorrent, whose releases carry only the ad-hoc signature Apple Silicon requires, so the system asks whether the engine may accept incoming connections; the evening records what the prompt names, and whether the answer holds across an update, since an ad-hoc signature changes with every build. The Windows firewall ships turned on, so there the security alert appears, and a standard user who presses Cancel gets a block rule they never asked for. The evening records exactly what each OS did and did not do, because that is the ground Good5 stands on. Windows is checked on Windows.

## Good5: The firewall and the router, read-only

- [ ] Good5a: Read the macOS firewall: the global state from the alf preferences, and whether ftorrent and the engine are on the allow list from the command-line tool when it answers without sudo, unknown otherwise.
- [ ] Good5b: Read the Windows firewall through netsh show: each profile's state, the current network's profile, and whether a rule names the engine; report unknown on Linux.
- [ ] Good5c: Write net_natpmp in Rust: the two-byte public address request to the gateway on port 5351, answering the router's reported external address and epoch.
- [ ] Good5d: Ask the router over UPnP through igd-next, behind a command that takes the action name: GetExternalIPAddress and a walk of the existing port mappings.
- [ ] Good5e: Compute the carrier-grade NAT likelihood from the router's external address against the reflector's, combined with the shared-address-space signal and the mapping verdict, with its reasons listed.

Observation only. The panel reports the standing of the machine's firewall and asks the router what it knows, and changes nothing.

On macOS the global firewall state is a key in the alf preferences, readable without privilege, which is what Chromium reads on macOS 15 after Apple's command-line tool changed shape; whether ftorrent and its engine are on the allow list comes from that tool if it answers without sudo and reads as unknown if it does not. The firewall is off by default on macOS, and the panel says so when it is, since an off firewall is not a problem. On Windows the netsh show commands give each profile's state, which profile the current network is on, and whether a rule names the engine, all without elevation; adding one is Good9's business. Linux reports unknown, since ufw and nftables need root to read, and says so.

The router is asked two ways, both reads. NAT-PMP's public address request is two bytes to the gateway on port 5351, written by hand in a net_natpmp command that takes the gateway and answers the router's reported external address and its epoch. UPnP is heavier: multicast discovery, the device description, a SOAP call for GetExternalIPAddress and another to walk the existing port mappings; the igd-next crate carries that, behind a command that takes the action name, because SSDP and SOAP teach little beyond their existence. PCP is left for Good9, since it has no pure read of the external address and a short-lived mapping is still a mapping. The panel records which protocols the router speaks at all, which is itself a fact about that router.

The payoff is the carrier-grade NAT check that cannot be dodged: the router's own external address against the address the reflector saw. When they differ, a second translator sits above the home router, whatever address range the carrier chose. Combined with the shared-address-space signal from Good3 and the mapping verdict from Good2, the panel gives a likelihood with its reasons listed, with the note that address-plus-port sharing slips past all three beside it.

## Good6: moon

- [ ] Good6a: Write the public files in `good/moon/`: the cloud-init file, the compose file, the coturn configuration, the Go service's source, the README, and the deploy script.
- [x] Good6b: Make a key pair for moon alone in `.ssh`, named by an ssh config file in `good/moon/`.
- [x] Good6c: Choose the provider with the live prices in front of us, and create the box in its dashboard: the smallest plan on Ubuntu 24.04 with both address families, the cloud-init file as its user data, and moon's public key.
- [x] Good6d: Swap the three parked names in DNS, `moon.ftorrent.com` with both records and `ip4.moon.ftorrent.com` and `ip6.moon.ftorrent.com` with one each, to moon's real addresses, and have the same STUN counting rules on the main server by this evening at the latest, with a one-command read of the coturn container's network totals for a first number.
- [ ] Good6e: Put the STUN counting rules in moon's host firewall from the first evening: the no-target request and reply counters per family, the hashlimit dry-run rule per family, and the hourly cron line.
- [ ] Good6f: Bring up coturn in stun-only mode on moon, and Caddy with certificates for `moon.ftorrent.com` and the two single-family names, with the hardening block and unattended upgrades.
- [ ] Good6g: Write the Go service's reflector, the same contract as `ip.ftorrent.com`.
- [ ] Good6h: Write the Go service's UDP poke: a few small packets carrying a token to the requester's address at a port the requester names, from moon's own address and port.
- [ ] Good6i: Write the Go service's TCP dial: connect to the requester's address at the named port, optionally send a BitTorrent handshake, and report what happened; rate-limit all three calls per source in the Go service.

The second vantage, a small paid server at another place on the internet, entirely ours. This is the one milestone that is administration rather than measurement, and it is a new kind of work for this project.

We choose the provider on the evening, with the live prices in front of us, and nothing in moon depends on which one it is. What moon asks of a provider is short: static addresses in both families, ideally a routed IPv6 /64 so the namespaces in Good8 have room; a field on the create page for cloud-init user data and one for an SSH key; a web console, the virtual machine's own screen and keyboard, for the night a firewall rule locks out our SSH; a full virtual machine with its own kernel, KVM or Xen, so network namespaces and iptables behave as they do on real hardware; and outbound UDP left unfiltered, since sending packets is moon's job. Whichever it is, the box is Ubuntu Server 24.04 with Docker and Caddy from Ubuntu's own packages, the same builds the main server runs, so the two boxes differ in their place on the internet and not in their software. It is the same shape as the main server's good stack: coturn in stun-only mode on 3478 over both families with the same forty-line configuration, and one Go service on scratch behind Caddy, which terminates TLS with Let's Encrypt certificates it fetches itself for `moon.ftorrent.com` and the two single-family names. The containers run with the same hardening block, except that moon's Go service must originate packets, since originating packets is its job.

Everything that builds the box lives in `good/moon/` and is public: a cloud-init file that makes the box from nothing, the compose file, the coturn configuration, the Go source, a README in the usual shape for a reader with their own provider, and a deploy script. Nothing we run talks to the provider. Create the box once, in the provider's dashboard: paste the cloud-init file in as its user data and add moon's public key. From then on every step is SSH, so the workflow is the same at any provider, and the README tells a reader at another one what to fill in where. We make the key pair for moon alone, in the home directory's `.ssh` and named by an ssh config file in `good/moon/`, so the key that opens moon opens nothing else, and the README says in words where a reader puts theirs.

We administer moon directly over ssh: the deploy script, the compose commands, and whatever the evening needs on the box. moon runs nothing else, and if it burns down we rebuild it from cloud-init in minutes. It runs the hardening block, unattended upgrades, and a host firewall admitting only what its services need.

DNS goes early in the evening, since the certificate needs the name before anything else can be tested: moon's static v4 and v6 go into the zone under `moon.ftorrent.com`, replacing the documentation addresses parked there, with `ip4.moon.ftorrent.com` carrying only the A record and `ip6.moon.ftorrent.com` only the AAAA, the same single-family trick the page already rests on. All three names are parked ahead of time, so the evening's DNS step is an address swap and nothing new is named while the certificate waits. Nothing on the main server ever connects to moon.

The STUN port on both boxes starts being counted the same night. Anything answering on UDP 3478 over IPv4 is swept into reflector lists within days of coming up, announced or not, so the counting starts before anything else does. The instrumentation is small. Four firewall rules with no target in DOCKER-USER, where Docker's DNAT'd traffic actually passes, one for requests and one for replies per family, count packets and bytes and change nothing; a fifth per family carries the hashlimit match and no target, counting only packets above a per-source rate, so it climbs only when something is using the server as a reflector, a dry run of the guard; and a cron line appends the counter lines to a file hourly, so a burst shows as a time series rather than vanishing into a total. On moon the rules go in the host firewall script from the first evening and we read them ourselves. On the main server they go in ahead of this evening if they can and with the DNS swap at the latest, with a one-command read of the coturn container's cumulative network I/O for a first number the day they land. The guard itself, the same hashlimit rule with a drop target on both families, waits on a week of those numbers from both boxes and takes its threshold from them. We write the guide an operator section on all of it once the guard is running.

The Go service offers three things over HTTPS, and one rule governs all of them: moon only ever sends to the address a request came from, never to an address the request names, so it can never be pointed at anyone else. The first is the reflector, the same contract as `ip.ftorrent.com`, which gives both places a second HTTP vantage and a way to tell "our main server's v6 is down" from "the user has no v6," an embarrassing failure otherwise. The second is a UDP poke: on request, moon sends a few small packets carrying a token to the requester's address at a port the requester names, from moon's own address and a port of moon's choosing, and the requester reports whether they arrived. The third is a TCP dial: moon connects to the requester's address at the named port, optionally sends a BitTorrent handshake, and reports whether the connection opened and what came back. The Go service rate-limits all three per source address. The signaling room for the NAT peers is added in Good8.

## Good7: Two vantages

- [ ] Good7a: Replace Cloudflare's STUN with moon's as the page's second server, and fetch moon's reflector through `ip4.moon.ftorrent.com` and `ip6.moon.ftorrent.com` as a cross-check on the main server's.
- [ ] Good7b: Point the panel's second STUN server at moon.
- [ ] Good7c: Build the panel's filtering check: after the socket speaks to moon's STUN, ask moon to poke the mapped address from an unrelated port, and after it speaks only to the main server's STUN, from moon's address; sort the result into the three filtering kinds, over v4 and over v6.
- [ ] Good7d: Build the inbound proof: ask moon to poke the engine's listen port over UDP and dial it over TCP with a BitTorrent handshake, over v4 and v6, and watch the engine's alerts for what arrives.

With a second public address, the proofs that have been waiting.

On the page, moon's STUN server replaces Cloudflare's as the second entry, and the mapping verdict stands on our own infrastructure. The page also fetches moon's reflector over each family, through the two single-family names, as a cross-check on the main server's, so a failed v6 fetch to the main server with a successful one to moon is reported as our problem rather than the user's.

The panel runs the full behavior-discovery flow without needing RFC 5780 machinery on coturn, because moon can originate packets. Mapping: the same local port asks the main server's STUN and moon's STUN, and the two mapped ports agree or disagree. Filtering, from the same socket: the panel asks moon to poke the mapped address from an unrelated port after the socket has spoken to moon's STUN, and from moon's address after the socket has spoken only to the main server's, and which pokes arrive sorts the filtering into endpoint-independent, address-dependent, or address-and-port-dependent, the three cone subtypes. The same flow runs over v6, where mapping is free and only filtering remains, which is the shape worth showing: the v6 result is one question where the v4 result is two.

Then the proofs that matter most. With the engine's session listening and port mapping still off, the panel asks moon to poke the listen port over UDP and to dial it over TCP with a BitTorrent handshake, over v4 and over v6. The v4 attempts are expected to fail on any home NAT, and the panel watches the engine's alerts to see nothing arrive; the v6 attempts prove whether the router's firewall admits unsolicited inbound to a global address, which on many home routers it does not. An incoming_connection_alert that fires because moon dialed is the first time a stranger has reached this client on purpose, and it is the measurement Transmission's port test has offered its users for years, done with our own server and over both families.

## Good8: Peers behind our own NAT

- [ ] Good8a: Give moon two network namespaces with source-NAT rules we write: one with endpoint-independent mapping and port-restricted filtering, one symmetric.
- [ ] Good8b: Write a pion peer in Go and run one behind each namespace, gathering through moon's STUN and reporting which candidate pair carried the data channel.
- [ ] Good8c: Add a signaling room to moon's Go service over HTTPS.
- [ ] Good8d: Build the page's punch check: try both peers and report the outcome, the home-router peer only, neither, or both.
- [ ] Good8e: Write a tiny UDP peer in Go and run one behind each namespace.
- [ ] Good8f: Write net_punch in Rust: bind a socket, learn its mapped address from STUN, exchange addresses with the peer through moon, send both ways at once, and report which direction's packets arrived.

It is right to doubt that a WebRTC peer on a public address proves anything about hole punching: the user's own outbound packet opens the return path, so a public peer connects even to a user behind the worst NAT. The answer is to put the peer behind a NAT too, and since moon is ours, to build the NAT ourselves and shape it deliberately.

Give moon two network namespaces, each with a source-NAT rule we write rather than leave to Docker, whose masquerade may randomize ports and would make the test symmetric by accident; the first thing the evening checks is what Docker's rule actually does. One namespace translates like a home router, keeping the same public port regardless of destination and admitting replies only from the address and port already spoken to: endpoint-independent mapping with port-restricted filtering, the common case. The other picks a fresh port per destination: the symmetric case. Behind each sits a pion peer in Go, which gathers its reflexive candidate through moon's own STUN, exchanges descriptions with the user through a signaling room in moon's Go service over HTTPS, and opens a data channel, reporting which candidate pair carried it.

The page tries both peers. Connecting to the home-router peer and not the symmetric one is what endpoint-independent mapping with ordinary filtering looks like, and it is the hole punch demonstrated rather than inferred from STUN. Connecting to neither is the symmetric user. Connecting to both is a user whose NAT admits unsolicited inbound once any mapping exists, which is a thing the symmetric peer can teach that the first cannot. Each outcome also proves the user's browser can run WebRTC end to end, DTLS and SCTP included, which some networks and some privacy extensions prevent.

The panel does the same punch by hand, with no WebRTC: a tiny UDP peer in Go behind each namespace, and a net_punch command in Rust that binds a socket, learns its mapped address from STUN, hands it to moon over HTTPS, receives the peer's mapped address back, and then both sides send to each other at once and report which direction's packets arrived. That is the technique the How Can Two Peers Connect? essay describes, in about a hundred lines on each side, and watching it succeed through one NAT and fail through the other is the evening's whole purpose.

## Good9: Changing things, with the user's say-so

- [ ] Good9a: Turn libtorrent's two mappers on through the port_mapping setting, and show the portmap alerts' protocol and external port.
- [ ] Good9b: Send a hand-written NAT-PMP mapping request with a one-minute lifetime and delete it afterward.
- [ ] Good9c: Send a PCP MAP request with a one-minute lifetime and delete it afterward, recording whether the router speaks PCP at all.
- [ ] Good9d: Attempt a v6 pinhole over PCP and over UPnP's AddPinhole, by a hand-written request if igd-next does not carry it, expecting refusal and recording it.
- [ ] Good9e: Add the Windows firewall rule through an elevated netsh run the user approves, for an installed copy only.
- [ ] Good9f: On macOS report the firewall's standing and what the user can do by hand.
- [ ] Good9g: Re-ask the facts the OS can change behind our back when the window comes forward.

Now the panel changes the machine's standing, one thing at a time, and re-runs the proofs after each so every change shows its before and after.

The router first. libtorrent's two mappers turn on through the port_mapping setting the desktop planning doc describes, and the portmap alerts report which protocol won and what external port it gave; moon then dials and pokes the listen port again, and a connection that failed an evening ago succeeds, or does not, and the panel says which. Beside libtorrent's mappers, the panel's own hand-written NAT-PMP mapping request and a PCP MAP request, with lifetimes of a minute and an explicit delete afterward, show the two protocols on the wire and whether the router speaks the newer one; PCP is thought to live mostly in carrier equipment and enterprise gear, and the evening finds out what the router at hand says. The same evening tries the v6 side, where the question is a firewall pinhole rather than a mapping: a PCP MAP request over v6 to the router, and UPnP's IPv6 firewall control action, AddPinhole, from the IGDv2 service, by a SOAP request the panel writes by hand if igd-next does not carry it. NAT-PMP has no v6 at all. The expected result on a home router is refusal or silence, and the refusal is the finding: an earlier draft of the essay called the v6 firewall the simpler problem, and it is for punching and is not for listening, which the panel proves here before moon dials the listen port over v6 once more.

The firewall second. On Windows the panel adds the rule the desktop planning doc already specifies, through an elevated netsh run that the user approves, for an installed copy only, and re-runs the inbound proofs. On macOS the panel changes nothing, since there is no supported way to add an application to that firewall, and instead reports whether the firewall is on and whether it admits the engine, and when it doesn't, where the user makes the exception themselves; the proof re-runs after they do it. A Windows copy with no rule, a portable one or one whose user declined, gets the same report. The facts the OS can change behind our back are re-asked when the window comes forward, as planned.

Nothing here is required, and the panel keeps saying so: a user who declines runs outbound-only and the client keeps working, less well, with the panel showing exactly how much less.

## Good10: In concert

- [ ] Good10a: Label every result proven, inferred, or unknown.
- [ ] Good10b: Cross-check the HTTP address, the STUN address, the router's external address, libtorrent's peer-voted address, and the interface list against each other, and name what a disagreement exposes.
- [ ] Good10c: Apply the two axes, self-knowledge and reachability, as separate lines, and state one carrier-grade NAT likelihood with its reasons beside it.
- [ ] Good10d: Produce one self-describing document per run from both places, each entry carrying its value, its method, and its vantage, the page's a subset of the panel's.
- [ ] Good10e: Write the three-tier rating as one draft function over that document, not a screen.

The last evening builds nothing new on the wire and instead reasons across everything the others measured.

Label every result with what kind of claim it is: proven by a packet that arrived, inferred from a comparison, or unknown because the platform could not say. The cross-checks go in: the HTTP address, the STUN address, the router's reported external address, libtorrent's peer-voted external address, and the interface list, which between them expose a relay, a second translator, a translator the carrier runs, or nothing unusual. The two axes, self-knowledge and reachability, are applied explicitly so that "you have IPv6" and "a peer can reach you" stay separate lines, and NAT64, shared address space, mapping, and filtering feed one stated likelihood of carrier-grade NAT with its reasons beside it.

Both places produce the same thing at the end of a run: one self-describing document per run, each entry carrying its value, how it was measured, and from which vantage, with the page's document a subset of the panel's. A three-tier rating, kept as a candidate, is written as one function over that document and left at that, a draft rule set rather than a screen, since deciding whether and how to show a rating is the product phase's question. The spike ends with the dossier: everything we can honestly tell a person about their connection, from both places, with the basis for every line stated. The articles, the design, and the presentation start from there.

## After Good10: what it means, and what to do about it

Good10 ends this plan, and it does not end good. The ten milestones are the engineering spike: they build every measurement, prove each one against real networks, and report it plainly, until a run from the page or the panel produces a record of a connection that we can stand behind line by line. That record is the foundation, and on its own it is still a list of facts. Two stages follow it, in order. We will productize the page and the panel, so that each tells a person what their facts mean, and then give the panel the features that change a connection's standing, the firewall rule and the port mapping, made for the person with their say-so. The desktop client's shell is built, with installing, updating, and settling into each system done, so both stages sit on the near horizon rather than somewhere past it.

### What the facts mean

A person opens good to learn what their facts mean for them, and the first stage answers that: explaining what was measured, picturing it, sorting it into kinds a person recognizes, and documenting each kind, so that someone who runs good leaves understanding their own connection.

It speaks in three parts and never blurs them. The facts are certain, each one proven by a packet that arrived or read from the machine or the router: the router reports one external address while the reflector sees another, and two STUN servers asked from the same local port report two different mapped ports. The interpretation is named as one, in a few words, and stated as plainly as the evidence allows: a second translator sits above the router, which very likely means the provider runs carrier-grade NAT, and the path out gives each destination a fresh port, the symmetric kind. The outcome is what the person came for: over IPv4, no peer can open a connection to this machine and a hole punch will fail, so ftorrent reaches its peers by connecting out and downloads still work, while being reached directly rests on IPv6, which the same run answers on a line of its own. A fact never borrows the confidence of a reading, and a reading never hides the facts it stands on.

That stage is a body of work as large as this one. It needs a catalog of patterns, each a signature in the facts, the reading it most likely has, and what it changes for a person; kinds of connection a person can find themselves among, like a phone on cellular data, a home router, a VPN, or an office network; pictures that show a translator or a firewall rather than describing one; articles on docs.ftorrent.com that teach each pattern once, so the page and the panel link to the lesson instead of repeating it; and the choosing and ordering of what a person sees first, which is a product decision rather than a measurement. None of it belongs to this plan's milestones, and none of it can be done well until the facts beneath it exist and have held up on real networks.

### What a person can change

Understanding a connection is half of what good can offer, and the other half is changing it. Most of what stands between a machine and its peers can be adjusted, and much of it by software, once the person agrees.

The machine's own firewall comes first, and the first part of that feature is noticing it: ftorrent reads whether the system's firewall is on and whether it admits the engine, and when it doesn't, tells the person so and where to make the exception themselves. The macOS firewall ships turned off, so most Macs need nothing. With it on, it asks the first time the engine listens, since its allowance for signed software doesn't cover ftorrent, and it gives an app no supported way to add itself, so the panel points to the prompt or to System Settings and checks the result afterward. The Windows firewall ships turned on and shows its alert the first time the engine listens. An installed copy can carry a rule naming the engine, added through an elevated run the person approves, and a copy without one gets the same report a Mac does.

The router comes second, and a home router may speak any of three protocols for this. UPnP is the oldest and the most widely supported. NAT-PMP is Apple's lighter protocol. PCP, the Port Control Protocol, succeeded NAT-PMP and adds IPv6, and in principle it reaches a carrier's translator as well as the home router. Over IPv4, each asks the router to forward a port to this machine for a stated time. Over IPv6, where nothing translates, the same kind of request asks the router's firewall to open a pinhole instead, through PCP or through UPnP's IPv6 firewall control, a request Good9 expects most home routers to refuse.

Past what software can reach sit the steps only the person can take, which the panel explains and leaves to them: a port forward typed into the router's own settings page, IPv6 switched on in the router, or a public IPv4 address asked of the provider, which some providers offer in place of a shared one.

Good9 makes each software change once, for a minute, and undoes it, to prove the mechanism and measure its before and after. The features are the finished form. A mapping stays renewed while ftorrent runs and is released when it quits; a firewall rule lives with the installed copy and leaves with it; and the panel makes each change only after the person says yes, shows what it did, and undoes it on request. libtorrent already carries a UPnP mapper and a NAT-PMP mapper, so part of this stage is turning on what the engine has and reporting what each mapper did, in the same three parts as everything else good says.

The spike ends with everything we can honestly say about a connection. The stages after it say what that means to the person whose connection it is and then help them change it, and that is what good is for.
