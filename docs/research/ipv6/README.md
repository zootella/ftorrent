> Prepared by Claude Code using Fable 5.1
> Created: 2026-Oct
> Last reviewed: 2026-Oct
> Python: 3.14
> Cloudflare Radar API: v4

# IPv6 and the Open Device: the scripts

This folder holds the report *IPv6 and the Open Device* and everything that produced it: three Python scripts, the data they pulled, the tables they computed, and the figures they drew. The report explains the question, the method, and the findings. This file explains how to run the scripts, so that you can reproduce the report's numbers from our frozen data, or pull fresh data and make the same measurement for a later month.

## What you need

- [Python 3](https://www.python.org/). The scripts use only [the standard library](https://docs.python.org/3/library/), so there is nothing to install. We ran them on Python 3.14; any recent Python 3 should do.
- An internet connection, for `fetch.py` only.
- A free [Cloudflare](https://www.cloudflare.com/) account and a read-only API token, for the [Cloudflare Radar](https://radar.cloudflare.com/) part of `fetch.py` only. The other four sources need no key. The token section below walks through it.

## The three scripts

Run them in order from this folder. Each one reads what the one before it wrote.

```
python3 fetch.py 2023-05 2026-09
python3 table.py
python3 chart.py
```

**`fetch.py` pulls the data.** It takes one month, like `2026-09`, or a range of months, like `2023-05 2026-09`, and writes every pull into the `data` folder exactly as the source served it, plus a manifest, `data/fetched.json`, recording each URL, the time it was fetched, its size, and its status. Four sources carry their whole history in one file each and are pulled once per run: [Google's IPv6 adoption file](https://www.google.com/intl/en/ipv6/statistics.html), [APNIC Labs' world IPv6 series](https://stats.labs.apnic.net/ipv6), [StatCounter's platform and operating system shares](https://gs.statcounter.com/), and [Wikimedia's pageview counts](https://wikitech.wikimedia.org/wiki/Analytics/AQS/Pageviews) by access method and agent type. [Cloudflare Radar's API](https://developers.cloudflare.com/api/resources/radar/) answers one question per request, so it is pulled month by month, eighteen requests a month, with a short pause between requests. A Cloudflare file already in the `data` folder is kept rather than fetched again, so extending the series by a month later costs only that month's requests. Delete a file to force it fresh. Without a token, the script pulls the other four sources and prints how to make one.

**`table.py` does the arithmetic.** It never touches the network. For every month with a complete set of Cloudflare pulls it builds the joint table: automated requests, and the requests people made on open and on closed devices, each by kind of address, and checks its own arithmetic against the totals Cloudflare reports directly, recording the residuals. It reduces the four other sources to monthly series on the same calendar. It writes `data/table.json`, which `chart.py` reads, and two spreadsheets: `data/table.csv`, one row per month of the joint table, and `data/history.csv`, one row per month of every source's long series. It prints the latest month's table so a run is checkable at a glance.

**`chart.py` draws the figures.** It reads `data/table.json` and writes SVG files into the `charts` folder: the cake for the latest month, the two forces by month, the IPv6 rate within each category by month, the two squares by month, the long view since 2008, and two sets of small multiples for the cross-checks. Every point and cell carries a sentence that appears on hover. There is no plotting library; the SVG is written directly. Each file opens on its own in a browser, and each is written so a [VitePress](https://vitepress.dev/) page can include it inline, where it takes the site's colors.

## The Cloudflare token

Cloudflare Radar's API is free but needs a token. Sign in at [dash.cloudflare.com](https://dash.cloudflare.com/profile/api-tokens), open My Profile, then API Tokens, then Create Token, then Create Custom Token. Under Permissions choose Account, then Radar, then Read, and nothing else. Leave Account Resources at all accounts. Create the token and copy it; the dashboard shows it once.

Then create a file named `.env` in this folder with one line, `RADAR_TOKEN=` followed by the token you copied. The script also reads a `RADAR_TOKEN` environment variable, if you would rather export one. Keep the `.env` file out of version control. Radar data is not tied to any account, so the token can belong to any account you hold.

## What the data folder contains

Every file is named by what it is, and the Cloudflare files are named by the question they asked: `cloudflare-2026-09-ip_version-likely_human-windows.json` is the IP version split of likely-human requests from Windows in September 2026. The sources, their formats, and their reuse terms are described in [the report](./ipv6-and-the-open-device.md) and in [`sources.md`](./sources.md).

The frozen pulls in this folder are the ones the report's numbers come from. Running `table.py` and `chart.py` against them reproduces the report exactly. Running `fetch.py` again refreshes the four keyless sources, whose files carry their whole history, and pulls any Cloudflare months not already here.

## Adding a month

To extend the series after a new month has ended, run `fetch.py` with the range ending at the new month, then `table.py` and `chart.py`. Only the new month's Cloudflare requests are made. The cake redraws for the newest month; the lines gain a point.

## Reuse

Cloudflare Radar's API data is [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/). StatCounter's is [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/) and asks for a link. APNIC [permits reuse with attribution](https://data1.labs.apnic.net/ipv6-data-format.html). Wikimedia's is [CC0](https://creativecommons.org/publicdomain/zero/1.0/). Google states no license on its statistics page; we cite it and link it. Our scripts and text are published under the repository's license.
