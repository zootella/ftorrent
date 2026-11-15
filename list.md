# list

list is ftorrent's tracker checker, and like good it lives in two places: list.ftorrent.com, which checks from the server and has a page in front of it, and the list tab in the desktop client. The two are separate code with a shared purpose and the same libtorrent, the maintainers' 2.1.1 wheel pinned by one hash. The research behind it is the article [Tracking the Trackers](https://docs.ftorrent.com/tracker-lists), and the server half is built and described in [list/README.md](list/README.md).

## The two places

- list.ftorrent.com is a record of uptime: every public tracker, checked every hour over IPv4 and IPv6 separately, with every hour kept and a year of it summed up in the report.
- The list tab is a check of right now: from the user's own machine and connection, when the user asks, so the trackers written into a new torrent are ones that answer this client today.
- Nothing a visitor does on list.ftorrent.com changes anything. There's no submission form; trackers arrive by appearing on the public lists list reads.
- Each place reads the other's output as one source among several, never as the only one, so either keeps working when the other is gone.

We split them on purpose. The server can afford memory, a schedule, and two networks to keep each family honest; the desktop can afford none of that, and doesn't need it, since its question is whether a tracker answers this machine now. What they share is the meaning of an outcome, so a tracker that's up on the page and up in the tab are the same claim.

## The list tab in the desktop client

- Keep the engine a roadway: add one general libtorrent command, roughly announce this hash to these trackers and pass back each result as it comes, and no application logic.
- The decisions live in the page: which lists, merging them, the outcomes, what to save. Rust fetches the lists through `net_get` and writes the file. Reread the essay in `lib.rs` on how the page and Rust divide the work before designing it.
- It acts only when the user clicks: fetch every list, check every candidate once, and show each result as it arrives, so the list builds over several seconds.
- Its list of lists holds `list.ftorrent.com/trackers.txt` beside ngosang's lists and newTrackon's, all in the same format, read by one parser. When none of them answers, it checks the trackers already saved.
- libtorrent announces from every local address the machine has, loopback and link-local among them, so the tab reads a tracker's result per family from the addresses that reach the internet, and the rest are noise to set aside. The outcomes are the server's, where they apply: a WebSocket tracker that answers is up, and extra peers are read from libtorrent's own list of peers, which drops the filler some trackers pad a reply with.
- The result is the user's tracker list: checked, saved, and theirs to edit before it goes into a new torrent.

The two lists become settings in `ftorrent.toml`: the list of lists, and the tracker list a new torrent starts from. The tracker list leaves the engine's table in `engine.py`, which keeps the STUN and DHT lists, the ones libtorrent itself is handed. Before adding them, rewrite the settings factory in `desktop/src/settings.js` as one template string that reads exactly as `ftorrent.toml` does, comments and all, with each setting's check beside it in code and lists written one item to a line. The first run writes that template and reads it back. Factory values reach a file only at its first run, so a later release's change to a factory list doesn't reach an existing user; the tab refreshes the tracker list anyway, and the list of lists is the one that could go stale.

Add the list of lists to Names and Numbers, for the client and for list.ftorrent.com, with list.ftorrent.com as a server the client contacts only on that click, and move the tracker list from the engine's table to `settings.js` there too.

## Open questions

- Whether to honor BEP 34, the DNS record a tracker's operator publishes to say "don't announce to me." libtorrent doesn't read it, so list.py would look it up itself, which takes a DNS library Python's standard one lacks.
- What the trackers with extra peers are doing with the connections they send their users' clients to. So far it's one tracker, listing the same two servers for every hash.
- Whether 30 days and 95% make a good list, looked at again once there are a few months of it.
- Whether to test WebSocket relaying, two clients connecting through a tracker, since a WebSocket tracker that answers but doesn't relay is useless to WebTorrent and reads as up today. All five that answer run standard software that relays by design, so the test would most likely confirm them for now.
