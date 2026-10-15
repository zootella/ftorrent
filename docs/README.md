
_ftorrent/docs/README.md [docs.ftorrent.com](https://docs.ftorrent.com)_

# Documentation Website

## Scaffolding

From the `docs/` workspace:

```bash
pnpm add -D vitepress
npx vitepress init
```

The init wizard log:

```
┌  Welcome to VitePress!
│
◇  Where should VitePress initialize the config?
│  ./docs
│
◇  Site title:
│  ftorrent
│
◇  Site description:
│  docs.ftorrent.com
│
◇  Theme:
│  Default Theme + Customization
│
◇  Use TypeScript for config and theme files?
│  No
│
◇  Add VitePress npm scripts to package.json?
│  Yes
│
└  Done! Now run npm run docs:dev and start writing.

Tips:
- Since you've chosen to customize the theme, you should also explicitly install vue as a dev dependency.
```

Then add Vue (needed because we chose "Default Theme + Customization"):

```bash
pnpm add -D vue
```

## Deployment

The build output (`docs/.vitepress/dist/`) is rsynced to the server, where the reverse proxy serves it as static files. `pnpm upload` builds and then runs `upload.js` at the repository root, which mirrors the build into the server directory named in `.env` beside it. That file is gitignored, since it names your server; the essay atop `upload.js` lists the values to write in it, and the same file carries the values the desktop client's installer upload reads.

```bash
pnpm upload
```

The deployed files land in the static directory on the server. The reverse proxy serves them as the docs.ftorrent.com site.

## Type and color

- Prose in Verdana, headings in Trebuchet MS, both with Inter behind them
- Code in Monaspace Krypton at weight 550, served to every reader
- One orange, Pantone 151 C, `#FF7900`, used as is for links and inline code
- Neutral grays, with headings in `#808080`
- Everything lives in `docs/.vitepress/theme/style.css`, and the fonts beside it in `theme/fonts/`

We styled the site to look like the web at the turn of the millennium. Verdana and Trebuchet are the faces Microsoft drew for the screen in the late 1990s, and for readers whose systems have them, the page downloads nothing. Code is set in Krypton, the mechanical face open.ftorrent.com uses for its LCD digits, whose squared-off letters look like the screens of The Matrix.

The orange is the brand color. We use it everywhere the site draws orange, including as text, even though 151C on white falls short of the contrast WCAG asks of body text, so that anything orange on the site is the brand and never a shade tilted away from it. Inline code carries it at a heavier weight on a pale chip. `brand-color.html` at the repository root compares 151C with the darker shades we tried for text.

The grays are the other half of that period's orange-and-silver look. We took the blue tint out of VitePress's grays, so orange is the only hue on the page, and set headings in `#808080`, the dark gray of the Windows 95 interface. Bold prose sits a step lighter than the text around it, because Verdana comes in regular and bold only. A link under the pointer stays orange, and its underline goes from solid to dotted, which is how links of that era said so. Dark mode still has VitePress's own grays, and the callout tint is still its indigo. On the home page, the name and the brand button are 151C, the button with white text.

## The installing page and its sidecars

Almost every page here is static: what the build writes is all a reader sees. The installing page is the exception. Its download boxes and install commands fetch each installer's sidecar, the small JSON file the desktop workspace publishes beside it, when the page opens, so publishing a new installer changes the hashes on the page without rebuilding this site. `docs/.vitepress/theme/downloads.js` has the details.

The sidecars live on the apex, `https://ftorrent.com/ftorrent.dmg.json` beside `https://ftorrent.com/ftorrent.dmg`, and this site is a different host, so the browser makes a cross-origin request. It lets the page read the answer only if the server that sends the downloads adds one response header:

```
Access-Control-Allow-Origin: *
```

We set it on every file in the downloads directory rather than on a list of names, so a new installer needs no change on the server. Without it, the page shows Not yet published for every installer. To check it:

```bash
curl -sI -H 'Origin: https://docs.ftorrent.com' https://ftorrent.com/ftorrent.dmg.json | grep -i access-control
```

The download host is written once, as `origin` in `docs/.vitepress/config.js`. In development, `pnpm local` needs no header: the dev server answers sidecar requests itself, from what `pnpm hash` has staged in `desktop/release` and `desktop/linux/release`, and proxies anything not staged there to the download host.

## Research

The `research/` folder beside the site source holds what produced the articles in the site's Research section: one folder per report, each with the scripts, the data they pulled, the figures they drew, and a README on running them. VitePress builds pages only from `docs/`, so nothing in `research/` becomes a page.

## Settings and plugins

The site's settings live in `docs/.vitepress/config.js`, each with the reason beside it. Search is VitePress's local provider, an index built at build time and searched in the browser, with no outside service. The sitemap and each page's last-updated date are generated at build time, the date from git, so the build has to run inside the checkout; the line itself is the theme's own `LastUpdated.vue`, swapped in for VitePress's through an alias in the config, so the date reads one way for every reader, `2026-Oct-08 01:04 UTC`, rather than in the browser's locale. Every page carries a link to its source on GitHub. One markdown plugin is installed, `markdown-it-task-lists`, which draws the planning articles' `- [ ]` boxes as real checkboxes; VitePress renders them as literal brackets without it. The theme's overrides are in `docs/.vitepress/theme/style.css`, with a comment on each rule saying what it changes and why. Under every page, the theme's `CreativeCommons` component puts the Creative Commons badge and the line that licenses the page's text under CC BY 4.0; the badge is Creative Commons' first 88 by 31 button, from 2002, `somerights.gif`, served from `docs/public/`.
