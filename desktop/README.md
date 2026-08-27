
_ftorrent/desktop/README.md_

# Scaffolding the ftorrent Desktop Client

> Prepared by [Claude Code](https://claude.ai/code) using Fable 5
> <br>Created: 2026-Aug
> <br>Last reviewed: 2026-Aug
> <br>[Tauri](https://tauri.app/): 2.11
> <br>[Vue](https://vuejs.org/): 3.5
> <br>[Vue Router](https://router.vuejs.org/): 5.2
> <br>[Pinia](https://pinia.vuejs.org/): 4.0
> <br>[Vite](https://vite.dev/): 8
> <br>[Tailwind CSS](https://tailwindcss.com/): 4
> <br>[pnpm](https://pnpm.io/): 10.28
> <br>Node: 22
> <br>[Rust](https://www.rust-lang.org/): 1.98

This workspace holds the ftorrent desktop client — a cross-platform BitTorrent and WebTorrent client for Windows, Mac, and Linux, built as a [Tauri](https://tauri.app/) app with a Vue frontend. The design planning lives on [docs.ftorrent.com](https://docs.ftorrent.com/); this guide records how we scaffolded the workspace and what we changed afterward, so the state of the code is reproducible and every departure from the scaffold has its reason written down.

## The scaffold command

We scaffolded with create-tauri-app, choosing the Vue template in plain JavaScript:

```
pnpm create tauri-app ftorrent --manager pnpm --template vue --identifier com.ftorrent --yes
```

Two notes on running it. First, this workspace already existed as a placeholder in the monorepo, so we scaffolded into a scratch directory and merged the output in, keeping the placeholder package.json's name, description, and homepage. Second, as of August 2026 the npm-published create-tauri-app lags Tauri's own templates and pins old versions (vite 6); we accepted what it produced and aligned versions immediately after, which is the next section.

## The versions

The scaffold's stale pins were raised to current versions:

- **Tauri 2.11** — npm packages `@tauri-apps/cli` ^2.11.4 and `@tauri-apps/api` ^2.11.1, Rust crates resolving to tauri 2.11.5. The Tauri CLI requires the npm packages and Rust crates to be on the same major/minor, so bump both sides together. On the Rust side, use a full `cargo update`, not targeted `-p` updates — the targeted form can leave an incoherent dependency tree that fails to compile.
- **Vue ^3.5.41** with **@vitejs/plugin-vue ^6.0.8** and **Vite ^8.2.2**. Vite 8 requires Node 20.19+ or 22.12+.
- **Tailwind 4** (^4.3.3) through its Vite plugin only: `@tailwindcss/vite` in vite.config.js and `@import "tailwindcss";` in src/index.css. There is no postcss.config.js, no tailwind.config.js, and no autoprefixer — Tailwind 4's Vite plugin handles prefixing and configuration natively. A tutorial telling you to create those files is describing Tailwind 3.
- **pnpm 10.28.2**, pinned once in the monorepo root package.json's `packageManager` field, enforced everywhere by corepack.
- **Rust edition 2021**, as Tauri's template ships it. An edition is a language dialect, not a compiler version: Rust ships a new compiler every six weeks and never breaks working code, so the rare change that would break something arrives instead as an edition a crate opts into. There are four — 2015, 2018, 2021, and 2024 — and a single compiler builds all of them, so a crate on one edition links fine against dependencies on another. This workspace sets no minimum compiler version of its own, and Tauri's crates ask for 1.77.2 or newer, which is the real floor.

The lockfiles are part of the design: one pnpm-lock.yaml at the monorepo root and this workspace's src-tauri/Cargo.lock are both committed and both cross-platform — pnpm records every platform's native binaries and selects at install time. A fresh clone on a different operating system should install and build without changing either file; if one changes, that's a finding to investigate, and deleting or regenerating a lockfile is never the fix.

## Modifications after scaffolding

**Scripts**, in package.json: `local` (tauri dev), the four build scripts described under the build depths, `app` (open the built Mac bundle), `win` (start the built Windows exe), and `vite-build`. A pnpm script cannot be named `run` — pnpm's builtin shadows it. And `local` fails with a stack trace that reads like a bug if a dev server is already running: vite.config.js sets `port: 1420` with `strictPort: true`, because Tauri needs to find the frontend at a fixed address, so the second one exits rather than sliding to another port. `lsof -ti :1420` on macOS, or `netstat -ano | findstr :1420` on Windows, names the process already holding it. Note that `tauri dev` and `tauri build` compile into separate profile directories, target/debug and target/release, which share no artifacts — the second full compile after the first is expected.

**Three depths of a build.** `tauri build` compiles once and then packages in stages, and package.json names each stopping point along the trail:

```
pnpm build-binary    # the native binary, at src-tauri/target/release/ftorrent
pnpm build-app       # that, then wrapped into ftorrent.app
pnpm build-dmg       # that, then every packager the targets list names
pnpm build           # the same as build-dmg, under the name a reader looks for first
```

Each stage builds on the one before, and all three run the same frontend build and the same Rust release compile — they differ only in how far the packaging goes. The binary is the compiled Rust with the frontend embedded in it. `ftorrent.app` is that binary placed in a directory alongside the `Info.plist` and icons, which is what makes macOS treat it as an application rather than a command-line tool — so a bare binary launches, but not quite like the shipped app. The last stage produces the packages a user downloads: on a Mac the `.dmg`, on Windows the NSIS `.exe`, on Ubuntu the `.deb`.

That last stage is worth knowing about on macOS. Building a `.dmg` mounts the disk image and drives Finder to position the icons and set the window background, so Finder windows open and close on the desktop while it runs. That is the bundler working, not a fault — but during an ordinary edit-and-check loop, `build-app` gets you a launchable app without the interruption. The script names follow what this stage produces on a Mac; on Windows the same full build hands back the NSIS installer instead, so `build-dmg` there means the whole trail rather than a disk image. `build-app` is macOS-only in a harder sense: `--bundles app` is rejected on Windows, where the CLI accepts only `msi` and `nsis`, so that script fails at argument parsing before any build work begins.

**Which depth a change deserves.** While working, the two halves are quicker on their own: `pnpm vite-build` finishes in a fraction of a second and catches every frontend error, and `cargo check` from `src-tauri` is nearly as quick and catches every Rust error — including a mistyped capability or permission identifier, which Tauri validates as it compiles. `pnpm build-binary` is the gate before handing work over, because it is the cheapest single command that runs the frontend build and a real release compile; `cargo check` type-checks without generating or linking any code, so it cannot stand in for that. Go further only when the change earns it: `build-app` once you have touched the bundle section of tauri.conf.json or the icons, since bundling is what exercises the `Info.plist` and the icon pipeline, and the full build before a release or a handoff to the other platform. Two things to expect — the release profile shares nothing with the debug profile `pnpm local` uses, so the first release build after a stretch of dev work compiles everything over again; and a green build says the code compiles, not that it works. Only running the app tells you that.

**tauri.conf.json**: `beforeBuildCommand` points at `pnpm vite-build`; the window is created 800 × 600 with `dragDropEnabled` true; the identity values are productName `ftorrent` and identifier `com.ftorrent`.

**Bundle targets.** The scaffold ships `"targets": "all"`, which builds everything each platform can build — on Windows an MSI beside the NSIS installer, on Linux an AppImage beside the Debian package. ftorrent distributes four packages and no more, so the four are named instead:

```
"targets": ["app", "dmg", "nsis", "deb"]
```

One list serves all three platforms: a target that doesn't apply to the machine doing the build is skipped, so macOS produces the .app and .dmg, Windows the NSIS .exe, and Ubuntu the .deb. The skipping is silent — a Windows build simply makes the one installer, with no notice that three of the four names didn't apply. The portable zip is assembled separately, later.

**The Windows install mode.** Beside the targets sits the NSIS installer's mode:

```
"windows": {"nsis": {"installMode": "currentUser"}}
```

`currentUser` installs into the user's own directory — no UAC elevation, and no wizard page asking the user to choose between a per-user and a machine-wide install. That is what the plan calls for on Windows, where the whole installation is per-user. Tauri 2.11 already defaults to this mode, so the line changes no behavior today; it's here because the plan depends on the mode and a default is not a promise.

**Content Security Policy.** We replaced the scaffold's `"csp": null` with a real policy:

```
"csp": "default-src 'self'; connect-src 'self' ipc: http://ipc.localhost; img-src 'self' data: blob:"
```

The webview loads only the bundled frontend and never navigates anywhere, and all networking lives in the Rust core — so the CSP is the second wall behind Vue's template escaping: if hostile text from torrent metadata or filenames ever managed to become markup, this policy stops it from executing or exfiltrating. Piece by piece: `default-src 'self'` allows only bundled assets (Tauri auto-nonces its own injected init scripts once csp is non-null); `connect-src` lists Tauri's two IPC endpoints — `ipc:` on Mac, `http://ipc.localhost` on Windows — so `invoke()` keeps working; `img-src data: blob:` covers rendering images from local bytes. Nothing gets added to this policy without a reason written next to it. `devCsp` stays unset on purpose: dev mode is unrestricted so Vite HMR works, and the policy guards what ships.

To verify the CSP, build and check the binary — Tauri injects the policy into the HTML embedded there, so the on-disk dist/index.html won't show it:

```
strings src-tauri/target/release/ftorrent | grep default-src
```

Failure modes are visibly loud: a blocked IPC endpoint means the UI can't reach Rust at all, and a blocked img-src means images don't render. If the built app behaves normally, the policy fits.

**Plugins and capabilities.** Tauri v2 gates what the webview may call. A plugin adds commands to the Rust core, and a capability file names which of those commands the window is allowed to invoke — a plugin registered but not permitted is unreachable from the page. The scaffold grants `opener:default`, a blanket set that includes opening arbitrary URLs. We name specific permissions instead, in `src-tauri/capabilities/default.json`:

```json
"permissions": [
	"core:default",
	"opener:allow-reveal-item-in-dir",
	"dialog:allow-open",
	"dialog:allow-save"
]
```

Two plugins are registered in `src-tauri/src/lib.rs`: **dialog**, for the familiar operating-system open and save boxes — picking a `.torrent` file, choosing a download folder — and **opener**, for revealing a finished download in Finder or File Explorer.

What is left out matters as much as what is in. Dialog's message, ask, and confirm boxes are excluded deliberately: a native-looking dialog an application can raise with arbitrary text is a social-engineering primitive, and ftorrent's own interface lives in the page where the user can see it for what it is. Opener's URL opening is excluded until a feature actually needs it, and should arrive then with an allowlist scoped to that feature rather than as a standing permission. The rule behind both: register only the plugins the app uses, and grant named permissions rather than a plugin's `:default` set.

`core:default` stays as the scaffold shipped it, and that is the deliberate exception. It is Tauri's own set — path, event, window, webview, app, image, resources, menu, and tray — the machinery that makes a Tauri app an app at all. Enumerating it by hand would mean dozens of identifiers to arrive at nearly the same place; the granular rule is aimed at plugins, which extend the app's reach past the framework itself.

Tauri validates permission identifiers when it compiles, so a typo here fails the build rather than failing silently at runtime. `cargo check` from `src-tauri` is the verification step.

**Disk access.** `src-tauri/src/disk.rs` holds the app's own filesystem commands — `disk_readdir`, `disk_stat`, `disk_read`, and `disk_copy` — and `src/disk.js` wraps each one in an `invoke()` call for the frontend. They are registered in the handler list in `lib.rs` and need no capability entry: the capability system governs plugin commands, and these are ours.

Tauri does ship a filesystem plugin, and we deliberately don't use it. Its scoping confines an app to directories named ahead of time, which is the right default for software that has no business reading a whole disk — but a torrent client reads and writes wherever the user tells it to, so that scoping would be a wall to climb rather than a protection. Writing our own commands is the supported way to take that power on deliberately.

What keeps it safe sits outside the module, in three layers. Every path these commands receive begins in a user gesture — a drag onto the window, a choice in a dialog — never in text that arrived from a peer, a tracker, or a torrent's metadata. Untrusted text reaches the page only through Vue's escaping interpolation, so a filename can never become script that calls `invoke()` on its own. And the Content-Security-Policy keeps foreign script out of the webview even if one of the first two layers ever failed. The module opens with an essay stating that contract; read it before calling anything in the file.

The write family — writing, renaming, deleting, creating directories — is sketched at the bottom of the file and deliberately unbuilt. Reading and copying can trust their caller; deleting should trust less, and what guard those operations deserve is a decision to make once the client's own features say what they need.

**Vue Router, in hash mode.** The frontend routes with [Vue Router](https://router.vuejs.org/) 5, configured with `createWebHashHistory()`. A router's other mode, history mode, writes real paths like `/about` and expects a server to answer a request for that path when the page reloads — but a Tauri window loads its frontend out of the bundle with no server behind it, so such a reload would find nothing. Hash mode keeps the whole route after a `#`, the part a browser resolves locally and never requests. The user never sees it: the window has no address bar.

The frontend is arranged around that. `src/router/index.js` names every page the window can show and is meant to be read as the app's table of contents. `src/App.vue` is the shell — the navigation and the `<router-view />` outlet the current page fills — and the scaffold's greet demo moved into `src/pages/MainPage.vue`. `src/pages/AboutPage.vue` is written as a lazy route, an `() => import(…)` in place of an imported component, so the build gives it a chunk of its own that the app fetches the first time someone opens it; that second chunk is visible in the `vite build` output.

A note on that naming, since Vue Router's own documentation says `views/HomeView.vue` and this says `pages/MainPage.vue`. The rest of this monorepo already calls them pages — the website workspace runs on Nuxt, where `pages/` is the framework's own directory — so one word across the repository beat matching the router's examples. It reads better out loud, too, in a project where every component file already ends in `.vue`.

Two version notes. This is the 5.x line, released January 2026, rather than the 4.x line that Vue 3 shipped alongside for years — 5.x is current, its peer requirements (Vue 3.5, Vite 8) match what this workspace already runs, and starting on the previous major would mean a migration later for nothing gained now. And 5.x installs a set of build-time dependencies that 4.x did not, because the file-based and typed-routes tooling that used to be a separate plugin now lives in the package; nothing in that tooling is imported here and none of it reaches the shipped bundle, where the router costs about 9 kB gzipped.

**Pinia, for state that outlives a view.** The frontend keeps its state in [Pinia](https://pinia.vuejs.org/) 4, Vue's official store, added at scaffold time for the reason the router was: it settles a shape, and a shape is cheap to settle before anything is built on it. The rule it answers is narrow — state that outlives the component displaying it, or that more than one component reads, belongs in a store; state that belongs to one view and dies with it does not.

A torrent client has a great deal of the first kind, and it arrives from more than one direction. The engine will push a stream of updates — progress, peers, alerts — that keeps arriving whether or not the view showing it happens to be mounted. Whether ftorrent owns the `.torrent` association, whether a firewall exemption is in place, whether the router accepted a port mapping are questions answered somewhere else entirely. Those facts have no single home beneath the interface where they could all meet, and a component the router unmounts on navigation is the wrong place to keep any of them. The store is where they meet: it fills itself by calling down through `invoke()` and by listening for events, and every component reads from it rather than from the source, so a fact crosses the boundary once when it changes rather than once for every view that shows it.

**What the desktop changes.** Pinia was written for the web, and several of its standard practices answer problems this app doesn't have.

Its instance model is the first. A `createPinia()` per request, with state serialized into the page and rehydrated on the client, exists for server-side rendering. This process opens one window once, so `createPinia()` is called a single time in `src/main.js` and none of that machinery is in play.

Listeners belong to the store rather than to a component. On the web a subscription opens inside a composable that a component uses and closes when that component unmounts. The engine's event stream has to outlive every component here — the window can be hidden for days with nothing rendered at all while transfers continue — so the subscription starts once and is held, rather than being tied to whatever is on screen.

The session is long. A page in a browser lives for minutes and takes its store with it when the tab closes, so a store that accumulates is never anyone's problem. This process runs for weeks. Anything a store appends to — an alert list, an event log — needs a bound written in from the start, because it fails slowly and no short test will show it.

And browser storage is not where anything persists. `localStorage` is the standard answer on the web because it is the only durable place a page has; here there is a filesystem behind the Rust core, and the webview's own storage sits in a per-user directory on the host machine, which the design keeps clear so that a copy running from a USB stick leaves nothing behind. So neither `pinia-plugin-persistedstate` nor `@tauri-apps/plugin-store` — both are the right answer to a constraint we don't have, and the wrong answer to the one we do.

**What the store holds today.** One value, in `src/stores/greet.js`: the name typed into the greet form on the main page. It used to be a `ref` inside `MainPage.vue`, which meant a trip to the about page and back returned an empty box — the router unmounted the page, and mounting it again ran its setup from scratch. Moving that ref into a store is the whole change, and it is the smallest honest instance of the rule above: a fact the interface owns outright, with nothing underneath to ask and nothing to save, that simply has to survive the component displaying it.

Wrapping the router outlet in `<KeepAlive>` would have kept the box filled too, and it is the more common reflex. It preserves the component instead of moving the state out of it, so the value stays welded to the one component that owns it, and a second component wanting the same fact still can't reach it. The reply from the Rust `greet` command is deliberately left as component state and still clears on navigation — the two behaviors sitting side by side in one small page are easier to see than to describe. Stores are written in the setup form, `defineStore` with a function that returns what the store holds, which reads like the `<script setup>` components around them.

**The package.** Pinia 4.0.3 is the only package this adds to the tree, which its manifest does not lead you to expect. It declares a runtime dependency on `nostics` — a small, dependency-free structured-diagnostics library first published in April 2026, living under `vercel-labs` rather than `vuejs` — and a non-optional peer dependency on `@vue/devtools-api` rather than the ordinary dependency that would usually be. Both were already installed here, pulled in by the build-time tooling Vue Router 5 carries, so the lockfile gains exactly one new resolution: `pinia 4.0.3(@vue/devtools-api@8.2.1)(vue@3.5.41)`.

The Vue Router line that moves alongside it is that package's own resolution key being re-stamped. Vue Router 5 declares Pinia as an optional peer for its data loaders, so installing Pinia satisfies a peer that was previously unmet — which rewrites the key without changing a line of what we import or ship.

That peer deserves one clarification, since its name suggests a benefit we are not collecting. Pinia's devtools timeline is a real convenience on the web and is no part of the case for a store here — we have neither set it up in the Tauri window nor needed it. The reason to keep state in a store in this app is the rule at the top of this section, not the tooling around it.

**How the window sizes itself.** The window is created hidden — `"visible": false` in tauri.conf.json — and `src/window.js` gives it a size before anything reveals it, so it appears once already correct rather than flashing at one size and jumping to another.

The sizing reads `currentMonitor()` and takes its `workArea`: the monitor rectangle minus the chrome the operating system keeps for itself, the menu bar and Dock on macOS, the taskbar on Windows. On Windows that resolves to `GetMonitorInfoW`'s `rcWork`, so it handles a taskbar on any edge, at any thickness, with auto-hide on or off. The window becomes 60% of the usable width and 80% of the usable height.

One conversion in there is easy to get wrong and hard to catch. `workArea` arrives in physical pixels while `setSize` speaks logical ones, so the measurement is divided by the monitor's `scaleFactor`. On a display at 100% scaling that division is multiply-by-one, and code with the conversion is byte-identical to code without it — the mistake surfaces only on a scaled or Retina display, where it produces a window roughly twice the screen in each direction.

What the code deliberately does not do is set a position. Where a window opens is the operating system's job, and leaving it there is what makes a second copy land beside the first instead of exactly on top of it — which matters here, because an installed copy and a portable copy are meant to run side by side.

The width and height in tauri.conf.json are 800 × 600, which is also Tauri's own default, and they are only a fallback. Tauri needs some size at creation, and this is what the window keeps if the monitor cannot be identified or the sizing throws. It is deliberately small: an aspirational size would put every failure path on a window too large for a modest screen, and on Windows it would constrain placement as well, since the OS picks the cascade position from the creation size before any resize runs.

Two capability grants make this work: `core:window:allow-set-size` and `core:window:allow-show`. Reading the monitor and checking visibility are already covered by `core:window:default`.

**Indentation and line endings.** All source files indent with tabs, per the style guide at the repository root (the scaffold's space-indented files were converted). Line endings are LF in the repository and in every working tree on every platform, enforced by the .gitattributes at the repository root.

**Ignore rules.** The scaffold's own .gitignore files were dropped — the monorepo root .gitignore already covers everything Tauri generates: target/, gen/, dist, node_modules.

## Verified

We built and smoke-tested from a fresh clone on both active platforms in August 2026. On macOS, `pnpm build` produces the .app bundle and .dmg, and `pnpm local` serves the dev window with hot module replacement. On Windows 10 22H2, `pnpm build` produces the NSIS installer alone — one bundle, no .msi — and that installer runs with no UAC prompt and no page asking whether to install for one user or for the whole machine, landing the app under `%LOCALAPPDATA%` with its uninstall entry in `HKCU` and nothing in `HKLM` or Program Files. On both platforms `pnpm install` left the two lockfiles byte-identical, the Tauri CLI reported no version mismatches, and the frontend-to-Rust IPC round-trip works in both the debug and release profiles. One gap: navigating between pages has been exercised on macOS but not yet on Windows.

The window sizing was measured on Windows rather than eyeballed. Against a 1920 × 1200 display whose work area is 1160 tall, the client rect came back 1152 × 928 — 60% and 80% of the usable space exactly, and 928 rather than the 960 that using the full monitor height would have produced, which proves the taskbar exclusion rather than assuming it. The window landed at (156, 111), placed by the OS as intended. One line remains unverified anywhere: both test machines run at 100% scaling, where the scale-factor conversion cannot be told apart from its absence.
