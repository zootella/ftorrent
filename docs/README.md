
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
