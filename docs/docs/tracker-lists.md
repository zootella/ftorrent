---
title: Tracking the Trackers
description: A torrent names its trackers once, and over the years they stop answering. Who runs the open trackers, who checks whether they work, who keeps the lists of the ones that still do, which clients read those lists, and what no one checks.
---

# Tracking the Trackers

> Prepared by [Claude Code](https://claude.ai/code) using Opus 5.5
> <br>Published: 2026 November 8
> <br>Cite as: ftorrent. *Tracking the Trackers.* November 2026. https://docs.ftorrent.com/tracker-lists.html

On 2016 April 24, Diego Heras, a developer in Madrid, published a list of 49 open BitTorrent trackers on GitHub. Heras's software has checked the list and republished it every day since, and ten years and 4,063 commits later, three of the 49 are still on it: `tracker.opentrackr.org`, `explodie.org`, and `open.stealth.si`. The others stopped answering, one by one, while every torrent made with their addresses went on naming them, because a torrent file or a magnet link carries the trackers written into it when it was made, and those never change.

So someone has to know which trackers still answer. This article maps who does that in 2026. A few dozen open trackers answer anyone who asks, run by individuals and small groups on two programs, opentracker and Aquatic. Checking whether they work happens in very few places: two services, each the work of one volunteer since 2016, and a stricter fork started in 2026 October. Everything else is aggregation, lists built by merging those two with lighter checks or none, which then merge one another, and which reach users through the clients that fetch them. One kind of tracker, the WebSocket tracker that browser peers use, isn't checked anywhere with a real request.

Every count here was taken on 2026 November 8 unless it carries another date. One disclosure belongs at the start: ftorrent runs one of the trackers on these lists, `open.ftorrent.com`, and the last section follows it through every list as a worked example.

## What a tracker does

A tracker answers one question: who else has this torrent? A client joining a swarm sends the tracker an announce, carrying the torrent's info hash, the port the client listens on, and how much it has left to download. The tracker answers with other peers that announced the same hash, and with an interval to wait before announcing again. It holds no files and no torrents, only recent announces, in memory. An open tracker answers anyone, for any info hash, with no account.

Trackers speak three protocols:

- **HTTP**, from BitTorrent's original specification, [BEP 3](https://www.bittorrent.org/beps/bep_0003.html). An announce is a web request, and the answer is a bencoded dictionary. HTTPS is the same exchange over TLS.
- **UDP**, from [BEP 15](https://www.bittorrent.org/beps/bep_0015.html). The same exchange in a few small datagrams, after a short connect step that proves the client's address isn't forged. Many open trackers speak only UDP, since it costs the server far less per announce.
- **WebSocket**, defined by the [WebTorrent](https://webtorrent.io/) project for peers in a web page, which can reach other peers only through WebRTC. The page keeps a WebSocket open to the tracker, and the tracker both lists peers and relays the WebRTC offers and answers that let two browsers connect directly.

Peers can find each other without any tracker, through the distributed hash table and peer exchange, which [How the DHT Works](/how-the-dht-works) explains. A tracker is usually the fastest first introduction, though, and a torrent whose trackers have all stopped answering depends on the DHT alone. So clients let the user add trackers to a torrent, and some add them automatically from a list. Whoever keeps that list decides which servers a great many clients announce to.

## The trackers

The open trackers that appear most often on the lists run on two programs. [opentracker](https://erdgeist.org/arts/software/opentracker/), written in C by erdgeist, is the older. [Aquatic](https://github.com/greatest-ape/aquatic), written in Rust by Joakim Frostegård since 2020, speaks all three protocols in separate binaries; ftorrent's [Software Selections](/software-selections) essay weighs the two against each other. Table 1 lists the trackers named most often, by what each one's own page says.

<figure class="breakout">
<figcaption>Table 1. Open trackers that appear on the major lists in 2026. Figures are what each operator's page published on 2026 November 8; a dash means the page publishes none or the fact isn't public. Sources are listed in the references.</figcaption>

| Announce address | Since | Software | Published figures |
|---|---|---|---|
| `udp://open.demonii.com:1337/announce` | 2013 | Aquatic | 50.0 million peers, 15.6 million torrents, 462,035 requests a second |
| `udp://tracker.opentrackr.org:1337/announce` | 2015 | opentracker, in 2020 | graphs of peers, torrents, and connections |
| `udp://explodie.org:6969/announce` | — | Aquatic, by another operator's 2023 account | 11.6 million peers and 7.3 million torrents over IPv4, 138,711 requests a second |
| `udp://open.stealth.si:80/announce` | 2016 | opentracker | — |
| `udp://tracker.torrent.eu.org:451/announce` | — | opentracker, on several servers kept in sync | — |
| `udp://exodus.desync.com:6969/announce` | on ngosang's list since 2018 | — | — |
| `wss://tracker.openwebtorrent.com` | — | its own C++ tracker, since 2026 November 4 | 283,572 peers, 155,942 torrents |
| `wss://tracker.webtorrent.dev` | — | Aquatic | 76,563 peers, 34,770 torrents |
| `wss://tracker.files.fm:7073/announce` | on ngosang's list since 2020 | — | — |
| `open.ftorrent.com`, all three protocols | 2026 | Aquatic | live counters at [open.ftorrent.com](https://open.ftorrent.com/) |

</figure>

One person or a small group runs most of these, and many name no one. opentrackr's operator told TorrentFreak in 2020 how it began: "I started the tracker in 2015 when I became frustrated by how many torrents no longer contained any working trackers." explodie.org's page says, in full, "I operate a public bittorrent tracker. Please feel free to use it." torrent.eu.org describes its job in one line, "This tracker's only role is to coordinate peers," and spreads it across several servers on different networks, balanced by DNS and kept in sync over a VPN. Brad Marsden runs `tracker.webtorrent.dev` on Aquatic's WebSocket tracker, and proposed it for create-torrent's defaults in 2023 as "a public WebTorrent tracker I run"; its page says it "is not associated with WebTorrent LLC or Feross Aboukhadijeh." The OpenWebTorrent project ran a Node tracker at `tracker.openwebtorrent.com` until 2026 November 4, when it replaced it with a C++ tracker of its own, its README recording the measurements it took on the live host. ftorrent's own numbers, and what serving them costs, are in [Tracker Load](/tracker-load).

A tracker's address is a hostname, and a hostname can outlive its tracker. `tracker.leechers-paradise.org`, one of the best-known open trackers of the 2010s, today resolves through a CNAME record to `tracker.dler.org`, so every old torrent that names it announces to whoever runs that one. The operator of `tracker.ducks.party` does this on purpose, and says so on its page: "I recover expired tracker domains, and these domains will expire again over time, so new torrents should use the stable announce address." The trackers written into a torrent are names, and whoever holds a name receives its announces.

## The checkers

Two projects check trackers by asking them for peers the way a client would, and between them they supply nearly every list downstream.

### ngosang/trackerslist

[ngosang/trackerslist](https://github.com/ngosang/trackerslist) is Diego Heras's list, kept under the GitHub name ngosang. It has 55,631 stars, and its bot commits once a day, a few minutes after midnight in Madrid. Heras is a longtime qBittorrent contributor, and the feature that carries lists like this one to users is partly Heras's own work: qBittorrent v3.3.0, released 2015 November 29, five months before the list began, credits ngosang with "Automatically add trackers to new downloads."

A tracker gets in by an issue. Its operator, or anyone, opens one; Heras tests the tracker and replies "Added." Removal is automatic: "Tracker is removed after several days of inactivity," Heras wrote in October 2026. A file in the repository, `blacklist.txt`, holds 391 lines of trackers kept off, each with its reason beside it: duplicates, trackers that answer only for registered torrents, trackers that malfunction or report fake seeds, and trackers whose operators asked to be left off.

Heras described the bot's method in 2023. It asks every tracker for peers on five reference torrents, swapped out from time to time, among them popular torrents, a very old one with few seeds, and a fake one. It measures uptime, latency, the numbers of seeds and peers, fake seeds, the server's country, and whether the server sits behind Cloudflare. It drops trackers that are offline, private, or duplicates of another, judging duplicates by domain, address, and seed count, and ranks the rest. The top twenty of that ranking are `trackers_best.txt`. `trackers_all.txt` holds 71 trackers, 44 over UDP, 17 over HTTP, and 10 over HTTPS, and separate files list each protocol, the I2P and Yggdrasil trackers, and every tracker by address instead of by name.

The bot's code is private, and the reason is the list itself. "Publishing the source code will lead to more abandoned forks and more outdated lists," Heras wrote in 2019. "I will keep updating the list and, if I can't at some point, I will release the source code." In 2021 Heras called it "just 200 lines of Python code," and told its history:

> The first implementation was a dirty Python2 script running in a RaspberryPi with a Cron task. … In any case, it was working for 2 years without maintenance, in fact I did not know where the Raspberry was for a few months and it worked the same (I forgot in a rental apartment and the next tenant paid the bills without knowing it).
>
> *Diego Heras, [ngosang/trackerslist issue #72](https://github.com/ngosang/trackerslist/issues/72), November 2021*

The same thread gives a second reason to keep one list instead of many: "If any of the trackers in the list is compromised it will be a big security risk because they can track your IP and the torrents you are downloading. That could happen, of course, but I will be notified and I will take actions." A list this widely read is a trust relationship between its maintainer and every client that reads it.

The thread began in 2018 with a request for IPv6 trackers, when the bot's server had no IPv6. By 2026 Heras's network had been dual stack for years, and the answer had become a design position: the project publishes no list of IPv6-only trackers, because "a public tracker is an infrastructure service and it must be configured with dual IP stack," and "the goal is to reach as many peers as possible, not to have a huge list of small trackers."

The WebSocket file, `trackers_all_ws.txt`, has existed since 2017 December 10 and holds four trackers. The bot opens a real WebSocket to each: an error Heras quoted from it in 2020 reads "Handshake status 403 Forbidden," the wording of Python's websocket-client library. Whether it then sends a WebTorrent announce isn't published.

### newTrackon

[newTrackon](https://newtrackon.com/) checks trackers continuously and publishes their uptime. It continues Trackon, which Uriel, of [cat-v.org](http://uriel.cat-v.org/), built on Google App Engine; the [repository](https://github.com/CorralPeltzer/newTrackon)'s history opens with Uriel's commits, from 2009 September 21 to 2011 July 6. Trackon then went quiet, and when TorrentFreak covered its revival in 2017, it wrote that the project had disappeared after its owner "reportedly passed away." Miguel Ángel Corral Peltzer, a student who had once exchanged emails with Uriel, rewrote it, because "the original Trackon used Google App Engine which can only process HTTP communications," and most open trackers answer over UDP. His commit of 2016 November 29 reads "Rewrite of almost all the python code, add UDP support, project taken out of the Google App engine," and he has maintained it since, with 69 commits in 2026.

Each check is a real announce, with a new random info hash, sent at the tracker's own announce interval; newTrackon rejects a tracker whose interval is under five minutes or over three hours. A check succeeds when the tracker answers with a valid list of peers. Since 2026 September a tracker that answers with more than ten peers fails, since no real swarm exists for a random hash and any peers beyond noise are invented. The FAQ defines uptime and its limit plainly: "Uptime is calculated based on the percentage of valid responses to the last 1000 attempts to contact the tracker. Because the interval between attempts will depend on the interval for that tracker and other factors, comparing the uptime values of different trackers is not completely 'fair'." The stable list is every tracker at 95 percent or better that was added at least ten days ago, and newTrackon deletes a tracker that hasn't answered once in a year and a half.

Anyone can submit a tracker through a form. newTrackon refuses a submission if the tracker isn't working at that moment, if its hostname or any of its addresses is already listed, if its DNS answers with a private or loopback address, or if the tracker's DNS TXT record, under [BEP 34](https://www.bittorrent.org/beps/bep_0034.html), says it doesn't want announces; the same record can move a listed tracker to another port or protocol. Every 24 hours newTrackon also pulls ngosang's `trackers_all.txt` into its own queue. The flow runs one way: asked in June 2026 about trusting newTrackon's list, Heras answered, "It's not related to this project. I don't know who is behind."

newTrackon publishes its lists as plain text at `/api/stable`, `/api/all`, `/api/live`, `/api/udp`, and `/api/http`, open to any page's requests. On 2026 November 8 newTrackon tracked 263 trackers, 76 of them answering and 48 in the stable list. 129 of the 263 were added in 2026.

Two limits follow from the design. newTrackon keeps one row per hostname, so it lists and checks a tracker that serves UDP and HTTPS at one name on one protocol only. And reading its code, each check goes to one of the hostname's addresses, so a tracker that answers over IPv4 and fails over IPv6 can still show 100 percent.

### A stricter fork, and a tool

[newTrackon NZ](https://newtrackon.co.nz/), a fork its author started on 2026 October 3, tests what newTrackon leaves out: it checks IPv4 and IPv6 separately, has a second client in another country confirm that the tracker hands out the first client's real address, and measures latency from four regions. No list draws from it. The [Torrust](https://github.com/torrust/torrust-tracker) project's tracker checker is a command-line tool for checking a list of UDP and HTTP trackers by hand, with a real announce and scrape against each.

## The lists built on them

Downstream of the two checkers sit the aggregators, lists that merge other lists. Table 2 gives the ones read in 2026, among them one that stopped updating in 2024 and is still merged by four of the others.

<figure class="breakout">
<figcaption>Table 2. Tracker lists on GitHub and how each is built, read from each repository's workflow and scripts, 2026 November 8. Stars and dates from GitHub.</figcaption>

| List | Kept by | Since | Stars | Updated | Its own check | Draws from |
|---|---|---|---:|---|---|---|
| [ngosang/trackerslist](https://github.com/ngosang/trackerslist) | Diego Heras | 2016 | 55,631 | daily | announces on five reference torrents | issues |
| [XIU2/TrackersListCollection](https://github.com/XIU2/TrackersListCollection) | XIU2 | 2019 | 32,384 | daily | DNS, then a TCP connection for HTTP, HTTPS, and WebSocket; none for UDP | ngosang, newTrackon, the itzmx list, animeTrackerList |
| [adysec/tracker](https://github.com/adysec/tracker) | AdySec | 2024 | 1,773 | every 4 hours | an HTTP announce with an all-zero hash; UDP's connect step only; a TCP connection for WebSocket | 38 sources, the lists above among them |
| [Tunglies/TrackersList](https://github.com/Tunglies/TrackersList) | Tunglies | 2023 | 328 | daily | none | ngosang, XIU2, hezhijie0327, animeTrackerList |
| [hezhijie0327/Trackerslist](https://github.com/hezhijie0327/Trackerslist) | hezhijie0327 | 2020 | 84 | daily | DNS, then whether the host answers at all | ngosang, XIU2, newTrackon, animeTrackerList, the itzmx list |
| [DeSireFire/animeTrackerList](https://github.com/DeSireFire/animeTrackerList) | DeSireFire | 2019 | 4,862 | last in 2024 January | — | — |

</figure>

XIU2's collection is the largest of them, and it is written first for users in mainland China; its README explains that most home broadband there has no public IPv4 address, and recommends IPv6. Its workflow, which moved from XIU2's own server to GitHub Actions in 2026 March, merges ngosang's lists, newTrackon's live and stable lists, a list kept by hand by the operator of the itzmx trackers ([1265578519/OpenTracker](https://github.com/1265578519/OpenTracker)), and animeTrackerList. Its best list is everything that came from a best-type source, sorted alphabetically rather than ranked, and eleven of its 72 entries trace only to animeTrackerList, which hasn't changed since 2024 January. It is published at `trackerslist.com` and on several mirrors.

The other aggregators fan out from there. adysec merges 38 sources every four hours, among them ngosang, XIU2, newTrackon, and several of the lists in Table 2, and keeps what passes its checks; Tunglies joins four repositories' files with no check at all; hezhijie0327 rewrites any `udp://` or `http://` address on port 443 as `https://`. So the original checking happens in two places, ngosang's bot and newTrackon. newTrackon reads ngosang every day, XIU2 reads both, and each list further down reads some combination of the three. A tracker submitted to ngosang can reach each of the active lists in Table 2 within weeks without its operator asking anyone else, and its uptime is judged along the way by two independent checks, whose maintainers work separately.

## The clients that read the lists

The lists reach users through a few clients and tools that fetch them.

- **qBittorrent** has added trackers to new downloads automatically since v3.3.0, in 2015, from a list typed into its settings. Since v5.1.0, released 2025 April 27, it can fetch that list from a URL instead, in a feature by Thomas Piccirello; the label reads "Automatically append trackers from URL to new downloads," qBittorrent refreshes the list every 24 hours and skips private torrents, and it sets no URL by default. v5.2.0 saves the fetched list so it is available at the next start. The qBittorrent Enhanced Edition fork sets ngosang's `trackers_best.txt` as its default URL, with fetching off.
- **Transmission** added a `default-trackers` setting in 4.0.0, in 2023 February, for trackers announced in every public torrent. The user types the list in; Transmission fetches nothing.
- **BiglyBT** has had tracker templates since 2017, and since 1.5.0.0, 2018 April 10, a template can fetch its list from a URL. Separately, it ships one built-in tracker, `udp://tracker.opentrackr.org:1337/announce`, which it uses to find a magnet link's metadata.
- **aria2** takes extra trackers on its command line with `--bt-tracker`. A widely used aria2 configuration, P3TERX's [aria2.conf](https://github.com/P3TERX/aria2.conf), with 3,422 stars, includes a script that fetches XIU2's list into that setting.

Libraries are the other route, and they work differently: instead of fetching a list, they ship one fixed at a release. [libtorrent](https://github.com/arvidn/libtorrent) ships no trackers at all; the hosts it names by default are a DHT bootstrap node and a STUN server. [create-torrent](https://github.com/webtorrent/create-torrent), the WebTorrent library that builds new torrents, writes this list into any torrent created without one of its own:

```
udp://tracker.leechers-paradise.org:6969
udp://tracker.coppersurfer.tk:6969
udp://tracker.opentrackr.org:1337
udp://explodie.org:6969
udp://tracker.empire-js.us:1337
wss://tracker.btorrent.xyz
wss://tracker.openwebtorrent.com
wss://tracker.webtorrent.dev
```

`tracker.leechers-paradise.org` now resolves to another tracker, `tracker.coppersurfer.tk` doesn't answer, and `tracker.btorrent.xyz` resolves to `127.0.0.1`, so three of the eight trackers in a torrent made with these defaults today are gone or answer as another tracker. [Trystero](https://github.com/dmotz/trystero), Dan Motzenbecker's library that uses WebTorrent trackers to introduce peers in web apps, ships five WebSocket trackers and uses the first three by default. A list a client fetches each day inherits its checker's judgment; a list built into a library is only as current as the library's last release.

## How trackers come and go

The largest open trackers of 2010 are gone. That June, TorrentFreak counted the largest public trackers by peers, and PublicBitTorrent and OpenBitTorrent led, with 21,694,091 and 21,186,589, both on opentracker. Both dropped TCP the next year to answer over UDP only, and in 2012 July they went offline in protest. Old torrents, and users adding the HTTP addresses, kept clients sending HTTP announces, and the trackers would stay offline, TorrentFreak reported from OpenBitTorrent's operator, "until a solution is found for the massive waste of bandwidth that BitTorrent users generate as the result of incorrect tracker announces." Within days µTorrent's beta began checking whether a tracker answers over UDP before trying its HTTP address, and the two came back on August 2, after three weeks. Neither hostname resolves today.

create-torrent's default list keeps a dated record of the turnover, since each change is a commit. Its first list, in 2014 May, named PublicBT and OpenBitTorrent alone. On 2015 December 1 a commit titled "remove dead trackers from the defaults list" took out PublicBT and demonii, and Coppersurfer and Leechers Paradise took their places. explodie and opentrackr joined in 2017 January, OpenBitTorrent left that March, and `tracker.webtorrent.dev` joined in 2023 June. demonii itself is back, and publishes the largest numbers in Table 1.

ngosang's history measures the same turnover. Counting the distinct hostnames in `trackers_all.txt`, the first commit, on 2016 April 24, named 49, and three of them are listed today. On 2026 January 1 the list named 83, and 32 of those are still listed ten months later. newTrackon's table tells it from the other side: of its 263 trackers, 129 were added in 2026, and most of the trackers it still carries have stopped answering.

The last academic studies of the population are old. Neglia and colleagues, at INFOCOM 2007, probed more than 1,400 trackers, usually every fifteen minutes, and saw the number answering fall from 416 to 354 over 58 days in 2006, from May 27 to July 24; assuming independent lifetimes, they "roughly estimate that the tracker lifetime is about 390 days." Zhang, Dhungel, Wu, and Ross, publishing in 2011, found 38,996 trackers named by five discovery sites over nine months, and on the morning of 2009 April 22, "only 728, less than 2%, of these trackers are active," counting a tracker active if it returned at least one peer. No later study of the open-tracker population turned up in the research for this article, though the daily histories of ngosang's list since 2016 and XIU2's since 2019 hold the data for one.

## What no one checks

The checking described above leaves three gaps, and the largest is WebSocket.

- **No one checks a WebSocket tracker with a real announce.** newTrackon accepts only `udp://`, `http://`, and `https://` addresses. A request for WebSocket support, [issue #7](https://github.com/CorralPeltzer/newTrackon/issues/7), was opened in 2017 May; in 2021 August Corral answered a volunteer, "Hi! Sure, PRs are welcomed," and in 2024 September the issue's author closed it, with no code merged. newTrackon NZ refuses WebSocket too. XIU2 and adysec open a TCP connection to the port, with no TLS, no WebSocket upgrade, and no announce, which shows only that something listens there. ngosang's bot completes the WebSocket handshake, and whether it announces isn't published. Meanwhile adysec's and XIU2's parsers keep only addresses ending in `/announce`, which three of the four WebSocket trackers in ngosang's file don't use, so WebSocket trackers rarely travel past ngosang's file, and a dead one can sit in a library's defaults with no list to say so.
- **Few checks test both address families.** newTrackon's check goes to one address, and its IPv4-only and IPv6-only lists sort trackers by their DNS records, not by what answered. ngosang publishes no IPv6-only list, by design. Only newTrackon NZ tests each family separately.
- **The most widely read checker is private.** ngosang's bot is closed by its author's choice, with a stated promise to publish it if Heras can no longer keep the list, so the method behind the list most clients read is known only from Heras's descriptions of it.

## One tracker through the lists

`open.ftorrent.com` serves all three protocols at one hostname: `udp://open.ftorrent.com:443/announce`, `https://open.ftorrent.com/announce`, and `wss://open.ftorrent.com`. Here is where each one appears on 2026 November 8, and why.

- **ngosang** lists the UDP address 43rd of 71 in `trackers_all.txt`, and the HTTPS address 51st, as `https://open.ftorrent.com:443/announce`. Neither is in `trackers_best.txt`. The WebSocket address is fourth of four in `trackers_all_ws.txt`, as `wss://open.ftorrent.com:443`.
- **newTrackon** lists the UDP address only, since it keeps one protocol per hostname, in its stable list at 99.20 percent. newTrackon NZ lists it among the trackers that pass all its tests over both families.
- **XIU2** carries the UDP address in its best list, which it reached through newTrackon's stable list, and the HTTPS address in its full list. It has no WebSocket address, because the one in ngosang's file doesn't end in `/announce`.
- **hezhijie0327** carries only `https://open.ftorrent.com:443/announce`, its script having rewritten the UDP address on port 443 as HTTPS. **adysec** and **Tunglies** carry the UDP and HTTPS addresses.
- **Trystero** lists `wss://open.ftorrent.com` first among its default trackers since its 0.25.3 release, on 2026 July 13. It reached Trystero through a pull request from ftorrent, not through any list.

So the UDP address reached every list and the HTTPS address most of them, while the WebSocket address reached one list, and its place in a library's defaults came from asking that library directly.

## References

1. Heras, D. (ngosang). *trackerslist.* GitHub, 2016–2026. https://github.com/ngosang/trackerslist
2. Heras, D. Reply in *How does the best ranking work?* ngosang/trackerslist issue #463, August 2023. https://github.com/ngosang/trackerslist/issues/463
3. Heras, D. Replies in *IPv6 trackers.* ngosang/trackerslist issue #72, January 2018 to September 2026. https://github.com/ngosang/trackerslist/issues/72
4. Heras, D. Replies in *Is your bot open source?* ngosang/trackerslist issue #51, 2017–2018. https://github.com/ngosang/trackerslist/issues/51
5. Heras, D. Reply in *Tracker contribution.* ngosang/trackerslist issue #687, October 2026. https://github.com/ngosang/trackerslist/issues/687
6. Heras, D. Reply in *About newTrackon's list.* ngosang/trackerslist issue #653, June 2026. https://github.com/ngosang/trackerslist/issues/653
7. Heras, D. Reply in *Finding More websocket trackers WebTorrent.* ngosang/trackerslist issue #257, June 2020. https://github.com/ngosang/trackerslist/issues/257
8. qBittorrent. *Changelog.* v3.3.0, 2015 November 29; v5.1.0, 2025 April 27; v5.2.0, 2026 May 3. https://github.com/qbittorrent/qBittorrent/blob/master/Changelog
9. Corral Peltzer, M. A. *newTrackon.* https://newtrackon.com/ and https://github.com/CorralPeltzer/newTrackon
10. newTrackon. *FAQ.* Fetched 2026 November 8. https://newtrackon.com/faq
11. Uriel. *Trackon,* 2009–2011, preserved in the newTrackon repository's history. https://github.com/CorralPeltzer/newTrackon/commits/master and http://uriel.cat-v.org/
12. *[Request] Web Torrent Support.* newTrackon issue #7, 2017 May to 2024 September. https://github.com/CorralPeltzer/newTrackon/issues/7
13. KolanTheKoala. *newTrackon NZ.* https://newtrackon.co.nz/ and https://github.com/KolanTheKoala/newtrackon-nz
14. Torrust. *Torrust Tracker,* tracker checker in `console/tracker-client`. https://github.com/torrust/torrust-tracker
15. XIU2. *TrackersListCollection.* GitHub, 2019–2026, and its workflow, https://github.com/XIU2/TrackersListCollection/blob/master/.github/workflows/update-trackers.yml. https://github.com/XIU2/TrackersListCollection
16. AdySec. *tracker.* GitHub, 2024–2026, with its checker in `tracker-checker/src/main.rs`. https://github.com/adysec/tracker
17. Tunglies. *TrackersList.* GitHub, 2023–2026. https://github.com/Tunglies/TrackersList
18. hezhijie0327. *Trackerslist.* GitHub, 2020–2026. https://github.com/hezhijie0327/Trackerslist
19. DeSireFire. *animeTrackerList.* GitHub, 2019–2024. https://github.com/DeSireFire/animeTrackerList
20. 1265578519. *OpenTracker,* with its hand-kept `tracker.txt`. https://github.com/1265578519/OpenTracker
21. erdgeist. *opentracker.* https://erdgeist.org/arts/software/opentracker/
22. Frostegård, J. *Aquatic.* https://github.com/greatest-ape/aquatic
23. Demonii. https://demonii.com/, fetched 2026 November 8.
24. opentrackr. https://opentrackr.org/, fetched 2026 November 8.
25. explodie.org. *open bittorrent tracker* and *Tracker Live Statistics.* Fetched 2026 November 8. https://explodie.org/opentracker.html and https://explodie.org/tracker-stats.html
26. Stealth. https://stealth.si/, fetched 2026 November 8.
27. torrent.eu.org. https://torrent.eu.org/, fetched 2026 November 8.
28. Marsden, B. (SilentBot1). *Update docs and add tracker.webtorrent.dev to the default announceList.* create-torrent pull request #226, 2023 June 3. https://github.com/webtorrent/create-torrent/pull/226
29. tracker.webtorrent.dev. https://webtorrent.dev/, fetched 2026 November 8.
30. OpenWebTorrent. *openwebtorrent-tracker.* README, 2026 November; and https://tracker.openwebtorrent.com/stats.json, fetched 2026 November 8. https://github.com/OpenWebTorrent/openwebtorrent-tracker
31. tracker.ducks.party. https://tracker.ducks.party/, fetched 2026 November 8.
32. BitTorrent.org. *The BitTorrent Protocol Specification.* BEP 3. https://www.bittorrent.org/beps/bep_0003.html
33. BitTorrent.org. *UDP Tracker Protocol for BitTorrent.* BEP 15. https://www.bittorrent.org/beps/bep_0015.html
34. BitTorrent.org. *DNS Tracker Preferences.* BEP 34. https://www.bittorrent.org/beps/bep_0034.html
35. Piccirello, T. *Support fetching tracker list from URL.* qBittorrent pull request #21828, merged 2025 January 8. https://github.com/qbittorrent/qBittorrent/pull/21828
36. qBittorrent Enhanced Edition. `src/base/preferences.cpp`, v5_2_x branch. https://github.com/c0re100/qBittorrent-Enhanced-Edition/blob/v5_2_x/src/base/preferences.cpp
37. Transmission. *4.0.0* release notes, 2023 February. https://github.com/transmission/transmission/releases/tag/4.0.0
38. BiglyBT. *ChangeLog,* https://github.com/BiglySoftware/BiglyBT/blob/master/ChangeLog.txt; *Tracker Templates and Lists,* https://github.com/BiglySoftware/BiglyBT/wiki/Tracker-Templates-and-Lists; and `MagnetPlugin.java`, https://github.com/BiglySoftware/BiglyBT/blob/master/core/src/com/biglybt/plugin/magnet/MagnetPlugin.java
39. aria2. *aria2c(1),* `--bt-tracker`. https://aria2.github.io/manual/en/html/aria2c.html
40. P3TERX. *aria2.conf,* `tracker.sh`. https://github.com/P3TERX/aria2.conf/blob/master/tracker.sh
41. libtorrent. `src/settings_pack.cpp`, RC_2_1. https://github.com/arvidn/libtorrent/blob/RC_2_1/src/settings_pack.cpp
42. WebTorrent. *create-torrent,* `index.js` and its history. https://github.com/webtorrent/create-torrent/blob/master/index.js and https://github.com/webtorrent/create-torrent/commits/master/index.js
43. Motzenbecker, D. *Trystero,* `CHANGELOG.md` and `packages/torrent/src/index.ts`. https://github.com/dmotz/trystero/blob/main/CHANGELOG.md and https://github.com/dmotz/trystero/blob/main/packages/torrent/src/index.ts
44. Van der Sar, E. *The World's 5 Largest Public BitTorrent Trackers.* TorrentFreak, 2010 June 14. https://torrentfreak.com/the-worlds-5-largest-public-bittorrent-trackers-100614/
45. Van der Sar, E. *Largest BitTorrent Trackers Go Offline in Protest,* 2012 July 16, https://torrentfreak.com/largest-bittorrent-trackers-go-offline-in-protest-120716/; *BitTorrent Trackers Cease Strike After Protocol Improvement,* 2012 July 29, https://torrentfreak.com/bittorrent-trackers-cease-strike-after-protocol-improvement-120729/; and *PublicBitTorrent and OpenBitTorrent Trackers Return,* 2012 August 2, https://torrentfreak.com/publicbittorrent-and-openbittorrent-trackers-return-120802/. TorrentFreak.
46. Neglia, G., Reina, G., Zhang, H., Towsley, D., Venkataramani, A., and Danaher, J. *Availability in BitTorrent Systems.* IEEE INFOCOM 2007, pp. 2216–2224. https://doi.org/10.1109/INFCOM.2007.256; authors' copy, http://www-sop.inria.fr/members/Giovanni.Neglia/publications/neglia07infocom.pdf
47. Zhang, C., Dhungel, P., Wu, D., and Ross, K. W. *Unraveling the BitTorrent Ecosystem.* IEEE Transactions on Parallel and Distributed Systems 22(7), pp. 1164–1177, July 2011. https://doi.org/10.1109/TPDS.2010.123; authors' copy, https://cse.engineering.nyu.edu/~ross/papers/PublicEcosystem.pdf
48. ftorrent. *How the DHT Works.* https://docs.ftorrent.com/how-the-dht-works.html
49. ftorrent. *Software Selections.* https://docs.ftorrent.com/software-selections.html
50. ftorrent. *Tracker Load.* https://docs.ftorrent.com/tracker-load.html
51. Maxwell, A. *NewTrackon Tracks Torrent Trackers.* TorrentFreak, 2017 January 8. https://torrentfreak.com/newtrackon-tracks-torrent-trackers-170108/
52. Van der Sar, E. *Opentrackr Coordinates Millions of Torrent Downloads on an Old Dell Server.* TorrentFreak, 2020 May 9. https://torrentfreak.com/opentrackr-coordinates-millions-of-torrent-downloads-on-an-old-dell-server-200510/
