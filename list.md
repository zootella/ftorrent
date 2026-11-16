# list

list is ftorrent's tracker checker, and like good it lives in two places: list.ftorrent.com, which checks from the server and has a page in front of it, and the list panel, the Trackers tab in the desktop client. The two are separate code with a shared purpose and the same libtorrent, the maintainers' 2.1.1 wheel pinned by one hash. The research behind it is the article [Tracking the Trackers](https://docs.ftorrent.com/tracker-lists), and the server half is built and described in [list/README.md](list/README.md).

## The two places

- list.ftorrent.com is a record of uptime: every public tracker, checked every hour over IPv4 and IPv6 separately, with every hour kept and a year of it summed up in the report.
- The list panel is a check of right now: from the user's own machine and connection, when the user asks, so the trackers written into a new torrent are ones that answer this client today.
- Nothing a visitor does on list.ftorrent.com changes anything. There's no submission form; trackers arrive by appearing on the public lists list reads.
- Each place reads the other's output as one source among several, never as the only one, so either keeps working when the other is gone.

We split them on purpose. The server can afford memory, a schedule, and two networks to keep each family honest; the desktop can afford none of that, and doesn't need it, since its question is whether a tracker answers this machine now. What they share is the meaning of an outcome, so a tracker that's up on the page and up in the panel are the same claim.

## The list panel in the desktop client

The panel is a check of right now, and its focus is the reverse of the page's: reply times in milliseconds, which tracker answered this machine first, where the page shows a calendar of hours. It reads and writes the `[trackers]` section of `ftorrent.toml`: `lists`, the lists it fetches; `always`, the trackers it checks beside them; `answered`, the last check's trackers, fastest first; and `checked`, when that check ran. The settings file's essay in `desktop/src/settings.js` covers the rest.

- Keep the engine a roadway: add general libtorrent commands and no application logic. One starts a session that only announces, with no DHT, no peer connections, and no port mapping, since those are session-wide; one announces a fresh random hash to one tracker; one forgets it. Replies come up through the poll the engine already answers.
- Put the decisions in `desktop/src/trackers.js`, with `TrackersPage.vue` thin, the way `connection.js` holds the Connection panel's: which lists, merging them, the outcomes, the order, what to save. Rust fetches the lists through `net_get` and looks up names through `net_resolve`.
- Before a check, show `lists` and `always` for the user to edit. On the click, ask `net_local` whether this machine has a route in each family, say which families it can check, and show results only for those.
- Look up every name at once through `net_resolve` before announcing, since libtorrent looks names up one at a time and a slow one would add its wait to every reply time behind it.
- Keep up to 20 announces in flight, starting the next as each finishes. Judge each tracker on its first try: a tracker that needs two isn't one to put in a torrent. Show each result as it arrives, fastest first, so the list builds in seconds and the user can copy it before the slow ones finish.
- libtorrent announces from every local address the machine has, loopback and link-local among them, so read a tracker's result per family from the addresses that reach the internet and set the rest aside. A WebSocket tracker gets one result, over whichever family the system picks. Extra peers are ignored here.
- Offer Everything, Fastest 5, Fastest 10, Fastest 20, and Random 20, and Copy. Five suits a magnet, which grows about fifty characters with each tracker, and should include a WebSocket tracker for browser peers, which have no DHT; ten suits a torrent file.
- Save `answered` and `checked` after each check. A check where nearly every tracker fails is the network's trouble, a captive portal or blocked UDP, so it saves nothing and says so.
- Treat every tracker address from a list as data: shown through Vue's interpolation, handed to `net_resolve` and the engine as values, and never put into markup.
- Closing a libtorrent session waits out every connection still open, which a dead WebSocket tracker can stretch to minutes, and the engine reads no other line meanwhile. Close the checking session after each check only if libtorrent's binding can close it without blocking; otherwise keep it open until ftorrent quits.
- Build it rough first, the engine's commands and a panel that prints raw results, to learn whether reply alerts carry the local address, whether alerts carry their own timestamps, and whether a session can close without blocking. Check on Windows whether the checking session brings up the firewall prompt.
- In Names and Numbers, add the lists as servers the client contacts only when the user checks.

## Open questions

- Whether to honor BEP 34, the DNS record a tracker's operator publishes to say "don't announce to me." libtorrent doesn't read it, so list.py would look it up itself, which takes a DNS library Python's standard one lacks.
- What the trackers with extra peers are doing with the connections they send their users' clients to. So far it's one tracker, listing the same two servers for every hash.
- Whether 30 days and 95% make a good list, looked at again once there are a few months of it.
- Whether to test WebSocket relaying, two clients connecting through a tracker, since a WebSocket tracker that answers but doesn't relay is useless to WebTorrent and reads as up today. All five that answer run standard software that relays by design, so the test would most likely confirm them for now.
