---
title: How Can Two Peers Connect?
description: A survey of the obstacles, protocols, libraries, and public services that let two ordinary machines on the internet reach each other directly.
---

# How Can Two Peers Connect?

The internet was designed so that any machine could reach any other. Most machines can't.

This is a survey of what stands in the way and what people built to get around it — the protocols, the libraries that implement them, the public services anyone can call, and the choices a few real projects made when they hit this problem. It is mostly about other people's work, and it describes the state of things in 2026.

## Why it matters

Somewhere in the last three decades the internet sorted its machines into two classes.

Servers listen. They hold a stable public address, they accept connections they didn't initiate, and anyone who knows where to look can find them. Everyone else got a connection that only speaks. You reach out to a service, it answers, and the page arrives — but nothing on the open internet can start a conversation with you. Your laptop has no address the world can dial. Your phone has less than that.

Most people never notice, because nearly everything they do runs in that one downhill direction. Browsing, streaming, scrolling, shopping — all of it is you asking a company's server for something and the server obliging. The architecture and the business model reinforce each other so neatly that the arrangement feels like the natural shape of a network rather than a thing that happened.

It isn't the shape the network was built in. The original design put every machine on equal footing: any host able to address any other, as ready to serve as to consume. What replaced it is an internet where the ability to be *reached* — to host, to publish, to accept a connection from a stranger — became a paid product, and where two ordinary people's computers can no longer talk to each other without a company in between.

A connection that can only speak and never listen gives you a kind of second-class standing on the network. Full consumer, partial participant. It's the difference between citizenship and consumerism.

## Three obstacles

Three separate things stand between two ordinary machines. They're often blurred together, and they have different causes and different fixes.

### Address scarcity, and the two internets

IPv4 has about 4.3 billion addresses, and around 2011 the regional registries began running dry. The response was to stop giving every device its own: your household gets one public IPv4 address, your router hands out private ones inside, and everything shares the single public one going out. That is NAT, and it is a direct consequence of the shortage.

IPv6 ends the shortage outright. The space is large enough to give every device on Earth a globally routable address with room to spare. On a working IPv6 network there is no translation at all. Your machine's address is its real address, the one the rest of the internet would use, and it knows it without asking anyone. That dissolves half the problem this essay is about. The whole business of discovering *what your address is* goes away, and only *can a packet reach it* remains.

That is the promise, and it was never a gamble. IPv6 has been the settled plan for a generation. It was published in 1998 as the standard that would replace IPv4 everywhere and became a full Internet Standard in 2017. The software caught up long ago. Pages, browsers, client applications, and operating systems all ship dual-stack, and frequent updates keep them current. And still, most connections run on IPv4. Three separate things stand in the way.

