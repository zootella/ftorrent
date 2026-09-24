
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
