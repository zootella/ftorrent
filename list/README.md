
_ftorrent/list/README.md [list.ftorrent.com](https://list.ftorrent.com)_

# The tracker checker

> Prepared by [Claude Code](https://claude.ai/code) using Opus 5.5
> <br>Created: 2026-Nov
> <br>Last reviewed: 2026-Nov
> <br>[libtorrent](https://www.libtorrent.org/): 2.1, the maintainers' wheel
> <br>[Python](https://www.python.org/): 3.13
> <br>[ergo](https://github.com/zootella/ergo): 0.1.2
> <br>Docker Engine: 29
> <br>Docker Compose: V2

## What this is

list.ftorrent.com checks the public BitTorrent and WebTorrent trackers every hour, over IPv4 and over IPv6 separately, and publishes which ones work. Two files come out of it:

- [`list.ftorrent.com/trackers.txt`](https://list.ftorrent.com/trackers.txt), the trackers that have been dependable for the last month, one address per line, in the format other lists and clients already read. Point a client's "add trackers from a URL" setting at it.
- [`list.ftorrent.com/trackers.json`](https://list.ftorrent.com/trackers.json), every tracker checked, its outcome in each family for the hour, and its history by day.

The candidates come from the public lists other people keep, which [Tracking the Trackers](https://docs.ftorrent.com/tracker-lists) surveys, plus ftorrent's own trackers. Those lists are checked by DNS lookups, TCP connections, or real announces over UDP and HTTP; this one announces to every tracker over each family on its own, WebSocket trackers included.

## How a tracker is checked

Each tracker gets a torrent of its own: a random info hash, made fresh every hour, that names no real torrent, so no one else is announcing it. [libtorrent](https://www.libtorrent.org/), the engine inside qBittorrent, Deluge, and the ftorrent desktop client, announces to it once, and the outcome is how far the announce got:

- the tracker's name resolves to an address in this family;
- something at that address answers;
- for HTTPS and WebSocket, the certificate is valid for the name and signed by a trusted authority;
- the answer is a tracker's reply.

A tracker that gets all the way is good for the hour. Using libtorrent rather than code of our own means a tracker is judged by the same implementation real clients run, with the same handling of redirects, compression, and protocol quirks.

One more thing gets recorded. A UDP or HTTP reply lists the peers announcing the same hash, and since no one else knows this one, the honest answer lists no one, or only the announcer itself, which some trackers include. A reply that lists others is still good, and the extra peers are counted. What a tracker gains from naming peers for a hash it has just heard of isn't known, so the count is published and decides nothing. Each tracker gets its announce and nothing more: libtorrent never connects to the peers a reply lists, runs no DHT, and maps no ports.

One kind of tracker is judged without an announce. A UDP tracker paired with the page.json of the [gauge](../open/gauge/README.md) beside it, as ftorrent's own is, takes its outcome from that page: good when the tracker served UDP announces over this family in the gauge's last minute, a timeout when it served none. Those announces are peers reaching it from the internet, the claim every other outcome makes. A page more than three minutes old, or one that counted nothing at all, gives no outcome for the hour, and an hour with no outcome counts against no one. The pairs are `gauges`, at the top of `list.py`.

## The outcomes

`trackers.json` gives each outcome as one letter, since it carries hundreds of trackers and months of days.

| Letter | Outcome | Meaning |
|---|---|---|
| `g` | good | the announce got a tracker's reply |
| `d` | dns | the name doesn't resolve, or libtorrent refuses it, as it refuses a name in another script that can be made to look like a different name |
| `n` | no address | the name resolves, but has no address in this column's family |
| `t` | timeout | nothing answered: the announce timed out, or there's no route to the address |
| `r` | refused | something at the address turned the announce away: a refused or reset connection, or an encrypted one cut off partway |
| `c` | cert | the HTTPS or WebSocket certificate is expired, for another name, or signed by no one trusted |
| `b` | bad reply | something answered, but not with a tracker's reply: an HTTP error, the tracker's own failure message, or a garbled packet |
| `v` | void | every tracker failed in this family that hour, so the trouble was at this end, and the hour counts against no one |
| `x` | extra peers | not an outcome but a count beside them: hours whose good reply listed peers for a hash no one else has |

When an announce fails more than one way in an hour, as libtorrent retries, the furthest it got is the one recorded, so a server that resets a connection and then fails its certificate is a certificate problem, since something did answer.

## What it publishes

### trackers.txt

A tracker is listed when, in at least one family, it has been a candidate for the whole last 30 days and was good in at least 95% of the hours it was checked in them. Void hours don't count. The bar is newTrackon's 95%, held over a fixed month, and a new tracker waits the full 30 days. Because the window is a month, one bad hour never drops a steady tracker, and a tracker with a perfect month that stops answering leaves the list after 36 hours, 5% of the month's 720.

The two numbers are `listing_window` and `listing_share`, at the top of `list.py`. The most dependable trackers come first.

### trackers.json

```json
{
	"hour": 1794207600000,
	"families": {"ipv4": "checked", "ipv6": "checked"},
	"listing": {"window": 2592000000, "share": 0.95},
	"trackers": [
		{
			"address": "udp://tracker.example.com:6969/announce",
			"since": 1794207600000,
			"lists": ["newtrackon", "ngosang"],
			"listed": true,
			"stack": "ipv4",
			"ipv4": {"outcome": "g", "share": 1.0, "days": [["2026-10-11", 30, {"g": 24}]]},
			"ipv6": {"outcome": "n", "message": "skipping tracker announce (unreachable)", "share": 0.0, "days": [["2026-10-11", 30, {"n": 24}]]}
		}
	]
}
```

- `hour` is the hour the report describes, and `since` the hour a tracker first became a candidate, both as milliseconds since 1970.
- `families` says whether each family was `checked` that hour, `void`, or `missing`, when its container recorded nothing.
- `lists` names the lists that carry the tracker now, and `ftorrent` for our own.
- `listed` says whether it's in `trackers.txt`, and `stack` which families its name had an address in this hour: `dual`, `ipv4`, `ipv6`, or `neither`.
- Each family's column holds the hour's `outcome`, libtorrent's `message` when it failed, `extra_peers` when a good reply listed some, the `share` of good hours over the listing window, and `days`.
- `days` covers the last 18 months, in UTC days. Each entry is a run of days with the same hours: its first date, how many days, and the hours of each outcome, with outcomes that had none left out. A tracker dead for a year is one entry.

## How it runs

Two containers run `list.py`, built from `libtorrent-list/`: `ftorrent-list4-1` on a Docker network with IPv4 only, and `ftorrent-list6-1` on one with IPv6 only, so libtorrent has no choice of family to make for any tracker. Each container listens on its one address and nothing connects to it; it only announces outward.

Both sleep to the top of every hour. The IPv4 container, with `LIST_REPORT` set, first publishes the hour that just ended; then both fetch the lists, check every tracker, and record the results. The report is always one whole hour, about an hour behind, which suits a record of uptime. A check of right now belongs to the desktop client.

Everything is kept in one SQLite file in the data folder the two containers share: every check of every hour, every tracker ever named, and each list's latest copy. The published files are built from it; the file itself is never published. A list that can't be fetched keeps its last good copy, so a site that's down for an hour doesn't empty the candidates.

A pass of today's 288 trackers takes about a minute. On Docker Desktop, passes peaked at 52 MiB of memory, 16 processes and threads, and 109 open files, the sockets of the announces in flight; the compose file allows each container 10000 open files, so the lists have room to grow.

## Where the candidates come from

- [ngosang/trackerslist](https://github.com/ngosang/trackerslist), its `trackers_all.txt` and `trackers_all_ws.txt`
- [newTrackon](https://newtrackon.com/), its full list at `/api/all`
- ftorrent's own trackers, `udp://open.ftorrent.com:443/announce`, `https://open.ftorrent.com/announce`, and `wss://open.ftorrent.com`

The larger collections in circulation are built mostly from these two. Addresses are compared in one spelling, lowercase and without the port their scheme implies, so a tracker two lists write differently is checked once.

## Files

- [`docker-compose.yml`](docker-compose.yml), the two containers and their two networks
- [`libtorrent-list/list.py`](libtorrent-list/list.py), the whole program
- [`libtorrent-list/pyproject.toml`](libtorrent-list/pyproject.toml) and [`libtorrent-list/uv.lock`](libtorrent-list/uv.lock), libtorrent and ergo, pinned by hash
- [`libtorrent-list/Dockerfile`](libtorrent-list/Dockerfile), the image

libtorrent is the same wheel the [DHT bootstrap node](../open/dht/README.md) and the desktop client's engine run, pinned by the same hash; [libtorrent Provenance](https://docs.ftorrent.com/libtorrent-provenance) is the record of where it comes from.

## Running it yourself

On a computer with [uv](https://docs.astral.sh/uv/), check every tracker once, now, over IPv4, and write the report:

```
cd list/libtorrent-list
LIST_FAMILY=4 LIST_REPORT=1 uv run --frozen python list.py now
```

That writes `list.sqlite` and `public/` into `list/data/`, the folder the compose file mounts. Run it again with `LIST_FAMILY=6` and no `LIST_REPORT` to record IPv6 for the same hour, then once more over IPv4 to publish both. With the listing window at 30 days, `trackers.txt` stays empty until there's a month of history; set `listing_window` to `1*Time.HOUR` and `listing_share` to `0.5` to watch it fill from one hour.

## Deploying it

Copy `docker-compose.yml` and `libtorrent-list/` from this folder into `/opt/list.ftorrent.com/` on the server, and make the data folder beside them, writable by the containers' user:

```
sudo mkdir -p /opt/list.ftorrent.com/data
sudo chown -R 65534:65534 /opt/list.ftorrent.com/data
```

Start both containers from there:

```
cd /opt/list.ftorrent.com
docker compose up -d --build
```

Then two things on the host, in your own setup's terms:

- **Outbound only, from the two containers.** Trackers listen on whatever port they like, so each container needs outbound UDP and TCP to any address and port, from its pinned address. Nothing comes in. Docker translates both networks' private addresses to the server's public ones on the way out, IPv6 included.
- **The files.** Serve `/trackers.txt` and `/trackers.json` at your domain from `/opt/list.ftorrent.com/data/public/`, with `Access-Control-Allow-Origin: *` so other pages and tools can fetch them, and `Cache-Control: no-cache` so a browser checks for the new hour every time and an unchanged file costs a 304. Name the two files rather than serving the folder, so nothing else the containers write is ever reachable. Anything else at the domain, like a page, comes from a folder of its own, such as `/opt/list.ftorrent.com/static/`, so replacing it never touches the reports.

The first report appears at the top of the hour after the first checks.

## What it can't tell you

- A tracker that misuses what it sees passes every check, as long as it answers.
- It checks from one place. A tracker blocked from other networks can still be good here.
- ftorrent's own trackers run on the same server as list.ftorrent.com, so its HTTPS and WebSocket announces to them never cross the internet. Those rows show the trackers answer, not that the rest of the internet can reach them. The UDP rows, from page.json, count peers that reached the tracker from the internet, and newTrackon and the other checkers show the rest.
