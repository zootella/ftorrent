
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

The build output (`docs/.vitepress/dist/`) is rsynced to the server where nginx serves it as static files. A `upload.hide.sh` script handles the rsync. This script is gitignored (via the `*.hide.*` pattern) because it contains your server's SSH details. Create your own:

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

The deployed files land in the static directory on the server. nginx serves them as the docs.ftorrent.com site.
