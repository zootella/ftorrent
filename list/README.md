
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
- [`list.ftorrent.com/trackers.json`](https://list.ftorrent.com/trackers.json), every tracker checked: its hours in each family over six horizons, from the last hour to the last year, its last 180 days, and the details of its replies and certificate.

The public lists it starts from check their trackers by DNS lookups, TCP connections, or real announces over UDP and HTTP. list.ftorrent.com announces to every tracker over each family on its own, WebSocket trackers included, and keeps every hour.

## Where the candidates come from

- [ngosang/trackerslist](https://github.com/ngosang/trackerslist), its `trackers_all.txt` and `trackers_all_ws.txt`
- [newTrackon](https://newtrackon.com/), its full list at `/api/all`
- a list of list.ftorrent.com's own, written into `list.py`: ftorrent's trackers, `udp://open.ftorrent.com:443/announce`, `https://open.ftorrent.com/announce`, and `wss://open.ftorrent.com`, and every public WebSocket tracker we could find that is or was in use, from ngosang's WebSocket lists across their history since 2017, the defaults WebTorrent's create-torrent has shipped since its version 3, and P2P Media Loader's. A tracker long gone stays on it, so the record shows it, and one that comes back is noticed.

[Tracking the Trackers](https://docs.ftorrent.com/tracker-lists) surveys these and the larger collections in circulation, which are built mostly from the first two. The lists are fetched every hour, and one that can't be fetched keeps its last good copy, so a site that's down for an hour doesn't empty the candidates. URLs are compared in one spelling, lowercase and without the port their scheme implies, for `udp`, `http`, `https`, `ws`, and `wss`, so a tracker two lists write differently is checked once.

## How a tracker is checked

Each tracker gets a torrent of its own: a random info hash, made fresh every hour, that names no real torrent, so no one else is announcing it. [libtorrent](https://www.libtorrent.org/), the engine inside qBittorrent, Deluge, and the ftorrent desktop client, announces to it, and the outcome is how far the announce got:

- the tracker's name resolves to an address in this family;
- something at that address answers;
- for HTTPS and WebSocket, the certificate is valid for the name and signed by a trusted authority;
- the answer is a tracker's reply.

A tracker that gets all the way is up for the hour. Using libtorrent rather than code of our own means a tracker is judged by the same implementation real clients run, with the same handling of redirects, compression, and protocol quirks. One announce decides it: a round trip between two announcers of our own, each looking for the other, would fail trackers that keep one peer per address, spread their swarms across servers, or enforce their announce interval, and catch nothing common that one announce misses.

The first step happens before libtorrent. list.ftorrent.com looks up every tracker's name itself, all at once, since libtorrent looks names up one at a time, and a tracker left waiting in that line past libtorrent's 30 seconds would read as a timeout. A name that doesn't resolve is `D`, and one with addresses but none in this family is `N`. The rest are announced to one a second, and each is listened to for at least 50 seconds after its own announce and at most two minutes, so each tracker is checked on its own, the way a client announces a torrent to its few trackers.

The 50 seconds give each tracker two complete tries. libtorrent never resends a lost UDP packet within an announce: it gives up after 10 seconds and tries again 30 seconds later, at 39.5, and that try has until 49.5. A failed HTTP announce is retried at 16.5 seconds and given 30. So a tracker that drops one packet still reads up, and one that drops two in a row reads as a timeout.

Beside the outcome, each check records how many tries the answer took, how long the tracker asked clients to wait before announcing again, and any warning it sent with its reply. Once a day, list.ftorrent.com reads the certificate of each HTTPS and WebSocket tracker over a connection of its own: when it was issued, when it expires, and who signed it.

Each tracker gets its announce and nothing more: libtorrent never connects to the peers a reply lists, runs no DHT, and maps no ports.

One kind of tracker is judged without an announce. A UDP tracker paired with the page.json of the [gauge](../open/gauge/README.md) beside it, as ftorrent's own is, takes its outcome from that page: up when the tracker served UDP announces over this family in the gauge's last minute, a timeout when it served none. Those announces are peers reaching it from the internet, the claim every other outcome makes. A page more than three minutes old, or one that counted nothing at all, gives no outcome for the hour, and an hour with no outcome counts against no one. The pairs are `gauges`, at the top of `list.py`.

## The outcomes

Each hour, for each tracker in each family, is exactly one of these, which `trackers.json` gives as one capital letter:

| Letter | Outcome | Meaning |
|---|---|---|
| `U` | up | the announce got a tracker's reply |
| `D` | name doesn't resolve | the name doesn't resolve, or libtorrent refuses it, as it refuses a name in another script that can be made to look like a different name |
| `N` | no address | the name resolves, but has no address in this family |
| `T` | timed out | nothing answered: the announce timed out, or there's no route to the address |
| `R` | connection refused or reset | something at the address turned the announce away: a refused or reset connection, or an encrypted one cut off partway |
| `C` | certificate didn't verify | the HTTPS or WebSocket certificate is expired, for another name, or signed by no one trusted |
| `B` | bad reply | something answered, but not with a tracker's reply: an HTTP error, the tracker's own failure message, or a garbled packet |
| `Z` | no reading | no result, because list.ftorrent.com's own monitor wasn't watching: the server or a container was down, the tracker wasn't on any list yet, or every tracker in the family failed at once, so the trouble was at this end |

When an announce fails more than one way in an hour, as libtorrent retries, the furthest it got is the one recorded, so a server that resets a connection and then fails its certificate is a certificate problem, since something did answer.

## Extra peers

A tracker's reply lists the peers announcing the same hash. For a hash made up a moment before, the honest reply lists no one, or only the announcer itself. A tracker whose reply lists others is handing out addresses that no swarm put there, and a client using it connects to them, telling them its own address and which torrent it wants. So list.ftorrent.com leaves a tracker that does this out of `trackers.txt`, whatever its uptime, until a month has passed without it. `trackers.json` names it under `extra_peers`, with how many hours it did it and the most peers it listed at once. In the first days of checking, one tracker did it every hour, listing the same two servers for every hash.

## What it publishes

### trackers.txt

A tracker is listed when, in at least one family, its uptime over the last 30 days is at least 95%, its up hours as a share of the hours with a reading, and its replies listed no extra peers in that time. The bar is newTrackon's 95%, held over a month, and the rule judges the hours there are: a tracker checked for only an hour so far, because list.ftorrent.com began an hour ago or because a list named it an hour ago, is listed if it was up that hour, and its standing steadies as its hours fill the month. Because the horizon is a month, one bad hour never drops a steady tracker, and a tracker with a perfect month that stops answering leaves the list after 36 hours, 5% of the month's 720.

The settings are `criterion_horizon`, `criterion_uptime`, and `criterion_extra_peers`, at the top of `list.py`. The most dependable trackers come first.

### trackers.json

```json
{
	"title": "list.ftorrent.com uptime record of the public BitTorrent and WebTorrent trackers, fresh each hour",
	"about": "https://list.ftorrent.com",
	"hour": 1794207600000,
	"when": "2026-11-09T07:00:00.000Z",
	"upstream": [
		{"name": "ngosang", "home": "https://github.com/ngosang/trackerslist", "code": "https://github.com/ngosang/trackerslist", "lists": ["https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all.txt", "https://raw.githubusercontent.com/ngosang/trackerslist/master/trackers_all_ws.txt"]},
		{"name": "newtrackon", "home": "https://newtrackon.com", "code": "https://github.com/CorralPeltzer/newTrackon", "lists": ["https://newtrackon.com/api/all"]},
		{"name": "ftorrent", "home": "https://list.ftorrent.com", "code": "https://github.com/zootella/ftorrent/tree/master/list", "trackers": ["udp://open.ftorrent.com:443/announce", "https://open.ftorrent.com/announce", "wss://open.ftorrent.com", "wss://tracker.webtorrent.dev", "..."]}
	],
	"downstream": ["https://list.ftorrent.com/trackers.txt", "https://list.ftorrent.com/trackers.json"],
	"passes": {
		"ipv4": {"checked": true, "trackers": 290, "up": 100, "seconds": 269},
		"ipv6": {"checked": true, "trackers": 290, "up": 55, "seconds": 168}
	},
	"criterion": {"horizon": "m", "uptime": 0.95, "extra_peers": 0, "candidates": 328, "listed": 100},
	"trackers": [
		{
			"url": "udp://tracker.example.com:6969/announce",
			"since": 1791615600000,
			"upstream": ["newtrackon", "ngosang"],
			"ipv4": {
				"h": {"U": 1},
				"d": {"U": 24},
				"w": {"U": 166, "T": 2},
				"m": {"U": 712, "T": 3, "B": 3, "Z": 2},
				"q": {"U": 713, "T": 3, "B": 3, "Z": 1441},
				"y": {"U": 713, "T": 3, "B": 3, "Z": 8041}
			},
			"ipv6": {
				"h": {"N": 1},
				"d": {"N": 24},
				"w": {"N": 168},
				"m": {"N": 718, "Z": 2},
				"q": {"N": 719, "Z": 1441},
				"y": {"N": 719, "Z": 8041}
			},
			"days": {
				"ipv4": "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZUUUUU9UUUUUUUUUUUUUUUUU8UUUUUUUUU",
				"ipv6": "ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"
			},
			"details": {
				"ipv4": {"retried": {"d": 1, "w": 2, "m": 4, "q": 4, "y": 4}, "interval": 1800000},
				"certificate": {"issued": "2026-10-20", "expires": "2027-01-18", "issuer": "Let's Encrypt (R11)"}
			}
		}
	],
	"cut": [],
	"extra_peers": [
		{"url": "udp://tracker.example.net:6969/announce", "ipv4": {"hours": 48, "most": 4}}
	]
}
```

- `title` and `about` say what the file is and where it comes from, for anyone reading it by hand.
- `hour` is the hour the report describes, as milliseconds since 1970, and `when` the same hour, readable. A tracker's `since` is the first hour list.ftorrent.com checked it, which is when a list first named it, in milliseconds as well.
- `upstream` is where the candidates come from: each source's `name`, its `home` page and its `code`, and either the `lists` fetched from it every hour or, for list.ftorrent.com's own, the `trackers` it names.
- `downstream` is what list.ftorrent.com publishes from them: `trackers.txt` and this file, so a copy says where the live one is.
- `passes` says how each family's pass went, a few numbers repeated here for a quick look at the hour: how many trackers it recorded, how many were up, and how many seconds it took. `checked` says whether the hour's results in that family mean anything: `true` when its container ran its pass and at least one tracker answered, and `false` when it recorded nothing or every tracker failed, which means the trouble was at this end, so the hour counts against no tracker.
- `criterion` is the test for `trackers.txt`: an uptime of at least `uptime` over the `horizon`, `m` for the last 30 days, in at least one family, with extra peers in no more than `extra_peers` of those hours. `candidates` is how many trackers were checked, and `listed` how many passed.
- `trackers` holds the ones that passed, the trackers `trackers.txt` lists, and `cut` the rest, each in the same shape and order, the most dependable first.
- `url` is the tracker's announce URL, and `upstream` names the sources that carry it now, from the top of the file.
- Each family's column, `ipv4` and `ipv6`, gives the tracker's hours over six horizons, each ending with the report's hour and named by a lowercase letter: `h`, `d`, `w`, `m`, `q`, and `y`, the last hour, day, and week, and the last 30, 90, and 365 days. Each holds the hours of each outcome by its capital letter, leaving out letters with none. Every hour is exactly one letter, so a horizon's hours add up to its length, 1, 24, 168, 720, 2160, and 8760, and its uptime is its `U` hours as a share of the hours that weren't `Z`.
- When a column's `h` is anything but `D`, `N`, or `Z`, the tracker's name has an address in that family, so a tracker like that in both columns is dual stack.
- `days` gives each family's last 180 days, oldest first and today so far last, one character a day, which the page draws as a calendar: `U` when every hour with a reading was up, a digit when some were, how many tenths of them, `0` through `9`, so a day with any hour down reads `9` at most; the letter of the outcome most of its hours had when none was up, like `T` or `D`, so a day reads the same letter as the hours it sums; and `Z` when no hour had a reading.
- `details` holds what's known beyond the hours, for anyone looking at one tracker closely, leaving out anything empty. For each family, `retried` is the up hours in each horizon that needed a second try, `interval` how long the tracker last asked clients to wait before announcing again, in milliseconds, and `warning` the last warning it sent with a reply. `certificate`, for an HTTPS or WebSocket tracker, gives the UTC dates it was `issued` and `expires` and its `issuer`, or an `error` when it couldn't be read.
- `extra_peers` names each tracker whose replies listed extra peers over the last 30 days, with, for each family where it happened, the `hours` it did and the `most` peers it listed at once.

The file stays about the same size however long the record grows, and grows only with the number of trackers: 260 KB for 326.

## How it runs

Two containers run `list.py`, built from `libtorrent-list/`: `ftorrent-list4-1` on a Docker network with IPv4 only, and `ftorrent-list6-1` on one with IPv6 only, so libtorrent has no choice of family to make for any tracker. Each container listens on its one address and nothing connects to it; it only announces outward.

Both sleep to the top of every hour. The IPv4 container, with `LIST_REPORT` set, first publishes the hour that just ended; then both fetch the lists, look up every name, announce to the trackers left one a second, and record the results. The report is always one whole hour, about an hour behind, which suits a record of uptime. The clock decides what goes into it, and SQLite's lock only keeps the two containers' writes apart, so no outcome depends on which container finishes first. A check of right now belongs to the desktop client.

Each container keeps one libtorrent session for as long as it runs, the way a client does, and every pass adds its torrents to it and removes them. Closing a session waits out every connection still open, and a dead WebSocket tracker can leave one that takes minutes to give up, so a session made and closed by each pass would stretch the pass by minutes; kept open, whatever a dead tracker leaves behind finishes in the background.

Everything is kept in one SQLite file in the data folder the two containers share: every check of every hour, every tracker ever named, each list's latest copy, each day's certificates, and a tally of each day that has ended, which the longer horizons are summed from, so a report takes as long to publish in its second year as on its first day. The published files are built from it; the file itself is never published. 18 months of hourly checks for 300 trackers came to 424 MB.

The file records its own version in SQLite's `PRAGMA user_version`, an integer SQLite sets aside for the program. When a container starts, `list.py` brings an older file forward through the numbered steps in `migrations`, one at a time, under SQLite's write lock, so of two containers starting together one migrates and the other finds it done; it stops rather than write into a file newer than it knows.

A pass takes a second for each tracker announced to plus the last one's window, so in list.ftorrent.com's first week, with 326 candidates, an IPv4 pass took 346 to 359 seconds and an IPv6 pass, with fewer trackers to announce to, 238 to 248. Publishing a report takes about a second. Announcing to every tracker at once, a pass on Docker Desktop peaked at 52 MiB of memory, 109 open files, which are the sockets of the announces in flight, and 16 threads, eight of them libdatachannel's: the WebRTC library libtorrent runs for WebSocket trackers starts one for each CPU the machine has and keeps them. One a second, a pass holds fewer sockets, and looking up the names takes up to 32 threads more for a few seconds. Publishing from 18 months of history for 300 trackers peaked at 17 MiB. The compose file's limits sit far above all of that and far below what a server notices, each with its reason beside it: 500 MiB of memory, 4 CPUs, 4096 open files, and 4096 processes and threads.

## Files

- [`docker-compose.yml`](docker-compose.yml), the two containers and their two networks
- [`libtorrent-list/list.py`](libtorrent-list/list.py), the whole program
- [`libtorrent-list/pyproject.toml`](libtorrent-list/pyproject.toml) and [`libtorrent-list/uv.lock`](libtorrent-list/uv.lock), libtorrent and ergo, pinned by hash
- [`libtorrent-list/Dockerfile`](libtorrent-list/Dockerfile), the image
- [`page/`](page/), the page at list.ftorrent.com, a view of `trackers.json` built with Vue and Vite, which `pnpm upload` puts in the static folder

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
- **The files.** Serve `/trackers.txt` and `/trackers.json` at your domain from `/opt/list.ftorrent.com/data/public/`, with `Access-Control-Allow-Origin: *` so other pages and tools can fetch them, and `Cache-Control: no-cache` so a browser checks for the new hour every time and an unchanged file costs a 304. Name the two files rather than serving the folder, so nothing else the containers write is ever reachable. Anything else at the domain, like a page, comes from a folder of its own, such as `/opt/list.ftorrent.com/static/`, so replacing it never touches the reports. The page routes by path, like `/udp/tracker.example.com:6969/announce`, so answer any path that isn't a file in that folder with its `200.html`, which the build writes as a copy of `index.html`.

The first report appears at the top of the hour after the first checks. To see one sooner, run a pass by hand inside each running container, which takes a few minutes each, IPv6 first, so the IPv4 run publishes both families, and not in the minutes around the top of an hour, when the hourly pass runs:

```
docker exec ftorrent-list6-1 /app/.venv/bin/python /app/list.py now
docker exec ftorrent-list4-1 /app/.venv/bin/python /app/list.py now
```

Before deploying a `list.py` that adds a step to `migrations`, stop the containers and copy the data folder's `list.sqlite` files aside together, so there's a copy the version before can still use.

To write the latest report again from what's already recorded, as after changing its shape, publish it by hand in the IPv4 container. It checks nothing and takes about a second:

```
docker exec ftorrent-list4-1 /app/.venv/bin/python /app/list.py publish
```

## What it can't tell you

- A tracker that misuses what it sees passes every check, as long as it answers.
- It checks from one place. A tracker blocked from other networks can still be up here.
- ftorrent's own trackers run on the same server as list.ftorrent.com, so its HTTPS and WebSocket announces to them never cross the internet. Those rows show the trackers answer, not that the rest of the internet can reach them. The UDP rows, from page.json, count peers that reached the tracker from the internet, and newTrackon and the other checkers show the rest.
