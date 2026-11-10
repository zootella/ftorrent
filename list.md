# list

list is ftorrent's tracker checker, and like good it lives in two places: list.ftorrent.com, which checks from the server and has a page in front of it, and the list tab in the desktop client. The two are separate code with a shared purpose and the same libtorrent, the maintainers' 2.1.1 wheel pinned by one hash. The research behind it is the article [Tracking the Trackers](https://docs.ftorrent.com/tracker-lists), and the server half is built and described in [list/README.md](list/README.md).

## The two places

- list.ftorrent.com is a record of uptime: every public tracker, checked every hour over IPv4 and IPv6 separately, with every hour kept and a year of it summed up in the report.
- The list tab is a check of right now: from the user's own machine and connection, when the user asks, so the trackers written into a new torrent are ones that answer this client today.
- Nothing a visitor does on list.ftorrent.com changes anything. There's no submission form; trackers arrive by appearing on the public lists list reads.
- Each place reads the other's output as one source among several, never as the only one, so either keeps working when the other is gone.

We split them on purpose. The server can afford memory, a schedule, and two networks to keep each family honest; the desktop can afford none of that, and doesn't need it, since its question is whether a tracker answers this machine now. What they share is the meaning of an outcome, so a tracker that's up on the page and up in the tab are the same claim.

## list.ftorrent.com, as built

- `list/libtorrent-list/list.py` runs in two containers, `ftorrent-list4-1` on a network with IPv4 only and `ftorrent-list6-1` on one with IPv6 only, so libtorrent has no choice of family for any tracker.
- Each hour, each tracker gets a fresh random info hash and one announce. Its outcome is how far the announce got: `u` up, `d` DNS, `n` no address in this family, `t` timeout, `r` refused, `c` certificate, `b` bad reply, or `z` asleep, for an hour list's own monitor wasn't watching. A tracker whose reply lists peers for a hash no one else has is flagged with `extra_peers`, which decides nothing.
- ftorrent's own UDP tracker gets no announce. Each container takes its outcome from open.ftorrent.com's page.json instead: up when the gauge counted UDP announces over that container's family in its last minute, a timeout when it counted none, and no outcome when the page is stale or counted nothing at all, so an hour without a reading counts against no one. The pairing is a setting, `gauges`, so a fork beside its own tracker and gauge uses it unchanged.
- Everything goes into one SQLite file the two containers share. At the top of each hour the IPv4 one publishes the hour that just ended, then both check the new one.
- `trackers.txt` lists a tracker that was up in at least 95% of the hours it was checked in the last 30 days, in either family, judged on the hours there are, so the list fills from the first hour. `trackers.json` holds each tracker's hours in each family over six horizons, from the last hour to the last year, with every hour exactly one letter.
- The 30 days and 95% are two presets at the top of `list.py`, `criterion_horizon` and `criterion_uptime`.

The design choices that shaped it, and why: libtorrent rather than our own protocol code, because a tracker should be judged by the implementation clients run. One announcer rather than two, because a round trip between two of our own sessions fails trackers that keep one peer per address, spread across servers, or enforce their announce interval, while catching nothing common. The clock steers the hourly report and SQLite's lock only guards it, so no outcome depends on which container finishes first. Names are looked up in Python first, all at once, and the announces then start one a second, each tracker listened to on its own window: libtorrent looks names up one at a time, so announcing to every tracker at once left the last ones waiting in line past their 30 seconds, recorded as timeouts, and one a second checks each tracker on its own, the way a client announces a torrent to its few trackers. And libtorrent sends the announces and nothing else: it would otherwise connect to the peers a reply lists, to fetch a torrent no one has, and the check needs nothing from them.

## Deploying it

- Copy `list/docker-compose.yml` and `list/libtorrent-list/` into `/opt/list.ftorrent.com/`, make the data folder beside them, owned by 65534, and start both with `docker compose up -d --build`.
- Serve `/trackers.txt` and `/trackers.json` at `list.ftorrent.com` from `/opt/list.ftorrent.com/data/public/`, with a certificate, `Access-Control-Allow-Origin: *`, and `Cache-Control: no-cache`, and everything else from `/opt/list.ftorrent.com/static/`, where the page goes.
- The networks are `ftorrent-list4` on `172.30.6.0/24` and `ftorrent-list6` on `fd00:cafe:8::/64`, with the containers at `.2` and `::2`. Docker translates both to the server's public addresses on the way out.
- Allow outbound UDP and TCP to any address and port from the two pinned addresses, and nothing inbound.

## The page

A later session builds the page at list.ftorrent.com, as a view of `trackers.json` the way open.ftorrent.com's page is a view of `page.json`, uploaded into `/opt/list.ftorrent.com/static/`, a folder of its own, so replacing the page never touches the reports. It shows each tracker's horizons in each family side by side, so a tracker dead on IPv6 and perfect on IPv4 reads at a glance; which trackers are dual stack; and the extra peers flag where there is one, stated as what was seen rather than what it means.

## The list tab in the desktop client

- Keep the engine a roadway: add one general libtorrent command, roughly announce this hash to these trackers and pass back each result as it comes, and no application logic.
- The decisions live in the page: which lists, merging them, the outcomes, what to save. Rust fetches the lists through `net_get` and writes the file. Reread the essay in `lib.rs` on how the page and Rust divide the work before designing it.
- It acts only when the user clicks: fetch every list, check every candidate once, and show each result as it arrives, so the list builds over several seconds.
- Its list of lists holds `list.ftorrent.com/trackers.txt` beside ngosang's lists and newTrackon's, all in the same format, read by one parser. When none of them answers, it checks the trackers already saved.
- libtorrent announces from every local address the machine has, loopback and link-local among them, so the tab reads a tracker's result per family from the addresses that reach the internet, and the rest are noise to set aside. The outcomes are the server's, where they apply: a WebSocket tracker that answers is up, and the extra peers flag comes from libtorrent's own list of peers, which drops the filler some trackers pad a reply with.
- The result is the user's tracker list: checked, saved, and theirs to edit before it goes into a new torrent.

The two lists become settings in `ftorrent.toml`: the list of lists, and the tracker list a new torrent starts from. The tracker list leaves the engine's table in `engine.py`, which keeps the STUN and DHT lists, the ones libtorrent itself is handed. Before adding them, rewrite the settings factory in `desktop/src/settings.js` as one template string that reads exactly as `ftorrent.toml` does, comments and all, with each setting's check beside it in code and lists written one item to a line. The first run writes that template and reads it back. Factory values reach a file only at its first run, so a later release's change to a factory list doesn't reach an existing user; the tab refreshes the tracker list anyway, and the list of lists is the one that could go stale.

Add the list of lists to Names and Numbers, for the client and for list.ftorrent.com, with list.ftorrent.com as a server the client contacts only on that click, and move the tracker list from the engine's table to `settings.js` there too.

## Open questions

- Whether to honor BEP 34, the DNS record a tracker's operator publishes to say "don't announce to me." libtorrent doesn't read it, so list.py would look it up itself, which takes a DNS library Python's standard one lacks.
- What the trackers flagged with extra peers are doing, and whether that should ever affect `trackers.txt`. For now it's published and decides nothing.
- Whether 30 days and 95% make a good list, looked at again once there are a few months of it.
- Whether to record WebSocket relaying, two sessions connecting through a tracker, as an observation that doesn't decide the outcome.