First, **you do not decide whether you get IPv6**. Your software is ready, but the address you actually receive comes from the parts you do not own: your internet provider, the router you rent from them, the network behind it. Those change on the provider's schedule, not yours, and no update you install touches them. Google's [running measurement](https://www.google.com/intl/en/ipv6/statistics.html) of IPv6 among its users shows the result: a long, uneven climb, with wide gaps between countries and between the networks inside one.

Second, the adoption which _does_ exist is **mostly on mobile** networks, and a phone is exactly where its owner does not choose what runs. The platform vendor's app store does. So the IPv6 that is winning is winning on the devices least able to run open peer-to-peer software at all, and the kind that would help, on machines their owners command, is rarer still.

And third, having IPv6 yourself is only half of it. A connection is only possible when IPv6 is at **both ends**, so the reachable share is the per-peer share, squared. Even odds per peer computes down to one pair in four. So per-connection availability will always trail per-peer adoption.

Put the three together and what remains is sadly only a sliver. We're 28 years into IPv6's certain future, and the software has been ready for most of that time. On the day of this writing in July 2026, we sampled UDP traffic to [open.ftorrent.com](https://open.ftorrent.com) and measured 15.6% of peers reached the tracker over IPv6. Squared, only 2.4% of them could meet over IPv6 at all.

### What the router does

**NAT** — network address translation — is the machinery that lets many devices share one public IPv4 address. When a machine inside sends a packet out, the router rewrites the source address to its own public one, picks a port to represent that particular conversation, and remembers the pairing so replies can be sent back to the right device inside.

Outbound works beautifully. Inbound has nowhere to go. A packet arriving unbidden at the public address carries no indication of which machine inside it belongs to, and the router has no entry for it, so it gets discarded. This is a translation problem rather than a security decision, though the effect is firewall-shaped and most routers add an actual firewall on top of it.

**This is an IPv4 story.** IPv6 hosts hold their own globally routable addresses and there is no translation, so the address problem doesn't arise. What remains on IPv6 is the firewall — most home routers block unsolicited inbound by default — which is a permission question, not an addressing one, and a much simpler one to solve.

**Hole punching** is the technique that gets through. When a machine inside sends a packet out, the router opens a temporary mapping so the reply can come back. If two peers each send outward at roughly the same moment, each one's outgoing packet creates the opening that lets the other's arrive. For IPv4 this does two jobs at once — it establishes the address translation *and* it satisfies the firewall. For IPv6 only the second job exists.

**Mapping behavior** decides whether the punch lands:

- **Endpoint-independent mapping** means the router assigns the same public port to your machine regardless of which destination you're sending to. A port you learned by asking one server is the port any peer can use. Hole punching works.
- **Endpoint-dependent mapping**, usually called **symmetric NAT**, means the router assigns a *different* port for each destination. The port you learned from one server is not the port your peer's packets will arrive on, so the punch misses, and no amount of coordination fixes it.

Older writing sorts NAT into four "cone types" from [RFC 3489](https://datatracker.ietf.org/doc/html/rfc3489) — full cone, restricted, port-restricted, symmetric. That taxonomy proved unreliable in the field and was superseded by [RFC 5780](https://datatracker.ietf.org/doc/html/rfc5780), which separates *mapping* behavior (which port do I get) from *filtering* behavior (whose packets will you accept). The mapping question is the one that decides whether a direct connection is possible at all; the three non-symmetric cone types all share endpoint-independent mapping and differ only in filtering.

**Carrier-grade NAT** — CGNAT, sometimes NAT444 — is the same idea applied a second time, at the ISP. Rather than giving each customer a public IPv4 address, the carrier shares one address across many customers and runs a large translator at its edge. Your router still does its own NAT inside, so there are now two layers, and the outer one isn't yours to configure. [RFC 6598](https://datatracker.ietf.org/doc/html/rfc6598) reserved the `100.64.0.0/10` range for the space between them. CGNAT is increasingly common, and it is close to a hard wall: those translators almost universally use endpoint-dependent mapping, because multiplexing thousands of subscribers through one address makes per-destination ports the obvious design. A customer behind CGNAT typically cannot accept a direct connection at all over IPv4.

### What the browser allows

The third obstacle isn't the network — it's where the person now sits. The mainstream personal computer is no longer the PC but the smartphone, and on a smartphone its owner does not decide what software runs. The app store is the gate and the platform vendor holds the key: you install what you're permitted to install. The one place that rule doesn't reach is the browser, which will still navigate to any address and run whatever it's handed there, no permission asked. For most people, most of the time, the browser is the last runtime they fully control — so if peer-to-peer is going to reach ordinary people at all, it has to work there. Not because much peer-to-peer software lives in the browser today, but because the browser is where a person is still free to run it.

That freedom is fenced, and the fence is the real obstacle. A page runs in a sandbox that forbids most of what this work needs. It cannot open a raw socket, cannot bind a port and listen for an inbound connection, cannot read the machine's network interfaces, and cannot even choose whether a request goes out over IPv4 or IPv6 — that belongs to the browser's [Happy Eyeballs](https://datatracker.ietf.org/doc/html/rfc8305) logic, which races both and takes whichever answers first. Every one of those limits is deliberate and, on balance, right; together they rule out the way a native application would do this — read its interfaces, bind a listener, ask the router for a port — and a page can do none of it.

What a page *does* get is [WebRTC](https://webrtc.org/), a narrow but real opening: the browser performs UDP exchanges on the page's behalf and reports what it learned. A native application reaches deeper — it can bind a listener and prove that unsolicited inbound packets actually arrive, read the machine's real addresses, and ask the router directly for a port via **UPnP**, **NAT-PMP**, or **PCP** ([RFC 6887](https://datatracker.ietf.org/doc/html/rfc6887)), the protocols by which a device requests that the router forward a port to it. A browser has none of that. So the same logic reaches different depths depending on where it runs, and any honest browser-based tool has to be clear about which of its answers are proven and which are inferred.

## The protocols

These names get used interchangeably in casual writing and they are not interchangeable.

**STUN** — Session Traversal Utilities for NAT, [RFC 8489](https://datatracker.ietf.org/doc/html/rfc8489). A client sends one small UDP packet to a STUN server; the server replies with the address and port it observed the packet arriving from. That's the entire protocol as most deployments run it. Your machine has no other way to learn this — locally it sees only its private address, and the router invented the port on the way out. STUN is a client-server protocol, and public STUN servers are the most widely shared infrastructure described here.

**TURN** — Traversal Using Relays around NAT, [RFC 8656](https://datatracker.ietf.org/doc/html/rfc8656). A relay that both peers connect to, which forwards traffic between them when a direct path can't be established. **TURN is not peer-to-peer.** Two machines communicating through a relay are two clients of a server, and everything that follows from that follows here. The relay operator sees both endpoints, the timing, and the volume of everything exchanged. The connection depends on that operator staying up and staying willing. Every packet takes two hops instead of one. And the operator pays for every byte twice, inbound and outbound, which is why providers sell relay capacity rather than giving it away. TURN rescues connections that would otherwise fail entirely, and it is also what makes a peer-to-peer application quietly stop being one.

**ICE** — Interactive Connectivity Establishment, [RFC 8445](https://datatracker.ietf.org/doc/html/rfc8445). Unlike STUN and TURN, ICE is not a client-server protocol and there is no such thing as an ICE server. It's a procedure that both endpoints implement: gather every address you might be reachable at (your local one, your STUN-discovered public one, a relay address if you have TURN), call them *candidates*, send the list to the other peer, then systematically test pairs until one works. Standardizing it matters because both sides have to agree on the order and the timing, not because anything external participates. The connectivity checks ICE sends are themselves STUN messages — sent peer to peer, with no STUN server involved — which is a large part of why these terms get tangled.

**SDP** — Session Description Protocol, [RFC 8866](https://datatracker.ietf.org/doc/html/rfc8866). The format of the blob describing what a peer supports: media and data parameters, encryption fingerprints, ICE credentials. A few kilobytes of text.

**JSEP** — JavaScript Session Establishment Protocol, [RFC 8829](https://datatracker.ietf.org/doc/html/rfc8829). The state machine around SDP for WebRTC: what `createOffer`, `setLocalDescription`, `createAnswer`, and `setRemoteDescription` mean, in what order, and what state the connection is in at each step. **Trickle ICE** ([RFC 8838](https://datatracker.ietf.org/doc/html/rfc8838)) extends it so candidates can be sent as they're discovered instead of waiting for gathering to finish.

**WebRTC** is the umbrella: a [W3C browser API](https://www.w3.org/TR/webrtc/) plus a set of IETF protocols, wrapping all of the above with **DTLS** for key exchange and **SCTP** for reliable ordered data channels. Its data channels give you TCP's semantics over UDP, which is the shape nearly all modern peer-to-peer transport has converged on — [QUIC](https://datatracker.ietf.org/doc/html/rfc9000) and BitTorrent's µTP arrived at the same answer independently, because middleboxes handle UDP better than they handle TCP.

**Signaling** is the gap, and the server that fills it is the **rendezvous server** (commonly called a *signaling server*). Two peers must exchange SDP and candidates *before* any direct connection exists, and the specifications say to do it and pointedly decline to say how. So the rendezvous server is the one piece in this whole picture with a name but no standard: every other term here answers to an RFC, and it answers to none. The rest of this essay is largely about what people ran in its place.

## BitTorrent's parallel answer

Everything above is the web's approach. Set it beside an older one that solved the same problem before any of that machinery existed.

BitTorrent has been connecting peers directly since 2001, which means it predates nearly all of that machinery — STUN arrived in 2003, ICE in 2010, WebRTC around 2011. Facing the same NATs and the same hostile middleboxes with none of those tools yet invented, it grew its own set of answers:

- **The [mainline DHT](https://www.bittorrent.org/beps/bep_0005.html)** distributes peer discovery across the swarm itself. A client asks nodes it already knows for nodes closer to the content it wants, converging on peers holding that content — no tracker required, and no server that can be switched off.
- **[Peer exchange](https://www.bittorrent.org/beps/bep_0011.html)** has connected peers tell each other about the peers they know, so a swarm keeps introducing its own members after the initial contact.
- **[BEP 55](https://www.bittorrent.org/beps/bep_0055.html)** coordinates hole punching through a third peer already connected to both parties. It's the same technique WebRTC uses, arranged without any dedicated infrastructure — the swarm supplies the rendezvous.
- **[µTP](https://www.bittorrent.org/beps/bep_0029.html)** carries data over UDP with congestion control that yields to other traffic, arriving at the reliable-stream-over-UDP shape years before QUIC.
- **Port forwarding** through UPnP, NAT-PMP, or PCP lets a desktop client simply ask the router to open a port, which sidesteps hole punching entirely when the router cooperates.

Desktop clients face the same three obstacles as browser peers, and they fail in the same places. A user on an IPv4-only connection behind carrier-grade NAT will see connections fail constantly, whether the client is a desktop application or a page. What makes BitTorrent work is scale rather than reliability: a swarm of several hundred peers, where any given connection attempt often fails, still assembles enough working connections to move the file. The failures are invisible because the successes are sufficient.

It's easy to read the browser section above and conclude the difficulty is a web problem. It isn't. The browser adds constraints, and the underlying network is hostile to direct connections regardless of what software is attempting one.

## The implementations

**Browsers** are the most widely deployed WebRTC implementation by a very large margin, and for most developers the browser's built-in stack *is* WebRTC.

**[pion](https://github.com/pion/webrtc)** is a complete WebRTC implementation in Go, pure Go with no cgo, which means it compiles to a single static binary. It's the usual choice when you need a WebRTC endpoint that isn't a browser. **[aiortc](https://github.com/aiortc/aiortc)** does the same for Python and **[libdatachannel](https://github.com/paullouisageneau/libdatachannel)** for C++. All three implement everything the specifications cover and none of them implement signaling, because the specifications don't.

**[simple-peer](https://github.com/feross/simple-peer)** wraps the browser's API into something smaller. Its entire signaling surface is two things — an event that hands you a blob to deliver, and a method that accepts the blob that came back — which draws the boundary about as clearly as it can be drawn.

**[PeerJS](https://peerjs.com)** goes one step further and ships the missing piece: a client library plus **PeerServer**, where each peer gets an ID and you connect by ID. It's the closest thing to a general-purpose, application-agnostic answer to the signaling problem.

**[coturn](https://github.com/coturn/coturn)** is the server side. It's the reference implementation of both STUN and TURN, written in C, and it is what nearly everyone runs — the public STUN servers listed below, and most private deployments, are coturn or something coturn-compatible.

**[WebTorrent](https://webtorrent.io)** brought BitTorrent into the browser by replacing TCP peer connections with WebRTC data channels. **[Trystero](https://github.com/dmotz/trystero)** generalized WebTorrent's rendezvous trick into a library for connecting any two pages. Both are discussed below.

**[LiveKit](https://livekit.io)**, **[Janus](https://janus.conf.meetecho.com)**, and **[mediasoup](https://mediasoup.org)** belong in a different category despite appearing in the same searches. They're **SFUs** — Selective Forwarding Units — and they sit in the media path permanently by design. An SFU exists because multi-party video doesn't work peer-to-peer: eight people in a call means each browser maintaining seven upstreams, and everyone's uplink collapses. Route it through a server and each browser sends once. That's a sound architecture for the problem it solves, and it is client-server, not peer-to-peer. Worth keeping distinct from TURN, which is also in the path but only because something failed.

## The public services

Some of this infrastructure is open to anyone, and some of it is sold. Which is which follows the cost of running it.

### STUN servers

Three see wide public use:

```
stun:stun.l.google.com:19302
stun:stun.cloudflare.com:3478
stun:global.stun.twilio.com:3478
```

**Google's** is almost certainly the most-queried STUN server on the internet, and Google publishes no documentation offering it as a public service, no terms, and no availability commitment. It became ubiquitous by appearing in tutorials and in library defaults and staying up. [simple-peer's](https://github.com/feross/simple-peer) built-in configuration, which WebTorrent inherits, ships exactly two servers:

```javascript
Peer.config = {
	iceServers: [{ urls: [
		'stun:stun.l.google.com:19302',
		'stun:global.stun.twilio.com:3478'
	]}]
}
```

So an enormous share of the world's WebRTC connections ask Google for their address, without anyone along the way ever making that decision. Note also the non-standard port — 19302, where [IANA registered 3478 for STUN](https://datatracker.ietf.org/doc/html/rfc8489).

**Cloudflare** documents theirs at [developers.cloudflare.com/realtime/turn](https://developers.cloudflare.com/realtime/turn/), and offers an alternate on UDP port 53 for networks that filter unusual ports but pass anything resembling DNS.

**Twilio** documents theirs at [twilio.com/docs/stun-turn](https://www.twilio.com/docs/stun-turn). Their API hands back both kinds of server in one response, and the difference between the two entries is the whole economics of this field: the STUN entry carries no credentials, and every TURN entry carries a username and a password. Providers give STUN away because a binding exchange is two small packets with no session behind it. They meter TURN because it moves real bandwidth — Cloudflare publishes five cents per gigabyte for relay traffic.

One primitive given away by everyone, its neighbor sold by everyone. Discovery is cheap enough to run as a commons; relay is not, and no amount of goodwill changes the arithmetic.

### Address reflection over HTTP

A simpler cousin: services that tell you the IPv4 or IPv6 address your web request came from. [ipify](https://www.ipify.org) is the reference implementation of the pattern, with separate names that force each address family — `api.ipify.org` for IPv4, `api6.ipify.org` for IPv6, `api64.ipify.org` for whichever your stack chose. [icanhazip.com](https://icanhazip.com), now operated by Cloudflare, answers tens of billions of plain-text requests a day. [ifconfig.co](https://ifconfig.co) publishes its implementation as *echoip*.

These answer a narrower question than STUN does. An HTTP reflector reports an address; STUN reports an address *and a port*, and the port is the part hole punching needs. They also observe different paths — one a TCP connection, the other a UDP flow — so when the two disagree, something is carrying one path and not the other.

### Rendezvous servers

No public rendezvous server is offered the way STUN is. The sections below cover what people use instead.

### Relay

Commercial, almost entirely. Twilio, Cloudflare, and Xirsys sell TURN, and some open-source projects run small relays for their own users. Nobody offers general-purpose relay to the public for free, and the per-byte cost structure explains why nobody sensibly could.

## The page-to-page exchange

The rendezvous server is the one entry on that list with no public utility to point to, and the specifications' silence makes it sound larger than it is. Here it is in the concrete: two browser pages, with everything else already in place.

By this point every WebRTC piece is in hand. STUN has told each page its public address and port. ICE stands ready to test candidate paths and, once it has somewhere to aim, to punch through NAT on its own. SDP describes the session and DTLS will encrypt it. The only thing missing is that page A holds all of this about *itself* and nothing about page B, and B the reverse. Signaling closes that gap — and it closes it by moving *information*, not by firing a "start now" pulse. Give each side the other's details and ICE begins its connectivity checks unprompted; the mutual checks are the hole punch, and there is no clock tick to coordinate.

Two kinds of object cross. The first is the **session description**, the SDP, which one page generates with `createOffer()` and the other responds to with `createAnswer()`. It's a small object — `{ type: "offer", sdp: "…" }` — whose `sdp` string, trimmed to the lines that carry weight, reads:

```
v=0
m=application 9 UDP/DTLS/SCTP webrtc-datachannel
a=ice-ufrag:F7g3
a=ice-pwd:x9cLk2m8Qz0v4Rt7Bn1w
a=fingerprint:sha-256 9B:2C:1A:…:E4
a=setup:actpass
```

The `m=` line says what's being negotiated — here, a data channel. The three below it do the real work. `ice-ufrag` and `ice-pwd` are a fresh random username and password; every ICE connectivity check the two peers later fire at each other carries them, so each side can tell a genuine check from its partner apart from a stray or forged packet. `fingerprint` is a hash of this page's self-signed certificate: after ICE finds a working path, the peers run a DTLS handshake across it and each verifies the certificate it is handed hashes to the fingerprint that arrived here. That is what lets a connection over a wide-open, untrusted path still be private — the trust is bootstrapped through signaling once, and then the data never touches a server again.

The second kind of object is the **ICE candidate**, one per address the page might be reachable at, emitted as ICE discovers them:

```
{
  candidate: "candidate:842163049 1 udp 1677729535 203.0.113.7 54321 typ srflx raddr 192.168.1.7 rport 54321",
  sdpMLineIndex: 0
}
```

Reading the string: transport `udp`, then `203.0.113.7 54321`, the address and port a peer should try, then `typ srflx` — server-reflexive, the public mapping a STUN server reported, with `raddr`/`rport` noting the private address behind it. One page usually emits several: a `host` candidate for its local interface, an `srflx` for the STUN-seen public one, a `relay` if it has TURN.

On the wire the whole conversation is short, and then it's spent:

```
page A                 rendezvous server               page B
  │── {sdp: offer} ───────►│───────── {sdp: offer} ──────►│
  │◄──────── {sdp: answer} │◄──────── {sdp: answer} ──────│
  │── {candidate} ────────►│───────── {candidate} ───────►│
  │◄──────── {candidate} ──│◄──────── {candidate} ────────│
       (a few kilobytes, then the rendezvous server has nothing left to do)
  │◄═══════════ direct, DTLS-encrypted data channel ═════►│
```

Each page's half is the shape every WebRTC guide teaches:

```javascript
const pc = new RTCPeerConnection({ iceServers: [{ urls: 'stun:stun.ftorrent.com:3478' }] })

// As ICE finds candidates, send each to the other peer.
pc.onicecandidate = ({ candidate }) => candidate && signaling.send({ candidate })

// Apply whatever the other peer sends back.
signaling.onmessage = async ({ sdp, candidate }) => {
  if (sdp)            await pc.setRemoteDescription(sdp)     // their offer or answer
  else if (candidate) await pc.addIceCandidate(candidate)   // one of their addresses
}

// The caller opens the channel and offers; the answerer instead calls createAnswer() on receiving the offer.
pc.createDataChannel('data')
await pc.setLocalDescription(await pc.createOffer())
signaling.send({ sdp: pc.localDescription })
```

And the rendezvous server that `signaling` talks to — the entire "system" the specifications leave out — copies each message to the other peers in the same room and reads none of it:

```javascript
const rooms = new Map()
wss.on('connection', (ws, req) => {
  const room = new URL(req.url, 'http://_').searchParams.get('room')
  const peers = rooms.get(room) ?? rooms.set(room, new Set()).get(room)
  peers.add(ws)
  ws.on('message', data => { for (const p of peers) if (p !== ws) p.send(data) })
  ws.on('close', () => peers.delete(ws))
})
```

Ten lines. The rendezvous server never parses an SDP or knows the first thing about WebRTC. It is a pipe keyed by a room name.

## How real projects answered the signaling question

The rendezvous server can be almost anything, and that is the whole story of how real projects filled the gap. Each reached for one it already ran rather than standing up a new one.

### WebTorrent used a tracker

A BitTorrent client already contacts a tracker to learn who else has the file it wants — that round trip is happening regardless. So WebTorrent extended the WebSocket tracker protocol to carry offers and answers alongside the peer list. The tracker was already positioned to introduce strangers; relaying a few kilobytes between them was nearly free to add.

The result is that a WebTorrent tracker does two jobs a conventional tracker doesn't have to distinguish: the ordinary directory job of matching peers to a topic, and a rendezvous server carrying opaque blobs between browsers that have no other way to reach each other. It doesn't parse SDP or know anything about WebRTC — it forwards addressed messages within a swarm.

### Trystero generalized it, then moved off it

[Trystero](https://github.com/dmotz/trystero) noticed that the tracker's role has nothing to do with files. An infohash is an arbitrary twenty-byte string; two peers agreeing on any string get introduced to each other. That makes any public BitTorrent tracker a general-purpose rendezvous server for any application — a multiplayer game, a collaborative editor, a chat room.

Then it went further and shipped seven interchangeable strategies: [Nostr](https://nostr.com), [BitTorrent](https://www.bittorrent.org), [MQTT](https://mqtt.org), [Firebase](https://firebase.google.com), [Supabase](https://supabase.com), [IPFS](https://ipfs.tech), and a self-hosted [WebSocket](https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API) relay. The default is Nostr, chosen for redundancy across hundreds of public relays.

Trystero doesn't pick a rendezvous server. It enumerates the free public infrastructure that already plays the role and lets the developer choose, which is only possible because the requirement is so weak.

### Nostr became the default

[Nostr](https://nostr.com) is a protocol for publishing signed messages to relays — deliberately simple, with no consensus mechanism and no tokens. Clients hold keypairs, relays accept and forward signed events, and anyone can run a relay. It was built for censorship-resistant social media, and hundreds of public relays exist that accept events from anyone without an account.

None of that has anything to do with WebRTC. It works as a rendezvous server because the whole job of one is *to be a third party both peers can reach, willing to copy a few kilobytes from one to the other* — and Nostr relays are exactly that, while being numerous, free, unauthenticated, and already deployed.

### Why there is no standard, and won't be

That ten-line rendezvous server is the reason, and it holds against any future standard too. Compare the two problems directly.

**STUN needed something specific**: a server that observes the source address on your UDP packet and reports what it saw. Nothing else does that. A unique requirement produced a protocol, an RFC, an IANA-registered port, and a handful of public deployments the whole world shares.

**A rendezvous server needs almost nothing.** Any server both peers can reach that will forward a small message. Every one of Trystero's seven strategies qualifies. So does a Redis instance, an IRC channel, a shared document, or a person reading a code over the phone — pion's own examples have you copy a base64 blob between two terminals by hand, and it works.

When any server will do, no particular server becomes the standard. The specifications left signaling undefined not from neglect but because the requirement is too weak to be worth naming, and because it inevitably sits next to application-specific concerns — who may connect to whom, how peers are named, what a "room" means — that no transport specification should be deciding.

It also means the browser gives you nothing here. There is no API by which two pages on different machines find each other. `BroadcastChannel` is same-origin and same-browser; WebSocket, WebTransport, and `fetch` all require a server you supply. Every path requires a third party, and the only real question is which one you already have.

## What's actually scarce

Count the public rendezvous servers that need no account, no API key, and nothing of your own to run: Nostr relays, BitTorrent trackers, a few public MQTT brokers, IPFS. It's a short list, and it's short for a reason that isn't technical.

Writing a rendezvous server takes an afternoon. Running one open to the public, unauthenticated, free, indefinitely — absorbing the bandwidth, staying up for years with nobody paying — is a different commitment entirely. That's why the free and open category is populated almost entirely by protocol commons rather than by products. The scarce resource in peer-to-peer connectivity was never the technology. It's somebody willing to operate the introduction infrastructure as a public good.

Which is a reasonable note to end on, because the whole stack described here rests on a small number of parties doing exactly that. An enormous share of the world's direct connections are brokered by a Google server nobody documented, sitting on a non-standard port, that has simply never gone down.

## ftorrent's part

[ftorrent](https://ftorrent.com) runs some of this infrastructure, in the spirit of the above. There's a public STUN server at `stun:stun.ftorrent.com:3478` — plain binding only, no relay, no account, its full configuration [in the repository](https://github.com/zootella/ftorrent/tree/master/good). A WebSocket BitTorrent tracker runs at [open.ftorrent.com](https://open.ftorrent.com); per the Trystero observation, it serves as a general rendezvous server for applications that have nothing to do with files. And [good.ftorrent.com](https://good.ftorrent.com) reports how much of all this applies to your own connection.

The tracker is also a small piece of the collaboration this essay keeps pointing at. We reached out to the Trystero project, and open.ftorrent.com now rides in the list of WebSocket trackers Trystero ships — one more public rendezvous server in a set meant to hold many. That's how the open web is supposed to grow: projects strengthening each other's defaults instead of each raising its own walls. It's the commitment ftorrent works by, and the same bet the whole stack above rests on — that shared infrastructure, openly operated, outlasts anything a single party could own.
