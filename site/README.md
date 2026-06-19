
_ftorrent/site/README.md [ftorrent.com](https://ftorrent.com)_

# Home Page

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.8
> <br>Created: 2026-Jun
> <br>Last reviewed: 2026-Jun
> <br>[Nuxt](https://nuxt.com/): 4.4
> <br>[Vue](https://vuejs.org/): 3.5
> <br>Node: 22

This workspace builds the page at the apex of [ftorrent.com](https://ftorrent.com/) — the front door of the project. It's a small [Nuxt](https://nuxt.com/) single-page application, built on a development machine into a handful of static files and served by a reverse proxy. Nothing runs on the server at request time but the proxy handing back the page.

## What it's for

From a visitor's point of view, the home page does a few things, and will grow into a few more:

- **The project homepage.** A short, plain landing page that says what ftorrent is and points at everything else.
- **Downloads.** Where you'll get the desktop client (the `desktop` workspace) for Mac, Windows, and Linux.
- **A map of the subdomains.** Links out to the running services — the trackers and dashboard at [open.ftorrent.com](https://open.ftorrent.com/), the documentation at [docs.ftorrent.com](https://docs.ftorrent.com/), the connectivity diagnostic at [good.ftorrent.com](https://good.ftorrent.com/), and the DHT bootstrap node.
- **A magnet link introducer and inspector.** A small tool to read a magnet link — to see what it names and how it's put together — entirely in the browser. (A later pass.)
- **Maybe an in-browser client.** Over time this page may grow a working client built on [WebTorrent](https://webtorrent.io/), which speaks the BitTorrent protocol from a web page over WebRTC.

The page is deliberately light. It's the lobby, not the building.

## Why Nuxt

The page is a front end and nothing more — no server-rendered HTML, no API routes, no database. We could have assembled it by hand from [Vite](https://vite.dev/), [Vue](https://vuejs.org/), and [Vue Router](https://router.vuejs.org/) as three separate dependencies wired together, the way the `open/page` dashboard is built. For a page that's going to grow several small features over time, we reached for [Nuxt](https://nuxt.com/) instead and let it do the wiring.

What Nuxt gives us over the raw pieces:

- **File-based routing.** A file at `app/pages/about.vue` *is* the `/about` route. There's no router table to hand-maintain — adding a page is adding a file. (Under the hood it's still Vue Router; Nuxt generates the route map from the directory.)
- **Auto-imports.** Components, composables, and Vue APIs are available without import lines.
- **`useHead`** for per-page `<title>` and meta, and **layouts** for shared page chrome as the site grows.
- **A module ecosystem** — drop-in packages for things like internationalization (the page is English now, and may add Dutch) that would otherwise be hand-rolled.
- **Consistency** with the project's other Nuxt-style front ends, so the same mental model carries across workspaces.

The framework earns its keep even though we switch off half of what it can do — because the half we keep is exactly the tedious half.

## SPA-only mode

Nuxt has two clean deployment shapes. **Full-stack** — server-rendered HTML plus `/api` routes — runs on a JavaScript runtime at request time: a Node server, or a Cloudflare Worker that Nitro can emit, with a CDN serving the client chunks alongside it. **Frontend-only** drops the runtime entirely: `nuxt generate` writes static files, and nothing executes on the server when a request arrives. This page is the second kind — a **pure single-page application**, a front-end bundle and nothing else.

Two settings in `nuxt.config.ts` do it:

```ts
export default defineNuxtConfig({
	// Frontend only: no runtime SSR, no Nitro server, no API routes.
	ssr: false,

	// Pure single-shell SPA: don't prerender a static HTML file per route.
	nitro: {
		prerender: {
			crawlLinks: false,
		},
	},
})
```

**`ssr: false`** turns off server rendering. `nuxt generate` now emits a front-end bundle: an `index.html` shell, the hashed JavaScript and CSS chunks under `_nuxt/`, and two fallback copies of the shell, `200.html` and `404.html`. There's no `.output/server/` to deploy — nothing runs at request time.

**`nitro.prerender.crawlLinks: false`** is the second, less obvious step, and it's the one that keeps the output pure. Out of the box, `nuxt generate` walks your routes and writes a *separate* static HTML file for each one — `about/index.html`, and so on for every page. In SPA mode each of those files is just a byte-for-byte copy of the same empty shell (the real content is in the JavaScript, rendered in the browser), so they're pure duplication. Turning off link-crawling stops it: the build emits only the single `index.html` shell plus the `200.html` / `404.html` fallbacks, and every route — `/about` included — is served that one shell and rendered client-side.

We do this for simplicity: one shell, one cache rule, no per-route files to reason about. The route still exists and still works — it's resolved in the browser by Vue Router, not by a file on disk.

### The build

```bash
pnpm build      # nuxt generate → .output/public/
```

The output lands in `.output/public/` — **not** `dist/`. This trips people up: in Nuxt 2, `nuxt generate` wrote to `dist/`. Since Nuxt 3, the Nitro build system writes to `.output/`, and the deployable static files are specifically in `.output/public/`. (Nuxt still drops a `dist` → `.output/public` symlink at the project root for old muscle memory, but `.output/public` is the real, canonical path, and the one to point a deploy at.)

A finished build:

```
.output/public/
├── index.html       the SPA shell (no page content — boots the app)
├── 200.html         SPA fallback (a copy of the shell)
├── 404.html
└── _nuxt/
    ├── *.HASH.js     entry + per-route chunks, content-hashed
    └── *.HASH.css
```

### Deployment

The `.output/public/` directory is rsynced to the server, where the reverse proxy serves it as static files — the same pattern as the project's other front-end workspaces. A gitignored `upload.hide.sh` holds the rsync with your server's SSH details. Create your own:

```bash
#!/bin/bash
rsync -avz --delete .output/public/ youruser@yourserver:/opt/ftorrent.com/static/ -e "ssh -p 22"
```

Then build and deploy in one step:

```bash
pnpm upload     # nuxt generate && ./upload.hide.sh
```

### The same idea in Next.js

This deployment shape isn't particular to Nuxt: the equivalent in [Next.js](https://nextjs.org/) is **static export** (`output: 'export'`), which writes a static `out/` directory with no Node server at runtime — Next prerenders each route's content into its HTML where our `ssr: false` build ships a blank, client-rendered shell, but both are identical to serve. In neither case does "static" mean serverless: a bundled SPA still needs a static HTTP server and won't open over `file://`, because its shell loads chunks from absolute, `/`-rooted paths and ES-module scripts require an `http(s)` origin (`pnpm preview` serves it locally). The only thing that needs no server at all is a single self-contained HTML file — which is what this page was before it became a bundle.

## Serving a single-page app from static files

The whole architecture rests on one idea: **the path picks the document; the browser, not the server, picks the route.** When someone visits `/about`, the server doesn't need to know what `/about` means — it just hands back the SPA shell, and the JavaScript figures out the rest.

That takes one rule in the web server: *for any path that isn't a real file on disk, serve `index.html`.* A request for `/_nuxt/entry.HASH.js` or `/favicon.ico` is a real file and is served directly; a request for `/about` is not a file, so it falls back to the shell, which boots and lets Vue Router read the path and render the page. This is the same primitive every static SPA host uses, under different names — it's nginx's `try_files`, Netlify's `_redirects`, Vercel's rewrites.

That's the *first* load. Once the bundle boots, navigation stays in the browser: clicking a `<NuxtLink>` has Vue Router intercept the click, push the new path with `history.pushState`, and re-render in place — no HTTP request at all. The back button fires `popstate`, the router reads the now-current path, and re-renders. The server has no idea any of it happened; it's pulled back in only on a hard refresh, a URL pasted into a fresh tab, an external link in, or the fetch of a lazy route chunk — each of which is just another `try_files` lookup landing on the shell or a real file.

Two cache rules go alongside it:

- The hashed `_nuxt/` chunks can be cached **forever** (`immutable`). The content hash in the filename is the cache-buster — a new build produces new filenames, so a stale cache can never serve the wrong chunk.
- The **shell** must be served **no-cache**, so that after a deploy a visitor doesn't keep loading an old shell that points at chunks that no longer exist. It's served at *every* route, not just at `/index.html` — which is why the rule below matches "everything that isn't a hashed asset" rather than the literal `/index.html` path.

Below are the same three pieces — the SPA fallback and the two cache rules — for Caddy and for nginx.

### Caddy

If Caddy terminates TLS itself — the simplest setup, where it fetches and renews its own certificate automatically just by naming the domain:

```
ftorrent.com {
	root * /opt/ftorrent.com/static
	try_files {path} /index.html
	file_server

	@immutable path /_nuxt/*
	header @immutable Cache-Control "public, max-age=31536000, immutable"
	@dynamic not path /_nuxt/*
	header @dynamic Cache-Control "no-cache"
}
```

If instead a TLS-terminating reverse proxy sits in front of Caddy — so Caddy should serve plain HTTP and not reach for a certificate of its own — name the site with the `http://` scheme and bind it where the proxy expects, for example a Unix socket, so Caddy's automatic HTTPS stays out of the way:

```
http://ftorrent.com {
	bind unix//run/caddy/caddy.sock
	root * /opt/ftorrent.com/static
	try_files {path} /index.html
	file_server

	@immutable path /_nuxt/*
	header @immutable Cache-Control "public, max-age=31536000, immutable"
	@dynamic not path /_nuxt/*
	header @dynamic Cache-Control "no-cache"
}
```

The cache directives are identical in both; only the TLS-and-binding wrapper changes. They're written as two complementary matchers — `/_nuxt/*` immutable, *everything else* no-cache — rather than the obvious `header /index.html "no-cache"`. That's deliberate: under Caddy's default directive order, `header` runs *before* `try_files`, so its matcher sees the original request path; a `/index.html` matcher would catch only a literal request for `/index.html`, never `/` or `/about`, which are handed the shell through the fallback. Matching anything that isn't a hashed asset tags the shell no-cache no matter which route served it.

### nginx

The same three pieces in nginx, for the many readers who already run it:

```nginx
server {
	listen 80;
	server_name ftorrent.com;
	root /opt/ftorrent.com/static;

	# Hashed build assets cache forever.
	location /_nuxt/ {
		add_header Cache-Control "public, max-age=31536000, immutable";
	}

	# SPA fallback: try the real file, then the directory, then the shell —
	# and tag everything this location serves (including every fallback shell) no-cache.
	location / {
		try_files $uri $uri/ /index.html;
		add_header Cache-Control "no-cache";
	}
}
```

TLS is omitted here for brevity — terminate it on this `server` block with `listen 443 ssl` and your certificate, or at a proxy in front, the same way your existing stack already handles your other sites.

## Bookmark fragments and privacy

There's a third part of a URL that neither the file lookup nor the SPA fallback above ever touches: the **fragment**, the part after `#`. Most readers know it by sight — the `#History` at the end of a Wikipedia link is a fragment, and clicking it scrolls you to that article's _History_ section. That's what fragments were made for: pointing at a place *within* a document, not at a different document. The browser fetches the page, then uses the fragment to find the spot — *which* document is the server's question, *where in* it is the browser's; named anchors have worked this way since the early web and still do. And because that job always belonged to the browser, the fragment is never handed to the server at all — which turns out to be a small, durable idea worth building on: something a page can do entirely in the visitor's browser, with the server never in the loop.

Three kinds of link land on this one page, and the server treats each differently:

```
https://ftorrent.com/logo.png       real file    →  the proxy serves it from disk
https://ftorrent.com/about          SPA route    →  the proxy serves index.html; Vue Router renders /about
https://ftorrent.com/#/some-magnet  fragment     →  the browser never sends the part after #
```

The first two we've covered. The third is different in kind. By the design of the web ([RFC 3986](https://www.rfc-editor.org/rfc/rfc3986)), a URL's fragment is **separated off by the browser before the request is ever made** and kept as client-side state. It isn't part of the request line, so it isn't something the server can choose to log or to ignore — it simply never arrives.

It's worth walking a fragment all the way to the server and watching it not get there. Say someone posts `https://ftorrent.com/#/some-magnet` somewhere public:

1. **The browser parses the URL** and splits the fragment off. Everything after `#` is set aside as `location.hash`; what's left to fetch is just `https://ftorrent.com/`.
2. **DNS** resolves `ftorrent.com` to an address.
3. **The TLS handshake** sets up the encrypted channel. The server name — `ftorrent.com` — is visible to the network in the handshake (the SNI field), but only the hostname: no path, no fragment.
4. **The encrypted request goes out:** `GET / HTTP/2`. The path and any query string *are* sent to the server — encrypted on the wire, then decrypted and read by the server, which is how it knows which page you asked for. The fragment is **not in the request at all.** So "is the fragment encrypted?" doesn't quite apply: it isn't sent in the clear, it isn't sent encrypted, it isn't sent. The server receives a request for `/` and has no mechanism by which it could learn what came after the `#`.

So where does the fragment live? In the URL bar, in browser history, in `location.hash` — and with whoever the link was shared with. Posted to a public forum, the forum and everyone reading it can see it; that's expected, and it's no different from any other link. What's notable is narrower and architectural: **the web server, as a piece of infrastructure, never receives the fragment** — because the web is built that way — which means a feature that takes its input from the fragment can run with no back end at all.

That's the foundation the magnet introducer is built on. A magnet link carried in the fragment (`/#/<magnet>`) is handed to the *page*, not the server — so the page can work with a magnet the server was never sent, and do several things a raw `magnet:` link on its own can't:

- **Inspect the magnet.** Parse it and lay its parts out — info hash, display name, trackers — and let you edit them, so the link is something you can read and understand rather than an opaque string.
- **Hand off to a real client.** A bare `magnet:` link is a dead end for a newcomer: if no BitTorrent client is registered to handle the scheme, clicking it does nothing. The introducer can first point a visitor without a client to one — including ftorrent's own desktop client — and *then* open the magnet through the `magnet:` protocol handler, so their installed client takes it. A working path instead of a broken link.
- **Adapt to mobile.** Most of the web is on phones, where the `magnet:` handoff is unreliable. The page can detect a mobile browser and, rather than firing a handoff that will fail, suggest coming back on desktop.
- **Eventually, run in the page.** With [WebTorrent](https://webtorrent.io/) it can go all the way — join the swarm and fetch the file directly in the browser over WebRTC, with no external client at all.

(The interactive tool is a later pass; this section sets out the mechanism it stands on.)

One footnote, since we opened on the fragment's first job — the jump to a named section. In a client-rendered SPA that jump takes a little extra work: the section a fragment points at isn't in the HTML the server returns, it only exists once the JavaScript renders the page, so the browser's native scroll-to-anchor runs against the empty shell, finds nothing, and gives up. Vue Router fills the gap — after the route's components mount it re-runs the scroll itself (its `scrollBehavior`, which Nuxt wires up by default). Same-page anchor clicks still scroll natively, since the target is already in the DOM; cross-route ones ride the client-side navigation and scroll once the new page has rendered. The one case to watch is content that appears only after async data resolves — the scroll can fire before the element exists, which is what `scrollBehavior`'s `nextTick` and promise escape hatches are for.

## Adding modules

A front-end build is also a door to the npm ecosystem — something a single hand-written `index.html` couldn't reach. Three tiers of it are relevant here:

- **Framework modules** we already lean on — Vue Router (via Nuxt's routing), and Nuxt modules such as an i18n package when the page goes bilingual.
- **Small utilities** we'll likely want soon. For the magnet inspector, [`magnet-uri`](https://www.npmjs.com/package/magnet-uri) parses and builds magnet links in a few lines instead of a hand-rolled URL parser; a tiny QR-code package could render a magnet as a scannable code.
- **WebTorrent**, eventually — [`webtorrent`](https://www.npmjs.com/package/webtorrent) is the BitTorrent protocol implemented for the browser over WebRTC, and it's what an in-browser client on this page would be built from.

Each is `pnpm add`-ed into this workspace and tree-shaken into the bundle, so the page ships only the code a feature actually uses.

## Project structure

```
site/
├── nuxt.config.ts        ssr: false + the single-shell prerender setting
├── package.json          local / dev / build / preview / upload scripts
├── app/
│   ├── app.vue           the root component (<NuxtPage />)
│   └── pages/
│       ├── index.vue     the home page         → /
│       └── about.vue     a demonstration route → /about
└── upload.hide.sh        gitignored rsync deploy (your SSH details)
```

Nuxt 4 keeps application code under `app/`. The scripts follow the workspace convention: `pnpm local` (and its alias `pnpm dev`) run the dev server, `pnpm build` generates the static site into `.output/public/`, `pnpm preview` serves a built copy, and `pnpm upload` builds and rsyncs.
