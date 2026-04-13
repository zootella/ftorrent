
_ftorrent/open/page/README.md [open.ftorrent.com](https://open.ftorrent.com)_

# Public Tracker Dashboard Front End

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.6
> <br>Created: 2026-Apr
> <br>Last reviewed: 2026-Apr
> <br>[Vue](https://vuejs.org/): 3.5
> <br>[Vite](https://vite.dev/): 8.0
> <br>Node: 22
> <br>[Three.js](https://threejs.org/): 0.183

The dashboard at [open.ftorrent.com](https://open.ftorrent.com/) is a Vue + Vite single-page application. It shows statistics from the three Aquatic tracker containers (UDP, HTTP, WebSocket) by reading a `page.json` file that the gauge container generates every minute. The page is built on a development machine and deployed as static files served by nginx — no application server in the request path.

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
│   ├── main.js        Creates and mounts the Vue app
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

## Earth texture

The globe banner uses NASA Blue Marble: Next Generation satellite composites — one per month, cloudless, with ocean bathymetry. The originals are public domain (US government work, no restrictions).

**Source:** [NASA Earth Observatory — Blue Marble: Next Generation, Base Map with Topography and Bathymetry](https://science.nasa.gov/earth/earth-observatory/blue-marble-next-generation/base-topography-bathymetry/) — downloaded all twelve monthly `world.topo.bathy.2004MM.3x21600x10800.jpg` composites (each 21,600 x 10,800 pixels, ~27 MB).

**Downscaled** each to 8,192 x 4,096 (power of two for WebGL) at JPEG quality 80 using ImageMagick — for example, April:

```bash
magick world.topo.bathy.200404.3x21600x10800.jpg -resize 8192x4096! -quality 80 apr.jpg
```

The same command was run for every month, writing `jan.jpg` through `dec.jpg` into `public/earth/`. Each result is around 3 MB — sharp on retina displays at the ISS-arc zoom level, well within WebGL texture size limits on all modern GPUs.

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
