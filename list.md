# list

list is ftorrent's tracker checker, and like good it lives in two places: list.ftorrent.com, which checks from the server and has a page in front of it, and the list panel, the Trackers tab in the desktop client. The two are separate code with a shared purpose and the same libtorrent, the maintainers' 2.1.1 wheel pinned by one hash. The research behind it is the article [Tracking the Trackers](https://docs.ftorrent.com/tracker-lists), and the server half is built and described in [list/README.md](list/README.md).

## The two places

- list.ftorrent.com is a record of uptime: every public tracker, checked every hour over IPv4 and IPv6 separately, with every hour kept and a year of it summed up in the report.
- The list panel is a check of right now: from the user's own machine and connection, when the user asks, so the trackers written into a new torrent are ones that answer this client today.
- Nothing a visitor does on list.ftorrent.com changes anything. There's no submission form; trackers arrive by appearing on the public lists list reads.
- Each place reads the other's output as one source among several, never as the only one, so either keeps working when the other is gone.

We split them on purpose. The server can afford memory, a schedule, and two networks to keep each family honest; the desktop can afford none of that, and doesn't need it, since its question is whether a tracker answers this machine now. What they share is the meaning of an outcome, so a tracker that's up on the page and up in the panel are the same claim.

## The list panel in the desktop client

The panel is a check of right now, and its focus is the reverse of the page's: reply times in milliseconds, which tracker answered this machine first, where the page shows a calendar of hours. The essay in `desktop/src/trackers.js` describes how a check runs, and the one in `desktop/src/settings.js` the `[trackers]` settings it reads and writes. Still to do:

- Check on Windows whether the checking session, which listens on a port while it announces, brings up the firewall prompt.
- Start a new torrent's trackers from `answered`, fastest first, or from `always` when there has been no check, once there's a flow that creates torrents.
- Have the session that downloads read `trackers.announce` the way the Connection tab's does, so a torrent's trackers each get an announce, as qBittorrent and Transmission give them, rather than only the first that answers, libtorrent's own default.
- Give the panel its finished look when the client comes together; until then it stays rough, as a tool for people who know what a tracker is.

## Open questions

- Whether to honor BEP 34, the DNS record a tracker's operator publishes to say "don't announce to me." libtorrent doesn't read it, so list.py would look it up itself, which takes a DNS library Python's standard one lacks.
- What the trackers with extra peers are doing with the connections they send their users' clients to. So far it's one tracker, listing the same two servers for every hash.
- Whether 30 days and 95% make a good list, looked at again once there are a few months of it.
- Whether to test WebSocket relaying, two clients connecting through a tracker, since a WebSocket tracker that answers but doesn't relay is useless to WebTorrent and reads as up today. All five that answer run standard software that relays by design, so the test would most likely confirm them for now.
