---
title: Open Address, Closed Device
description: A measurement of the two forces acting on the ordinary web click, forty-one months of the web requests people made, cut by whether the device is one its owner controls and whether the address is one others can reach.
---

# Open Address, Closed Device

> Prepared by [Claude Code](https://claude.ai/code) using Fable 5.1
> <br>Published: 2026 October 12
> <br>Revised: 2026 October 12
> <br>Period: 2023 May through 2026 September, monthly
> <br>Code: [docs/research/ipv6](https://github.com/zootella/ftorrent/blob/master/docs/research/ipv6/README.md)
> <br>Data: [docs/research/ipv6/data](https://github.com/zootella/ftorrent/tree/master/docs/research/ipv6/data)
> <br>Cite as: ftorrent. *Open Address, Closed Device.* October 2026. https://docs.ftorrent.com/open-address-closed-device.html
> <br>Keywords: IPv6 adoption, open and closed devices, web requests, Cloudflare Radar, peer-to-peer reachability

## Abstract

Two properties of a web request describe the standing of the person who made it: whether the device is one its owner controls, and whether the address is one others can reach. Google's IPv6 adoption chart, the most cited measure of the second, has climbed since 2008 to nearly half of Google's users; no series tracks the first, or the two together. This report measures both on the same requests. Cloudflare Radar classifies every HTTP request Cloudflare serves as likely human or likely automated. We take the human requests for each month from May 2023 through September 2026 and cut them two ways: by device, open (Windows, macOS, and Linux, where the owner holds root) against closed (iOS, Android, ChromeOS, and smart TV, where the vendor holds root), and by address family, IPv4 against IPv6. The joint table is built from the operating system shares and checked against Cloudflare's direct figures; the IPv6 share is cross-checked against Google and APNIC, and the open-device share against StatCounter and Wikimedia. In September 2026, 46.6% of requests were made on open devices and 41.0% came from IPv6 addresses; 13.7% were both, up from 9.5% in May 2023, about half of the rise reflecting Cloudflare's operating system classification rather than people's devices. The IPv6 share was 29.3% on open devices against 48.0% on closed, and over the 41 months it rose 2.8 points on open devices and 8.8 on closed. No measure shows the open-device share falling: the migration to closed devices happened between about 2011 and 2017, and the share has stayed between about 37% and 46% since. Two requests drawn at random were both on open devices and from IPv6 addresses 1.9% of the time. IPv6 is arriving through phones and carriers, and slowest on the devices that could use a reachable address.

## 1. Introduction

Google has published [one chart](https://www.google.com/intl/en/ipv6/statistics.html) continuously since 2008: the share of its users who reach it over [IPv6](https://en.wikipedia.org/wiki/IPv6). The line starts at a fraction of a percent, bends upward around 2012, and in late 2026 sits near half. It is one of the most encouraging pictures in networking, because an IPv6 address is a globally routable address — a device that holds one can be reached directly, by anyone, with no translation box in the way. Every point of that line is a few more people whose computers can be reached, and not only reach out.

But the line counts users, not the devices they hold or the software those devices will run. And over the same fifteen years a second shift happened on the other side of the same screen. Picture one household with a Windows PC and an iPhone. In one year, the person uses both about equally — books and clothes bought in a browser on the PC, the phone for messages and maps. The next year, banking, shopping, travel, and news have all moved into apps, and the phone carries four interactions in five. The person deals with the same bank, the same store, the same airline, and nothing about them has changed. What changed is where their clicks happen. On the PC, they could install any software they chose, including a program that opens a port and listens. On the phone, which they also paid for and also own, the vendor decides what runs. And because that phone is on a cellular carrier, it very likely reached each of those apps from an IPv6 address. Their address got better. Their choice of software got narrower. Google's chart shows the first. Nothing shows the second.

Those two things, the address and the device, set a person's standing on the internet, and the household noticed neither. The first decides whether others can reach them. For most of the web's life it was the limit: behind a shared IPv4 address a person could reach out but not be reached, and the IPv6 migration has been lifting that limit year by year. The second decides whether the device in their hand will run what they choose, and across the same years people replaced open devices with closed ones. They paid for them and own them, and the vendor holds root and decides what they may run.

This report measures both forces together, on the same requests. For every month from May 2023 through September 2026, it asks what share of the web requests people made were made on a device whose owner can run software of their own choosing, and what share of those came from an IPv6 address — and so what share of the whole is both. The newest month is drawn as one picture, the whole of a month's requests as one mosaic plot. The forty-one months are drawn as lines. And because the two forces have been watched separately for much longer than that, the report sets the lines beside the two longest records of each: Google's since 2008, and [StatCounter](https://gs.statcounter.com/)'s count of page views from desktop computers since 2009.

## 2. Definitions

The words that carry this report are few. The data sorts everything it saw into two classes first, **automated requests** and the **requests** people made, and the next section explains that sorting, sets the automated class aside, and fixes the word *request*, for the rest of the report, to mean a request a person made. Every request is then cut two ways, one preposition each. It was made **on an open device**, one whose owner holds root, or **on a closed device**, one where the vendor holds root. And it came **from an IPv6 address** or **from an IPv4 address**. The cell this report is about is the request made on an open device from an IPv6 address.

### 2.1 Requests, and automated requests

Cloudflare's data holds every HTTP request its network answered, and before anything else it sorts them into two classes. One class is the request a person causes when they do something on the web: tap an app, click a link, type an address, pick up the phone or sit down at the keyboard. The other is everything no person caused directly: crawlers indexing pages, monitors polling endpoints, scripts and agents calling APIs on a schedule, machine-to-machine traffic of every kind. In September 2026 that second class was more than a third of all traffic, and it is growing. The data carries both classes and keeps them apart, so it could be used to study the relationship between them, how much of the web is people and how much is machinery, and the report's tables and scripts carry both through for that reason: Table 1 shows the whole of a month with the automated row in place, and Section 5.5 sets Cloudflare's automated share beside Wikimedia's.

But that is not this report's question. The two forces it measures act on a person at a device: the software that device will run, and the address it holds. An automated request has no person at it, no one choosing to pick up a phone rather than open a laptop, so it says nothing about either force. The report therefore counts the automated class, sets it aside, and from here on uses the word **request** to mean a request a person made. Automated requests reappear only where they are named, and **all traffic** means both classes together, which is the denominator whenever the automated share is quoted. The same convention holds for the page views that StatCounter and Wikimedia count: a page view means one a person made, unless the report says otherwise. The sorting itself is Cloudflare's bot score, described in Section 3.2, and it is a proxy: some automation passes for a person, and Section 5.2 says what that does to the numbers.

### 2.2 On an open device, or a closed one

An **open device** is one whose owner holds root — [Windows](https://www.microsoft.com/windows), [macOS](https://www.apple.com/macos/), and [Linux](https://www.kernel.org/) computers. On these, the owner can install software from any source, including software no vendor reviewed or signed, and that software can run in the background and open a listening socket. These are the devices that can be peers in the fullest sense, a peer being a computer that can be reached as well as reach out, running software its owner chose.

A **closed device** is one where the vendor holds root — phones and tablets running [iOS](https://www.apple.com/ios/) or [Android](https://www.android.com/), [Chromebooks](https://www.google.com/chromebook/), smart TVs. The person paid for the device and owns it. The vendor holds root, and decides what it may run. Closed here is about who holds root, not about source code: Android's system is open source, and the device is closed all the same.

Root is a proxy for the property that matters for peer-to-peer: whether the owner can run a program of their own choosing that stays running, listens for inbound connections, and talks to the network directly. On an open device all three are ordinary. On a closed device each is constrained by policy the owner did not set: what may be installed, what may run in the background, what a background process may do. Who holds root is the cleanest single line through those constraints, and it is a property of the operating system, which the data sources report. So open and closed are decided by operating system, not by screen size. Vendors have reasons for closing a device, security and battery life among them, and this report does not weigh those reasons against the cost. It counts the cost, in the one dimension it can measure.

Android is the boundary case. Android lets its owner install software from outside the store, which is more than iOS allows, and [BitTorrent clients run on it today](https://f-droid.org/packages/org.proninyaroslav.libretorrent/). What the owner still does not hold is root: the vendor's signed system decides what an installed program may do, and background execution and network access are governed by policy the owner cannot override without unlocking the device in ways most hardware forbids or penalizes. So Android counts as closed here, with that explained rather than assumed. Chromebooks fall the same way: a closed, vendor-verified system with [a sandboxed container](https://chromeos.dev/en/linux) as the escape hatch.

One more line, between the device and its browser. A closed device's browser can still be a peer of a narrower kind — a [WebRTC](https://webrtc.org/) connection from a phone's browser tab is real peer-to-peer, and [ftorrent's own browser work](/how-peers-connect) depends on it. This report measures the ability to run native software of the owner's choosing. It does not claim a closed device can do nothing; it claims the owner does not decide.

### 2.3 From an IPv6 address, or an IPv4 one

The second cut is simple: did the request come from an IPv4 address or an IPv6 address? Every source here reports that directly, because a server sees the address family of every connection it accepts. The figures and tables say **v6 address** and **v4 address** for short.

The cut means one specific thing. A request from an IPv6 address shows the device had working IPv6 at that moment on that connection — a globally routable address, with no address translation between it and the internet. That is the precondition for being reached directly, and it is what this report measures.

It understates what the devices could do, and the error runs one way. A device with both kinds of address resolves both and [races them](https://www.rfc-editor.org/rfc/rfc8305), with IPv6 given a short head start, so on a healthy connection nearly every request to a dual-stack site goes out from the IPv6 address. Such a device still lands in the IPv4 column three ways: the site it asked for is IPv4-only, so there was nothing to race; the IPv6 path was slow or broken at that moment and the race picked IPv4; or the app used an older networking library that only asks for IPv4. The reverse cannot happen. Nothing sends from an IPv6 address a device doesn't have, now that the old tunneling mechanisms have faded — [Google's data](https://www.google.com/intl/en_ALL/ipv6/statistics/data/adoption.js) shows native and total IPv6 identical in 2026. So every IPv6 figure in this report is a floor on IPv6 capability, and a Windows PC that reached an IPv4-only site is the clearest case: it counts as IPv4 here, while a BitTorrent client on it would use IPv6 for any peer that had it. The client-side part of this gap can be bounded, because [APNIC measures both](https://stats.labs.apnic.net/ipv6) whether a user *can* use IPv6 and whether they *prefer* it when offered both: in the world figures for September 2026 the two differ by about two points, so roughly one capable user in twenty ends up on IPv4 when both are offered. The site-side part cannot be measured from these sources, and it is why [Cloudflare's own adoption chart switched in January 2023](https://developers.cloudflare.com/radar/glossary/) to counting IPv6 requests only against requests for dual-stacked content. The API this report uses gives the plain share, IPv6 requests against every request, and the comparison to the user-weighted sources below brackets the difference.

Having an address is not the same as being reachable, and this report leaves reachability outside its scope. Several things sit between an address and an inbound connection. Nearly every IPv6 home router runs [a stateful firewall](https://www.rfc-editor.org/rfc/rfc6092) that admits inbound traffic only on flows the inside device opened first; in practice [the simple hole-punching exchange](/how-peers-connect) works through that filter most of the time, because nothing is translating the address, only screening it. On the IPv4 side the picture runs the other way: a growing share of IPv4 connections sit behind [carrier-grade NAT](https://arxiv.org/abs/1605.05606), two layers of translation where hole punching often fails outright, and that translation is invisible from the server, so no source here can count it. Some IPv4 consumers hold a static public address, which is rare but makes them reachable with nothing in the way. All of it pushes in known directions: IPv6 tends to be more reachable than its address count suggests, IPv4 less. None of it is measured here. This report counts address families, and reachability would be a report of its own.

## 3. Data

### 3.1 The unit of measurement

The unit of measurement here is the request — the HTTP request a browser or app sends when a person does something — and this is a deliberate choice rather than a limitation. The alternative would be to count people, the way Google's chart does, and for this question that would hide what is being measured. The household in the introduction is one person in both years. A count of people sees no change. A count of requests sees the migration directly, because the migration *is* a migration of requests from one device to another. The freedom in question lives in the click: each interaction either happens on a device where the owner chose the software or on one where the vendor did. So the measure follows the click.

Three of the sources below count something slightly different from one another, and the report names which wherever it shows a number. [Cloudflare](https://www.cloudflare.com/) counts HTTP requests. StatCounter and [Wikimedia](https://www.wikimedia.org/) count page views, which sit closer to a single human action — one page view may be a dozen requests, and a mobile page and a desktop page do not fire the same number. All three are proxies for the click, not the click itself. The user-weighted sources, Google and [APNIC](https://www.apnic.net/), measure something different again, and where this report compares them to the request-weighted figures it expects a gap and reads the gap as information: an IPv6-heavy phone generating a growing share of requests shows up as request-weighted IPv6 adoption running ahead of user-weighted adoption.

### 3.2 Sources

No single public dataset reports the kind of address, the operating system, and whether a person made the request, all together, over time. The method here uses one source for the joint picture and four others to check its edges.

**[Cloudflare Radar](https://radar.cloudflare.com/)** is the source for the joint cut. Cloudflare fronts a large fraction of all websites, and its [Radar API](https://developers.cloudflare.com/api/resources/radar/) reports the share of HTTP requests by IP version, by operating system, by device type, and by bot class, with any of those applied as a filter on the others. [Bot class](https://developers.cloudflare.com/radar/concepts/bot-classes/) is Cloudflare's [bot score](https://developers.cloudflare.com/bots/concepts/bot-score/): requests scoring 1 through 29 are classed as likely automated, 30 and above as likely human. Those two classes are the sorting Section 2.1 describes, and the classification is a proxy — an app's background refresh on a phone, caused by a person indirectly, lands on the person side — and the report treats it as one. Radar's numbers are shares of requests seen by Cloudflare's network, which is a sample of the web with its own shape: sites that chose Cloudflare, nearly all of them dual-stacked by default. The API is free with an account and [a read-only token](https://developers.cloudflare.com/radar/get-started/first-request/), and [its data is published](https://radar.cloudflare.com/about) under a [Creative Commons Attribution-NonCommercial license](https://creativecommons.org/licenses/by-nc/4.0/).

**[Google's IPv6 statistics](https://www.google.com/intl/en/ipv6/statistics.html)** is the chart this report sets out to complete: the share of Google's users reaching it over IPv6, daily since September 2008, published as [a data file anyone can fetch](https://www.google.com/intl/en_ALL/ipv6/statistics/data/adoption.js). It counts users, not requests, and has no device or operating-system dimension. It is the report's first cross-check on the IPv6 share.

**[APNIC Labs](https://stats.labs.apnic.net/ipv6)** measures IPv6 capability with a small script delivered inside online advertisements, which gives it a sample drawn from the general population of web users rather than from any one site. It publishes [daily figures](https://data1.labs.apnic.net/v6stats/v6region/XA.json) since October 2013 for the world, every region, every country, and every network, in [a documented format](https://data1.labs.apnic.net/ipv6-data-format.html) — and, alone among these sources, the raw sample count beside every percentage, about thirty million measurements a day in 2026. It also counts users. It is the second, independent cross-check on the IPv6 share.

**[StatCounter Global Stats](https://gs.statcounter.com/)** reports the share of page views [by platform](https://gs.statcounter.com/platform-market-share/desktop-mobile-tablet/worldwide) — desktop, mobile, tablet — and [by operating system](https://gs.statcounter.com/os-market-share), monthly since January 2009, from more than three billion page views a month across more than a million sites carrying its tracking code, [with known bots excluded](https://gs.statcounter.com/faq). It has no IP version dimension. It is the cross-check on the open-versus-closed split, and because it reaches back to 2009 it is also the long record of the second force on its own.

**[Wikimedia's pageview statistics](https://wikitech.wikimedia.org/wiki/Analytics/AQS/Pageviews)** report page views across all Wikimedia projects by access method — desktop, mobile web, mobile app — and by agent type — user, spider, automated — monthly since July 2015, from [a public API](https://wikimedia.org/api/rest_v1/#/Pageviews%20data) with no key. It is the only one of these sources that reports the automated share directly, and it is the second cross-check on the open-versus-closed split.

Each of these was chosen for the same reasons: the data is public, the method is documented, the denominator is known, and the terms allow reuse with credit. [Akamai](https://www.akamai.com/visualizations/state-of-the-internet-report/ipv6-adoption-visualization) and [Meta](https://www.facebook.com/ipv6/) publish IPv6 figures of their own, but only as charts rendered in a browser, with no data file the reader could fetch and check, so they are not used.

## 4. Method

The unit of time is the calendar month, every one of them from May 2023, the first month Cloudflare's HTTP data covers, through September 2026, the most recent complete month when this was measured. A month rather than a day because web traffic has a weekly rhythm — IPv6 runs higher on weekends, when people are on home and cellular connections, and lower on weekdays, when office networks dominate, and the saw-tooth is visible in Google's own chart — and a month holds four full cycles of it. A calendar month rather than a rolling window because StatCounter and Wikimedia report by calendar month, so every source can be read over the same days. And every month rather than a chosen few because there is then nothing to choose and nothing to defend: forty-one months, the same questions each, with the seasonal swings in view.

For each month, the scripts ask Cloudflare Radar the same eighteen questions: the share of all traffic by bot class; within each bot class, the share by operating system and by device type; and the share by IP version within each bot class, and within each operating system and each device type among requests. Those shares combine into one table per month — automated, open, and closed, each split by kind of address, six cells that together are all of the month's traffic. The width of a column is its share of all traffic: the automated share directly from the bot class query; the open and closed shares as the share of traffic that people made times the operating system shares of the open systems and the closed systems. The IPv6 share of a column is the weighted mean of its operating systems' IPv6 shares, each measured directly. A cell is a width times a share.

The arithmetic is checked rather than trusted. Cloudflare reports the IPv6 share of all traffic, and of requests, directly, and the table must reproduce both from its parts. It does so within two points in every month, and the scripts record the residuals month by month. The largest and most consistent residual says something real: the operating system shares cover only requests Cloudflare could classify by operating system, and the IPv6 share of requests built from them runs between a fifth of a point and two points below the figure Cloudflare reports directly, in every one of the forty-one months. The unclassified remainder is more IPv6 than the rest. The device type route, which classifies nearly everything, reproduces that figure within a tenth of a point through 2024 and within a point after, which is why it is computed alongside as the check on the operating system split.

Four details of the API that its documentation leaves unstated were settled by asking the API itself, and the answers are kept with the data. Omitting the bot class filter returns all traffic: the unfiltered figure sits between the figure for requests and the automated one, and every operating system's unfiltered IPv6 share is lower than its share among requests alone, since automated requests skew IPv4. Filters stack, so bot class and operating system apply together in one request; the operating system filter takes one value at a time, which is why a column spanning three systems is a weighted sum of three queries. The IP version shares sum to exactly one hundred, so they are shares of every request in the group asked about, the plain definition rather than the dual-stack one. And the history begins in May 2023: the API refuses dates before late December 2021 and answers empty for everything earlier than May 2023, on its current and [its deprecated endpoints](https://developers.cloudflare.com/api/resources/radar/subresources/http/subresources/summary/) alike.

For the same months, the four other sources are reduced to the same calendar. Google's daily values are averaged over each month. APNIC's daily values are averaged over each month, weighted by each day's sample count. StatCounter's monthly figures are read as published, and an open-device share is built from its operating system columns the same way the Cloudflare table builds its own. Wikimedia's monthly counts become the share of page views that came through the desktop site, and the automated share of all page views. Two things are then set side by side: the IPv6 share from Cloudflare against Google and APNIC, and the share on open devices from Cloudflare against the desktop shares at StatCounter and Wikimedia. The report expects these to differ, says by how much, and explains why in terms of what each one counts.

## 5. Results

### 5.1 September 2026

<figure>

<!--@include: ../research/ipv6/charts/mosaic-2026-09.svg-->

<figcaption>Figure 1. Web requests, September 2026, by kind of device and kind of address, as a mosaic plot. Each cell is a percent of the month's requests; automated requests are not in the picture. Orange is v6 addresses and gray is v4; the closed column is plain and the open column is striped, so the striped orange cell is requests on open devices from v6 addresses, and the two forces act on it from two sides: the closed column squeezes it from the right, and the rising IPv6 share makes it taller. What matters is its area, and over time that grows only if the cell gets taller faster than it gets narrower. Source: Cloudflare Radar.</figcaption>
</figure>

The mosaic is every request Cloudflare saw in September 2026. Automated requests, 36.2% of all traffic, are left out of the figure, since they say nothing about people, and Table 1 still carries them. Each column is one kind of device, as wide as its share of the requests. Within each column, requests from IPv6 addresses are on top and requests from IPv4 addresses below, with the cut at that column's IPv6 share. The area of a cell is its share of the month's requests, and each is labeled with that share.

The picture is a single month. Drawn for 2009, it would have the vertical line between the columns almost at the right edge: nearly every request came from a PC. Through the 2010s the line slides left, as the phone, then the tablet, the Chromebook, and the streaming stick took over the clicks a household makes, and by about 2017 it stops near the middle and has sat there since, which is the migration Section 5.4 draws from StatCounter's longer record. Across the forty-one months this report measures, the line barely moves, and what motion it has is mostly Cloudflare's classifier.

The orange cells grow upward, at different speeds, and the reasons are economic. On the right, the solid orange cell rises as mobile carriers hand out IPv6 addresses on their cellular networks. They had the strongest reason to: their subscriber counts grew by billions of handsets after 2007, each needing an address of its own with no home router to share through, and most of that growth came after the free pool of IPv4 addresses ran out, so the choice was IPv6 or carrier-grade NAT at a scale no one had run before. A carrier also controls the handset's connection end to end, so a rollout is a change in its own core network that depends on no equipment in the customer's home. The result is a reachable address delivered, by the billion, to devices whose owners do not choose what listens on it.

On the left, the striped orange cell rises more slowly, as fixed-line providers upgrade their DSL, cable, and fiber networks. The pressure there is weaker. A household shares one address through its home router, so a fixed-line provider's IPv4 holdings, mostly allocated before the pool ran out, stretch over a subscriber base that grows slowly, and staying on IPv4 costs it little. And the upgrade is not in the provider's hands alone: it depends on the router in each home and the firewall in each office, an installed base of other people's equipment, much of it old. Offices are slowest of all, with internal networks built on IPv4 and no customer asking for the change. So the cell that could put a reachable address to use is the one gaining addresses slowest, and the cell that cannot is the one gaining them fastest.

Start with the striped orange cell. Only 13.7% of requests were made on an open device *and* came from an IPv6 address: a person, a device that runs what its owner chooses, and an address others can reach. The 32.9% below it are the same devices without a reachable address. The 53.4% beside it are people on devices where the vendor decides, nearly half of them with a reachable address, on a device whose owner does not decide what listens on it. Hovering any cell, here and on every figure that follows, gives a sentence saying what it is.

<figure>
<figcaption>Table 1. All traffic, September 2026, automated requests included. The six cells are built from the operating system shares as Section 4 describes, and the bottom row is Cloudflare's direct figure, which the cells reproduce within a quarter of a point. Source: Cloudflare Radar.</figcaption>

| | share of all traffic | from a v4 address | from a v6 address | v6 share of the row |
|---|---:|---:|---:|---:|
| Automated requests | 36.2% | 31.5% | 4.6% | 12.8% |
| Requests on an open device | 29.8% | 21.0% | 8.7% | 29.3% |
| Requests on a closed device | 34.1% | 17.7% | 16.4% | 48.0% |
| All traffic | 100% | 70.5% | 29.5% | 29.5% |


</figure>

Two figures in that table matter most. **Requests on an open device from an IPv6 address were 8.7% of all traffic in September 2026**, automated included, and 13.7% of requests. That is the slice of the web's activity that is both free to run software of its owner's choosing and directly addressable. And **of requests on open devices, 29.3% came from an IPv6 address**, which is the figure that matters to anyone running a peer-to-peer service: of the devices that could run a peer, somewhat under a third hold a routable address, and somewhat over two thirds do not. That figure carries an uncertainty. Weighted by their widths, the open and closed rates give 39.3% for all requests, not the 41.0% Cloudflare reports directly, because the operating system route places only the requests whose system it recognizes, and the ones it cannot place run more IPv6 than the rest. Cloudflare's device type route, which places nearly every request, puts the IPv6 share of requests from desktops at 33.7%, four points higher. The two routes draw the line between open and closed in different places, and the sources cannot say which sits nearer the truth, so the four points between them is the uncertainty on the figure this report quotes, and it quotes the lower.

Now the closed column. Of requests on closed devices, 48.0% came from an IPv6 address, more than half again the figure on open devices, and the cell for closed devices and IPv6 addresses, at 16.4% of all traffic, is nearly twice the striped orange cell. Measured by where requests come from, the IPv6 internet is mostly a mobile internet.

<figure>
<figcaption>Table 2. Requests, September 2026, by operating system. Source: Cloudflare Radar.</figcaption>

| Operating system | share of requests | v6 share of its requests |
|---|---:|---:|
| Windows | 35.2% | 27.2% |
| Android | 33.7% | 45.0% |
| iOS | 18.9% | 54.4% |
| macOS | 9.2% | 38.9% |
| Linux | 2.3% | 22.8% |
| ChromeOS | 0.7% | 22.3% |
| Smart TV | under 0.1% | 40.7% |


</figure>

Windows is the largest single platform on the web and, among the major ones, the least on IPv6, for the reason given above: a Windows PC sits on fixed broadband at home or on an office network, where IPv6 has trailed the cellular carriers by years. An iPhone's address is usually the carrier's choice; a desktop's is usually the home or office router's.

The automated column is counted and set aside, but its numbers explain one gap. Automated requests were 36.2% of all traffic, and only 12.8% of them came from an IPv6 address. Crawlers and scripts live in data centers, and data centers are IPv4 country. This is why the IPv6 share of all traffic, 29.5%, is so much lower than the IPv6 share of requests, 41.0%: the difference is the automated share.

### 5.2 Forty-one months

<figure>

<!--@include: ../research/ipv6/charts/forces-monthly.svg-->

<figcaption>Figure 2. Of every 100 web requests each month, May 2023 through September 2026. Orange is v6 addresses, dashed is open devices, and dashed orange is both: requests on open devices from v6 addresses. The complements, v4 addresses and closed devices, are each 100 minus the line shown, and Table 3 lists them. Source: Cloudflare Radar.</figcaption>
</figure>

Figure 2 takes every 100 requests in a month and draws three lines, in the styles every figure in this report uses: orange for v6 addresses, dashed for open devices, and dashed orange for requests on open devices from v6 addresses, which runs lowest. The two complements are not drawn, since each is 100 minus its partner, and Table 3 carries them as the mirror rows. The dashed orange line divided by the open-device line is the share of open-device requests with v6 addresses, which Figure 3 draws directly. Table 3 reads the first and last months off the chart, rounded to whole percents, with the report's reading of each change, and the rest of this section uses the same two months and the same rounding.

<figure>
<figcaption>Table 3. The three lines of Figure 2 and their two complements at the first and last month. Source: Cloudflare Radar.</figcaption>

| Of every 100 requests | May 2023 | September 2026 | Change | What happened |
|---|---:|---:|---:|---|
| From a v6 address | 35 | 41 | up 6 | The opening force, arriving through phones and carriers: the v6 share rose 9 points on closed devices and 3 on open ones. |
| From a v4 address | 65 | 59 | down 6 | The mirror of the line above. |
| On an open device | 36 | 47 | up 11 | Mostly classification. Cloudflare's operating system route recognizes more requests than it did, and its device type route, which classified nearly everything throughout, rose 3. Some is automation passing as people. No migration back to the PC is in the data. |
| On a closed device | 64 | 53 | down 11 | The mirror of the line above. |
| Open device with v6 address | 10 | 14 | up 4 | Both forces as measured: a better address picture on open devices, and a larger open share that is partly the classifier's. |


</figure>

<figure>

<!--@include: ../research/ipv6/charts/rates-monthly.svg-->

<figcaption>Figure 3. Share of requests with v6 addresses each month, on open devices and on closed devices. Source: Cloudflare Radar.</figcaption>
</figure>

Figure 3 draws two lines, each the share of one group of requests that have v6 addresses: open devices, dashed, and closed devices, solid. Figure 2's v6 line, Cloudflare's direct figure for any device, runs between them, a little above their weighted mean for the reason Section 4 gives.

<figure>

<!--@include: ../research/ipv6/charts/ipv6-by-system.svg-->

<figcaption>Figure 4. The share from a v6 address by operating system, each month. Open systems on the left, dashed; closed on the right, solid. Source: Cloudflare Radar.</figcaption>
</figure>

Figure 4 splits Figure 3's lines by operating system, one panel each on the same axis, open systems on the left and closed on the right. iOS went from 46% to 54%, crossing half in February 2025; Android from 37% to 45%; macOS from 35% to 39%; Linux from 20% to 23%; Windows, the largest system on the web, from 25% to 27%; and ChromeOS, a small share and a noisy series, stayed near 22%. The closed-device rise is iOS and Android together. The open-device figure is slow because Windows is slow, and Windows is most of it.

**The opening force is slow on open devices.** The IPv6 share of requests on open devices rose from 27% to 29%, under three points in a little over three years, with a high of 32% in December 2025 and a slight easing since. Over the same months the IPv6 share of requests on closed devices rose from 39% to 48%, three times as far, and the user-weighted sources rose by similar amounts: Google's users on IPv6 from 40% to 48%, APNIC's users able to use IPv6 from 35% to 44%. The opening force is arriving through phones and carriers, and the gap between the closed and open figures widened from thirteen points to nineteen. In September of each year, Windows alone stood at 26%, 26%, 28%, and 27%.

**The closing force does not show up in this window.** The report expected the share of requests made on open devices to fall across these months. It did not. By operating system it rose from 36% to 47%; by device type, the share made on desktops rose from 43% to 46%. StatCounter's share of page views from a desktop, a noisy series, swung between 35% and 50% across these months, with yearly means of 44%, 37%, 40%, and 46%, and no trend in it. Wikimedia's share of page views through the desktop site rose from 32% to 40%. Every measure is flat or rising. Two things are going on, and the report can separate them only partly.

The first is classification, and Figure 9 shows it directly. Cloudflare has two routes to the same quantity, by operating system and by device type, and they were seven points apart in May 2023 and had met by 2026. The operating system route counts only requests whose operating system Cloudflare recognized, and in 2023 a good share of desktop requests evidently carried no recognized one; as the classifier improved, those requests joined the open column. So most of the eleven-point rise by operating system is the classifier, and the device type route, which classified nearly every request throughout, is the better trend reading: a rise of three points. The two routes also disagree on the IPv6 rate within the open column, and there they have not met: by device type, requests from desktops were 30% IPv6 in May 2023 and 34% in September 2026, against 27% and 29% for the open systems by operating system, a gap of three to seven points in every month. The operating system route is the one that can be split by system, which is why the report draws it, but the open-device rate it gives is the lower of two readings throughout.

The second is harder, and it touches every source. Automated requests have been growing and getting better at passing for people. Cloudflare's automated share of all traffic rose from 29% to 36% over these months. Wikimedia's automated share of all page views rose from 34% to 51%, and in 2025 [the Wikimedia Foundation found](https://diff.wikimedia.org/2025/10/17/new-user-trends-on-wikipedia/) that a surge of apparently human traffic, much of it from one country, was bots built to evade detection, updated its detection, and reclassified the months from March to August 2025. Automated requests overwhelmingly identify themselves as desktops: among automated requests in September 2026, Windows, macOS, and Linux together were 69%, against 47% among requests. Any automation that slips past the bot score is counted as a request, wearing a desktop's clothes, and raises the share on open devices without a single person having changed devices. The report cannot measure how much of the rise is that. It can say which way the error runs: both Cloudflare's and Wikimedia's automated shares rose sharply across the same months, and the rise in the desktop share of requests coincides with it.

What is left, after both of those, is this: across these three years, no measure here shows the share of requests made on open devices falling. It held, in the middle forties by every request-weighted and page-view-weighted measure here. The migration from the PC to the phone that the report set out to measure is not visible in 2023 to 2026, because, as Section 5.4 shows, it had largely finished before 2023 began.

**The net.** The share of requests made on an open device and from an IPv6 address rose from 10% in May 2023 to 14% in September 2026. Both forces contributed in Cloudflare's data. Of the four points, about three come from the wider open column and one from the better address picture on open devices; and of those three, about two are the classifier's, by the device type route's reading of how much the open column really widened. By device type, the route the classifier moved least, the share of requests both from a desktop and from an IPv6 address rose from 13% to 16%, two thirds of it from addresses. As a share of all traffic the same cell went from 7% to 9%, a smaller rise, because automated requests grew faster than everything else. And the IPv6 share of all traffic barely moved, up less than a point, which is not a fact about people: the IPv6 share of requests rose six points while the automated share of all traffic grew seven, and automated requests come mostly from IPv4 addresses, so the two cancelled.

### 5.3 The square

<figure>

<!--@include: ../research/ipv6/charts/square-monthly.svg-->

<figcaption>Figure 5. Pick two requests at random, each month: the chance that the pair are both open, both v6, and both open and v6. Each line is a share from Figure 2, squared, in that line's style. Source: Cloudflare Radar.</figcaption>
</figure>

<figure>

<!--@include: ../research/ipv6/charts/square-open.svg-->

<figcaption>Figure 6. Pick two requests at random from open devices only, each month: the chance that the pair are both v6. This is Figure 3's open-device line squared, on the same axis as Figure 5. Source: Cloudflare Radar.</figcaption>
</figure>

For two peers to connect over IPv6, both must have it. If two requests are drawn at random from a group in which a share p has some property, the chance that both have it is p squared. Which group the pair is drawn from depends on who the two parties are, and that is the difference between the two figures.

Figure 5 draws from the whole web. Picture a person who picks up whichever device is nearest in the moment, the phone in their lap or the PC in the next room, and a second person like them anywhere on earth. The chance that both happen to be on open devices, so that both can choose their own software, was 21.7% in September 2026. The chance that both have IPv6 addresses, addresses built to be reached, was 16.8%, the figure the IPv6 chart alone would give for reaching a peer, if the device did not matter. The chance that both are both, the dashed orange line, was 13.7% squared, 1.9%, up from 0.9% in May 2023: one pair in fifty. That is the figure for the web as a whole, and it is the figure that matters to peer-to-peer software that means to reach people who are mobile first or mobile only, for whom [a browser tab on WebRTC](/how-peers-connect) is the only peer their device will run. The distance from the solid orange line down to the dashed one is the cost of the closed device.

Figure 6 draws from open devices only, and it is the swarm's figure. The people in a BitTorrent swarm are already running a client, so they are on open devices by definition, and the only question left is the address. The chance that two of them both have IPv6 is the open-device IPv6 share squared, Figure 3's open-device line squared: 29.3% squared in September 2026, 8.6%. That is four times the web's figure, and still low: a random pair of peers can meet over IPv6 about one time in twelve. It is the figure a BitTorrent client or a tracker designs against.

Both figures simplify, and both understate what can connect. The dashed orange line in Figure 5 is the assured case: an open device with an IPv6 address can run a peer and be reached, and nothing in the definitions stands in the way. Some closed devices can be peers of a kind, a phone running a BitTorrent client on Android, a browser tab on WebRTC, so the true share of pairs that could connect sits somewhere above that line, by an amount these sources cannot measure. And a device with IPv6 almost always has IPv4 as well, so the 8.6% of peer pairs that can use IPv6 between them are pairs that gain a better path, not the only pairs that can connect. IPv6 is an addition to a peer's reach for now, not a replacement, and any peer-to-peer software that means to connect the other eleven pairs in twelve has to carry IPv4 and [the whole apparatus of NAT traversal](/how-peers-connect) that comes with it for a long time yet.

The square assumes the two peers are chosen independently, which is about right for a global swarm and less right for a local one. Two peers in the same country share that country's adoption rate, which may be far above or below the world's, so the square overstates the chance for low-adoption regions and understates it for high ones. The figure is a world average, stated as such.

### 5.4 The long view

<figure>

<!--@include: ../research/ipv6/charts/history.svg-->

<figcaption>Figure 7. The two forces since 2008: the share of Google's users on v6, as monthly means of its daily figure, and the share of StatCounter's page views from a desktop. The shaded span is the forty-one months the Cloudflare data covers. Sources: Google, StatCounter.</figcaption>
</figure>

Figure 7 draws the two forces since 2008, each from the source that has watched it longest. Google's line, orange, is the share of its users on v6. StatCounter's, dashed, is the share of page views from desktop computers, which is the closest long record of the share of a person's activity on open devices: laptops count as desktop, and the line includes the Chromebooks and consoles that this report counts as closed, so it runs slightly high as a measure of open devices, by two to eight points across 2026. The shaded span marks the months Cloudflare's data covers, where the joint table exists.

<figure>
<figcaption>Table 4. The two forces in September of selected years, with StatCounter's mean for the year beside its September figure, since its monthly series swings as much as ten points from one month to the next; the 2026 mean covers January through September. Sources: Google, StatCounter.</figcaption>

| September | Users on v6, Google | Page views from a desktop, StatCounter | The same, mean of the year |
|---|---:|---:|---:|
| 2009 | 0.2% | 98.9% | 99.0% |
| 2012 | 0.8% | 85.3% | 88.0% |
| 2015 | 7.4% | 55.9% | 59.1% |
| 2017 | 18.0% | 43.3% | 43.7% |
| 2020 | 30.6% | 47.2% | 45.6% |
| 2023 | 41.7% | 45.1% | 42.9% |
| 2026 | 47.5% | 39.2% | 45.9% |


</figure>

The two lines have different shapes, and that difference is the result. The closing force was a migration, and it happened between about 2011 and 2017: desktop went from nearly all page views to under half in six years, as the smartphone arrived, and then stopped. From 2017 on, StatCounter's yearly mean has stayed between 37% and 46%, a band it has not left in nine years, with 2024 the low and 2026 back near the top. The opening force is not a migration but a slope, and it has not stopped: IPv6 went from nothing to nearly half of Google's users across the same seventeen years, and the last three added six points. Why a change announced with deadlines arrived as a slope that shallow is the subject of a companion article, [Waiting for IPv6](/waiting-for-ipv6).

So the two forces are not symmetric in time, and the picture this report set out to draw turns out to have a different shape than expected. A person's click did lose the liberty to choose its software, by about half, and it lost it a decade ago, one purchase at a time, as closed devices replaced open ones. Since then that share has held. Meanwhile the same click has been gaining a reachable address, steadily, and is still gaining one, though as the forty-one months show, more slowly on the devices that could use it than on the ones that can't.

One caution about reading the two long lines together. The share of requests that are both on an open device and from an IPv6 address is not the product of the two lines, because the two are not independent: in every month Cloudflare covers, the IPv6 share on open devices runs at about seven tenths of the IPv6 share of all requests. Multiplying Google's line by StatCounter's would overstate the overlap by that much. The joint figure exists only where Cloudflare's data does, from 2023 on, and the shaded span in the figure marks it.

### 5.5 Cross-checks

<figure>

<!--@include: ../research/ipv6/charts/check-ipv6.svg-->

<figcaption>Figure 8. The share on v6 by four sources over the same months: all traffic at Cloudflare with automated requests included, requests at Cloudflare, Google's users, and APNIC's users able to use v6. Sources: Cloudflare Radar, Google, APNIC.</figcaption>
</figure>

Figure 8 has four panels, one line each, on one axis: top left, the IPv6 share of all traffic at Cloudflare, automated included; top right, the IPv6 share of requests at Cloudflare; bottom left, Google's users on IPv6; bottom right, APNIC's users able to use IPv6. The IPv6 share of requests tracks [APNIC's measure of users who prefer IPv6](https://data1.labs.apnic.net/ipv6-data-format.html) when offered both remarkably closely: within two points in most months and four at the widest, with Cloudflare's figure running one to three points above through most of 2024 and 2025, and one to two points below from late 2025 on. These are two entirely different measurements, one of requests to one network's sites, one of users sampled through advertisements, and they agree on the level and on the slope. Google's figure runs five to seven points above both, and the reasons are in what it counts: users rather than requests, and it counts every user who reached it over IPv6, which is closer to APNIC's capable measure than to its preferred one. The IPv6 share of all traffic runs well below the rest, six points under the figure for requests at the start of the series and twelve at the end, and that gap is the automated requests.

<figure>

<!--@include: ../research/ipv6/charts/check-open.svg-->

<figcaption>Figure 9. The share on an open device or a desktop by four measures over the same months: Cloudflare by operating system, Cloudflare by device type, StatCounter's page views from a desktop, and Wikimedia's page views through the desktop site. Sources: Cloudflare Radar, StatCounter, Wikimedia.</figcaption>
</figure>

Figure 9 has four panels again: top left, the share of requests made on an open device, at Cloudflare by operating system; top right, the share made on a desktop, at Cloudflare by device type; bottom left, the share of page views from a desktop at StatCounter; bottom right, the share of page views through the desktop site at Wikimedia. In 2026 they agree within a few points, the two Cloudflare routes and StatCounter all in the middle forties, Wikimedia a little below. In 2023 they did not: Cloudflare's operating system route sat seven points below its own device type route, and Wikimedia sat ten to fifteen points below StatCounter. The convergence is partly classifiers improving and partly, the report suspects, automation counted as people by every source in the same years. What the four agree on is the thing the report most wanted to know: none of them shows the share on open devices falling across these three years.

A note on the automated share itself, since two sources report it directly. In September 2026 automated requests were 36.2% of all traffic at Cloudflare, and automated page views were 50.7% of all page views at Wikimedia, counting its spider and automated classes together. The two populations differ, a general network against an encyclopedia that every crawler and model trainer reads, so the levels needn't match. The slopes do: both rose sharply over the forty-one months.

## 6. Discussion

Two limits have sat on a person's standing on the internet, and this report has tried to measure them together. The first is the address. Behind a shared IPv4 address a person can reach any server but no one can reach them, which makes them a visitor to the internet rather than a citizen of it. IPv6 lifts that limit: nearly half of Google's users, and more than four in ten requests at Cloudflare, now come from an IPv6 address. But on the devices that could use a reachable address to run a peer, the open ones, the figure is under three in ten, and it rose under three points in three years. The address limit is lifting slowest exactly where lifting it would matter most.

The second limit is the device. A person who buys a phone owns it, and the vendor decides what it may run, what may run in the background, and what may listen on the network. That limit was imposed, purchase by purchase, between about 2011 and 2017, when closed devices went from a novelty to more than half of all page views, and it has held at about that level since. Across the forty-one months measured here it held, within the classification caveats of Section 5.2, and nothing in the data says it is about to move.

Where the two meet is the cell this report was written to measure: requests on an open device from an IPv6 address, 13.7% of the web requests people made in September 2026, up from 9.5% in May 2023. That is the share of the web's human activity on a device that runs what its owner chooses and at an address anyone can reach. One click in seven. It is growing, slowly, from a slightly better address picture on open devices and a slightly larger share on open devices, the second of which is mostly a classifier's doing.

For anyone building peer-to-peer software, the count sets the design. Six in seven requests come from a device that is closed, or from an IPv4 address, or both. A peer that only speaks IPv6 reaches under a third of the open devices, and two such peers chosen at random can meet about one time in twelve. So the software carries IPv4 and NAT traversal for a long time yet, and it carries a browser path too, because a closed device's browser tab is the one peer its owner is still allowed to run. This is a count, taken the same way every month, with the scripts beside it so anyone can take it again.

## 7. Limitations

Every number here is a share of requests seen by one network, Cloudflare's, or of page views seen by StatCounter's sites or Wikimedia's, or of users sampled by Google or APNIC. None of them is the web. Cloudflare's sample is large and broad, and its agreement with APNIC on the IPv6 share is reassuring, but a reader should hold the open and closed figures as Cloudflare's, with the classification caveats above.

The three cuts are proxies. Cloudflare's bot score decides whether a request was made by a person or automated, and some automation passes it. Operating system decides open against closed, and some requests carry no recognized one, while some carry a misleading one: Safari on an iPad asks for the desktop version of a site by default and identifies itself as macOS, so a share of the macOS column the sources cannot measure is tablets, counted here as open. The kind of address is a floor on IPv6 capability, for the dual-stack reasons given in Section 2.3. Each proxy errs in a known direction: the automated share is understated, the share on open devices correspondingly overstated, and every IPv6 figure understated.

The history is short. Cloudflare's HTTP data reaches back only to May 2023, so the joint table exists for forty-one months and no earlier, and three years is enough to read a level and a slope but not a cycle. The long view borrows two older series that each measure one force alone, and they cannot be combined into the joint figure, for the dependence reason given in Section 5.4.

And reachability is out of scope. A routable address is the precondition for being reached; firewalls, carrier-grade NAT, and the rest of what sits between an address and an inbound connection are real, push in known directions, and are not counted here.

## 8. Reproducibility

Three [Python](https://www.python.org/) scripts are in the repository, with [the data they pulled](https://github.com/zootella/ftorrent/tree/master/docs/research/ipv6/data) and [the figures they drew](https://github.com/zootella/ftorrent/tree/master/docs/research/ipv6/charts), and [a README](https://github.com/zootella/ftorrent/blob/master/docs/research/ipv6/README.md) that walks through running them. [`fetch.py`](https://github.com/zootella/ftorrent/blob/master/docs/research/ipv6/fetch.py) pulls every source and keeps a manifest of what came from where and when; [`table.py`](https://github.com/zootella/ftorrent/blob/master/docs/research/ipv6/table.py) does the arithmetic and writes the tables, including every residual mentioned above, as JSON and CSV; [`chart.py`](https://github.com/zootella/ftorrent/blob/master/docs/research/ipv6/chart.py) draws the figures. They use only [Python's standard library](https://docs.python.org/3/library/), so there is nothing to install. The four keyless sources need an internet connection and nothing else; Cloudflare Radar needs [a free account](https://dash.cloudflare.com/sign-up) and a read-only token, and the README says how to make one. Running the two offline scripts against the frozen data reproduces this report's numbers exactly. Running the fetch again brings the series forward to whatever month has most recently ended, and Figure 1 redraws for it. The scripts and this report were written with [Claude Code](https://claude.com/claude-code), and the README's header says so, with the versions the work depends on.

The data is used under its sources' terms: Cloudflare Radar's API data under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/), StatCounter's under [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/), APNIC's with attribution as its files request, Wikimedia's under [CC0](https://creativecommons.org/publicdomain/zero/1.0/), and Google's by citation and link, since [its statistics page](https://www.google.com/intl/en/ipv6/statistics.html) states no license.

## References

1. Cloudflare. *Cloudflare Radar HTTP API: summary by dimension.* https://developers.cloudflare.com/api/resources/radar/
2. Cloudflare. *Radar bot classes.* https://developers.cloudflare.com/radar/concepts/bot-classes/
3. Google. *IPv6 adoption statistics.* https://www.google.com/intl/en/ipv6/statistics.html
4. APNIC Labs. *IPv6 measurement: world series and data format.* https://stats.labs.apnic.net/ipv6 and https://data1.labs.apnic.net/ipv6-data-format.html
5. StatCounter Global Stats. *Platform and operating system market share, worldwide, monthly.* https://gs.statcounter.com/
6. Wikimedia Foundation. *Pageviews API, Analytics Query Service.* https://wikitech.wikimedia.org/wiki/Analytics/AQS/Pageviews
7. Miller, M. *New User Trends on Wikipedia.* Wikimedia Foundation Diff, 17 October 2025. https://diff.wikimedia.org/2025/10/17/new-user-trends-on-wikipedia/
8. Schinazi, D., and Pauly, T. *Happy Eyeballs Version 2: Better Connectivity Using Concurrency.* RFC 8305, December 2017. https://www.rfc-editor.org/rfc/rfc8305
9. Woodyatt, J., ed. *Recommended Simple Security Capabilities in Customer Premises Equipment (CPE) for Providing Residential IPv6 Internet Service.* RFC 6092, January 2011. https://www.rfc-editor.org/rfc/rfc6092
10. Richter, P., Wohlfart, F., Vallina-Rodriguez, N., Allman, M., Bush, R., Feldmann, A., Kreibich, C., Weaver, N., and Paxson, V. *A Multi-perspective Analysis of Carrier-Grade NAT Deployment.* ACM Internet Measurement Conference, 2016. https://arxiv.org/abs/1605.05606
11. ftorrent. *How Can Two Peers Connect?* https://docs.ftorrent.com/how-peers-connect.html
