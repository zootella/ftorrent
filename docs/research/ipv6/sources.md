# Data sources

Working notes from the source research on 2026-10-11, updated the same day with what running the Cloudflare API settled. [The report](https://docs.ftorrent.com/open-address-closed-device.html) and [the README](./README.md) are the finished account; this note is the research trail behind them. Each entry records what a source measures, what its percentages are percentages of, how far back it goes, how to fetch it, and its reuse terms. Everything here was fetched and inspected, not taken from a description, except where marked "to verify."

## What the report needs

Each snapshot cuts requests three ways by who caused them (automated activity, a person on a device they hold root on, a person on a vendor-rooted device) and crosswise by IP version, with snapshots over time for trendlines. So a source needs some of: a long history, a device or platform dimension, an IP version dimension, a human-versus-automated dimension, and a stated denominator. No single source has all five. The ones below cover them in combination.

## Google IPv6 statistics

- Page: https://www.google.com/intl/en/ipv6/statistics.html
- Data: https://www.google.com/intl/en_ALL/ipv6/statistics/data/adoption.js (daily series) and https://www.google.com/intl/en_ALL/ipv6/statistics/data/worldmap.js (per-country snapshot)
- Measures: the percentage of users that access Google over IPv6. User-weighted. Rows are `[year, month (zero-based), day, total IPv6 %, native IPv6 %]`, where native excludes 6to4 and Teredo.
- Coverage: daily from 2008-09-04 through the fetch date, 6,610 rows on 2026-10-11. Latest value 48.7% total IPv6.
- Dimensions: none beyond date and country. No device, OS, or bot split.
- Access: plain HTTPS, no key. The file is a JavaScript array; parse it with a regular expression.
- Reuse: no license stated on the page. The report cites it and links the page.
- Role: the opening force over the longest history, and the chart the article completes.

## APNIC Labs IPv6 measurement

- Page: https://stats.labs.apnic.net/ipv6
- Data: https://data1.labs.apnic.net/v6stats/v6region/XA.json (world), with the same shape for regions (`v6region/XQ.json` is Northern America), economies (`v6economy/US.json`), and networks (`v6as/AS7922.json`). Format documented at https://data1.labs.apnic.net/ipv6-data-format.html
- Measures: IPv6 capability among users reached by an ad-delivered measurement script. Each daily record carries raw sample counts (`seen`) and percentages for `capable` (can fetch over IPv6) and `preferred` (does so when dual-stacked), plus 10, 30, 60, and 90 day smoothed versions. User-weighted, and the sample size is published alongside every point.
- Coverage: daily from 2013-10-07 through the fetch date. World sample roughly 300 thousand per day in 2013, roughly 30 million per day in 2026. Latest world value 44.4% capable, 42.0% preferred.
- Dimensions: geography and network only. No device, OS, or bot split.
- Access: plain HTTPS, no key. About 4 megabytes for the world file.
- Reuse: the file header says "re-use with attribution permitted."
- Role: an independent, user-weighted cross-check on Google for the opening force, with per-network detail available if the article wants it.

## StatCounter Global Stats

- Page: https://gs.statcounter.com/
- Data: CSV export from the chart endpoint, for example https://gs.statcounter.com/chart.php?device_hidden=desktop%2Bmobile%2Btablet&statType_hidden=comparison&region_hidden=ww&granularity=monthly&statType=Comparison&region=Worldwide&fromInt=200901&toInt=202610&fromMonthYear=2009-01&toMonthYear=2026-10&csv=1 for platform share, and `statType_hidden=os_combined` for operating system share.
- Measures: page views across sites carrying the StatCounter tracking code, more than 3 billion page views a month from more than a million sites, with bots identified and excluded. Page-view-weighted. Mobile is a pocket-sized device, tablet is a larger touchscreen device, desktop is everything else including laptops.
- Coverage: monthly from 2009-01. In 2009-01 desktop was 99.3% of page views; in 2026-10 it was 40.2%, mobile 58.3%, tablet 1.6%. Operating system columns include Windows, Android, iOS, OS X, macOS, Linux, Chrome OS, and others.
- Dimensions: platform and operating system by month and by country. No IP version.
- Access: plain HTTPS, no key.
- Reuse: Creative Commons Attribution-ShareAlike 3.0, and the FAQ asks for credit with a link.
- Role: the closing force over the longest history, matching Google's horizon.
- Caution: the platform series jumps about ten points between 2026-08 and 2026-09 (desktop 49.1% to 39.2%). That needs a look before the series is used as a trendline; StatCounter has had methodology discontinuities before. The report reads the series as yearly means beside the monthly figure for this reason.

## Wikimedia pageviews

- API: https://wikimedia.org/api/rest_v1/metrics/pageviews/aggregate/all-projects/{access}/{agent}/monthly/2015070100/2026100100 where access is `all-access`, `desktop`, `mobile-web`, or `mobile-app`, and agent is `all-agents`, `user`, `spider`, or `automated`.
- Measures: page views across all Wikimedia projects, counted by access method and by agent type. Page-view-weighted.
- Coverage: monthly from 2015-07. The `automated` agent class begins 2020-04; before that, automated traffic sits inside `user`.
- Dimensions: access method (a clean desktop-versus-mobile split) and agent type (a clean human-versus-automated split). No IP version.
- Access: plain HTTPS, no key, but rate-limited. Requests must carry a descriptive User-Agent, and fetching the sixteen access-by-agent series back to back hit the limit. Space the requests a second or two apart.
- Reuse: CC0.
- Role: a second source for the closing force since 2015, and the only no-key source that reports the automated share directly over time, which is the report's first category.

## Cloudflare Radar

- Docs: https://developers.cloudflare.com/radar/ and the API reference under https://developers.cloudflare.com/api/resources/radar/
- Endpoint: `GET https://api.cloudflare.com/client/v4/radar/http/summary/{dimension}` with dimension `IP_VERSION`, `OS`, `DEVICE_TYPE`, or `BOT_CLASS`, and a matching `timeseries_groups` endpoint for time series. The older per-dimension paths an earlier draft used, such as `summary/ip_version`, are marked deprecated.
- Filters: `botClass` (`LIKELY_AUTOMATED`, `LIKELY_HUMAN`), `deviceType` (`DESKTOP`, `MOBILE`, `OTHER`), `os` (`WINDOWS`, `MACOSX`, `IOS`, `ANDROID`, `CHROMEOS`, `LINUX`, `SMART_TV`), `ipVersion`, `location`, `asn`, and dates as `dateRange` (relative, at most 364 days) or `dateStart` and `dateEnd` (absolute). Filters stack: bot class and operating system apply together in one request. The operating system filter takes one value per request, so a column spanning several systems is a weighted sum of separate pulls. Verified by running the API on 2026-10-11.
- Measures: HTTP requests reaching Cloudflare's network. Request-weighted. Bot class is Cloudflare's bot score: 1 through 29 is likely automated, 30 through 99 is likely human. Omitting `botClass` returns all requests: the unfiltered IPv6 share sits between the human and automated figures, and every operating system's unfiltered rate is below its human-filtered one. Verified 2026-10-11.
- IPv6 definition: Radar's own adoption chart has counted IPv6 requests over requests for dual-stacked content since 2023-01-23; before that it was IPv6 over all requests. The API's `IP_VERSION` summary is the plain share of all requests: IPv4 plus IPv6 sums to exactly 100 in every pull. Verified 2026-10-11.
- Coverage: Radar's HTTP summaries hold data from May 2023 onward. The API refuses dates before 2021-12-27 and answers empty for every month before 2023-05, on the current and the deprecated endpoints alike, and a time series request may span at most 374 days. So the joint table exists for 41 months, May 2023 through September 2026. Verified 2026-10-11.
- Dimensions: the only source found with IP version, operating system, device type, and bot class together, which makes it the only source for the joint distribution the report is built on.
- Access: free, but every request needs an API token with the permission Account, then Radar, then Read, created at dash.cloudflare.com under My Profile, API Tokens, Create Custom Token. An earlier draft named a different permission; that was wrong.
- Reuse: data from the API is CC BY-NC 4.0 with attribution. Graphs and images from radar.cloudflare.com are CC BY 4.0.
- Role: the joint cut, for as many periods as the API allows, and the bridge between the two long marginal series.

## Looked at and set aside

- Akamai's IPv6 adoption visualization still exists but renders only in a browser, with no export found.
- Meta's IPv6 statistics page renders only in a browser. It reports per-country user share, the same kind of figure Google and APNIC already give.
- Internet Society Pulse aggregates the sources above and offers an API. Secondary; cite the originals.
- Wikimedia does not publish an IP version split of its traffic. A 2025 paper by Rye and Levin, "WikIPedia: Unearthing a 20-Year History of IPv6 Client Addressing," analyzes the IPv6 addresses of logged-out editors from 2003 to 2024, which is a different population.

## How the sources fit together

Two long marginal series, 2009 to now: the opening force from Google with APNIC as its cross-check, and the closing force from StatCounter with Wikimedia as its cross-check from 2015. One joint source, Cloudflare, for the cross-tab in recent years. The joint is not the product of the marginals, because mobile devices are more often on IPv6 than desktops, and Cloudflare lets the report measure how far from independent the two forces are. The sanity checks are Cloudflare's overall IPv6 share against Google and APNIC for the same windows, and Cloudflare's desktop-versus-mobile split against StatCounter and Wikimedia for the same months.
