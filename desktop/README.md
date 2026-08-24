
_ftorrent/desktop/README.md_

# Scaffolding the ftorrent Desktop Client

> Prepared by [Claude Code](https://claude.ai/code) using Fable 5
> <br>Created: 2026-Aug
> <br>Last reviewed: 2026-Aug
> <br>[Tauri](https://tauri.app/): 2.11
> <br>[Vue](https://vuejs.org/): 3.5
> <br>[Vite](https://vite.dev/): 8
> <br>[Tailwind CSS](https://tailwindcss.com/): 4
> <br>[pnpm](https://pnpm.io/): 10.28
> <br>Node: 22

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
- **Rust edition 2021**, as Tauri's template ships it.

The lockfiles are part of the design: one pnpm-lock.yaml at the monorepo root and this workspace's src-tauri/Cargo.lock are both committed and both cross-platform — pnpm records every platform's native binaries and selects at install time. A fresh clone on a different operating system should install and build without changing either file; if one changes, that's a finding to investigate, and deleting or regenerating a lockfile is never the fix.

## Modifications after scaffolding

**Scripts**, in package.json: `local` (tauri dev), the four build scripts described under the build depths, `app` (open the built Mac bundle), `win` (start the built Windows exe), `vite-build`, and `wash` (delete build output and dependencies for a from-scratch install). `wash` calls `rimraf`, which is deliberately not a declared dependency of this workspace — install it globally (`pnpm add -g rimraf`) or that one script fails while everything else works. A pnpm script cannot be named `run` — pnpm's builtin shadows it. Note that `tauri dev` and `tauri build` compile into separate profile directories, target/debug and target/release, which share no artifacts — the second full compile after the first is expected.

**Three depths of a build.** `tauri build` compiles once and then packages in stages, and package.json names each stopping point along the trail:

```
pnpm build-binary    # the native binary, at src-tauri/target/release/ftorrent
pnpm build-app       # that, then wrapped into ftorrent.app
pnpm build-dmg       # that, then every packager the targets list names
pnpm build           # the same as build-dmg, under the name a reader looks for first
```

Each stage builds on the one before, and all three run the same frontend build and the same Rust release compile — they differ only in how far the packaging goes. The binary is the compiled Rust with the frontend embedded in it. `ftorrent.app` is that binary placed in a directory alongside the `Info.plist` and icons, which is what makes macOS treat it as an application rather than a command-line tool — so a bare binary launches, but not quite like the shipped app. The last stage produces the packages a user downloads: on a Mac the `.dmg`, on Windows the NSIS `.exe`, on Ubuntu the `.deb`.

That last stage is worth knowing about on macOS. Building a `.dmg` mounts the disk image and drives Finder to position the icons and set the window background, so Finder windows open and close on the desktop while it runs. That is the bundler working, not a fault — but during an ordinary edit-and-check loop, `build-app` gets you a launchable app without the interruption. The script names follow what this stage produces on a Mac; on Windows the same full build hands back the NSIS installer instead, so `build-dmg` there means the whole trail rather than a disk image.

**Which depth a change deserves.** While working, the two halves are quicker on their own: `pnpm vite-build` finishes in a fraction of a second and catches every frontend error, and `cargo check` from `src-tauri` is nearly as quick and catches every Rust error — including a mistyped capability or permission identifier, which Tauri validates as it compiles. `pnpm build-binary` is the gate before handing work over, because it is the cheapest single command that runs the frontend build and a real release compile; `cargo check` type-checks without generating or linking any code, so it cannot stand in for that. Go further only when the change earns it: `build-app` once you have touched the bundle section of tauri.conf.json or the icons, since bundling is what exercises the `Info.plist` and the icon pipeline, and the full build before a release or a handoff to the other platform. Two things to expect — the release profile shares nothing with the debug profile `pnpm local` uses, so the first release build after a stretch of dev work compiles everything over again; and a green build says the code compiles, not that it works. Only running the app tells you that.

**tauri.conf.json**: `beforeBuildCommand` points at `pnpm vite-build`; the window opens 1200×1050 with `dragDropEnabled` true; the identity values are productName `ftorrent` and identifier `com.ftorrent`.

**Bundle targets.** The scaffold ships `"targets": "all"`, which builds everything each platform can build — on Windows an MSI beside the NSIS installer, on Linux an AppImage beside the Debian package. ftorrent distributes four packages and no more, so the four are named instead:

```
"targets": ["app", "dmg", "nsis", "deb"]
```

One list serves all three platforms: a target that doesn't apply to the machine doing the build is skipped, so macOS produces the .app and .dmg, Windows the NSIS .exe, and Ubuntu the .deb. The portable zip is assembled separately, later.

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

**Indentation and line endings.** All source files indent with tabs, per the style guide at the repository root (the scaffold's space-indented files were converted). Line endings are LF in the repository and in every working tree on every platform, enforced by the .gitattributes at the repository root.

**Ignore rules.** The scaffold's own .gitignore files were dropped — the monorepo root .gitignore already covers everything Tauri generates: target/, gen/, dist, node_modules.

## Verified

We built and smoke-tested from a fresh clone on both active platforms in August 2026: on macOS, `pnpm build` produces the .app bundle and .dmg and `pnpm local` serves the dev window with hot module replacement; on Windows, the same commands produce the dev window and the installers, with both lockfiles byte-identical after install. That Windows pass ran before the bundle targets were narrowed, when the build still produced an MSI alongside the NSIS installer; the next Windows build should produce the NSIS installer alone. The frontend-to-Rust IPC round-trip works in both debug and release profiles on both platforms.
