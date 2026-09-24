
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

The build output (`docs/.vitepress/dist/`) is rsynced to the server where the reverse proxy serves it as static files. A `upload.hide.sh` script handles the rsync. This script is gitignored (via the `*.hide.*` pattern) because it contains your server's SSH details. Create your own:

```bash
#!/bin/bash
rsync -avz --delete docs/.vitepress/dist/ youruser@yourserver:/opt/docs.ftorrent.com/static/ -e "ssh -p 22"
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

The deployed files land in the static directory on the server. The reverse proxy serves them as the docs.ftorrent.com site.

## Type and color

- Prose in Verdana, headings in Trebuchet MS, both with Inter behind them
- Code in Monaspace Krypton at weight 550, served to every reader
- One orange, Pantone 151 C, `#FF7900`, used as is for links and inline code
- Neutral grays, with headings in `#808080`
- Everything lives in `docs/.vitepress/theme/style.css`, and the fonts beside it in `theme/fonts/`

We styled the site to look like the web at the turn of the millennium. Verdana and Trebuchet are the faces Microsoft drew for the screen in the late 1990s, and for readers whose systems have them, the page downloads nothing. Code is set in Krypton, the mechanical face open.ftorrent.com uses for its LCD digits, whose squared-off letters look like the screens of The Matrix.

The orange is the brand color. We use it everywhere the site draws orange, including as text, even though 151C on white falls short of the contrast WCAG asks of body text, so that anything orange on the site is the brand and never a shade tilted away from it. Inline code carries it at a heavier weight on a pale chip. `brand-color.html` at the repository root compares 151C with the darker shades we tried for text.

The grays are the other half of that period's orange-and-silver look. We took the blue tint out of VitePress's grays, so orange is the only hue on the page, and set headings in `#808080`, the dark gray of the Windows 95 interface. Bold prose sits a step lighter than the text around it, because Verdana comes in regular and bold only. A link under the pointer stays orange, and its underline goes from solid to dotted, which is how links of that era said so. Dark mode still has VitePress's own grays, and the button fill and callout tint are still its indigo.

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
