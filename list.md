# list

list is ftorrent's tracker checker, in two places: list.ftorrent.com, a record of every public tracker checked every hour from a server, and the list panel, the Trackers tab in the desktop client, a check of right now from the user's own machine. [Tracking the Trackers](https://docs.ftorrent.com/tracker-lists) tells why each works the way it does, [list/README.md](list/README.md) how the server half runs, and the essays in `desktop/src/trackers.js` and `desktop/src/settings.js` how the panel does. What's left is below.

## The list panel in the desktop client

- Check on Windows whether the checking session, which listens on a port while it announces, brings up the firewall prompt.
- Start a new torrent's trackers from `answered`, fastest first, or from `always` when there has been no check, once there's a flow that creates torrents.
- Have the session that downloads read `trackers.announce` the way the Connection tab's does, so a torrent's trackers each get an announce, as qBittorrent and Transmission give them, rather than only the first that answers, libtorrent's own default.
- Give the panel its finished look when the client comes together; until then it stays rough, as a tool for people who know what a tracker is.

## Open questions

- Whether to honor BEP 34, the DNS record a tracker's operator publishes to say "don't announce to me." libtorrent doesn't read it, so list.py would look it up itself, which takes a DNS library Python's standard one lacks.
- What the trackers with extra peers are doing with the connections they send their users' clients to. So far it's one tracker, listing the same two servers for every hash.
- Whether 30 days and 95% make a good list, looked at again once there are a few months of it.
- Whether to test WebSocket relaying, two clients connecting through a tracker, since a WebSocket tracker that answers but doesn't relay is useless to WebTorrent and reads as up today. All five that answer run standard software that relays by design, so the test would most likely confirm them for now.
