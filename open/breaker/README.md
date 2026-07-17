
_ftorrent/open/breaker/README.md [open.ftorrent.com](https://open.ftorrent.com)_

_Five guides cover this deployment: [dockerizing Aquatic and configuring the Linux server](../README.md), the [dashboard back end](../gauge/README.md), the **circuit breaker** (this guide), the [dashboard front end](../page/README.md), and the [DHT bootstrap node](../dht/README.md)._

# Public Tracker Circuit Breaker

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.7
> <br>Created: 2026-May
> <br>Last reviewed: 2026-Jul

## What this is and why it exists

Run a public tracker for a while and you learn its first hard truth: you don't control what arrives. Anyone's client, anywhere, can send you anything at any rate, and your only move is to answer or to stay silent — you cannot ask the incoming traffic to slow down and expect it to listen. The web has a polite convention for the moment a server is genuinely past its limits — the NIC's receive backlog saturating, the conntrack table filling, the TLS-handshake budget maxing out: return `503 Service Unavailable` with a `Retry-After` header that tells clients when to come back. In a world where every client read that header and obeyed it, a tracker under strain would fill, return 503s, and the load would ebb.

That is not the world, and the second hard truth follows straight from the first: shedding load doesn't reduce it in the short term — it *raises* it. The busiest open.ftorrent.com has ever been was not a peak announce hour. It was the first few hours after one service — HTTPS over IPv4 — tripped and went to cool-down. We answered every request to it with a `503` and a `Retry-After` of 24 hours, and rather than wait, the whole swarm came back at once, immediately, and kept coming. Because that service terminates TLS, every one of those retries is a *fresh* handshake — the most expensive thing the box does. Switching a service off, for a few hours, makes it briefly the single most demanding thing on the machine.

So why build a circuit breaker at all? Because the short-term storm buys a real long-term win. A tracker that stays unavailable slides down the rankings that clients and public tracker lists keep between them: the reachability probes watching open trackers record the downtime, the lists demote or drop the entry, and clients are handed it less often. The offered load falls — not because anyone honored a header, but by attrition. The breaker plays that longer game on purpose: shed the overload gracefully, absorb the counterintuitive spike, and let the ecosystem's own bookkeeping settle the sustained load back to something the hardware can hold.

Mechanically, that's all the breaker is. It watches the gauge's rolling 24-hour served counts. The tracker runs six **services** in all — three protocols (UDP, HTTP, WebSocket) times two IP versions (v4, v6) — each metered and tripped on its own. When a service exceeds its configured threshold it's paused for 24 hours, and while paused its requests are rejected at the earliest point in the kernel network path: for HTTP and WebSocket, a fast `503 Service Unavailable` from the reverse proxy before the request reaches a tracker; for UDP, an `iptables raw PREROUTING DROP` before the kernel even creates a conntrack entry.

Two design decisions shape the rest:

**Per service, not per protocol.** UDP IPv4 trips independently from UDP IPv6; if one is saturated, the other keeps serving. This nudges dual-stack clients toward whichever IP version still works, and clients that speak multiple protocols toward whichever protocol still works. The asymmetry is the point: under load, we'd rather serve announce-over-UDP-IPv6 than refuse everyone equally.

**Independent layers.** The gauge decides; the breaker enforces. The gauge writes a single file with six timestamps that say "this service may run again at this moment." The breaker reads that file, compares it to the wall clock, and reconciles whatever the proxy and firewall need to look like. Either layer can be replaced without touching the other.

## Two example implementations

The deployment at [open.ftorrent.com](https://open.ftorrent.com/) runs on port 443 with TLS, the modern web's default for HTTPS — encrypting every request in transit and following the same convention as the rest of the web. The consequence is that the server terminates TLS for every new connection, and at high traffic that's the most CPU-expensive thing the box does. The HTTP and WebSocket trackers together handle tens of millions of TLS handshakes per day at this scale (fewer than total announce requests, because HTTP keep-alive and HTTP/2 multiplexing amortize the handshake cost over many requests on the same connection). On modest hardware, the TLS handshake budget is what limits scale for the TCP services.

The first version of this deployment used [nginx](https://nginx.org/) for everything — TLS termination, reverse-proxying to the tracker containers, static hosting of the dashboard, plus the breaker's HTTP/WS gating — with [certbot](https://certbot.eff.org/) for certificate issuance and renewal. nginx is a fine general-purpose web server and the configuration is documented well enough that anyone with similar infrastructure can copy it.

After a while we migrated to a stack better matched to the breaker's update pattern. [HAProxy](https://www.haproxy.org/) exposes a **runtime admin socket** that the breaker can push per-service toggles to on the live process, no config reload — the right shape for state that changes minute-by-minute. HAProxy doesn't do static-file hosting, so we paired it with [Caddy](https://caddyserver.com/) for the dashboard, and replaced certbot with [lego](https://go-acme.github.io/lego/) as a lighter ACME client. HAProxy now terminates TLS, gates per-service via the breaker, and routes tracker traffic (`/announce`, `/scrape`, WebSocket upgrades) directly to the Aquatic containers; Caddy is the catch-all that handles the dashboard's static files and the `/page.json` alias. The breaker's HTTP/WS gating, which used to live in the nginx site config, now lives in a HAProxy runtime **map** that the breaker rewrites and pushes to the live process without a reload. (For why this stack was chosen — and why the original nginx + certbot also cleared the bar — see [Software Selections](https://docs.ftorrent.com/software-selections.html#web-stack).)

Both implementations are working real deployments. The repository keeps both as example sets:

- [`nginx-example/`](nginx-example/) — the nginx + certbot implementation. Frozen as of the migration; if a future nginx release breaks something here we won't notice, because nothing on the deployed side uses it anymore.
- [`haproxy-caddy-example/`](haproxy-caddy-example/) — the HAProxy + Caddy + lego implementation. This matches what's currently deployed at open.ftorrent.com and is the one we'll keep current as the surrounding software evolves.

A reader can follow either example directly, or read the rest of this guide to extract the underlying pattern (the gauge writes intent, the breaker reconciles, the proxy does TLS termination + routing + the actual 503 gating) and adapt it to a different reverse proxy entirely. The pattern is the durable part; the choice of proxy software is the volatile part.

## The pieces

Three files are invariant across both example implementations:

| File | Deploys to | Role |
|---|---|---|
| `breaker.py` | `/usr/local/bin/breaker.py` | Reconciler. Reads `breaker.json`, computes desired per-service state, applies to the reverse proxy and iptables. The reverse-proxy half differs between examples; the structure and the UDP half are the same. |
| `breaker.path` | `/etc/systemd/system/breaker.path` | systemd path unit watching `breaker.json`. Structurally identical between examples (the description comment names the relevant proxy). |
| `breaker.service` | `/etc/systemd/system/breaker.service` | systemd oneshot service running `breaker.py`. Structurally identical between examples (same — description comment differs). |

One file varies by implementation, since each proxy reads its runtime state in its own format:

| File | Deploys to | Role |
|---|---|---|
| `breaker.conf` (nginx) | `/etc/nginx/breaker.conf` | Four nginx `set` directives defining `$service_http4`, `$service_http6`, `$service_ws4`, `$service_ws6` — included by the nginx site config |
| `breaker.map` (HAProxy) | `/etc/haproxy/maps/breaker.map` | Six rows of `service state` — read by HAProxy at start, refreshable at runtime via the admin socket |

And the reverse-proxy site configuration itself, which the breaker doesn't write but which contains the per-service gating logic that reads the volatile file above:

- In `nginx-example/`, the entire reverse-proxy story lives in a single self-contained file: `open.ftorrent.com`. TLS termination, routing, static hosting, and the gating logic all in one place — nginx's per-vhost idiom lets this ship as a complete artifact.
- In `haproxy-caddy-example/`, the same responsibilities are split between HAProxy (TLS termination, breaker gating, tracker routing) and Caddy (static hosting, the `/page.json` alias). The full HAProxy and Caddy configs on the deployed server bundle other domains and so can't ship whole. The "Reading the implementations" section below shows the open.ftorrent.com-relevant excerpts inline.

## Data flow

```
gauge tick (every minute)
  └─ writes /opt/open.ftorrent.com/data/breaker.json
                  │
                  ▼
        systemd .path watches the file
                  │
                  ▼ (PathChanged fires every minute even when content didn't change)
        breaker.service (Type=oneshot, runs as root)
          └─ /usr/local/bin/breaker.py
              │
              ├─ read /opt/open.ftorrent.com/data/breaker.json
              ├─ compute desired_off[service] = (startOn[service] > now)
              ├─ reconcile the reverse-proxy state to match (http4, http6, ws4, ws6)
              │   └─ if changed: rewrite the proxy's state file atomically,
              │      then signal the live process to pick it up
              │      (config reload for nginx; runtime-socket push for HAProxy)
              ├─ iptables  -t raw -C PREROUTING ... → -I or -D as needed (udp4)
              └─ ip6tables -t raw -C PREROUTING ... → -I or -D as needed (udp6)
```

The wall clock plus the file are the breaker's two inputs. Live reverse-proxy state plus live iptables rules are the breaker's "what's currently in effect" memory — there's no separate cache file anywhere. The script is wholly self-comparing.

## breaker.json: the gauge's intent

The gauge writes this file every minute to `/opt/open.ftorrent.com/data/breaker.json`:

```json
{
	"startOn": {
		"udp4":  0,
		"udp6":  0,
		"http4": 1779043200000,
		"http6": 0,
		"ws4":   0,
		"ws6":   0
	}
}
```

Six entries. Each value is an epoch-ms timestamp. Semantics:

- `now > startOn[service]` → service should be running
- `now < startOn[service]` → service should be paused
- `0` means "no order to be off has ever been issued" — a release that expired in 1970, permanently in the past

The file is **not** under `data/public/`, so the reverse proxy never serves it. Knowing exactly when a cooled service returns would let clients schedule synchronized retries against the very moment we open again — the operational opposite of what a circuit breaker should produce. Keeping the file private prevents that.

The gauge rewrites `breaker.json` every minute even when the timestamps don't change. The unchanging rewrite is the heartbeat that fires `breaker.path` — the wall clock is always advancing, so the breaker must re-evaluate against the latest moment regardless of whether the gauge's intent changed. A service's `startOn` timestamp doesn't get reset to 0 when it expires; it just becomes a past time, and the wall clock crossing it is the release signal.

## The reconciler

`breaker.py` is small and deliberately defensive. Five characteristics, true of both example implementations:

**Self-comparing.** No cache file. The current contents of the proxy's volatile state file and the current iptables rule presence are the breaker's "what did I last apply" memory. If someone hand-edits either, the next breaker tick detects the drift and reconverges to what `breaker.json` says.

**Idempotent.** The script can run a thousand times in a row with no input change and produce zero side effects. Each step is gated on a "is the live state already what we want?" check before acting.

**Quiet on no-change.** `breaker.path` fires roughly 1440 times a day (once per gauge tick). The script returns silently when nothing's changed. The journal entries you see in `journalctl -t breaker` are an audit log of real state transitions, not 1440 daily "nothing happened" lines.

**Fail-closed on bad input.** If `breaker.json` is missing or malformed, the script logs once and exits without touching the proxy or iptables. The previous good state stays in effect rather than reverting to defaults — bad input never moves the system.

**Concurrency-safe.** iptables calls use `-w` (wait for the xtables lock) so a concurrent network reconfiguration elsewhere on the host doesn't crash the breaker. The proxy's runtime API is called only after the on-disk state file has been atomically updated, so a crash mid-update can't leave the on-disk and in-memory states inconsistent.

## What the reverse-proxy layer needs to do

This section is framework-agnostic. The reverse-proxy layer in front of the trackers must satisfy the list below regardless of which software implements it. The two example directories show two ways to satisfy this list. A reader using a different reverse proxy entirely — say Caddy for everything, or Envoy, or HAProxy with embedded Lua — can read this section as a checklist and adapt their existing config.

**1. Terminate TLS on port 443.** Public clients connect over HTTPS. The server terminates the encrypted connection so the tracker containers serve plaintext HTTP behind the proxy on internal ports unreachable from the internet. Certificate issuance and renewal is automated (certbot, lego, or any other ACME client).

**2. Route HTTP requests by path.** Requests to `/announce` and `/scrape` go to the HTTP tracker container (default `127.0.0.1:8081`). Requests to `/page.json` serve the gauge's output file aliased from `/opt/open.ftorrent.com/data/public/page.json`. Everything else not handled by another route falls through to static-file serving.

**3. Route WebSocket upgrades, preserving the client's IP family.** Connections that include an `Upgrade: websocket` header go to the WebSocket tracker container — IPv4 clients dialed to `127.0.0.1:8082`, IPv6 clients to the container's pinned IPv6 address (`[fd00:cafe:2::82]:8082` in the Aquatic guide's example compose). The split matters because aquatic_ws has no `X-Forwarded-For` support and classifies each peer's IP family from the accepted TCP socket, so a proxy that dials every client over IPv4 silently counts every WebSocket peer as `ws4` — see the [Aquatic guide](../README.md)'s reverse proxy section. The v4-versus-v6 discrimination this needs is the same one requirement 6 provides for gating.

**4. Serve the dashboard as static files.** The Vue front end's compiled output lives at `/opt/open.ftorrent.com/static`. Non-API, non-WS requests serve files from there.

**5. Read four per-service toggles for HTTP/WS gating.** The proxy needs to be able to read four runtime toggles — `http4`, `http6`, `ws4`, `ws6` — each `on` or `off`. When a toggle is `off`, requests to that service return `503 Service Unavailable` with `Retry-After: 86400`, before the request reaches the tracker. The toggles must be updatable from outside the proxy without a full config reload, so the breaker can flip them every minute without restarting anything. The two example implementations show two natural ways to satisfy this: nginx variables set from an `include`-d file (reloaded with `nginx -s reload`), or a HAProxy map file refreshable via the runtime admin socket.

**6. Discriminate v4 from v6 per connection.** The proxy must know whether a given connection arrived over IPv4 or IPv6 — the per-family gating and the WebSocket family-preserving dial both depend on it. Different proxies express this differently, and the obvious-looking way in HAProxy is wrong. In nginx, test `$server_addr` for a colon. In HAProxy, the intuitive pair — `acl is_v4 src 0.0.0.0/0` versus `acl is_v6 src ::/0` — does **not** work: HAProxy compares across address families by converting IPv4 sources to their v4-mapped form (`::ffff:a.b.c.d`) and widens `/0` patterns across families, so each of those "catch-alls" matches **both** families (verified empirically on HAProxy 2.8.16 in July 2026). The one precise classifier is the v4-mapped /96 range — `acl is_v4 src ::ffff:0:0/96` — which IPv4 clients land inside after conversion and native IPv6 sources cannot; IPv6 conditions are written as its negation, `!is_v4`.

**7. The `Retry-After: 86400` choice is structural.** Always 24 hours, regardless of the actual remaining cool-down. Many clients ignore the header outright and retry immediately — the short-term storm the breaker is built to weather — but the value still matters for the ones that do read it. If it gave the real remaining time, every client that received a 503 in the same minute would schedule its retry for the same moment, the exact second the service unpauses: a synchronized hammer at the worst possible time. A uniform 24-hour value sidesteps that. Well-behaved clients spread their retries across the day by their own backoff logic, and the header still tells them "this is a paused service, not a permanently broken one" — useful semantic signal — without coordinating their return.

## Reading the two implementations

The two example directories satisfy the requirements above in two different ways. The interesting parts of each are below; the full nginx site config ships in `nginx-example/`, while the HAProxy and Caddy snippets are quoted here from the live configs on the deployed server (the full files there bundle other domains and so don't ship in this repo).

### Implementation A — nginx

The volatile file the breaker rewrites is four nginx `set` directives, included at the top of the server block:

```
set $service_http4 "on";
set $service_http6 "on";
set $service_ws4   "on";
set $service_ws6   "on";
```

Inside `location ~ ^/(announce|scrape)`, the gate reads the toggle by IP family and returns 503 if off. nginx doesn't allow nested `if`, so the family selector lives in two mutually-exclusive ifs that set `$http_state`, then a third `if` gates on it:

```nginx
set $http_state "ignore";
if ($server_addr !~ ":") { set $http_state $service_http4; }
if ($server_addr  ~ ":") { set $http_state $service_http6; }
if ($http_state = "off") {
    add_header Retry-After 86400 always;
    return 503;
}
proxy_pass http://127.0.0.1:8081;
```

The WebSocket gate inside `location /` uses a small encoding trick — combining the upgrade-header check and the toggle into a single equality test, again because nested `if` isn't allowed:

```nginx
set $ws_state "ignore";
if ($server_addr !~ ":") { set $ws_state $service_ws4; }
if ($server_addr  ~ ":") { set $ws_state $service_ws6; }
set $ws_action "${http_upgrade}-${ws_state}";
if ($ws_action = "websocket-off") {
    add_header Retry-After 86400 always;
    return 503;
}

if ($http_upgrade = "websocket") {
    proxy_pass http://127.0.0.1:8082;
    break;
}
try_files $uri $uri/ =404;
```

Static-file requests fall through both gates untouched and reach `try_files`. The full file in [`nginx-example/open.ftorrent.com`](nginx-example/open.ftorrent.com) shows the TLS termination (certbot-managed), the `/page.json` alias, and the port-80 redirect to HTTPS.

### Implementation B — HAProxy + Caddy

HAProxy reads the volatile state from a map file with one row per service:

```
udp4 on
udp6 on
http4 on
http6 on
ws4 on
ws6 on
```

The `https` frontend defines reusable ACLs — route detection, family detection, per-service map lookups — and then expresses the four 503 returns by ANDing one ACL from each group. Family detection is the part to pause on: an earlier revision of this config used the intuitive `src 0.0.0.0/0` / `src ::/0` pair, which silently matches both families (requirement 6 above explains the mechanism) — family-blind gating that sat dormant here until the WebSocket family-split work of July 2026 forced an empirical test. The map lookup uses HAProxy's `map(...)` converter: the input is the literal service key, passed through the converter against `breaker.map`, matched if the looked-up value equals `"off"`:

```haproxy
acl is_open hdr_dom(host) -i open.ftorrent.com
acl is_announce_or_scrape path_beg /announce /scrape
acl is_websocket hdr(Upgrade) -i websocket

# Client family. HAProxy compares across address families by mapping
# IPv4 into IPv6 (::ffff:a.b.c.d), and widens /0 patterns across
# families — so bare `src 0.0.0.0/0` and `src ::/0` each match BOTH
# families. The v4-mapped /96 range is the one precise test: IPv4
# clients land inside it, native IPv6 clients cannot. IPv6 conditions
# are its negation, `!is_v4`.
acl is_v4 src ::ffff:0:0/96

acl breaker_http4_off str(http4),map(/etc/haproxy/maps/breaker.map) -m str off
acl breaker_http6_off str(http6),map(/etc/haproxy/maps/breaker.map) -m str off
acl breaker_ws4_off   str(ws4),map(/etc/haproxy/maps/breaker.map) -m str off
acl breaker_ws6_off   str(ws6),map(/etc/haproxy/maps/breaker.map) -m str off

http-request return status 503 hdr "Retry-After" "86400" if is_open is_announce_or_scrape is_v4 breaker_http4_off
http-request return status 503 hdr "Retry-After" "86400" if is_open is_announce_or_scrape !is_v4 breaker_http6_off
http-request return status 503 hdr "Retry-After" "86400" if is_open is_websocket is_v4 breaker_ws4_off
http-request return status 503 hdr "Retry-After" "86400" if is_open is_websocket !is_v4 breaker_ws6_off
```

`http-request return` (HAProxy 2.4+) is the synchronous reply path — it produces a complete response without touching any backend, which is exactly the semantics we want for a 503 from a paused service.

Beyond the gate, the same frontend routes tracker traffic directly to the Aquatic backends; anything else falls through to Caddy as the default. WebSocket upgrades pick their backend by client family — the family-preserving dial aquatic_ws needs (requirement 3):

```haproxy
use_backend aquatic_http if is_open is_announce_or_scrape
use_backend aquatic_ws6  if is_open is_websocket !is_v4
use_backend aquatic_ws   if is_open is_websocket
default_backend caddy

backend aquatic_http
    option h1-case-adjust-bogus-server
    http-request set-header X-Forwarded-For %[src]
    http-request set-header X-Real-IP %[src]
    http-request set-header X-Forwarded-Proto https
    server http1 127.0.0.1:8081

backend aquatic_ws
    # IPv4 clients' WebSocket upgrades — the localhost publish.
    server ws1 127.0.0.1:8082

backend aquatic_ws6
    # IPv6 clients' WebSocket upgrades — the ws container's pinned IPv6
    # address, matching the Aquatic guide's compose file. A loopback
    # publish can't carry this leg: with the userland proxy disabled,
    # published ports aren't reachable over ::1.
    server ws1 [fd00:cafe:2::82]:8082

backend caddy
    server caddy /run/caddy/caddy.sock
```

One address note: the `aquatic_ws6` dial matches the [Aquatic guide](../README.md)'s example compose, which pins only the ws container and deliberately picks a high address (`fd00:cafe:2::82`) that Docker's dynamic allocator — handing out low addresses first — can't collide with. If you followed the guide, the block above works as written. The deployed open.ftorrent.com server does it differently: it pins *every* container in the compose project low in the subnet (`::2` udp, `::3` ws, `::4` http, and so on, with matching IPv4 fourth octets) because its firewall rules target containers by address, so its real dial is `[fd00:cafe:2::3]:8082` — the one value in these excerpts adapted from the live config. Either way, dial whatever your ws container's pinned address actually is.

The `h1-case-adjust-bogus-server` directive activates three header-case mappings declared in `global`, restoring the original casing on outgoing forwarding headers:

```haproxy
global
    h1-case-adjust x-forwarded-for X-Forwarded-For
    h1-case-adjust x-real-ip X-Real-IP
    h1-case-adjust x-forwarded-proto X-Forwarded-Proto
```

HAProxy 2.x normalizes outgoing HTTP/1 header names to lowercase on the wire, matching HTTP/2 conventions. Aquatic's HTTP tracker is configured with `reverse_proxy_ip_header_name = "X-Forwarded-For"` and does a case-sensitive lookup of that exact string. Without these mappings, HAProxy emits `x-forwarded-for`, Aquatic's lookup fails to find it, the aquatic-http worker thread panics fatally on the first request, and the container enters a Docker restart loop. We hit this on cutover and lost ~38 minutes of HTTP-tracker uptime before diagnosing it. The three mappings restore the conventional casing for those specific headers on the backend's outgoing requests — mandatory plumbing for this combination of HAProxy 2.x and aquatic-http, not polish.

Caddy receives traffic from HAProxy over a unix socket. For open.ftorrent.com it handles two things: the static dashboard, and the `/page.json` alias to the gauge's writable data directory. The relevant block of the Caddyfile is straightforward:

```caddyfile
http://open.ftorrent.com {
    bind unix//run/caddy/caddy.sock

    @page_json path /page.json
    handle @page_json {
        root * /opt/open.ftorrent.com/data/public
        file_server
    }

    handle {
        root * /opt/open.ftorrent.com/static
        file_server
    }
}
```

Two globals at the top of the Caddyfile are non-obvious for a Caddy-behind-HAProxy setup:

```caddyfile
{
    auto_https off
    admin off
}
```

`auto_https off` is needed because HAProxy is the sole TLS terminator — without it, Caddy starts ACME runs for every site it knows about, all failing because HAProxy holds 443. `admin off` disables Caddy's remote admin API for hardening; the breaker doesn't need it (the runtime updates go through HAProxy's admin socket, not Caddy's).

The rest of the Caddy usage for this site is garden-variety `file_server` and `handle` — the [Caddy documentation](https://caddyserver.com/docs/) covers it well, and we don't duplicate it here.

## UDP enforcement: raw PREROUTING DROP

UDP traffic bypasses the reverse proxy entirely — Docker's DNAT rule in PREROUTING NAT routes inbound UDP packets straight to the `aquatic_udp` container. To pause UDP without touching that path, the breaker inserts a DROP rule in the **raw table, PREROUTING chain**:

```
iptables  -t raw -I PREROUTING -p udp --dport 443 \
          -m comment --comment "breaker-udp" -j DROP

ip6tables -t raw -I PREROUTING -p udp --dport 443 \
          -m comment --comment "breaker-udp" -j DROP
```

Three reasons to drop here:

- **Earliest filter point in the kernel.** Packet is freed before any further processing.
- **Before conntrack creates an entry.** The Aquatic guide's sysctl tuning sized the conntrack table for steady-state load. A flood during a cool-down doesn't churn that table at all — relieving exactly the pressure that's most likely a contributor to the trip.
- **Doesn't interact with Docker's chains.** Docker manages the NAT and filter tables; raw is ours alone, no rule-ordering collision.

The `-m comment --comment "breaker-udp"` is structural, not cosmetic. `iptables -C` requires an exact match including the comment, so the comment makes our rule uniquely identifiable when the script asks "is the breaker-udp drop currently in effect?"

DROP, not REJECT — REJECT for UDP would emit ICMP unreachables back to every dropped client, costing outbound bandwidth at exactly the wrong moment. DROP is silent. BitTorrent clients retry on timeout regardless.

The UDP half of the breaker is identical between the two example implementations. The only difference is which lines of `breaker.py` carry it (it's the same function in both).

## Dropping cleartext announces on port 80

The breaker's gating on `:443` and the UDP drops above both shed *our own* overload — legitimate traffic, paused when a service exceeds its capacity. Port 80 has a related but separate edge behavior, not driven by the breaker at all, that shares the same goal: keeping the tracker from spending anything on traffic it should never serve.

The tracker is published on three transports only — UDP, HTTPS, and WebSocket over TLS — and deliberately offers no plain-HTTP tracker. Cleartext HTTP serves neither thing a client might want from it: it is no more private than UDP and less efficient than either alternative, so a client that needs a secure channel should announce over HTTPS and one that doesn't should announce over UDP. HTTP sits in the middle, worse at both, and we don't run it.

Even so, a steady trickle of clients announce to a cleartext `http://open.ftorrent.com/announce` — almost always because a stale public tracker list carries the wrong scheme for a tracker that only ever spoke HTTPS. The correct, standards-compliant response is a `301` redirect to the `https://` URL, and any client that read it would reconnect on the right port. These clients don't read it. They ignore the redirect the same way they ignore the breaker's `Retry-After: 86400`, and re-announce to port 80 immediately, indefinitely.

This is worth internalizing if you run your own fork, because you will meet it too: doing the correct thing to web standards accomplishes nothing when the client never looks at the response. Worse, a valid `301` is actively counterproductive — a well-formed redirect reads to such a client as "the tracker is alive," which is the signal that keeps it coming back. The only thing that gets it to stop is the absence of an answer. Drop the connection with no response, and two things follow: the client's own dead-tracker backoff eventually retires the URL, and the stale lists carrying the wrong `http://` entry demote or remove it as "down" — the right outcome for a URL that was never correct.

So on port 80 we drop `/announce` and `/scrape` outright, with no reply, while leaving the ordinary port-80-to-HTTPS redirect in place for real browser traffic — someone who types the bare domain still lands on the site. In HAProxy, in the `:80` frontend:

```haproxy
frontend http
    bind :80,:::80

    # ACME HTTP-01 challenges must reach whatever issues your certificates,
    # so route that path to your ACME client's listener before anything else.
    use_backend acme_challenge if { path_beg /.well-known/acme-challenge/ }

    # Cleartext announces and scrapes: drop the connection with no reply.
    # These clients ignore a redirect and re-announce regardless, so only the
    # absence of a response makes them back off. The set-log-level line is
    # load-bearing: a silently-dropped session is deny-class, which
    # dontlog-normal does not suppress, so without it every drop writes a log
    # line.
    http-request set-log-level silent if { path_beg /announce /scrape }
    http-request silent-drop if { path_beg /announce /scrape }

    # Everyone else → https. The `unless` keeps the ACME path out of the
    # redirect: HAProxy evaluates http-request rules before use_backend
    # selection, so without it this rule fires first and the challenge never
    # reaches the backend above.
    http-request redirect scheme https code 301 unless { path_beg /.well-known/acme-challenge/ }
```

`silent-drop` makes HAProxy's side of the connection vanish without sending the client anything to parse, and leaves no `TIME_WAIT` behind on the server. This is the form running in production; dropping these requests cut the cleartext arrival rate steeply within minutes and held it there.

The nginx equivalent is `return 444` — nginx's non-standard code for "close the connection and send no response." A regex `location` outranks the `/` prefix, so the tracker paths match and drop before the redirect is reached:

```nginx
server {
    listen 80;
    listen [::]:80;
    server_name open.ftorrent.com;

    # ACME HTTP-01 challenges, served locally and never redirected.
    location /.well-known/acme-challenge/ {
        root /var/www/certbot;
    }

    # Cleartext announces and scrapes: 444 closes with no response — nginx's
    # equivalent of a silent drop.
    location ~ ^/(announce|scrape) {
        return 444;
    }

    # Everyone else → https.
    location / {
        return 301 https://$host$request_uri;
    }
}
```

We run the HAProxy form in production, so its behavior is the measured one; `return 444` is the direct nginx analog, shown here for readers on that stack.

## Deployment

The breaker sits on top of the rest of the stack. It assumes the [Aquatic guide](../README.md) (tracker containers, kernel tuning), the [gauge](../gauge/README.md) (writing `breaker.json` to the bind-mounted data directory), the [page front-end](../page/README.md) (static files at `/opt/open.ftorrent.com/static/`), and your chosen reverse-proxy stack are all in place.

**Runtime requirements** that aren't installed by default on minimal Debian/Ubuntu:

- **Python 3.10 or newer** for `breaker.py` (the script uses PEP 604 union types like `dict[str, int] | None`). Ubuntu 22.04+ and Debian 12+ already satisfy this.
- **`jq`** is only needed for the optional test-trip recipe below. `sudo apt install jq` if you'd like to run it.

The breaker installs in the same shape regardless of which example you follow:

1. Copy `breaker.py`, `breaker.path`, and `breaker.service` from your chosen example directory to their target paths (the table above).
2. Copy the example's proxy state file (`breaker.conf` for nginx, `breaker.map` for HAProxy) to the location the proxy expects.
3. Integrate the proxy-side gating into your reverse-proxy config. For nginx, that means adding the `include /etc/nginx/breaker.conf;` line and the two `if`-gates from `nginx-example/open.ftorrent.com` into your own site config. For HAProxy, that means adding the four `acl breaker_*_off` definitions and the four `http-request return status 503` lines from the snippet above into your `https` frontend.
4. `chmod 755 /usr/local/bin/breaker.py`, then `systemctl daemon-reload && systemctl enable --now breaker.path`.
5. The first run waits for the next gauge write of `breaker.json`. After that the breaker keeps the reverse proxy and iptables in sync with the gauge's decisions automatically.

If you fork this repo for your own tracker, substitute your domain for `open.ftorrent.com` in `breaker.py`'s path constants and anywhere your proxy config references it. The other deployable files are domain-agnostic.

### Verifying it works

```bash
# Audit log of breaker activity (only real state transitions appear here)
sudo journalctl -t breaker --since "1 hour ago"

# Path unit status
systemctl status breaker.path

# Current UDP DROP rules (present means cooled)
sudo iptables  -t raw -L PREROUTING -n -v --line-numbers | grep breaker-udp
sudo ip6tables -t raw -L PREROUTING -n -v --line-numbers | grep breaker-udp
```

Implementation-specific verification:

```bash
# nginx: the actual on/off state being enforced
cat /etc/nginx/breaker.conf

# HAProxy: the live map state via the runtime admin socket
echo "show map /etc/haproxy/maps/breaker.map" | socat - /run/haproxy/admin.sock
```

When a service trips, the journal records the apply; the relevant file shows the toggle flipped to `"off"`; and (for udp4/udp6) the iptables rule appears with the `breaker-udp` comment. Packet/byte counters on the rule increment as drops accumulate — visual evidence the breaker is doing work.

### Inducing a test trip

To exercise the breaker without waiting for real load, hand-edit `breaker.json` to set one service's `startOn` to a future moment:

```bash
# Pause http4 for one minute (timestamp = now + 60_000 ms)
sudo jq '.startOn.http4 = (now * 1000 + 60000 | floor)' \
    /opt/open.ftorrent.com/data/breaker.json \
    > /tmp/breaker.json.new
sudo mv /tmp/breaker.json.new /opt/open.ftorrent.com/data/breaker.json
```

The next gauge tick preserves the timestamp (the gauge never bumps an existing `startOn` forward), the path unit fires, the breaker computes `desired_off=true` for http4, regenerates the proxy state, and applies it. A minute later when the wall clock crosses the timestamp, the next tick's reconcile flips http4 back on automatically. To force-release earlier, edit `breaker.json` and set the timestamp to `0`.

## Tuning thresholds

The trip thresholds live in the gauge, not the breaker: one measured announces-per-day ceiling per service, in the `SERVICE_BREAKERS` dict in [`open/gauge/src/gauge.py`](../gauge/src/gauge.py), with a comment above it that explains how the values were chosen. The reasoning, in short: the per-protocol costs are wildly asymmetric — a UDP announce is cheap, while an HTTPS announce is dominated by the TLS handshake, the resource that actually limits scale on modest hardware (see "Two example implementations" above), and WebSocket rides that same TLS budget, so all four TCP cells share one ceiling. The values are sized to leave real CPU headroom even in the worst case where every protocol sits at its ceiling at once. And within each protocol, v4 and v6 carry the same value on purpose: v6 is a small fraction of the traffic, so under real load a v4 cell trips first while its v6 sibling keeps serving — the tracker's answer to congestion becomes a nudge toward IPv6.

Read the actual numbers in the code — they live only there, so they can be retuned without any prose to keep in sync — and tune them to your own hardware once you've watched a few real trips and have a sense of your own ceilings. The cool-down duration, how long a tripped service stays paused, is another constant in the same file (`COOL_DURATION`, 24 hours). Changes to either take effect when the gauge container is rebuilt and restarted.
