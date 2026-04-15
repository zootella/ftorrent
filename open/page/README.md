
_ftorrent/open/page/README.md [open.ftorrent.com](https://open.ftorrent.com)_

_Three guides cover this deployment: [dockerizing Aquatic and configuring the Linux server](../README.md), the [dashboard back end](../gauge/README.md), and the **dashboard front end** (this guide)._

# Public Tracker Dashboard Front End

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.6
> <br>Created: 2026-Apr
> <br>Last reviewed: 2026-Apr
> <br>[Vue](https://vuejs.org/): 3.5
> <br>[Vite](https://vite.dev/): 8.0
> <br>Node: 22
> <br>[Three.js](https://threejs.org/): 0.183

The dashboard at [open.ftorrent.com](https://open.ftorrent.com/) is a Vue + Vite single-page application. It shows statistics from the three Aquatic tracker containers (UDP, HTTP, WebSocket) by reading a `page.json` file that the gauge container generates every minute. The page is built on a development machine and deployed as static files served by nginx — no application server in the request path.

## Development

```bash
pnpm install     # from the monorepo root, installs all workspaces
cd open/page
pnpm dev     # starts Vite dev server with hot reload
pnpm build   # produces dist/ for deployment
pnpm preview # serves the built dist/ locally for testing
```

**page.json in dev vs production.** The Vue app fetches `/page.json` on load for tracker statistics. The same URL resolves to different files depending on the environment:

```
Development (pnpm dev)
  Browser fetches    http://localhost:5173/page.json
  Vite serves from   open/page/public/page.json        ← placeholder with zeroes, committed to repo

Production (server)
  Browser fetches    https://open.ftorrent.com/page.json
  nginx serves from  /opt/open.ftorrent.com/data/public/page.json   ← written by gauge container every minute
```

The built `dist/page.json` from `pnpm build` is never used in production — nginx serves the gauge's live copy from the shared data volume instead.

## Deployment

The build output (`dist/`) is rsynced to the server where nginx serves it as static files. A `upload.hide.sh` script handles the rsync. This script is gitignored (via the `*.hide.*` pattern) because it contains your server's SSH details. Create your own:

```bash
#!/bin/bash
rsync -avz --delete dist/ youruser@yourserver:/opt/open.ftorrent.com/static/ -e "ssh -p 22"
```

Replace `youruser`, `yourserver`, the path, and the SSH port with your own. Make it executable with `chmod +x upload.hide.sh`.

Then build and deploy in one step:

```bash
pnpm upload
```

Or separately:

```bash
pnpm build
./upload.hide.sh
```

The deployed files land in the static directory on the server. nginx serves them for any request to your tracker domain that isn't a tracker announce, scrape, or WebSocket upgrade.

## Scaffolding

The project was scaffolded from the monorepo's `open/` directory using Vue's official scaffolder:

```bash
cd open
pnpm create vue@latest page
```

The scaffolder prompts for options. We chose:

- Project name: **page**
- TypeScript: **No**
- JSX: **No**
- Vue Router: **No**
- Pinia: **No**
- Vitest: **No**
- ESLint: **No**
- Prettier: **No**
- Skip all example code and start with a blank Vue project: **Yes**

After scaffolding, we cleaned up:

- **Removed** `.gitignore` — the monorepo root's `.gitignore` already covers `node_modules/`, `dist/`, `.DS_Store`, and everything else the scaffolder's version included
- **Removed** `README.md` — replaced by this guide
- **Removed** `jsconfig.json` — editor convenience for the `@/` import alias; not needed for building or running
- **Removed** `.vscode/` — editor-specific settings
- **Merged** `package.json` — combined the scaffolder's `dependencies` and `devDependencies` with our workspace metadata (`name: "open-page"`, `description`, `homepage`, `version: "0.1.0"`)
- **Removed** `engines` field from `package.json` — the monorepo root already requires Node 22+, which satisfies Vite 8's minimum

The result:

```
open/page/
├── index.html         Vite entry point
├── package.json       Dependencies and scripts
├── vite.config.js     Vite + Vue plugin configuration
├── src/
│   ├── main.js        Fetches page.json, then creates and mounts the Vue app
│   ├── style.css      Tailwind import, @font-face declarations, base styles
│   ├── App.vue        Root component
│   └── components/    Vue components (Page, Panel, Space, Quote, Url)
├── public/
│   ├── page.json      Gauge data (written by the gauge container)
│   ├── fonts/         Self-hosted web fonts (Jura, Monaspace Krypton, Radon)
│   ├── images/        Page images (sticker, social card)
│   └── earth/         Twelve monthly Earth textures (jan.jpg–dec.jpg, 8K, see below)
└── earth.hide.md      Research notes on the globe component (gitignored)
```

## Typography

Three typefaces do all the work on the page. [Jura](https://fonts.google.com/specimen/Jura) sets the headings and the labels, and two faces from GitHub Next's [Monaspace](https://monaspace.githubnext.com/) superfamily handle the monospaced runs.

Jura was designed by Daniel Johnson with the Cyreal foundry, and it sits in what its designer calls "the [Eurostile](https://en.wikipedia.org/wiki/Eurostile) vein" — that family of geometric, superellipse-based sans-serifs whose rounded-rectangle letterforms have, since the 1960s, been the house style for instrument panels, airport signage, and the consoles of cinematic spacecraft. The shapes read as technical and confident without feeling cold. Jura's particular origin makes it a nice fit here: Johnson started from a [Kayah Li](https://en.wikipedia.org/wiki/Kayah_Li_alphabet) script he was drawing for FreeFont, noticed the same strokes and curves would build a Roman alphabet, and grew the family outward from there. The published family covers Latin, Cyrillic, Greek, and Kayah Li from a shared set of shapes, which is exactly the spirit you want on a page about open infrastructure for a global network — a typeface that was literally drawn to speak in more than one script.

Monaspace Radon is a hand-drawn-feeling monospace, and we use it for the announce URLs. It's legible and a little informal, so a string like `udp://open.ftorrent.com:443/announce` reads as something you're invited to copy rather than as stern configuration boilerplate. Monaspace Krypton is sharper and more mechanical, and we use it for the LCD values on the panel, meaning every number that ticks over as the page runs. It sits cleanly under the panel's LCD color treatment and drop shadow, and its even rhythm keeps the digits from jostling as they change.

The fonts are self-hosted. We committed the three WOFF2 files to `public/fonts/` (`jura-latin.woff2`, `monaspace-krypton.woff2`, and `monaspace-radon.woff2`) and registered each one with an `@font-face` block in `src/style.css`:

```css
@font-face {
	font-family: 'Jura';
	font-style: normal;
	font-weight: 300 700;
	font-display: swap;
	src: url('/fonts/jura-latin.woff2') format('woff2');
}
```

All three faces are variable, so the `font-weight: 300 700` range covers every weight the page uses from a single file. The `font-display: swap` line tells the browser to render with a fallback immediately and swap the web font in when it arrives, so there's no flash of invisible text while the download is in flight. Vite copies everything under `public/` verbatim into the build output, which means `/fonts/...` resolves the same way in development (served by Vite) and in production (served by nginx), and the browser only fetches each font when a rule actually uses it.

All three faces are released under the [SIL Open Font License 1.1](https://openfontlicense.org/), which means they're free to embed, bundle, and self-host, commercial or otherwise, with no attribution required on the rendered page. Self-hosting the WOFF2 files rather than linking to Google Fonts or a CDN keeps the page fully served from `open.ftorrent.com` with no third-party requests.

## Earth

The globe banner uses NASA Blue Marble: Next Generation satellite composites — one per month, cloudless, with ocean bathymetry. The originals are public domain (US government work, no restrictions).

**Source:** [NASA Earth Observatory — Blue Marble: Next Generation, Base Map with Topography and Bathymetry](https://science.nasa.gov/earth/earth-observatory/blue-marble-next-generation/base-topography-bathymetry/) — downloaded all twelve monthly `world.topo.bathy.2004MM.3x21600x10800.jpg` composites (each 21,600 x 10,800 pixels, ~27 MB).

**Downscaled** each to 8,192 x 4,096 (power of two for WebGL) at JPEG quality 80 using ImageMagick — for example, April:

```bash
magick world.topo.bathy.200404.3x21600x10800.jpg -resize 8192x4096! -quality 80 apr.jpg
```

The same command was run for every month, writing `jan.jpg` through `dec.jpg` into `public/earth/`. Each result is around 3 MB — sharp on retina displays at the ISS-arc zoom level, well within WebGL texture size limits on all modern GPUs.

What we're after is the view astronauts call the *overview effect* — the real Earth, seen from far enough away to take it in whole. Not Google Earth. You can't grab the globe and spin it. There's no zoom, no pin, no tooltip. It turns at its own pace — one full rotation per day, the same as the real Earth — and if you want to see the other side, you wait twelve hours. The texture swaps each month, so winter months show snow reaching further south and summer months show more green vegetation in the temperate bands; if you want to see the Sahara in January, you wait until January.

The lighting makes the same point. The sun sits directly behind the viewer's head, which means the point at the center of the globe is wherever it's solar noon *right now*. A visitor in the western hemisphere who wants to see Asia in daylight doesn't click and drag — they come back in half a day, or they book a flight. The Earth has weight and rigidity: wait twelve hours, wait six months, travel thousands of miles. That's the opposite of the immediate gratification of a spinnable virtual globe, and it's the right centerpiece for a page about a service that runs on its own, at its own pace, doing its thing.

## Numeric Rate Animation

The six counters on the panel — UDP/HTTP/WebRTC over IPv4 and IPv6 — show 24-hour totals that the gauge container records once a minute and writes to `page.json`. We could just print those numbers and let them sit there. Instead we let them grow, in real time, as the page is open. The viewer sees activity that *looks* live. It isn't — it's a fresh draw each frame from the same statistical distribution the real traffic obeys — and we'd rather be honest about that here than pretend otherwise. The reason we bother: a static number reads as *recorded*, while a number that ticks reads as *running*. The tracker is in fact running; we're matching the visual to the truth, just by a different route than streaming live counts to every visitor.

### Poisson, briefly

Tracker events arrive independently, at a roughly constant average rate, with no memory of one another. That's the textbook setup for a Poisson process: the number of events in any window of length *t* is Poisson-distributed with mean λ = *rt*, where *r* is the long-run average rate. Each counter has its own *r*, derived from its 24-hour total: `rate = recorded / time_day` events per millisecond.

To advance a counter by *t* milliseconds, we draw a sample from Poisson(λ = *r·t*) and add it. That's the whole simulation. The reason to draw rather than just add `r·t` is that `r·t` is a smooth average, and a smooth average looks dead — what makes activity feel alive is the variance, the runs and gaps that a real Poisson process actually produces.

Our `poissonSample` uses two algorithms depending on λ:

- **λ < 20: Knuth's multiplication method.** Multiply uniform random numbers together until the product drops below `e^(-λ)`; the count of multiplications minus one is the sample. Exact, no approximation, but the loop length grows with λ — which is why we hand off to the next algorithm above 20.
- **λ ≥ 20: normal approximation via Box-Muller.** A Poisson(λ) with large λ is well-approximated by a Normal(λ, λ), so we draw a standard normal (one Box-Muller transform of two uniforms) and rescale. Constant-time, and the approximation error is invisible at counter scale.

In practice the per-frame bracket (1k–2M events per day, drawing every ~16 ms) always lands in Knuth's regime — λ stays under 1, so the loop fires zero or one time. The drumbeat bracket (>2M events per day, drawing every 500 ms) starts in Knuth's regime at λ ≈ 11.6 and crosses into Box-Muller around 3.4 million events per day, where λ for a 500 ms window hits 20. Counters busier than that get the approximation; everything else gets the exact draw.

### The three brackets

We pick how to animate each counter from its recorded daily total. Three regimes:

**Under 1,000 events per day — no animation.** At this scale a single simulated event is a visible jump on a number the viewer might read as a precise figure. Adding one would feel like an error rather than a sign of life. So we just print the recorded number and leave it.

**1,000 to 2 million per day — per-frame Poisson.** This is the most pleasing range. Every animation frame (about 60 times a second) we draw a Poisson sample for the elapsed time and add it. The result looks like soap bubbles surfacing — bursts and gaps, brief flurries followed by held breaths, the hundreds digit settling for a beat and then turning over twice in a row. It's the mathematical signature of independent arrivals, and the eye recognizes it as alive in the same way it recognizes a campfire.

**Over 2 million per day — drumbeat Poisson.** Up here the per-frame draw would be so large each tick that the ones and tens digits would blur into a smear, and the counter would look like a slot machine spinning. So we slow the draw to once every 500 ms — the same half-second drumbeat the colon in the clock blinks to. Tens still pop visibly, the machine looks composed and in control, and the viewer's eye gets a moment to settle on each new number before it changes.

### Snapping back

Every 10 minutes, every counter snaps back to the recorded total and starts simulating forward again. If we never reset, a long-open tab would drift further and further from the real number — a viewer who left the page open all afternoon would come back to a UDP counter twice as high as it should be. Ten minutes is short enough that drift stays small (well under a percent of the recorded total even at the high end), and long enough that the snap is invisible to anyone actually watching: most viewers either glance and move on, or leave it open for many minutes, never seeing the moment of correction.

### Working with the browser, not against it

The animation runs on `requestAnimationFrame`, the browser's render-synchronized callback. This buys us two important things for free:

- **Tab-hidden pause.** When the tab loses focus, the browser stops calling our rAF handler. We do exactly zero work in a backgrounded tab — no timers firing, no battery drain, no wasted samples that nobody can see.
- **Render-aligned updates.** Vue's reactivity batches DOM writes to animation frames anyway, so our once-per-frame number updates land at exactly the moment the browser is about to paint. No extra reflows, no jank.

When a hidden tab is brought back to the foreground, we don't want to pretend nothing happened — that would visibly stall the counters for a moment. So on the first beat after thaw, we count how many drumbeats *should* have elapsed during the hide and draw a single Poisson(λ = *r · drum · beats_elapsed*) sample to cover the whole gap. This is statistically identical to having ticked each beat separately: the sum of N independent Poisson(λ) random variables is itself Poisson(N·λ). One draw, no loop, mathematically honest.

Vue's DOM diffing handles the rest. The clock's hour and minute spans only update when those `ref`s actually change — so the hour repaints once an hour, the minute once a minute, and only the colon and counters repaint continuously. The result is a page that feels alive but costs almost nothing to run.
