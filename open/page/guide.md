# Building the open.ftorrent.com dashboard

*Prepared by [Claude Code](https://claude.ai/code) using Opus 4.6*
*Created: 2026-Apr-06 | Last reviewed: 2026-Apr-06*

| Component | Version |
|---|---|
| [Vue](https://vuejs.org/) | 3.5 |
| [Vite](https://vite.dev/) | 8.0 |
| Node (build-time only) | 22+ |

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

The result is six files:

```
open/page/
├── index.html         Vite entry point
├── package.json       Dependencies and scripts
├── vite.config.js     Vite + Vue plugin configuration
├── src/
│   ├── main.js        Creates and mounts the Vue app
│   └── App.vue        The dashboard component
└── public/
    └── favicon.ico    Site icon (served as-is, not processed by Vite)
```

## Development

```bash
pnpm install     # from the monorepo root, installs all workspaces
cd open/page
pnpm run dev     # starts Vite dev server with hot reload
pnpm run build   # produces dist/ for deployment
pnpm run preview # serves the built dist/ locally for testing
```

## Deployment

The build output (`dist/`) is rsynced to the server where nginx serves it as static files. A `deploy.hide.sh` script (gitignored) handles the rsync with server-specific SSH details. The pattern:

```bash
pnpm run build
./deploy.hide.sh
```

The deployed files land at `/opt/open.ftorrent.com/static/` on the server. nginx serves them for any request to `open.ftorrent.com` that isn't a tracker announce, scrape, or WebSocket upgrade.
