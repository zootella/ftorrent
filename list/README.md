
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

- [`list.ftorrent.com/trackers.txt`](https://list.ftorrent.com/trackers.txt), the trackers that answered in at least 95% of the hours they were checked over the last month, one URL per line, in the format other lists and clients already read. Point a client's "add trackers from a URL" setting at it.
- [`list.ftorrent.com/trackers.json`](https://list.ftorrent.com/trackers.json), every tracker checked, its outcome in each family for the hour, and its history by day.

The candidates come from the public lists other people keep, which [Tracking the Trackers](https://docs.ftorrent.com/tracker-lists) surveys, plus ftorrent's own trackers. Those lists are checked by DNS lookups, TCP connections, or real announces over UDP and HTTP; this one announces to every tracker over each family on its own, WebSocket trackers included.

## How a tracker is checked

Each tracker gets a torrent of its own: a random info hash, made fresh every hour, that names no real torrent, so no one else is announcing it. [libtorrent](https://www.libtorrent.org/), the engine inside qBittorrent, Deluge, and the ftorrent desktop client, announces to it once, and the outcome is how far the announce got:

- the tracker's name resolves to an address in this family;
- something at that address answers;
- for HTTPS and WebSocket, the certificate is valid for the name and signed by a trusted authority;
- the answer is a tracker's reply.

A tracker that gets all the way is up for the hour. Using libtorrent rather than code of our own means a tracker is judged by the same implementation real clients run, with the same handling of redirects, compression, and protocol quirks.

One more thing gets recorded. A UDP or HTTP reply lists the peers announcing the same hash, and since no one else knows this one, the honest answer lists no one, or only the announcer itself, which some trackers include. A reply that lists others is still up, and the tracker is flagged as listing extra peers. What a tracker gains from naming peers for a hash it has just heard of isn't known, so the flag is published and decides nothing. Each tracker gets its announce and nothing more: libtorrent never connects to the peers a reply lists, runs no DHT, and maps no ports.

One kind of tracker is judged without an announce. A UDP tracker paired with the page.json of the [gauge](../open/gauge/README.md) beside it, as ftorrent's own is, takes its outcome from that page: up when the tracker served UDP announces over this family in the gauge's last minute, a timeout when it served none. Those announces are peers reaching it from the internet, the claim every other outcome makes. A page more than three minutes old, or one that counted nothing at all, gives no outcome for the hour, and an hour with no outcome counts against no one. The pairs are `gauges`, at the top of `list.py`.

## The outcomes

Each hour, for each tracker in each family, is exactly one of these, which `trackers.json` gives as one letter:

| Letter | Outcome | Meaning |
|---|---|---|
| `u` | up | the announce got a tracker's reply |
| `d` | DNS | the name doesn't resolve, or libtorrent refuses it, as it refuses a name in another script that can be made to look like a different name |
| `n` | no address | the name resolves, but has no address in this column's family |
| `t` | timeout | nothing answered: the announce timed out, or there's no route to the address |
| `r` | refused | something at the address turned the announce away: a refused or reset connection, or an encrypted one cut off partway |
| `c` | certificate | the HTTPS or WebSocket certificate is expired, for another name, or signed by no one trusted |
| `b` | bad reply | something answered, but not with a tracker's reply: an HTTP error, the tracker's own failure message, or a garbled packet |
| `z` | asleep | no result, because list.ftorrent.com's own monitor wasn't watching: the server or a container was down, the tracker wasn't on any list yet, or every tracker in the family failed at once, so the trouble was at this end |

When an announce fails more than one way in an hour, as libtorrent retries, the furthest it got is the one recorded, so a server that resets a connection and then fails its certificate is a certificate problem, since something did answer.

## What it publishes

### trackers.txt

A tracker is listed when, in at least one family, its uptime over the last 30 days is at least 95%: its up hours as a share of the hours it wasn't asleep. The bar is newTrackon's 95%, held over a month, and the rule judges the hours there are: a tracker checked for only an hour so far, because list.ftorrent.com began an hour ago or because a list named it an hour ago, is listed if it was up that hour, and its standing steadies as its hours fill the month. Because the horizon is a month, one bad hour never drops a steady tracker, and a tracker with a perfect month that stops answering leaves the list after 36 hours, 5% of the month's 720.

The two settings are `criterion_horizon` and `criterion_uptime`, at the top of `list.py`. The most dependable trackers come first.

### trackers.json

```json
{
	"title": "list.ftorrent.com uptime record of the public BitTorrent and WebTorrent trackers, fresh each hour",
	"about": "https://list.ftorrent.com",
	"hour": 1794207600000,
	"when": "2026-11-09T07:00:00.000Z",
	"upstream": [
		{"name": "ngosang", "home": "https://github.com/ngosang/trackerslist", "code": "https://github.com/ngosang/trackerslist", "lists": ["https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all.txt", "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_ws.txt"]},
		{"name": "newtrackon", "home": "https://newtrackon.com", "code": "https://github.com/CorralPeltzer/newTrackon", "lists": ["https://newtrackon.com/api/all"]}
	],
	"downstream": ["https://list.ftorrent.com/trackers.txt", "https://list.ftorrent.com/trackers.json"],
	"checked": {"ipv4": true, "ipv6": true},
	"criterion": {"horizon": "month", "uptime": 0.95},
	"trackers": [
		{
			"url": "udp://tracker.example.com:6969/announce",
			"since": 1791615600000,
			"upstream": ["newtrackon", "ngosang"],
			"listed": true,
			"extra_peers": false,
			"ipv4": {
				"hour": {"u": 1},
				"day": {"u": 24},
				"week": {"u": 166, "t": 2},
				"month": {"u": 712, "t": 3, "b": 3, "z": 2},
				"quarter": {"u": 713, "t": 3, "b": 3, "z": 1441},
				"year": {"u": 713, "t": 3, "b": 3, "z": 8041}
			},
			"ipv6": {
				"hour": {"n": 1},
				"day": {"n": 24},
				"week": {"n": 168},
				"month": {"n": 718, "z": 2},
				"quarter": {"n": 719, "z": 1441},
				"year": {"n": 719, "z": 8041}
			}
		}
	]
}
```

- `title` and `about` say what the file is and where it comes from, for anyone reading it by hand.
- `hour` is the hour the report describes, as milliseconds since 1970, and `when` the same hour, readable. A tracker's `since` is the first hour list.ftorrent.com checked it, which is when a list first named it, in milliseconds as well.
- `upstream` is where the candidates come from: each source's `name`, its `home` page and its `code`, and the `lists` fetched from it every hour. ftorrent's own three trackers are checked every hour too, whatever the lists say.
- `downstream` is what list.ftorrent.com publishes from them: `trackers.txt` and this file, so a copy says where the live one is.
- `checked` says whether this hour's results in each family mean anything: `true` when that family's container ran its pass and at least one tracker answered, and `false` when it recorded nothing or every tracker failed, which means the trouble was at this end, so the hour counts against no tracker.
- `criterion` is the test for `trackers.txt`: an uptime of at least `uptime` over the `horizon`, in at least one family.
- `url` is the tracker's announce URL, and `upstream` names the sources that carry it now, from the top of the file.
- `listed` says whether it's in `trackers.txt`, and `extra_peers` whether, at least once in the last 30 days, a reply from it listed peers for a hash no one else has.
- Each family's column, `ipv4` and `ipv6`, gives the tracker's hours over six horizons, each ending with the report's hour: `hour`, `day`, `week`, `month`, `quarter`, and `year`, the last 1, 24, and 168 hours and the last 30, 90, and 365 days. Each holds the hours of each outcome by letter, leaving out letters with none. Every hour is exactly one letter, so a horizon's hours add up to its length, and its uptime is its `u` hours as a share of the hours that weren't `z`.
- When a column's `hour` is anything but `d`, `n`, or `z`, the tracker's name has an address in that family, so a tracker like that in both columns is dual stack.

The file stays about the same size however long the record grows: 113 KB for 300 trackers after 18 months.

## How it runs

Two containers run `list.py`, built from `libtorrent-list/`: `ftorrent-list4-1` on a Docker network with IPv4 only, and `ftorrent-list6-1` on one with IPv6 only, so libtorrent has no choice of family to make for any tracker. Each container listens on its one address and nothing connects to it; it only announces outward.

Both sleep to the top of every hour. The IPv4 container, with `LIST_REPORT` set, first publishes the hour that just ended; then both fetch the lists, check every tracker, listening until each one has answered or failed, at least 45 seconds and at most 15 minutes, and record the results. The report is always one whole hour, about an hour behind, which suits a record of uptime. A check of right now belongs to the desktop client.

Everything is kept in one SQLite file in the data folder the two containers share: every check of every hour, every tracker ever named, each list's latest copy, and a tally of each day that has ended, which the longer horizons are summed from, so a report takes as long to publish in its second year as on its first day. The published files are built from it; the file itself is never published. 18 months of hourly checks for 300 trackers came to 424 MB. A list that can't be fetched keeps its last good copy, so a site that's down for an hour doesn't empty the candidates.

A pass of today's 288 trackers takes about a minute, and publishing a report about a second. On Docker Desktop, a pass peaked at 52 MiB of memory, 109 open files, which are the sockets of the announces in flight, and 16 threads, eight of them libdatachannel's: the WebRTC library libtorrent runs for WebSocket trackers starts one for each CPU the machine has and keeps them. Publishing from 18 months of history for 300 trackers peaked at 17 MiB. The compose file's limits sit far above all of that and far below what a server notices, each with its reason beside it: 500 MiB of memory, 4 CPUs, 4096 open files, and 4096 processes and threads.

## Where the candidates come from

- [ngosang/trackerslist](https://github.com/ngosang/trackerslist), its `trackers_all.txt` and `trackers_all_ws.txt`
- [newTrackon](https://newtrackon.com/), its full list at `/api/all`
- ftorrent's own trackers, `udp://open.ftorrent.com:443/announce`, `https://open.ftorrent.com/announce`, and `wss://open.ftorrent.com`

The larger collections in circulation are built mostly from these two. URLs are compared in one spelling, lowercase and without the port their scheme implies, so a tracker two lists write differently is checked once.

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

That writes `list.sqlite` and `public/` into `list/data/`, the folder the compose file mounts. Run it again with `LIST_FAMILY=6` and no `LIST_REPORT` to record IPv6 for the same hour, then once more over IPv4 to publish both. `trackers.txt` fills from the first hour, each tracker judged on the hours it has been checked so far.

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

The first report appears at the top of the hour after the first checks. To see one sooner, run a pass by hand inside each running container, IPv6 first, so the IPv4 run publishes both families, and not in the minutes around the top of an hour, when the hourly pass runs:

```
docker exec ftorrent-list6-1 /app/.venv/bin/python /app/list.py now
docker exec ftorrent-list4-1 /app/.venv/bin/python /app/list.py now
```

## What it can't tell you

- A tracker that misuses what it sees passes every check, as long as it answers.
- It checks from one place. A tracker blocked from other networks can still be up here.
- ftorrent's own trackers run on the same server as list.ftorrent.com, so its HTTPS and WebSocket announces to them never cross the internet. Those rows show the trackers answer, not that the rest of the internet can reach them. The UDP rows, from page.json, count peers that reached the tracker from the internet, and newTrackon and the other checkers show the rest.
