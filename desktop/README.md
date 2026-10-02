
_ftorrent/desktop/README.md_

# The ftorrent Desktop Client

> Prepared by [Claude Code](https://claude.ai/code) using Opus 5.5
> <br>Created: 2026-Aug
> <br>Last reviewed: 2026-Oct
> <br>[Tauri](https://tauri.app/): 2.11
> <br>[Vue](https://vuejs.org/): 3.5
> <br>[Vue Router](https://router.vuejs.org/): 5.2
> <br>[Pinia](https://pinia.vuejs.org/): 4.0
> <br>[Vite](https://vite.dev/): 8
> <br>[Tailwind CSS](https://tailwindcss.com/): 4
> <br>[pnpm](https://pnpm.io/): 10.28
> <br>Node: 22
> <br>[Rust](https://www.rust-lang.org/): 1.98
> <br>[libtorrent](https://www.libtorrent.org/): 2.1
> <br>[Python](https://www.python.org/): 3.13
> <br>[uv](https://docs.astral.sh/uv/): 0.12
> <br>[dmgbuild](https://github.com/dmgbuild/dmgbuild): 1.6
> <br>[PyInstaller](https://pyinstaller.org/): 6.22

This workspace holds the ftorrent desktop client — a cross-platform BitTorrent and WebTorrent client for Windows, Mac, and Linux, built as a [Tauri](https://tauri.app/) app in three parts: a Vue page, a Rust core, and an engine, a small Python program holding [libtorrent](https://www.libtorrent.org/) that runs as a process beside the app. The design lives on [docs.ftorrent.com](https://docs.ftorrent.com/). This guide is the practical half: how to build it, how it was scaffolded and where it departs from the scaffold, and a map of where the code is. Each part of the code carries its own story as an essay at the top of its file, and this guide points to those rather than repeating them; what it explains itself is the configuration, which can't carry an essay of its own, and the few things that span many files.

## Before you start

- **Node 22** or newer, with corepack enabled, so `pnpm` is the version the monorepo root's package.json names in its `packageManager` field, 10.28.2, on every machine.
- **Rust**, through rustup. `src-tauri/rust-toolchain.toml` pins 1.98.0, and cargo installs and uses it on the first build, so the Mac, the Windows box, and the Linux containers compile with the same compiler. Raising it is a deliberate edit that lands in a diff.
- **[uv](https://docs.astral.sh/uv/)**, which installs the engine's pinned Python itself, so nothing depends on whatever Python the machine already has.
- Tauri's own [prerequisites](https://tauri.app/start/prerequisites/) for the platform.

Then `pnpm install` at the repository root, and `pnpm engine` once before any Rust build. The engine's freeze is a prerequisite, not an option: tauri-build copies resources at compile time and refuses a resource path that matches nothing, so until the freeze exists even `cargo check` stops with `glob pattern ../engine/dist/ftorrent-engine path not found`. That message is the build asking for `pnpm engine`. Run it again whenever `engine.py` or the engine's lockfile changes.

## The commands

The commands in package.json are named so each says where it stops:

```
pnpm local        run the app here, in development mode with hot reload
pnpm engine       freeze the engine, described below
pnpm compile      build the release binary, and stop there
pnpm installer    build the installer, all the way through the app to the dmg on a Mac or the exe on Windows
pnpm reveal       open the file manager on that installer, to install it as a person would
pnpm hash         stage what is built under its published name and write its sidecar, building nothing
pnpm upload       check what is staged against its sidecar, then send the package and its sidecar to ftorrent.com
pnpm icons        regenerate every platform's icons from the SVG sources
```

`compile` never makes an installer, `installer` never hashes, `hash` never builds, and `upload` never builds. A name means the same thing on every machine while doing different work underneath: `installer` makes a `.dmg` on macOS and an `.exe` on Windows, and `hash` and `upload` act on whichever this machine can build. The Linux packages use the same vocabulary in the workspace nested at `linux/`, which has a guide of its own. Everything past calling tauri lives in `scripts.js`, reached by a verb, except the Mac's disk image, described below.

**Which command a change deserves.** While working, the two halves are quicker on their own: `pnpm vite-build` finishes in a fraction of a second and catches every frontend error, and `cargo check` from `src-tauri` is nearly as quick and catches every Rust error, including a mistyped capability or permission identifier, which Tauri validates as it compiles. `pnpm compile` is the gate before handing work over, because it is the cheapest single command that runs the frontend build and a real release compile and link; `cargo check` type-checks without generating or linking any code, so it cannot stand in for that. `pnpm installer` when something has to be installed, and before a release or a handoff to another platform. The release profile shares nothing with the debug profile `pnpm local` uses — they compile into target/release and target/debug — so the first release build after a stretch of dev work compiles everything over again. And a green build says the code compiles, not that it works. Only running the app tells you that.

Two things about `local`. A pnpm script cannot be named `run`, since pnpm's builtin shadows it. And `local` fails with a stack trace that reads like a bug if a dev server is already running: vite.config.js sets `port: 1420` with `strictPort: true`, because Tauri needs to find the frontend at a fixed address, so the second one exits rather than sliding to another port. `lsof -ti :1420` on macOS, or `netstat -ano | findstr :1420` on Windows, names the process already holding it.

## Starting from the scaffold

The workspace began as create-tauri-app's Vue template in plain JavaScript, and the command is the first step for anyone starting an app like this one on the same framework:

```
pnpm create tauri-app ftorrent --manager pnpm --template vue --identifier com.ftorrent --yes
```

Where the workspace already exists, as this one did as a placeholder in the monorepo, scaffold into a scratch folder and merge the output in, keeping the placeholder package.json's name, description, and homepage. And expect the npm-published create-tauri-app to lag Tauri's own templates and pin older versions — here it wrote Vite 6 — so accept what it produces and align the versions straight after, which the section on versions below covers.

The next section lists everything that now differs from what that command writes, file by file. Scaffolding fresh on a later Tauri and comparing the two is how to tell which of these choices a new version has made obsolete.

## What differs from the scaffold

**tauri.conf.json** carries the identity card, the values every platform's package is stamped with. The product name is `ftorrent`, lowercase everywhere, which is the brand. The identifier is `com.ftorrent.ftorrent`, the domain reversed and the app's name, three components as macOS bundle identifiers and Flatpak expect, where the scaffold command wrote the two-component `com.ftorrent`. It was changed before any release, and must never change after one, because the identifier names the app data directory on every platform, the macOS bundle identifier that Launch Services and the privacy prompts key on, and the Windows uninstall entry; changing it later would leave orphans of all three on every user's machine. Beside those: publisher `ftorrent`, the homepage, `Copyright © 2026 ftorrent`, license `GPL-3.0-only`, the one-line description and its longer form, category `Utility`, and a macOS minimum system version of 15.0, because the libtorrent wheel the engine is frozen against is built for macOS 15 and an installer that accepted macOS 14 would deliver an engine that cannot load. `beforeBuildCommand` points at `pnpm vite-build`. The `windows` list, which in the scaffold declares one window, is empty on purpose: the window is made in Rust, from `window.rs`, once startup knows whether this launch is the running copy, and made hidden; the page places it and shows it.

The bundle targets. The scaffold ships `"targets": "all"`, which builds everything each platform can build — on Windows an MSI beside the NSIS installer, on Linux an AppImage beside the Debian package. ftorrent distributes six packages, and the three Tauri targets a Mac, a Windows box, or a Linux checkout use are named instead:

```
"targets": ["app", "nsis", "deb"]
```

One list serves all three platforms: a target that doesn't apply to the machine doing the build is skipped, so macOS produces the .app, Windows the NSIS .exe, and a Linux checkout the .deb. The skipping is silent — a Windows build simply makes the one installer, with no notice that two of the three names didn't apply. The .dmg is absent on purpose, as the next paragraph says. The other two Linux packages never appear in this list: the pipeline in `linux/` asks its own containers for `deb,rpm` as an argument to `tauri build`, and wraps the Flatpak from the x86-64 `.deb` rather than through Tauri at all. Beside the targets, `bundle.linux` points the deb and rpm bundlers at `ftorrent.desktop.hbs`, a desktop-entry template whose one departure from Tauri's default is its categories: `Network;FileTransfer;P2P;` is what qBittorrent and Transmission declare, so launchers and app menus file ftorrent beside them rather than under Utility. The portable zip isn't a Tauri target either; it's assembled separately, from the same builds.

The macOS disk image. On a Mac, `pnpm installer` runs `dmg.js` after `tauri build`, which makes the dmg from the .app with [dmgbuild](https://github.com/dmgbuild/dmgbuild) through `uvx`, the same uv that freezes the engine. The essay atop `dmg.js` says why, and where each number in the layout comes from.

The Windows installer. `bundle.windows.nsis` sets the installer's mode, its icons, and a hook:

```
"windows": {"nsis": {"installMode": "currentUser", "installerIcon": "icons/icon.ico", "uninstallerIcon": "icons/icon.ico", "installerHooks": "windows/hooks.nsh"}}
```

`currentUser` installs into the user's own directory — no UAC elevation, and no wizard page asking the user to choose between a per-user and a machine-wide install, which is what the plan calls for on Windows, where the whole installation is per-user. Tauri 2.11 already defaults to this mode, so the line changes no behavior; it's here because the plan depends on the mode and a default is not a promise. The installer keeps NSIS's stock look, with no header or sidebar images. `installerHooks` names `src-tauri/windows/hooks.nsh`, which Tauri's installer script includes, and which takes back on uninstall what the running app registered with Windows; its own comments say how.

The resources. `bundle.resources` lands beside the executable the engine's frozen folder, the Windows 10 Start tile's manifest and images, and the `.torrent` document icon. The engine's entry is in the directory form, `{"../engine/dist/ftorrent-engine": "ftorrent-engine"}`, and the form matters: the glob form, `.../**/*`, flattens every file into one directory and loses the `_internal` folder the freeze depends on. There is no `bundle.fileAssociations`, on purpose: on Windows it adds the NSIS macro that seizes each type's default at install, and the essay in `src/associate.js` says what ftorrent does instead.

**Content Security Policy.** The scaffold's `"csp": null` is a real policy here:

```
"csp": "default-src 'self'; connect-src 'self' ipc: http://ipc.localhost; img-src 'self' data: blob:"
```

The webview loads only the bundled frontend and never navigates anywhere, and all networking lives in the Rust core — so the CSP is the second wall behind Vue's template escaping: if hostile text from torrent metadata or filenames ever managed to become markup, this policy stops it from executing or exfiltrating. Piece by piece: `default-src 'self'` allows only bundled assets (Tauri auto-nonces its own injected init scripts once csp is non-null); `connect-src` lists Tauri's two IPC endpoints — `ipc:` on Mac, `http://ipc.localhost` on Windows — so `invoke()` keeps working; `img-src data: blob:` covers rendering images from local bytes. Nothing gets added to this policy without a reason written next to it. `devCsp` stays unset on purpose: dev mode is unrestricted so Vite HMR works, and the policy guards what ships.

To verify the CSP, build and check the binary — Tauri injects the policy into the HTML embedded there, so the on-disk dist/index.html won't show it:

```
strings src-tauri/target/release/ftorrent | grep default-src
```

Failure modes are visibly loud: a blocked IPC endpoint means the UI can't reach Rust at all, and a blocked img-src means images don't render. If the built app behaves normally, the policy fits.

**Plugins and capabilities.** Tauri v2 gates what the webview may call. A plugin adds commands to the Rust core, and a capability file names which of those commands the window is allowed to invoke — a plugin registered but not permitted is unreachable from the page. The scaffold grants `opener:default`, a blanket set that includes opening arbitrary URLs. `src-tauri/capabilities/default.json` names specific permissions instead:

```json
"permissions": [
	"core:default",
	"core:window:allow-show",
	"core:window:allow-set-position",
	"core:window:allow-set-size",
	"core:window:allow-maximize",
	"core:window:allow-set-theme",
	"opener:allow-reveal-item-in-dir",
	{
		"identifier": "opener:allow-open-url",
		"allow": [{"url": "ms-settings:defaultapps*"}, {"url": "https://ftorrent.com/*"}]
	},
	"dialog:allow-open",
	"dialog:allow-save"
]
```

The five `core:window` grants beyond `core:default` are the page's handling of its own window: placing it where the settings remember, maximizing it if it was, giving it its light or dark theme, and showing it. Two plugins are registered in `src-tauri/src/lib.rs`: **dialog**, for the familiar operating-system open and save boxes — picking a `.torrent` file, choosing a download folder — and **opener**, for revealing a finished download in Finder or File Explorer, for opening one page of Windows Settings, Default apps, where a user confirms ftorrent as the app that opens torrents, and for opening the project's homepage in the system's browser from the About page.

What is left out matters as much as what is in. Dialog's message, ask, and confirm boxes are excluded deliberately: a native-looking dialog an application can raise with arbitrary text is a social-engineering primitive, and ftorrent's own interface lives in the page where the user can see it for what it is. Opener's URL opening is scoped to the two places the page links to: Windows Settings' Default apps, through the pattern `ms-settings:defaultapps*`, and the homepage, through `https://ftorrent.com/*`. A link to anywhere else is refused. The homepage is written here a second time, beside `bundle.homepage` in tauri.conf.json where the page reads it, because a capability file can't read the configuration, so a fork with its own homepage changes both. The rule behind both: register only the plugins the app uses, and grant named permissions rather than a plugin's `:default` set, with a scope wherever the permission would otherwise reach further than the feature.

`core:default` stays as the scaffold shipped it, and that is the deliberate exception. It is Tauri's own set — path, event, window, webview, app, image, resources, menu, and tray — the machinery that makes a Tauri app an app at all. Enumerating it by hand would mean dozens of identifiers to arrive at nearly the same place; the granular rule is aimed at plugins, which extend the app's reach past the framework itself. Tauri validates permission identifiers when it compiles, so a typo here fails the build rather than failing silently at runtime, and `cargo check` from `src-tauri` is the verification step.

**Cargo.toml** carries the same description as tauri.conf.json and an authors line that becomes the `.deb`'s Maintainer field, the project rather than a person, since it is public in every installed copy. Tauri's `tray-icon` feature is on for the Windows tray, and a few crates are named for Windows only and a few for macOS only, each with a comment on the line saying which module needs it and that Tauri already brings it at that version. `rust-toolchain.toml` beside it is ours, not the scaffold's.

**package.json** replaces the scaffold's `build` and `preview` scripts with the commands above; `dev` stays as the script `beforeDevCommand` runs, and `vite-build` is the one `beforeBuildCommand` runs.

**The frontend.** Tailwind 4 arrives through its Vite plugin only: `@tailwindcss/vite` in vite.config.js and `@import "tailwindcss"` in `src/style.css`, the one stylesheet, whose essay sets out how the page is styled. There is no postcss.config.js, no tailwind.config.js, and no autoprefixer — Tailwind 4's Vite plugin handles prefixing and configuration natively, and a tutorial telling you to create those files is describing Tailwind 3. vite.config.js also stops watching `engine/` and `linux/`, whose builds write thousands of files no page imports. Vue Router and Pinia are added, each described below, and the scaffold's demonstration page, its logos and its greet form and the Rust command behind it, is gone.

**Across the repository.** All source files indent with tabs, per the style guide at the repository root, where the scaffold's were indented with spaces. Line endings are LF in the repository and in every working tree on every platform, enforced by the .gitattributes at the repository root. The scaffold's own .gitignore files are gone: the monorepo root .gitignore already covers everything Tauri generates, target/, gen/, dist, and node_modules.

## How the code is arranged

### The Rust core

The Rust core follows one rule, which the essay at the top of `src-tauri/src/lib.rs` states: Rust stays general. Its commands read like a general-purpose API that any desktop application could use, and the page holds all of ftorrent's own logic. The handler list in `lib.rs` is the index of every command the page can call, and each module's essay is the long version of what it does:

- `paths.rs`: where the program, the user's home, and the data folder are, worked out once at startup, and what makes a copy portable rather than installed.
- `instance.rs`: one running copy per copy of the app, held by a file lock, and how a second launch hands over what it carried and leaves.
- `lifecycle.rs`: closing hides the window and quitting is explicit; the Windows tray and menu bar, the Mac's Dock and its fullscreen Space, and why the window takes focus only when the user asks for it.
- `window.rs`: the one window, made hidden from Tauri's `Ready` event rather than declared in the config, and the browser habits turned off in a release build.
- `disk.rs`, `locks.rs`, `registry.rs`, and `launch.rs`: the disk, exclusive file locks, the Windows registry, and the Mac's Launch Services, each as general commands that hold no guard.
- `engine.rs`: the engine's process, below.
- `log.rs`, `desktop.rs`, and `queue.rs`: the log, text the page hands down to be written as the app exits, and the drained queue that lines wait in until the page takes them.

**Disk access.** `disk.rs` holds the app's own filesystem commands, each one a single operation with POSIX semantics: `disk_readdir`, `disk_stat`, `disk_read`, `disk_write`, `disk_mkdir`, `disk_copy`, and `disk_hide`, which sets a file's hidden attribute on Windows and does nothing elsewhere, where a name that starts with a dot is already hidden. `src/disk.js` wraps each one in an `invoke()` call for the frontend. They are registered in the handler list in `lib.rs` and need no capability entry: the capability system governs plugin commands, and these are ours. Tauri does ship a filesystem plugin, and we deliberately don't use it. Its scoping confines an app to directories named ahead of time, which is the right default for software that has no business reading a whole disk — but a torrent client reads and writes wherever the user tells it to, so that scoping would be a wall to climb rather than a protection. The commands take any path and hold no guard, by the rule above. `disk_rename`, `disk_unlink`, and `disk_rmdir` are sketched at the bottom of the file, to be brought up when a feature needs them.

**Download folders and their locks.** Each download folder keeps a hidden `.ftorrent` folder for the session data of the torrents in it, and while ftorrent uses the folder it holds an exclusive lock on `.ftorrent/ftorrent.lock`, so two copies, such as an installed one and a portable one, never load the same torrents. The locks are two general commands in `locks.rs`, `lock_take` and `lock_release`, which lock any file; the page decides which folders to lock and when, makes `.ftorrent`, and hides it on Windows. One folder listed under two spellings is held once. Startup locks only folders that already exist and never makes one. Until there are torrents, a **Prepare download folder** button on the main page stands in for starting one: it makes the default folder if it isn't there, and locks it.

### The engine

The client's BitTorrent and WebTorrent work happens in a second process rather than in the app's own: a small Python program that holds libtorrent, frozen together with its interpreter and libtorrent into a folder the app carries beside itself. Two documents on [docs.ftorrent.com](https://docs.ftorrent.com/) carry the reasoning: the desktop architecture document explains the processes, the files, and the pipes between them, and the libtorrent provenance document records where libtorrent comes from and the hashes we pin. The essay atop `src-tauri/src/engine.rs` is the app's side, the one road down and one road up that lets the page use a new libtorrent call without new Rust; the comments atop `engine/engine.py` are the engine's side, where JSON meets libtorrent in one dispatch table; and `engine/ftorrent-engine.spec` says why the freeze is a folder rather than a single file. No shell plugin is registered, on purpose: the only code in the app that can start a process is `engine.rs`.

`engine/` holds the program, `engine.py`; its manifest, `pyproject.toml`; the lockfile, `uv.lock`, which pins every platform's wheel by hash; `.python-version`, which pins the interpreter; and the PyInstaller recipe, `ftorrent-engine.spec`. One script does the rest:

```
pnpm engine    # uv sync --frozen, then PyInstaller: the result is engine/dist/ftorrent-engine/
```

Tauri copies that folder into the bundle and, during development, next to the debug binary. The freeze comes to about 45 MB on macOS and 33 MB on Windows, and it's most of the installed app.

### The page

`src/main.js` wires the page together and starts it, and `src/router/index.js` names every page the window can show, meant to be read as the app's table of contents. The rest, each with an essay at the top of its file:

- `src/window.js`: where the window opens, remembering where the user left it and opening it there, and what a fresh place is.
- `src/settings.js` and `src/stores/settings.js`: the settings, defined once as a schema and kept in `ftorrent.toml`, and the two moments that write the file.
- `src/associate.js` and `src/stores/associations.js`: whether ftorrent opens `.torrent` and `.ftorrent` files and `magnet:` and `ftorrent:` links, the policy and the flow around it, with `src-tauri/macos/Info.plist` and `src-tauri/windows/hooks.nsh` as the two platforms' halves.
- `src/stores/incoming.js`: what comes up from the Rust core several times a second, the engine's lines and the requests that reach this copy.
- `src/style.css`: the one stylesheet, and how the page is styled.
- `src/log.js`: the page's half of the log, off at the factory.

**Vue Router, in hash mode.** The frontend routes with [Vue Router](https://router.vuejs.org/) 5, configured with `createWebHashHistory()`. A router's other mode, history mode, writes real paths like `/about` and expects a server to answer a request for that path when the page reloads — but a Tauri window loads its frontend out of the bundle with no server behind it, so such a reload would find nothing. Hash mode keeps the whole route after a `#`, the part a browser resolves locally and never requests. The user never sees it: the window has no address bar.

`src/App.vue` is the shell — the banner that asks about being the default torrent app, when it's up, the navigation, and the `<router-view />` outlet the current page fills. `src/pages/MainPage.vue` is the page the window opens on; `src/pages/SettingsPage.vue` and `src/pages/AboutPage.vue` are lazy routes, an `() => import(…)` in place of an imported component, so the build gives each a chunk of its own that the app fetches the first time someone opens it, and those chunks are visible in the `vite build` output. Vue Router's own documentation says `views/HomeView.vue` where this says `pages/MainPage.vue`: the rest of this monorepo already calls them pages — the website workspace runs on Nuxt, where `pages/` is the framework's own directory — so one word across the repository beat matching the router's examples. It reads better out loud, too, in a project where every component file already ends in `.vue`.

This is the router's 5.x line rather than the 4.x line that Vue 3 shipped alongside for years: 5.x is current, its peer requirements, Vue 3.5 and Vite 8, match what this workspace runs, and starting on the previous major would mean a migration later for nothing gained now. It installs a set of build-time dependencies that 4.x did not, because the file-based and typed-routes tooling that used to be a separate plugin now lives in the package; nothing in that tooling is imported here and none of it reaches the shipped bundle, where the router costs about 9 kB gzipped.

**Pinia, for state that outlives a view.** The frontend keeps its state in [Pinia](https://pinia.vuejs.org/) 4, Vue's official store, here from the start for the reason the router is: a shape is cheap to settle before anything is built on it. The rule it answers is narrow — state that outlives the component displaying it, or that more than one component reads, belongs in a store; state that belongs to one view and dies with it does not. A torrent client has a great deal of the first kind, and it arrives from more than one direction: the engine pushes a stream of updates whether or not the view showing it is mounted, and whether ftorrent opens `.torrent` files, whether a firewall exemption is in place, whether the router accepted a port mapping are answered somewhere else entirely. A component the router unmounts on navigation is the wrong place to keep any of them. The store is where they meet: it fills itself by calling down through `invoke()` and by listening for events, and every component reads from it rather than from the source, so a fact crosses the boundary once when it changes rather than once for every view that shows it.

Pinia was written for the web, and several of its standard practices answer problems this app doesn't have. A `createPinia()` per request, with state serialized into the page and rehydrated on the client, exists for server-side rendering; this process opens one window once, so `createPinia()` is called a single time in `src/main.js`. Listeners belong to the store rather than to a component: the engine's event stream has to outlive every component, since the window can be hidden for days with nothing rendered while transfers continue, so the subscription starts once and is held. The session is long: a browser tab lives for minutes, and this process runs for weeks, so anything a store appends to, an alert list or an event log, needs a bound written in from the start, because it fails slowly and no short test will show it. And browser storage is not where anything persists: here there is a filesystem behind the Rust core, and the webview's own storage sits in a per-user directory on the host machine, which the design keeps clear so that a copy running from a USB stick leaves nothing behind. So neither `pinia-plugin-persistedstate` nor `@tauri-apps/plugin-store` — both are the right answer to a constraint we don't have, and the wrong answer to the one we do.

Three stores, each in the setup form, `defineStore` with a function that returns what the store holds, which reads like the `<script setup>` components around it: `src/stores/settings.js`, `src/stores/incoming.js`, and `src/stores/associations.js`, named in the list above.

Two alternatives are worth knowing. Wrapping the router outlet in `<KeepAlive>` would also keep a page's state alive across a trip to another page and back, and it's the common reflex, but it preserves the component rather than moving the state out, so a second component wanting the same fact still can't reach it. The honest competitor is a module-scope ref: an ES module is a singleton, so a ref exported from a plain file already outlives every component and is importable anywhere, a store in all but name. What Pinia adds over that is narrower than its reputation — `acceptHMRUpdate`, so editing a store during development doesn't wipe what it holds; `$patch` and `$subscribe`; the devtools timeline, which isn't set up in the Tauri window and is no part of the case for a store here; and a convention any Vue developer recognizes on sight, which in a workspace meant to be read and forked is most of the weight. We would rather spend a small dependency on a shape a reader already knows than grow a private one.

Pinia 4.0.3 declares a runtime dependency on `nostics`, a small, dependency-free structured-diagnostics library published under `vercel-labs`, and a non-optional peer dependency on `@vue/devtools-api`. Vue Router 5's build-time tooling already brings both, so Pinia adds no other package to the tree; and Vue Router declares Pinia as an optional peer for its data loaders, which is why its resolution key in the lockfile names Pinia.

**The product's name, written once.** `productName` in tauri.conf.json is the only place the code spells ftorrent. The Rust core reads it through its package info for the window title, the tray, and the names of the settings file, the lock file, the engine's folder, and the pipe; the page imports the same file at build time through `src/brand.js`; the uninstall hook reads it as the installer's product name; and the engine receives it in `init` for the client name it gives peers and trackers. A fork renames the app there, and edits its own identifier and crate name beside it.

## Packages and releases

**Icons.** Every icon starts as one drawing in `icon-studio/`, whose README is the map of where each icon file comes from and which build reads it: the application icon through `pnpm icons` and Tauri's generator, and the `.torrent` document icon and the Windows tray icon through the studio's own scripts. Run `pnpm icons` after changing the artwork, and also after upgrading the Tauri CLI, since the generated files are committed artifacts frozen at the CLI that made them.

**Publishing.** `scripts.js` carries the essays: where the published names come from, what `hash` writes beside each package and where, how `upload` sends them to ftorrent.com, what `upload.hide.env` holds for a machine that uploads, and why the Mac build is signed the way it is and what a downloader meets on each platform.

**On Windows.** The NSIS installer is about 12 MB, and installs without a UAC prompt or a page asking whether to install for one user or the whole machine, into `%LOCALAPPDATA%\ftorrent`, about 42 MB, with its uninstall entry under `HKCU` and nothing in `HKLM` or Program Files. The uninstaller removes that folder with the engine inside it, the uninstall entry, the Start menu shortcut, and through the hook everything the app registered for file types and links. A choice the user saved for ftorrent in Windows' own settings stays, since only those screens change it, and applies again when ftorrent is installed again, so upgrading with a newer installer, which uninstalls first by default, keeps it. It leaves two things unless the user ticks its delete-application-data box, which is unticked by default: `HKCU\Software\ftorrent\ftorrent`, holding the install path and the installer's language, and the data folder `%LOCALAPPDATA%\com.ftorrent.ftorrent`, with the settings, the lock file, and WebView2's profile. Downloads are never touched. The installer isn't signed yet, so a downloader meets SmartScreen, as the signing essay in `scripts.js` describes. SmartScreen inspects only a file carrying the mark of the web, the `Zone.Identifier` stream a browser writes on download, so an installer built on the machine running it never shows the screen; to see what a downloader sees, give a local copy that stream by hand.

**On Linux,** the packages come from containers, from a Mac or a Linux machine, and `linux/README.md` is the guide.

## Versions and lockfiles

- **Tauri** — npm packages `@tauri-apps/cli` and `@tauri-apps/api` on 2.11, and Rust crates resolving to tauri 2.11. The Tauri CLI requires the npm packages and Rust crates to be on the same major and minor, so bump both sides together. On the Rust side, use a full `cargo update`, not targeted `-p` updates — the targeted form can leave an incoherent dependency tree that fails to compile.
- **Vue 3.5**, with **@vitejs/plugin-vue 6** and **Vite 8**. Vite 8 requires Node 20.19+ or 22.12+.
- **pnpm**, pinned once in the monorepo root package.json's `packageManager` field and enforced everywhere by corepack.
- **Rust**, pinned by `src-tauri/rust-toolchain.toml`, with the crate on **edition 2021**, as Tauri's template ships it. An edition is a language dialect, not a compiler version: Rust ships a new compiler every six weeks and never breaks working code, so the rare change that would break something arrives instead as an edition a crate opts into. There are four — 2015, 2018, 2021, and 2024 — and a single compiler builds all of them, so a crate on one edition links fine against dependencies on another.

The lockfiles are part of the design: one pnpm-lock.yaml at the monorepo root and this workspace's src-tauri/Cargo.lock are both committed and both cross-platform — pnpm records every platform's native binaries and selects at install time. A fresh clone on a different operating system should install and build without changing either file; if one changes, that's a finding to investigate, and deleting or regenerating a lockfile is never the fix.
