
_ftorrent/good/README.md [good.ftorrent.com](https://good.ftorrent.com)_

# The peer-to-peer connection checker

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.8
> <br>Created: 2026-Jul
> <br>Last reviewed: 2026-Jul
> <br>Go: 1.24
> <br>coturn: the [coturn/coturn](https://hub.docker.com/r/coturn/coturn) image (STUN)
> <br>Docker Engine: 29
> <br>Docker Compose: V2

## What this is

Every device on the internet can ask "what's my IP address?" and plenty of sites will answer. [ipchicken.com](https://ipchicken.com) is the classic — a page, an address, and not much else; many of them still don't even show your IPv6. Others measure how *fast* the line is ([Speedtest](https://www.speedtest.net), [Fast.com](https://fast.com)), and a newer crop watches for VPN leaks — what's quietly showing through when you thought you were hidden. All of them answer real questions. None of them answer this one.

good.ftorrent.com asks whether your connection is *good*, and here that word has a specific meaning. Not "can you load a web page," and not "how many megabits can you pull down." The question is whether your connection lets you be **both a speaker and a listener** — whether some other ordinary machine, somewhere else on the internet, can reach *you* directly, and not merely whether you can reach a server.

Somewhere along the way the internet quietly sorted its machines into two classes. Servers get to listen: they accept connections, they're reachable, they can be found. Everyone else was handed a connection that only speaks — you can reach out to a service and pull a page down, but nothing on the open internet can start a conversation with you. Most people never notice, because browsing, streaming, and scrolling all run in that one downhill direction. But it's a fraction of what the network was built to be. The original promise was a network of *peers*: any machine able to reach any other, as ready to serve as to consume. A connection that can only speak and never listen gives you a kind of second-class standing on the network — full consumer, partial participant. It's the difference between citizenship and consumerism.

That difference is exactly what peer-to-peer depends on. Two people's computers talking directly, with no company in the middle, only works if machines can listen for one another. good.ftorrent.com measures where you actually stand — and closing that gap, getting both ends of the connection back onto ordinary desks, is what ftorrent is for.

## Three ways to use it

good.ftorrent.com comes in three layers, and you can meet it at whichever one fits.

- **The finished product.** Point a browser at [good.ftorrent.com](https://good.ftorrent.com) and it runs the checks and shows you the answer. No account, no install, nothing to configure — a page you just visit, the way ipchicken is.
- **The APIs underneath.** The page keeps nothing to itself; it calls a few small public services that anyone else can call too. They're open primitives — no key, no signup — and good.ftorrent.com uses them exactly as your own code would, which is the proof they're solid enough to build on.
- **The code itself.** It's all here in this repository, open. Fork the page, keep our APIs as its backend, or fork the whole thing and run your own copy. If you'd rather not trust ours, good — running your own is the point.

## The public APIs

### IP reflection — `ip.ftorrent.com`

Ask it your address and it hands it back:

```
GET https://ip.ftorrent.com/               → your IP, plain text
GET https://ip.ftorrent.com/?format=json   → {"ip":"203.0.113.42"}
```

Two sibling names each force a single address family — the piece the plain what's-my-IP services don't offer:

```
GET https://ip4.ftorrent.com/   → your IPv4 (the name carries only an A record, so the request can only travel over IPv4)
GET https://ip6.ftorrent.com/   → your IPv6 (only AAAA — the request is forced onto IPv6)
```

Call the pair, and the two outcomes *are* your dual-stack verdict: both answer and you have both families; one fails and you know precisely which one you're missing. Every response carries `Access-Control-Allow-Origin: *`, so browser JavaScript on any page can read its own address here. This is the same mechanism good.ftorrent.com's own page runs.

### STUN — `stun.ftorrent.com`

A plain STUN binding server on the standard port, ready to drop into any WebRTC configuration:

```javascript
{ urls: "stun:stun.ftorrent.com:3478" }
```

STUN is how a browser or app learns the public address and port its NAT has assigned it — the first step in forming a direct peer connection. Ours does the same job the big public STUN servers do (Google's is the one most WebRTC tutorials paste in) with one difference worth stating plainly: ours does nothing else. It sees the IP and port that STUN by its nature must see, keeps none of it, and has no account, cookie, or ad profile to attach the visit to. There's no TURN here — this is self-discovery, not a relay.

## How it works

The trick behind the dual-stack check is **single-family DNS**. `ip4.ftorrent.com` publishes only an IPv4 (`A`) record and `ip6.ftorrent.com` only an IPv6 (`AAAA`) record, though both point at the same server. A browser offers no way to say "make this request over IPv4" — but if the only address it can resolve for a name is an IPv4 one, it has no other choice. So the page fetches both names at once: whichever succeeds proves that family works and returns the address the server saw the request arrive on; whichever fails means that family doesn't reach the open internet from where you're sitting. The IPv6-readiness checkers like [test-ipv6.com](https://test-ipv6.com) have leaned on this same approach for years.

On the server side the reflector is deliberately tiny — a single static Go binary, no operating system inside its container image, nothing to do but read one header and write one line. It runs behind a TLS-terminating proxy, so the address it reports is the real client address the proxy recorded on the way in (the last `X-Forwarded-For` entry), never the proxy's own.

**What's shipped today** is that foundational layer: your public address on each family, and whether you have one, the other, or both. That reads as modest, and it's the single most valuable signal this check can give — a routable IPv6 address usually means a peer can reach you directly, with none of the NAT gymnastics that IPv4 demands. The STUN server runs alongside it, the groundwork for the NAT-behavior checks that build on it next. We ship the honest first layer and grow from there.

## The files

```
good/
├── docker-compose.yml   the container stack: ip reflector, coturn, and a deferred pion slot
├── go-ip/               the ip reflector — Dockerfile, go.mod, main.go
├── coturn/              turnserver.conf — coturn in plain-STUN-only mode
├── pion/                a WebRTC peer, deferred: a Dockerfile only, until its Go source lands
└── static/index.html    the page good.ftorrent.com serves
```

The reflector and coturn run as hardened containers — an unprivileged user, a read-only root filesystem, every Linux capability dropped, and tight memory and process limits — the same security posture as the rest of the ftorrent deployment. The page is plain static files, served directly by the proxy. Bring the live services up with `docker compose up -d go-ip coturn`; the pion slot stays down until its source arrives.
