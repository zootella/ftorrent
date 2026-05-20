
_ftorrent/open/breaker/README.md [open.ftorrent.com](https://open.ftorrent.com)_

_Four guides cover this deployment: [dockerizing Aquatic and configuring the Linux server](../README.md), the [dashboard back end](../gauge/README.md), the **circuit breaker** (this guide), and the [dashboard front end](../page/README.md)._

# Public Tracker Circuit Breaker

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.7
> <br>Created: 2026-May
> <br>Last reviewed: 2026-May

## What this is and why it exists

A public tracker that grows past the capacity of its hardware — or hits a sustained spike that saturates the NIC RX backlog, fills the conntrack table, or overwhelms the TLS handshake budget — shouldn't fall over. It should shed load gracefully and tell new clients to come back later. That's the circuit breaker.

The breaker watches the gauge's rolling 24-hour served counts. The tracker has six **services** in total — three protocols (UDP, HTTP, WebSocket) multiplied by two IP versions (v4, v6) — and each one is metered and tripped on its own. When any service exceeds its configured threshold, that service is paused for 24 hours. While paused, requests against it are rejected at the earliest possible point in the kernel network path — for HTTP and WS, a fast `503 Service Unavailable` from nginx before the request reaches a tracker; for UDP, an `iptables raw PREROUTING DROP` before the kernel even creates a conntrack entry.

Two design decisions shape the rest:

**Per service, not per protocol.** UDP IPv4 trips independently from UDP IPv6; if one is saturated, the other keeps serving. This nudges dual-stack clients toward whichever IP version still works, and clients that speak multiple protocols toward whichever protocol still works. The asymmetry is the point: under load, we'd rather serve announce-over-UDP-IPv6 than refuse everyone equally.

**Independent layers.** The gauge decides; the breaker enforces. The gauge writes a single file with six timestamps that say "this service may run again at this moment." The breaker reads that file, compares it to the wall clock, and reconciles whatever the proxy and firewall need to look like. Either layer can be replaced without touching the other.

## The pieces

Five files. Each is verbatim what runs in production:

| File in this repo | Deploys to | Role |
|---|---|---|
| `breaker.py` | `/usr/local/bin/breaker.py` | Reconciler. Reads `breaker.json`, computes desired per-service state, applies to nginx and iptables. |
| `breaker.path` | `/etc/systemd/system/breaker.path` | systemd path unit watching `breaker.json`. |
| `breaker.service` | `/etc/systemd/system/breaker.service` | systemd oneshot service running `breaker.py`. |
| `breaker.conf` | `/etc/nginx/breaker.conf` | Volatile nginx include. Four `set $service_* "on\|off";` lines, rewritten by `breaker.py`. |
| `open.ftorrent.com` | `/etc/nginx/sites-available/open.ftorrent.com` | Reverse-proxy site config containing the gates that enforce `$service_*`. |

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
              ├─ render four set-lines, compare to /etc/nginx/breaker.conf
              │   └─ if different: atomic rename in, nginx -t, nginx -s reload
              ├─ iptables  -t raw -C PREROUTING ... → -I or -D as needed (udp4)
              └─ ip6tables -t raw -C PREROUTING ... → -I or -D as needed (udp6)
```

The wall clock plus the file are the breaker's two inputs. Live nginx config plus live iptables rules are the breaker's "what's currently in effect" memory — there's no separate cache file anywhere. The script is wholly self-comparing.

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

The file is **not** under `data/public/`, so nginx never serves it. Knowing exactly when a cooled service returns lets clients schedule synchronized retries against the very moment we open again — the operational opposite of what a circuit breaker should produce. Keeping the file private prevents that.

The gauge rewrites `breaker.json` every minute even when the timestamps don't change. The unchanging rewrite is the heartbeat that fires `breaker.path` — the wall clock is always advancing, so the breaker must re-evaluate against the latest moment regardless of whether the gauge's intent changed. A service's `startOn` timestamp doesn't get reset to 0 when it expires; it just becomes a past time, and the wall clock crossing it is the release signal.

## The reconciler

`breaker.py` is small and deliberately defensive. Five characteristics:

**Self-comparing.** No cache file. The current contents of `/etc/nginx/breaker.conf` and the current iptables rule presence are the breaker's "what did I last apply" memory. If someone hand-edits either, the next breaker tick detects the drift and reconverges to what `breaker.json` says.

**Idempotent.** The script can run a thousand times in a row with no input change and produce zero side effects. Each step is gated on a "is the live state already what we want?" check before acting.

**Quiet on no-change.** `breaker.path` fires roughly 1440 times a day (once per gauge tick). The script returns silently when nothing's changed. The journal entries you see in `journalctl -t breaker` are an audit log of real state transitions, not 1440 daily "nothing happened" lines.

**Fail-closed on bad input.** If `breaker.json` is missing or malformed, the script logs once and exits without touching nginx or iptables. The previous good state stays in effect rather than reverting to defaults — bad input never moves the system.

**Concurrency-safe.** iptables calls use `-w` (wait for the xtables lock) so a concurrent network reconfiguration elsewhere on the host doesn't crash the breaker.

## UDP enforcement: raw PREROUTING DROP

UDP traffic bypasses nginx entirely — Docker's DNAT rule in PREROUTING NAT routes inbound UDP packets straight to the `aquatic_udp` container. To pause UDP without touching that path, the breaker inserts a DROP rule in the **raw table, PREROUTING chain**:

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

## HTTP/WS enforcement: nginx

> This section describes the part of the breaker that depends on which HTTP/WS proxy is in use. Everything above this section — the gauge, `breaker.json`, the systemd plumbing, the reconciler pattern, UDP enforcement — is independent of the proxy choice. When this deployment moves to a different HTTP/WS proxy in a future iteration, this section will be rewritten to describe the equivalents while the rest of the breaker stays unchanged.

The nginx site config defines four runtime variables at the top of the server block via an `include`:

```nginx
include /etc/nginx/breaker.conf;
```

`/etc/nginx/breaker.conf` (the *volatile* file the breaker rewrites) holds only four lines:

```
set $service_http4 "on";
set $service_http6 "on";
set $service_ws4   "on";
set $service_ws6   "on";
```

The reverse-proxy gates inside the relevant location blocks read these variables. Inside `location ~ ^/(announce|scrape)`:

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

The two `if` blocks select the right family's toggle based on whether `$server_addr` (the local address the connection landed on) contains a colon. nginx's separate `listen 443 ssl;` and `listen [::]:443 ssl;` keep the IPv4 and IPv6 sockets independent, so each connection lands cleanly on one family.

The WS gate inside `location /` uses a slight twist because nginx doesn't allow nested `if`. The check needs to combine "is this a WebSocket upgrade?" AND "is the WS toggle off?" — encoded as a single equality test:

```nginx
set $ws_state "ignore";
if ($server_addr !~ ":") { set $ws_state $service_ws4; }
if ($server_addr  ~ ":") { set $ws_state $service_ws6; }
set $ws_action "${http_upgrade}-${ws_state}";
if ($ws_action = "websocket-off") {
    add_header Retry-After 86400 always;
    return 503;
}
```

Static-file requests through the same location fall through this gate untouched — only WebSocket upgrades against a cooled service return 503.

### The Retry-After: 86400 choice

503 responses for cooled services carry an `add_header Retry-After 86400 always;` — always 24 hours, regardless of the actual remaining cool-down. This is deliberate.

If the header said "retry in N seconds" where N is the real remaining time, every client receiving a 503 in the same minute would schedule its retry for the same moment — the exact second when the service unpauses. That's a synchronized retry hammer at the worst possible time.

A uniform 24-hour value sidesteps this. Clients distribute their retries across the day, by their own backoff logic. The header still tells well-behaved clients "this is a paused service, not a permanently broken one" — useful semantic signal — without coordinating their return.

## Deployment

The breaker sits on top of the rest of the stack — it assumes the [Aquatic guide](../README.md) (tracker containers, nginx, certbot, kernel tuning), the [gauge](../gauge/README.md) (writing `breaker.json` to the bind-mounted data directory), and the [page front-end](../page/README.md) (static files at `/opt/open.ftorrent.com/static/`) are all in place. The four pieces together form the running deployment at [open.ftorrent.com](https://open.ftorrent.com).

**Runtime requirements** that aren't installed by default on minimal Debian/Ubuntu:
- **Python 3.10 or newer** for `breaker.py` (the script uses PEP 604 union types like `dict[str, int] | None`). Ubuntu 22.04+ and Debian 12+ already satisfy this.
- **`jq`** is only needed for the optional test-trip recipe below. `sudo apt install jq` if you'd like to run it.

**About the nginx site config we ship.** `open.ftorrent.com` is the full production site config, not a breaker-only fragment. It combines five roles in one file: TLS termination (certbot-managed), serving the [page front-end](../page/README.md) as static files from `/opt/open.ftorrent.com/static`, aliasing the gauge's output at `/page.json`, proxying `/announce|/scrape` and WebSocket upgrades to the Aquatic containers, and the breaker enforcement gates on those proxy paths. If you're following all four guides, drop it in. If you're using the breaker without the page front-end or with a different static-host arrangement, integrate the three breaker-relevant additions (`include /etc/nginx/breaker.conf;` at the top of the server block, and the two `if`-gates inside the HTTP and WS location blocks) into your own site config instead.

**Install:**

```bash
# Files in place
sudo cp open/breaker/breaker.py /usr/local/bin/breaker.py
sudo chmod 755 /usr/local/bin/breaker.py

sudo cp open/breaker/breaker.path /etc/systemd/system/
sudo cp open/breaker/breaker.service /etc/systemd/system/

sudo cp open/breaker/breaker.conf /etc/nginx/breaker.conf

# Reverse-proxy config (replaces the existing site file)
sudo cp open/breaker/open.ftorrent.com /etc/nginx/sites-available/open.ftorrent.com

# Validate and reload nginx
sudo nginx -t && sudo systemctl reload nginx

# Enable the path unit so it survives reboots and starts immediately
sudo systemctl daemon-reload
sudo systemctl enable --now breaker.path
```

The path unit triggers a first reconcile on the next gauge write of `breaker.json`. That reconcile checks live state against `breaker.json`; if everything's already in agreement (initial install, all toggles on, no iptables rules present), no journal log is emitted.

If you fork this repo for your own tracker, substitute your domain for `open.ftorrent.com` in `breaker.py`'s path constants and in the site config's `server_name`, `root`, and certbot certificate paths. The other four files (`breaker.path`, `breaker.service`, `breaker.conf`, the nginx site's structural directives) are domain-agnostic.

### Verifying it works

```bash
# Audit log of breaker activity (only real state transitions appear here)
sudo journalctl -t breaker --since "1 hour ago"

# Path unit status
systemctl status breaker.path

# Current nginx include — the actual on/off state being enforced
cat /etc/nginx/breaker.conf

# Current UDP DROP rules (present means cooled)
sudo iptables  -t raw -L PREROUTING -n -v --line-numbers | grep breaker-udp
sudo ip6tables -t raw -L PREROUTING -n -v --line-numbers | grep breaker-udp
```

When a service trips, the journal records the apply, `breaker.conf` shows the toggle flipped to `"off"`, and (for udp4/udp6) the iptables rule appears with the `breaker-udp` comment. Packet/byte counters on the rule increment as drops accumulate — visual evidence the breaker is doing work.

To induce a test trip without waiting for real load:

```bash
# Pause http4 for one minute (timestamp = now + 60_000 ms)
sudo jq '.startOn.http4 = (now * 1000 + 60000 | floor)' \
    /opt/open.ftorrent.com/data/breaker.json \
    > /tmp/breaker.json.new
sudo mv /tmp/breaker.json.new /opt/open.ftorrent.com/data/breaker.json
```

The next gauge tick will preserve the timestamp (the gauge never bumps an existing `startOn` forward), the path unit will fire, the breaker will compute `desired_off=true` for http4, regenerate `breaker.conf`, reload nginx, and journal an apply. A minute later when the wall clock crosses the timestamp, the next tick's reconcile flips http4 back on automatically. To force-release earlier, edit `breaker.json` and set the timestamp to `0`.

## Tuning thresholds

The trip thresholds live in the gauge, not the breaker — see [`open/gauge/src/gauge.js`](../gauge/src/gauge.js):

```js
const serviceBreaker = 150_000_000  // 24h served, applied to each of the six services independently

const SERVICES = [
	{ key: 'udp4',  breaker: serviceBreaker },
	{ key: 'udp6',  breaker: serviceBreaker },
	{ key: 'http4', breaker: serviceBreaker },
	{ key: 'http6', breaker: serviceBreaker },
	{ key: 'ws4',   breaker: serviceBreaker },
	{ key: 'ws6',   breaker: serviceBreaker },
]
```

All six services currently share a single threshold value. The `SERVICES` array's per-row `breaker` field is the seam where you could split them later — give UDP a higher number than HTTP, say, or tighten one IP version that's the bottleneck.

The shipped value is deliberately tuned low so trips are observable in normal operation. Raise it once you've watched a few real trips and have a sense of your hardware's actual ceilings. The 24-hour cool-down duration is also in `gauge.js`:

```js
const COOL_DURATION = 24 * 60 * 60 * 1000
```

Changes to either take effect on the next gauge tick.
