
_ftorrent/good/README.md [good.ftorrent.com](https://good.ftorrent.com)_

# The peer-to-peer connection checker

> Prepared by [Claude Code](https://claude.ai/code) using Opus 5.5
> <br>Created: 2026-Jul
> <br>Last reviewed: 2026-Oct
> <br>Go: 1.24
> <br>Vue: 3, on Vite 8
> <br>coturn: [coturn/coturn](https://hub.docker.com/r/coturn/coturn) 4.14.0 (STUN)
> <br>Docker Engine: 29
> <br>Docker Compose: V2

## What this is

Every device on the internet can ask "what's my IP address?" and plenty of sites will answer. [ipchicken.com](https://ipchicken.com) is the classic — a page, an address, and not much else; many of them still don't even show your IPv6. Others measure how *fast* the line is ([Speedtest](https://www.speedtest.net), [Fast.com](https://fast.com)), and a newer crop watches for VPN leaks — what's quietly showing through when you thought you were hidden. All of them answer real questions. None of them answer this one.

good.ftorrent.com asks whether your connection is *good*, and here that word has a specific meaning. Not "can you load a web page," and not "how many megabits can you pull down." The question is whether your connection lets you be **both a speaker and a listener** — whether some other ordinary computer, somewhere else on the internet, can reach *you* directly, and not merely whether you can reach a server.

Somewhere along the way the internet quietly sorted its computers into two classes. Servers get to listen: they accept connections, they're reachable, they can be found. Everyone else was handed a connection that only speaks — you can reach out to a service and pull a page down, but nothing on the open internet can start a conversation with you. Most people never notice, because browsing, streaming, and scrolling all run in that one downhill direction. But it's a fraction of what the network was built to be. The original promise was a network of *peers*: any computer able to reach any other, as ready to serve as to consume. A connection that can only speak and never listen gives you a kind of second-class standing on the network — full consumer, partial participant. It's the difference between citizenship and consumerism.

That difference is exactly what peer-to-peer depends on. Two people's computers talking directly, with no company in the middle, only works if computers can listen for one another. good.ftorrent.com measures where you actually stand — and closing that gap, getting both ends of the connection back onto ordinary desks, is what ftorrent is for.

If you want the full survey of how two computers on today's internet manage to reach each other — the protocols, the libraries, the public services, and what other projects chose — we wrote that up separately as [How Can Two Peers Connect?](https://docs.ftorrent.com/how-peers-connect). This guide is about the checker and the code in this directory.

## Three things decide whether you can connect

Connection failure is normal. A BitTorrent client reaching out to fifty peers does not get fifty connections, and that's true on the desktop and in the browser alike — swarms work because they're large enough that a fraction succeeding is still plenty. Nobody tells you which fraction you're in, or why. That's what this checker is for, and three separate things are in play.

**Which address families you have.** IPv4 ran out of addresses around 2011, so your household shares one public IPv4 address among every device in it. IPv6 has no such shortage: on a working IPv6 network your computer holds its own globally routable address, and most of the difficulty below simply doesn't arise. Whether you have IPv6, and whether it actually carries traffic, is the single most useful thing to know — and plenty of people who have it don't know they do, while others see IPv6-looking addresses on their computer that lead nowhere.

**What your router does with ports.** Sharing one IPv4 address means your router rewrites addresses and ports on the way out and remembers the pairing so replies can return. Two peers can exploit that by each sending outward at the same moment, so each one's packet opens the door for the other's — but it only works if your router assigns you the *same* public port regardless of who you're contacting. Some routers assign a different port per destination, which breaks the technique entirely. Even with the same port, the router decides who may send to it: some let in anyone once you've sent anything, and others only the exact address and port you sent to, which is why both peers send at once. If your ISP also shares one address across many customers, called carrier-grade NAT, you're behind a second layer you can't configure and the odds get worse.

**Whether your browser can be a peer.** Browser-based BitTorrent — [WebTorrent](https://webtorrent.io) — connects pages to each other over WebRTC, since a page can't open the raw sockets a desktop client uses. That path has its own requirements and its own failure modes, and a computer whose desktop client swarms happily may still have a browser that can't hold a single peer connection.

Today the checker answers all three. It asks three STUN servers in three places, two of them ours, and then punches a hole through two translators of our own, one shaped like a home router and one that changes the port for every destination, so you see a direct connection succeed or fail rather than a prediction. From the page that punch is a WebRTC data channel, which answers the third question too; the desktop client punches by hand, packet by packet, and also tests which strangers your router lets through. The sections below say which parts run now.

## Three ways to use it

good.ftorrent.com comes in three layers, and you can meet it at whichever one fits.

- **The finished product.** Point a browser at [good.ftorrent.com](https://good.ftorrent.com) and it runs the checks and shows you the answer. No account, no install, nothing to configure — a page you just visit, the way ipchicken is.
- **The APIs underneath.** The page keeps nothing to itself; it calls a few small public services. They're general-purpose tools, free for anyone to use, and good.ftorrent.com uses them exactly as your own code would — which is the proof they're solid enough to build on.
- **The code itself.** It's all here in this repository, open. Fork the page, keep our APIs as its backend, or fork the whole thing and run your own copy. If you'd rather not trust ours, good — running your own is the point.

## The public APIs

### IP reflection — `ip.ftorrent.com`

Ask it your address and it hands it back:

```
GET https://ip.ftorrent.com/               → your IP, plain text
GET https://ip.ftorrent.com/?format=json   → {"ip":"203.0.113.42"}
```

Two sibling names each force a single address family — that is, one name reachable only over IPv4 and one only over IPv6:

```
GET https://ip4.ftorrent.com/   → your IPv4 (the name carries only an A record, so the request can only travel over IPv4)
GET https://ip6.ftorrent.com/   → your IPv6 (only AAAA — the request is forced onto IPv6)
```

Call the pair, and the two outcomes *are* your dual-stack verdict: both answer and you have both; one fails and you know which one you're missing. The same two names exist on our second server as `ip4.moon.ftorrent.com` and `ip6.moon.ftorrent.com`, so a failed fetch to one server can be told from a connection with no IPv6 at all, and the page asks both. This is the same mechanism good.ftorrent.com's own page runs — see [`page/src/connection.js`](page/src/connection.js), where every check the page makes lives.

Every response carries `Access-Control-Allow-Origin: *`. That header is CORS, and it matters because a browser will *send* a request to another origin but won't let the page *read* the response unless the server permits it — permission the server grants, not something the caller can ask for. We set it in [`go-ip/main.go`](go-ip/main.go), on the same handler that writes the address.

None of this shape is new. [ipify](https://www.ipify.org) established it years ago and still runs it, with the same split across three names: `api.ipify.org` for IPv4, `api6.ipify.org` for IPv6, `api64.ipify.org` for whichever you've got. [icanhazip.com](https://icanhazip.com), now operated by Cloudflare, answers tens of billions of plain-text requests a day, and [ifconfig.co](https://ifconfig.co) publishes its implementation as *echoip*. We run our own because the checker needs a reflector it controls and because it pairs with the STUN server below. If all you need is an address, ipify has been doing this well for a long time.

### STUN — `stun.ftorrent.com`

STUN is how a browser or app learns the public address *and port* its router assigned it — the first step in forming a direct peer connection, and the part a plain what's-my-IP service can't supply. Ours is a plain binding server on the standard port, and you can drop it into any WebRTC configuration.

Browsers don't come with one. A browser is client software, so of course it isn't a STUN server itself — but it also ships no *default list* of servers to ask. Whatever page or application you're running has to supply that list, which is why every WebRTC library hardcodes one. Here's a working configuration that lists ours alongside the three in wide public use:

```javascript
const pc = new RTCPeerConnection({
	iceServers: [{ urls: [
		'stun:stun.l.google.com:19302',
		'stun:stun.cloudflare.com:3478',
		'stun:global.stun.twilio.com:3478',
		'stun:stun.ftorrent.com:3478'
	]}]
})
```

Google's is the one most WebRTC tutorials paste in and is probably the most-queried STUN server anywhere, though Google publishes no documentation offering it as a service — the hostname answers STUN and nothing else, so there's no page to link. [Cloudflare](https://developers.cloudflare.com/realtime/turn/) and [Twilio](https://www.twilio.com/docs/stun-turn) only mention theirs in passing, inside the docs for their paid TURN services — yet all three are used globally well beyond that, free and reliable.

**Why we run our own**, given three good ones already exist. First, to understand it — you learn a protocol properly by operating it, and everything in this checker rests on knowing what STUN really does. Second, so the [ftorrent desktop client](https://github.com/zootella/ftorrent/tree/master/desktop) has a first-party option: a desktop application, unlike a browser, gets to choose its own default, and we'd rather ours ask our server than route every user's first move through someone else's. Third, because the decentralized web needs more of this. Public STUN is infrastructure the whole peer-to-peer world depends on, and nearly all of it currently sits at three companies. One more operator is one more.

Ours answers on 3478, the port [IANA registered for STUN](https://datatracker.ietf.org/doc/html/rfc8489), and so does its twin at `moon.ftorrent.com:3478`, on a second server at another place on the internet; Google's answers on 19302. For a checker that difference matters, because a server on an unusual port can succeed on a network where a real peer connection would fail, and we'd rather reproduce the failure than hide it.

There's no TURN here — this is self-discovery, not a relay.

## How it works

The dual-stack check runs on **single-family DNS**. `ip4.ftorrent.com` publishes only an IPv4 (`A`) record and `ip6.ftorrent.com` only an IPv6 (`AAAA`) record, though both point at the same server. A browser offers no way to say "make this request over IPv4" — but if the only address it can resolve for a name is an IPv4 one, it has no other choice. So the page fetches both names at once: whichever succeeds proves that family works and returns the address the server saw the request arrive on; whichever fails means that family doesn't reach the open internet from where you're sitting. The IPv6-readiness checkers like [test-ipv6.com](https://test-ipv6.com) have leaned on this same approach for years. The page fetches moon's pair too, from another place on the internet, so a family that reaches one of our servers and not the other reads as trouble on our side rather than yours.

The answer can't be faked, because the page never asks your computer what its addresses are — it reports what actually arrived. A link-local `fe80:` address, a private `fd00:` one, or a dead [Teredo](https://en.wikipedia.org/wiki/Teredo_tunneling) tunnel can't complete a connection to a globally-routed host, so they produce a failure rather than a false positive.

**What that check can't tell you** is why the page asks STUN as well. Both fetches travel over HTTPS, which runs on TCP. Hole punching runs on UDP. Offices, schools, hotels, and some mobile networks allow outbound TCP on 443 while blocking UDP wholesale — often by blocking [QUIC](https://en.wikipedia.org/wiki/QUIC) and catching everything else with it. A visitor on such a network has genuinely working IPv6, passes this check, and still can't reach a peer. Asking our STUN server for a binding over each address family closes that gap, because it moves the finding from "IPv4 and IPv6 reach a web server" to "IPv4 and IPv6 carry UDP."

A page has no socket of its own, so it asks STUN through WebRTC, by starting ICE (Interactive Connectivity Establishment), the dance by which two WebRTC peers find a path to each other. The dance's first step is gathering, where the browser opens sockets on each of the machine's addresses, in both families, and asks the STUN servers from each. The page makes a peer connection naming the server and asks for a data channel, which starts the gathering, then reads the server-reflexive candidates the browser hands back, which are the public address and port the server saw, and stops there, with no peer on the other end. The page sets that address beside the one the reflector saw over HTTPS. They should agree, and when they don't, something sends the two out apart: over IPv4, a carrier's translator with more than one public address, or in either family a VPN, a proxy, or a relay such as iCloud Private Relay carrying one path and not the other. The table states both addresses, and a sentence under the verdict says what most likely put them apart.

The page also asks whether your router keeps one public port for every destination, with three more gatherings: moon's server alone, Cloudflare's alone, and all three together, which the browser asks from the same sockets. When a socket keeps one port, every server sees the same address and port and the browser hands back one candidate for it; when it gets a new port per destination, one for each server. A browser may gather from several sockets and never says how many, but asked by our server alone, each socket answers once, so that count is the number of sockets: the same count from all three together is endpoint-independent mapping, the kind hole punching works through, and three times the count is endpoint-dependent, where it fails. The box under the verdict keeps everything the browser reported along the way, every candidate line, every error, and the browser's own record of each local candidate, so a run pasted into an issue holds all of it. The [ftorrent desktop client](https://github.com/zootella/ftorrent/tree/master/desktop) asks the same question from one socket it chooses itself, which is the authoritative form of the test.

Then the page runs the rest of ICE, against a peer that is ours. On moon, two peers each sit in a network namespace of their own behind a NAT we make with Linux's own translation, one that keeps a socket's port for every destination and one that picks a new port for each. For each, the page gathers its candidates through our STUN server and posts the offer to moon, which hands it to the peer and returns the peer's answer, one HTTPS exchange, the shape [WHIP](https://www.rfc-editor.org/rfc/rfc9725) standardized. Both sides then send STUN checks to each other's candidates, a NAT lets them in only once its socket has sent toward where they come from, and when a pair gets through, DTLS and the data channel open over it and the peer sends one message naming the pair. Getting through the first NAT and not the second is the common case, a hole punch shown working rather than inferred; through neither is a NAT on your side that changes the port too, or a network that holds WebRTC back. The peer checks every public IPv4 candidate the browser offers, wherever it is, since a phone's carrier can send its HTTPS and its UDP out from different addresses, and a check is a STUN request only that browser can answer.

On the server side the reflector is small: [`go-ip/main.go`](go-ip/main.go) is about sixty lines, a single static Go binary with no operating system in its container image, reading one header and writing one line. It runs behind a reverse proxy that terminates TLS — HAProxy in this deployment — so the address it reports comes from the `X-Forwarded-For` header rather than the socket. A client can write that header itself, so the proxy deletes whatever arrived and appends the address it actually accepted the connection from, which makes the **last** entry the trustworthy one. The first entry is the one a client could have forged.

The STUN server takes the opposite path: nothing sits in front of it. Docker publishes UDP 3478 with kernel DNAT and the userland proxy off, the same way the tracker's UDP port is published, so coturn sees the client's real address and port on the packet itself and reports exactly those. The tracker guide in the `open` workspace explains the mechanism.

## What's running, and what's next

- **Running.** The page, the three reflection names, and the dual-stack verdict they produce. A globally routable IPv6 address usually means a peer can reach you directly, without any of the port gymnastics IPv4 requires.
- **Running.** `stun.ftorrent.com`, and the page's two STUN checks on it: your public address and port over UDP in each family, set beside what HTTPS saw, and the mapping verdict from ours, moon's, and Cloudflare's asked from the same sockets.
- **Running.** A second vantage of our own at `moon.ftorrent.com`, a small virtual server at another place on the internet, built from the files in [`moon/`](moon/) and described in its own guide there: a second STUN server, a second reflector at `ip4.moon.ftorrent.com` and `ip6.moon.ftorrent.com`, and, for the desktop client, a poke, a dial, and a ping, a few UDP packets, a TCP connection, or one packet a BitTorrent client answers, sent back to whoever asks, which prove a peer can reach you rather than infer it. The page asks its STUN server and its reflector, and the desktop client its STUN server and its poke.
- **Running.** Two peers on moon, each behind a translator we make ourselves with Linux's own: one that keeps a socket's port for every destination, the way most home routers do, and one that picks a new port for each, the symmetric kind. The page punches through both over WebRTC and the desktop client by hand, so each shows a hole punch succeed or fail rather than predicting it.
- **Running.** The measurements only a desktop application can make, in the desktop client's Connection tab: the same address and STUN checks from a socket it controls, with every STUN answer shown whole, and the device's own network interfaces, every address on each named for what it is, which explains the `fe80:` and `fd00:` addresses that lead so many people to believe they have working IPv6. It also asks the system's resolver whether the network makes up IPv6 addresses for IPv4-only names, reads the machine's firewall, asks the router what it speaks of NAT-PMP, PCP, and UPnP, runs a libtorrent session that counts who connects in, and has moon poke a socket that has spoken to one server, which shows whether your router lets in anyone, only an address you've sent to, or only the exact address and port. With a session running, it also has moon ping and dial the session's own port, over UDP and TCP in both families, which proves whether a stranger can reach the client at all.
- **Planned.** The rest of what only a desktop application can do: asking the router for a port mapping, and adding the firewall rule, each only when you say yes.

We ship the part we can stand behind and grow from there.

## The files

```
good/
├── docker-compose.yml   the container stack: ip reflector and coturn
├── go-ip/               the ip reflector — Dockerfile, go.mod, main.go
├── coturn/              turnserver.conf — coturn in plain-STUN-only mode
├── moon/                the second vantage at moon.ftorrent.com: cloud-init, the firewall and its counters, coturn, the Go service, two NATs of our own and the peer behind each, Caddy, the deploy and firewall scripts, and its README
└── page/                the page good.ftorrent.com serves, a Vue app on Vite, with its checks in src/connection.js
```

[`go-ip/`](go-ip/) is ours, built from source. [`coturn/turnserver.conf`](coturn/turnserver.conf) is forty lines of configuration we wrote for an image we pull unchanged from upstream, and nearly every line turns something off — `stun-only` refuses TURN allocations, and `no-tcp`, `no-tls`, `no-dtls`, `no-rfc5780`, and `no-cli` shut down listeners and interfaces we don't use.

Both running containers are hardened the same way as the rest of the ftorrent deployment, set in [`docker-compose.yml`](docker-compose.yml): an unprivileged user, a read-only root filesystem, every Linux capability dropped, and tight memory and process limits. The same posture and reasoning appear in the [tracker's guide](https://github.com/zootella/ftorrent/blob/master/open/README.md), which covers it in more depth. Both services are also stateless — they answer a request and keep nothing, so nothing has to survive a restart and there's no database to run.

The page is static files with no server of its own behind them, served directly by the proxy. `pnpm upload` in `page/` builds it with Vite into `page/dist` and mirrors that to the server through `upload.js` at the repository root, the same way the other sites ship. Bring the services up with `docker compose up -d`.
