---
title: Desktop Client Planning
description: Design and planning notes for the ftorrent desktop client — stack, paths, lifecycle, and distribution.
---

# ftorrent Desktop Client

ftorrent is a desktop BitTorrent client for Windows, Mac, and Linux. The source lives at [github.com/zootella/ftorrent](https://github.com/zootella/ftorrent) in the `desktop` workspace — the project is "ftorrent," always lowercase, and the workspace is "desktop." The app participates in both traditional BitTorrent swarms (TCP/uTP) and browser-based WebTorrent swarms (WebRTC) through a single hybrid engine.

The Rust filesystem module is ported from a separate Tauri app, Fuji, where it lives as [`io.rs`](https://github.com/zootella/fuji/blob/main/src-tauri/src/io.rs). In ftorrent it will be renamed `disk.rs` to avoid ambiguity with network I/O.

## Epics

```
notes

write three, in the style and for the 'roadmap in reverse' in docs

first introduces a next generation desktop bittorrent and webtorrent client for windows, mac, and linux, picking tauri as the framework and libtorrent as the engine. talk about values: performance, transparency, usable undersatndable traditional feature set, that then as a platform for next generation expansion

second is portable ftorrent, talk about that, novel in first cross platform portable

and a third upcoming epic for the roadmap is dht.ftorrent.com
```


## User Stories

### Scaffolding

**Scaffold.** Scaffold a new Tauri v2 app with a Vue frontend in the monorepo. `tauri dev` opens a window rendering a Vue component, and `tauri build` produces a macOS `.dmg` and a Windows `.exe`. macOS and Windows are the active test matrix; Linux is expected to work but not regularly verified.

**Router and dependencies.** Add Vue Router to `package.json` and configure it for hash mode, which is the correct choice for Tauri since there's no server to handle `pushState` URLs. The app has at least two navigable views — main and about. Add an additional npm dependency (e.g., `dayjs`) and display its output in the about view as proof that the frontend dependency pipeline works end to end.

**Filesystem commands.** Port Fuji's `io.rs` into the Tauri command layer as `disk.rs` (renamed to avoid ambiguity with network I/O), bringing all commands from the start: `disk_readdir`, `disk_stat`, `disk_read`, `disk_copy`, `disk_write`, `disk_rename`, `disk_unlink`, `disk_rmdir`, `disk_mkdir`. All are callable from Vue through `invoke()` with explicit permission grants in Tauri v2's capability configuration. The main view surfaces a directory listing — the user pastes a path, the app lists contents below using `disk_readdir` and `disk_stat` over IPC — as proof the full pipeline works. The remaining commands are ported and available but not individually surfaced in the UI at this stage. Smoke test: paste a directory path and confirm its contents appear; confirm all commands are registered and invocable from the frontend console.

**Build libtorrent (macOS).** Build libtorrent from the master branch with `webtorrent=on` and its Python bindings on macOS. The build steps live in a script that can be run from scratch on a clean Mac. Validate by running a Python script that imports libtorrent, prints the version, and confirms the WebTorrent session flag is present and configurable.

**Build libtorrent (Windows).** Same outcome as the macOS build, but on Windows. This is a separate story because the compiler toolchain, Boost configuration, and OpenSSL setup are substantially different. The build script is independent and runs from scratch on a clean Windows machine.

**Sidecar.** Bundle the Python libtorrent wrapper with PyInstaller into a standalone binary. The libtorrent shared library is built from source, not pip-installed, so PyInstaller will likely need a custom spec file or hook to locate and include the `.dylib`/`.dll`. Configure Tauri to launch the binary on app start via `externalBin` with the correct target-triple suffix. The app displays the sidecar's status on the main view. When the app closes, the sidecar is terminated — no orphan processes.

**IPC end to end.** Wire up NDJSON communication between the Tauri Rust core and the Python sidecar over stdin/stdout. The Rust core's first message to the sidecar on startup is an `init` command containing all paths the sidecar needs to operate: the state file location, the list of download folders from `ftorrent.json`, and any session configuration. The sidecar loads session state, scans `.ftorrent` directories in each reachable download folder, loads resume data, starts the libtorrent session, and emits a `ready` event on stdout. From there, the Rust core writes JSON commands on stdin and the sidecar emits NDJSON events on stdout (progress, peer updates, alerts). Vue invokes a Tauri command, the Rust core forwards it to the sidecar, and the result comes back to the UI. The main view displays a live libtorrent session value — like DHT node count or listen port — proving the full path works. Malformed JSON from either side is handled without crashing. Smoke test: launch the app, confirm the sidecar receives init, emits ready, and the UI shows a live session value; send a malformed JSON line and confirm no crash.

**Icons and graphics.** Produce all icon and graphic assets per the design document and wire them into the build. Two app icon sources: full-bleed for Windows/Linux and Apple HIG-padded for macOS, with the build swapping only `icon.icns`. Tray icons provided directly: monochrome template for macOS, small color for Windows/Linux, with the Rust core overriding to template mode at runtime on macOS. DMG background with drag-arrow cue. Windows 10 `VisualElementsManifest.xml` and tile PNGs placed by the NSIS installer. Website favicon in the website repo. Smoke test: confirm macOS Dock icon matches neighboring app sizing; confirm tray icon is legible in both light and dark menu bar; confirm DMG opens with background and correctly positioned icons; confirm Windows 10 shows the branded Start tile.

### Distribution

**Installers.** Configure Tauri to produce three packages: NSIS `.exe` for Windows, `.dmg` for macOS (Apple Silicon only), and `.deb` for Ubuntu Desktop and derivatives. Disable Tauri's default `.AppImage` output — we ship `.deb` only for now. NSIS runs completely silently with no wizard UI, set to `currentUser` mode so no UAC prompt appears. The DMG uses the background image and icon positions from the icons story. Build artifacts are renamed from Tauri's versioned filenames to `ftorrent.exe`, `ftorrent.dmg`, and `ftorrent.deb` for website distribution. Smoke test: confirm NSIS installs with no visible UI and the app launches from `AppData\Local\ftorrent\`; confirm the DMG opens with the drag-to-Applications layout; confirm the `.deb` installs and launches on Ubuntu.

**Portable mode.** Implement portable detection and path resolution per §4. On startup, look for `portable/ftorrent.json` alongside the executable before checking the platform data directory. If found, all state — settings, lock, session state, crash log — stays under `portable/`; the platform data directory is never touched. `./` paths resolve relative to the executable on Windows and Linux, and relative to the `.app` bundle on macOS. The build produces `ftorrent.zip` with binaries for all three platforms and `portable/ftorrent.json` defaulting downloads to `./downloads`. `start_at_login` is grayed out in portable mode. Smoke test: launch from a USB stick on both macOS and Windows, confirm all state stays on the stick and nothing is written to the host; add a torrent, move the stick to the other platform, confirm it resumes; confirm installed and portable run side by side without lock collision.

**Automatic update.** Implement Tauri's built-in updater with Ed25519 signing. The build generates a keypair once; the public key ships in `tauri.conf.json`. Each release publishes a static JSON manifest per platform at `ftorrent.com/update/{target-triple}`. The app checks on launch and every 24 hours, failing silently if offline. When a newer version is verified, a non-modal text indicator appears at the bottom of the window — no popup, no interruption. On click, confirm, close, apply silently, relaunch. On Linux and in portable mode, the indicator opens `ftorrent.com` in the browser instead. An `update_check` setting in `ftorrent.json` defaults to `true`; when `false`, no requests are made and no indicator appears. Smoke test: host a manifest locally with two versions, confirm detection, indicator, and successful apply-and-relaunch; confirm a bad signature is rejected; confirm `update_check: false` suppresses everything.

### Lifecycle

**Start at login.** When the user enables `start_at_login` in settings, ftorrent calls `tauri-plugin-autostart`'s `enable()` to register for launch at OS login. When the user disables it, ftorrent calls `disable()`. The UI reflects the current state via `isEnabled()`. In portable mode, the setting is grayed out and the plugin is not initialized. On macOS, the first enable may trigger a system notification about background items — this is expected and documented in the settings UI.

**Instance lock.** Enforce single-instance per settings file. After resolving which `ftorrent.json` to use, claim an OS lock: on Windows, a named mutex keyed on a SHA-256 hash of the absolute path; on macOS and Linux, `flock()` on a `ftorrent.lock` file created empty alongside `ftorrent.json` (a separate file because `ftorrent.json` itself gets atomically replaced on save, which would break the lock's inode). If the lock is already held, signal the running instance to focus its window and exit — on Windows via window class lookup, on macOS via `NSRunningApplication`. This replaces Tauri's single-instance plugin, which keys on bundle ID and would incorrectly block installed-plus-portable from running side by side. Smoke test: launch ftorrent, launch it again from the same shortcut, confirm the existing window comes forward and no second instance appears; then launch a portable copy alongside the installed copy and confirm both run independently; then test two OS users on the same machine and confirm no collision.

**Sleep detection and prevention.** A timer in the Rust core writes a timestamp every 60 seconds. If the next tick sees a gap longer than two minutes, the system slept — the Rust core tells the sidecar to re-announce to trackers. An optional `prevent_sleep` setting, off by default, holds a platform power assertion while ftorrent is running to prevent idle sleep. macOS and Windows only.

**Periodic saves.** The sidecar periodically writes resume data while running, not only at shutdown. When libtorrent fires a `save_resume_data_alert` — triggered by state changes like piece completion, tracker replies, or peer list updates — the sidecar serializes the resume blob to the torrent's `resume` file in its `.ftorrent` directory. This ensures crash recovery has something recent to fall back on. Without it, a crash means full re-check of every torrent from launch. Smoke test: start a download, let it make progress, kill the sidecar with `SIGKILL` (or `taskkill /F` on Windows), relaunch, confirm the torrent resumes near where it was rather than re-checking from zero.

**Sidecar crash and recovery.** The Rust core captures the sidecar's stderr to a 100-line ring buffer in memory. During normal operation, nothing is written to disk. If the sidecar exits with a non-zero code, the Rust core writes the buffer to `crash.log` in the application data directory (or `portable/`), overwriting any previous file. The UI surfaces a message indicating the engine has stopped — not a silent failure. The user can restart the sidecar from the UI without relaunching the app; on restart, the sidecar reloads session state and the most recent resume data written by periodic saves, and resumes normally. Normal shutdown produces no `crash.log`. Smoke test: trigger a deliberate sidecar crash via a debug command, confirm `crash.log` appears with the traceback and the UI shows the failure state; click restart in the UI and confirm the sidecar comes back and torrents resume; crash again and confirm `crash.log` is overwritten not appended; confirm clean exit produces no `crash.log`.

**Close and exit.** Clicking the window's close button hides the window — the webview stays alive, JS keeps running, the sidecar keeps transferring. The window is created once at startup and destroyed once at shutdown; hiding and showing is the only transition during normal use. On Windows, the tray icon indicates ftorrent is still running and clicking it shows the window. On macOS, the app remains in the dock. On Linux, the app remains in the taskbar. Quitting from the tray menu, dock menu, or `⌘Q` / keyboard shortcut triggers graceful shutdown. A torrent client that disappears when you close the window is broken.

**Graceful shutdown.** When the app exits — whether from user action, OS shutdown, or sidecar crash — ftorrent saves all state. Per-torrent resume data is written to each torrent's `resume` file inside the appropriate `.ftorrent` directory, as described in §3. Global libtorrent state — the DHT routing table and session-wide settings — is written to the `state` file in the application data directory (or `portable/state` in portable mode). Without this, every restart means re-checking pieces and rebuilding the DHT from scratch. If the sidecar crashes rather than exiting cleanly, the Rust core detects the lost process and surfaces the failure in the UI rather than silently doing nothing.

## (notes, scraps, future)

>remote
how does the sidecar talk to tauri securely
localhost page for control panel dashboard <--interesting
localhost server that a third party site using ftorrent can reach <--super interesting
and is this on the same pc? or same lan, like a media server??

other stuff it can do
- download tracker lists
- test trackers on those lists, show statistics
- fetch good.json from good.ftorrent.com and tell the user their ip addresses and nat information

>first example features
magnet link maker and inspector
good.ftorrent.com client for connection nat cone cgnat ipv6 status checker
torrent maker

## Stack

This is a desktop BitTorrent client that participates fully in both traditional BitTorrent swarms (TCP/uTP) and browser-based WebTorrent swarms (WebRTC). The architecture is Tauri + Vue on the frontend, with a Python sidecar wrapping libtorrent on the backend.

**Tauri** is the application framework. It provides a native webview, a Rust core for system-level operations, and a well-defined sidecar mechanism for embedding external binaries. Tauri was chosen over Electron because it uses the operating system's native webview rather than shipping a full Chromium instance, resulting in dramatically smaller binaries, lower memory usage, and a smaller attack surface. Its Rust core also provides a strong security model: Tauri v2's capabilities system requires explicit, scoped permission grants for everything the frontend can access, including sidecar execution and argument validation.

**Vue** is the UI framework running inside Tauri's webview. It was chosen as the team's strongest frontend framework and pairs naturally with Tauri, which is frontend-framework-agnostic. The UI communicates with the Tauri Rust core via Tauri's built-in IPC (commands and events).

**Vue Router** is used for top-level view management even though the app has no visible location bar. This is standard practice in Tauri and Electron apps — a client-side router provides lazy-loaded route components, route guards for flows like first-run setup, and a single declarative file (`router/index.ts`) that serves as a readable table of contents for the app's view structure. The alternative of swapping components via `v-if` or dynamic `<component :is>` works for trivial cases but degrades quickly, leading teams to reinvent history stacks, guard logic, and lazy-loading wrappers — a router in all but name. Vue Router runs in hash mode (`createWebHashHistory()`) since there is no server to handle `pushState` URLs; the hash fragment is purely an internal concern, invisible to the user in a chromeless Tauri window.

**libtorrent** (arvidn/libtorrent, aka libtorrent-rasterbar) is the BitTorrent engine. It is the most mature, full-featured open-source implementation of the BitTorrent protocol, with twenty years of development, and it powers major clients including qBittorrent and Deluge. It supports the full modern BitTorrent protocol suite: DHT, PEX, magnet links, uTP, protocol encryption, IPv6, and web seeds. Critically, libtorrent is the only C/C++ BitTorrent library with first-party WebTorrent support, allowing a single engine to serve both traditional TCP/uTP peers and WebRTC-based browser peers as a hybrid client with a unified piece store and unified swarms. Pure-Rust alternatives like librqbit were evaluated but lack WebTorrent support entirely.

**libtorrent is built from source** (the `master` branch) rather than installed from PyPI or a system package. This is necessary because WebTorrent support is experimental and not yet included in any stable release (the current stable is 2.0.11). Building from master with the `webtorrent=on` flag enables WebRTC peer connectivity and WebSocket tracker support. This also means the project carries responsibility for tracking upstream changes and managing its own build pipeline for libtorrent.

**Python** serves as the bridge between the Tauri Rust core and the libtorrent C++ library. libtorrent's C++ API uses templates, virtual classes, and Boost types that are impractical to call directly via Rust FFI, and the only existing Rust binding crate (libtorrent-sys) is unmaintained and covers a tiny fraction of the API. However, libtorrent maintains first-party Python bindings in its own repository, built as part of the same compilation step as the C++ library, exposing nearly the full API surface including WebTorrent features. No equivalent bindings exist for JavaScript or any other language. Python is not a preference — it is the only maintained, first-party bridge available.

The Python code and the libtorrent shared library are packaged into a standalone executable using **PyInstaller**, producing a single binary with no external dependencies. Tauri bundles this binary as a **sidecar** via its `externalBin` configuration, with platform-specific target-triple suffixes for cross-platform distribution.

**Communication between Tauri and the sidecar uses stdin/stdout pipes**, not a local HTTP server. OS-level pipes between a parent and child process cannot be connected to or eavesdropped on by other processes on the same machine (absent root-level access). A localhost HTTP server, by contrast, binds a TCP port visible to any local process. The sidecar emits newline-delimited JSON (NDJSON) on stdout for events (progress, peer updates, alerts) and accepts JSON commands on stdin (add torrent, pause, configure). Tauri's sidecar API is designed around this pattern.

The resulting architecture, from top to bottom:

```
Vue UI (Tauri webview)
  ↕  Tauri IPC (commands / events)
Tauri Rust core
  ↕  stdin / stdout (NDJSON)
Python sidecar (PyInstaller binary)
  ↕  Python bindings (in-process, Boost.Python)
libtorrent C++ (built from master, webtorrent=on)
  ↕  TCP / uTP + WebRTC
Traditional and browser peers
```

## Security

The app exposes POSIX-style filesystem commands through Tauri IPC: `disk_readdir` (list directory contents), `disk_stat` (file metadata), `disk_read` (read file bytes), `disk_copy` (copy a file), and eventually `disk_write`, `disk_rename`, `disk_unlink`, `disk_rmdir`, and `disk_mkdir`. Together these give the webview effectively the same filesystem access as a native desktop application — arbitrary reads, writes, deletes, and directory manipulation across any path the user account can reach.

Tauri deliberately does not ship this kind of access as a built-in. Traditional desktop frameworks like Win32 or Qt grant full filesystem access because all running code is compiled by the developer — there is no mechanism for foreign code to appear at runtime. A webview is a browser engine, and if an application loads remote content or renders unsanitized HTML, injected script runs with whatever privileges the framework has exposed. Tauri protects against this by scoping its filesystem plugin to specific directories, so that apps built by teams who may not fully understand the threat model don't ship dangerous defaults. For our app, we bypass those guardrails intentionally by writing our own Rust commands and exposing them through IPC.

The entire security posture depends on preventing foreign script from executing in the webview, because IPC is trust-by-origin — any script running in the webview can call `invoke()` exactly as our own Vue components do, and the Rust core cannot distinguish between them. The threat is concrete: if a torrent were named `<img onerror="invoke('io_unlink',{path:'/'})" src=x>` and that string were inserted into the DOM via `innerHTML` or `v-html`, the attack payload would execute with full filesystem access.

Our app closes this vector at two layers. First, the webview serves only our own bundled assets from Tauri's custom protocol (`tauri://` / `asset://`), never from `http://` or `https://` origins — there is no path for remote content to enter the renderer. Second, all untrusted data from trackers, peers, and the user is rendered through Vue's template interpolation (`{{ }}`), which calls `createTextNode()` under the hood and produces plain text nodes, never parsed HTML. The rule is simple: never use `v-html` with data that originated outside the app. CSP with compile-time nonces is the backstop — even if something somehow bypassed Vue's escaping, an injected script without a valid nonce would not execute.

The sidecar runs as a regular OS child process with the user's own filesystem permissions — Tauri's security model governs the webview, not child processes. libtorrent reads and writes freely without any special configuration, and this is secure because the trust boundary sits between the webview and the Rust core, not between the Rust core and its children.

A remaining theoretical vector is a compromised npm dependency importing `@tauri-apps/api` and calling `invoke()` from inside our own bundle, where it would pass CSP and origin checks. We set this aside by choosing a minimal, curated frontend dependency tree — Vue, Vue Router, and a small number of well-known, widely-audited packages. This is not a plugin system, not an extension platform, and not an app that loads third-party widgets. The attack surface is narrow by design.

## Paths

### Paths §1: Built packages and setup files

**Packages.** ftorrent ships four packages:

- `ftorrent.exe` — NSIS installer for Windows 10 and later.
- `ftorrent.dmg` — macOS disk image, supporting Sequoia and Tahoe, Apple Silicon only.
- `ftorrent.deb` — Debian package for Ubuntu Desktop and its derivatives (Mint, Pop!_OS, Zorin, elementary), which together account for the majority of desktop Linux users. Fedora and Arch users can build from source, and adding `.rpm` or `.AppImage` targets is straightforward if demand appears.
- `ftorrent.zip` — portable distribution, described separately below.

During development, macOS and Windows are the active test matrix — the platforms where daily work and testing happen. Linux is a first-class target but not a hot path. The expectation is that by keeping choices simple and standard, Tauri's Linux build will work at the end with little or no correction.

On Windows, the entire installation is per-user — binary, settings, and downloads all live under the user's home directory, with no UAC elevation prompt during setup. On macOS and Linux, the binary is shared system-wide but all state is per-user: settings, session data, and downloads are separated by OS account. This follows platform conventions on all three systems, and two users on the same machine never interact with each other's ftorrent data.

Two settings in `tauri.conf.json` diverge from Tauri's scaffold defaults. First, the NSIS installer's `installMode` is set to `"currentUser"` — Tauri defaults to `"both"`, which presents a dialog asking the user to choose between per-user and system-wide installation. Since the document specifies strictly per-user installation on Windows with no UAC prompt, the dialog is unnecessary and the mode is pinned. Second, the Linux bundler targets are narrowed to produce only a `.deb` — Tauri defaults to building both a `.deb` and an `.AppImage`, but the document ships only the Debian package for now. Both are one-line changes in the config file. Everything else in the distribution and filesystem layout follows Tauri's out-of-box defaults.

**Build output.** In the `ftorrent` monorepo's `desktop` workspace, Tauri produces installer artifacts for Windows, macOS, and Debian Linux as follows:
```
ftorrent/desktop/src-tauri/target/release/bundle/nsis/ftorrent_0.1.0_x64-setup.exe
ftorrent/desktop/src-tauri/target/release/bundle/dmg/ftorrent_0.1.0_aarch64.dmg
ftorrent/desktop/src-tauri/target/release/bundle/deb/ftorrent_0.1.0_amd64.deb
```
The `.dmg` contains `ftorrent.app` for the user to drag into Applications. The `.exe` is the NSIS setup program, not the application binary itself — it extracts and installs the app into the program files location described below. These are renamed to `ftorrent.dmg`, `ftorrent.exe`, and `ftorrent.deb` for distribution on the website.

### Paths §2: Installed and running

**Program files.** Where the application binary lives after installation:
```
C:\Users\username\AppData\Local\ftorrent\       # Windows, per-user
/Applications/ftorrent.app/                     # macOS, system-wide
/usr/bin/ftorrent                               # Linux, system-wide
```
On macOS, the `.dmg` presents `ftorrent.app` for the user to drag into `/Applications/` in the standard way. On Windows, the NSIS installer runs in `currentUser` mode and places files under `AppData\Local\ftorrent\` — no admin privileges required. On Linux, the `.deb` installs to `/usr/bin/`.

**Application settings.** ftorrent's own settings file, keyed by the Tauri bundle identifier:

```
C:\Users\username\AppData\Roaming\com.ftorrent\ftorrent.json    # Windows
~/Library/Application Support/com.ftorrent/ftorrent.json        # macOS
~/.local/share/com.ftorrent/ftorrent.json                       # Linux
```

`ftorrent.json` contains user-facing configuration: the ordered list of download folders, the default download location, UI preferences, and any other settings the user knows about and can change. The Windows registry is avoided except where a feature absolutely requires it, like file type associations for `.torrent` files. On Linux, the base path follows the XDG Base Directory Specification — `$XDG_DATA_HOME` defaults to `~/.local/share/` on Ubuntu Desktop, and virtually no one changes it.

**Global libtorrent state.** Separate from user settings, libtorrent maintains session-wide state that persists across restarts:

```
C:\Users\username\AppData\Roaming\com.ftorrent\state            # Windows
~/Library/Application Support/com.ftorrent/state                # macOS
~/.local/share/com.ftorrent/state                               # Linux
```

This file holds the DHT routing table and session-wide settings — global state that belongs to the running instance, not to any individual torrent. It is serialized via libtorrent's `write_session_params()` on shutdown and restored with `read_session_params()` on startup. Without it, every launch would bootstrap DHT from scratch. Per-torrent state — resume data, piece completion, tracker lists — lives alongside downloads in `.ftorrent` directories, described below.

**Downloads.** Downloaded content — torrent data and the potentially large files and folder trees it describes:
```
C:\Users\username\Downloads\ftorrent\                       # Windows
~/Downloads/ftorrent/                                       # macOS
~/Downloads/ftorrent/                                       # Linux
```
The user can change this in `ftorrent.json`, but the out-of-box default puts everything in a clearly named subfolder of Downloads rather than scattering files into the user's home directory. On Linux, `~/Downloads/` is the default value of `$XDG_DOWNLOAD_DIR`, a system variable that distributions can override — Ubuntu Desktop leaves it at the default.

### Paths §3: Download locations and session data

ftorrent does not keep a central session store. Session data lives alongside the downloaded files it describes — each download folder carries its own `.ftorrent` directory. If you move a folder, copy it to a different drive, or carry it on a USB stick, the session information travels with it.

`ftorrent.json` maintains an ordered list of download folder paths:

```
{
  "download_folders": [
    "~/Downloads/ftorrent",
    "E:/big torrents"
  ]
}
```

The first entry is the default — new torrents go here unless the user specifies otherwise. This list changes only when the user adds or removes an entire download location, not when individual torrents are added or removed.

Inside each download folder, a hidden `.ftorrent` directory contains one subfolder per active torrent, named by info hash with a version prefix:

```
~/Downloads/ftorrent/
~/Downloads/ftorrent/some-linux-iso/
~/Downloads/ftorrent/some-movie/
~/Downloads/ftorrent/.ftorrent/
~/Downloads/ftorrent/.ftorrent/v1.abc123de...88ff/
~/Downloads/ftorrent/.ftorrent/v1.abc123de...88ff/resume
~/Downloads/ftorrent/.ftorrent/v1.abc123de...88ff/metadata.torrent
```

`v1` denotes a BitTorrent v1 info hash (SHA-1, 40 hex characters). `v2` denotes a BitTorrent v2 info hash (SHA-256, 64 hex characters). Hybrid torrents use `v2` as the canonical folder name — the v1 hash is preserved inside the resume data. The dot prefix hides `.ftorrent` on macOS and Linux; on Windows, ftorrent sets the hidden file attribute.

Each info hash subfolder contains at minimum a `resume` file — the bencoded blob from libtorrent. It may also contain `metadata.torrent` (the original `.torrent` file) and in the future other per-torrent metadata. The folder-per-torrent structure provides room for this without changing the naming scheme.

**Startup.** ftorrent reads `ftorrent.json` and walks `download_folders` in order. For each reachable folder, it scans `.ftorrent/`, loads each `resume` file with `read_resume_data()`, overwrites `save_path` on the returned `add_torrent_params` with the absolute path of the download folder it's currently scanning, and passes it to `session.async_add_torrent()`. Unreachable folders — drive not mounted, path doesn't exist — are skipped, but not removed; a future session may be able to reach that folder, and list the torrents there. (For instance, the user might be running ftorrent installed, but not have plugged in a removable high capacity usb drive.)

**Adding a torrent.** ftorrent creates an info hash subfolder inside the target download folder's `.ftorrent` directory, writes the initial resume data, and passes `add_torrent_params` to libtorrent with `save_path` set to that download folder.

**Removing a torrent.** ftorrent deletes the info hash subfolder from `.ftorrent/`. The downloaded files remain. The torrent disappears from the list. Re-adding it later triggers a piece re-check against the files on disk.

**Graceful shutdown.** ftorrent calls `save_resume_data()` on every active torrent, waits for each corresponding `save_resume_data_alert`, and writes the serialized blob to the torrent's `resume` file. Global session state — DHT routing table and session-wide settings — is saved separately to the `state` file in the application data directory.

**Path resolution.** libtorrent bakes `save_path` into resume data as an absolute path. ftorrent ignores it. The path is determined by physical location: `.ftorrent` lives inside the download folder that contains the files, so ftorrent always knows the correct path from the folder it's scanning. This means a drive letter change — Windows assigns `E:\` one day and `F:\` the next — is a one-line fix in `download_folders`. On next launch, ftorrent finds `.ftorrent` at the new path, sets `save_path` accordingly, and all torrents recover without re-checking pieces.

### Paths §4: Portable ftorrent, startup, and path defaults

**Paths in settings.** Paths in `ftorrent.json` are normalized to forward slashes on write. Three forms are supported:

```
./    relative to the application directory
~     user's home directory
      absolute paths (e.g. D:/torrents)
```

`./` resolves relative to the application directory on every platform. On Windows and Linux, this is the directory containing the executable. On macOS, the running binary is inside `ftorrent.app/Contents/MacOS/`, but `./` resolves to the directory containing the `.app` bundle — the code detects that it's inside a bundle and walks up to the bundle's parent. This means `./downloads` resolves to the same level as `ftorrent.exe` on Windows, `ftorrent` on Linux, and `ftorrent.app/` on macOS — which is what the user expects. `~` resolves to `/Users/username/` on macOS, `C:\Users\username\` on Windows, `/home/username/` on Linux. Absolute paths are also allowed — a user who points downloads at `D:\torrents` stores exactly that. A user may wish to keep ftorrent portable on a thumb drive plugged into `G:\` which downloads and seeds torrents to a desktop USB drive on `H:\`, for instance. Relative and `~`-prefixed paths are portable across machines; absolute paths are intentionally machine-specific.

**Startup flow.** When ftorrent launches, it computes the path to its own running executable and looks for `portable/ftorrent.json` alongside it — on Windows, next to `ftorrent.exe`; on macOS, next to the `ftorrent.app` bundle. If that file exists, ftorrent is in portable mode and uses it as its settings file. If it doesn't, ftorrent looks for `ftorrent.json` in the platform-standard application data directory described in §2. If that file exists, ftorrent uses it. If neither exists — a clean first launch after installation — ftorrent creates `ftorrent.json` at the installed location, prepopulated with:

```
{"download_folders": ["~/Downloads/ftorrent"]}
```

All other settings follow the same pattern — if absent from `ftorrent.json`, a hardcoded default applies. The file only accumulates values the user has explicitly changed.

**What ftorrent.zip contains.** The portable distribution unpacks to a self-contained directory with binaries for all three platforms:

```
ftorrent/
ftorrent/ftorrent.exe                                               # Windows binary
ftorrent/sidecar-x86_64-pc-windows-msvc.exe                         # Windows sidecar
ftorrent/ftorrent.app/                                              # macOS app bundle
ftorrent/ftorrent.app/Contents/MacOS/ftorrent                       # macOS binary
ftorrent/ftorrent.app/Contents/MacOS/sidecar-aarch64-apple-darwin   # macOS sidecar
ftorrent/ftorrent                                                   # Linux binary
ftorrent/sidecar-x86_64-unknown-linux-gnu                           # Linux sidecar
ftorrent/portable/
ftorrent/portable/ftorrent.json                                     # triggers portable mode
```

`portable/ftorrent.json` is set during the build that produces the zip and contains:

```
{"download_folders": ["./downloads"]}
```

The `./downloads` path resolves relative to the executable on every platform, so the default portable download location works regardless of drive letter or mount point. This is a single zip that works on Windows, macOS, and Linux. The user plugs in a USB stick, launches the binary for their platform, and the same `portable/ftorrent.json` governs all three. Downloaded files, `.ftorrent` directories, and the `state` file are all platform-agnostic. A user can download a torrent on a Mac, eject the stick, plug it into a Windows machine, and resume seeding without re-checking pieces. Most portable app distributions are single-platform. A cross-platform portable BitTorrent client that carries its state between operating systems is, as far as we know, novel.

**Portable mode must not touch the host.** In portable mode, ftorrent never writes to the platform application data directory. Settings live at `portable/ftorrent.json`, global libtorrent state at `portable/state`, and the default download location is alongside the executable. A user can explicitly point downloads at a host drive, but out of the box, everything stays on the stick.

### Paths §5: Multiple running instances

ftorrent enforces single-instance per settings file. If Alice double-clicks `ftorrent.exe` while her installed copy is already running, the second launch detects the lock, signals the running instance to focus its window, and exits. She never sees two windows from the same installation.

However, multiple independent instances on the same machine are expected and allowed. Alice and Bob share a Mac — Alice is signed in and running her installed ftorrent, then locks the screen and Bob signs in and runs his. These are separate OS users with separate application data directories, separate `ftorrent.json` files, and separate locks. No conflict.

Alice can also run her installed ftorrent and her portable ftorrent side by side on the same desktop. These are two different settings files — one in the platform application data directory, one at `portable/ftorrent.json` on her stick — so the lock doesn't conflict. Two ftorrent windows appear on screen simultaneously, each managing its own torrents. This is correct.

**Why not Tauri's single-instance plugin.** Tauri v2 ships a single-instance plugin, but it keys on the bundle identifier. An installed copy and a portable copy share the same bundle ID, so the plugin would block the second from launching — exactly the case we want to allow. We also can't use it to distinguish two different OS users running installed copies, since the bundle ID is the same for both. The plugin solves a simpler problem than ours.

**Instance lock implementation.** On startup, after resolving which `ftorrent.json` to use (§4's startup flow), ftorrent hashes the absolute path to that file and uses the hash as a lock identity.

On Windows, this is a named mutex via `CreateMutex`. The name is a fixed prefix plus the hash — something like `ftorrent-{hash}`. If the mutex already exists, the second instance knows someone is running with the same settings file. Named mutexes are kernel objects: the OS releases them automatically when the owning process exits, including crashes, kills, and power loss. There are no stale locks.

On macOS and Linux, this is an advisory lock via `flock()` on a lock file inside the application data directory (or `portable/` in portable mode). `flock()` locks are tied to the file descriptor — the OS releases them automatically when the process exits for any reason, including crashes. Like named mutexes on Windows, there are no stale locks. PID-based lock files are deliberately not used because they can become stale if the process is killed without cleanup, leading to the worst case: nothing visibly running, but startup is blocked.

**Focusing the existing instance.** When the second launch detects the lock, it needs to bring the running instance's window to the front before exiting. On Windows, it finds the window by a registered window class name or title and sends a focus message via the Windows API. On macOS, it uses `NSRunningApplication` to activate the existing process. This is the one piece of platform-specific UI code in the lock system — the lock itself is simple, the "go find that other window" part is the fiddly bit.

If the focus step fails — the running instance is hung, or the window lookup doesn't find a match — the second instance exits anyway. It never forces past the lock. The user can kill the stuck process manually and relaunch; the lock will be gone because the OS cleaned it up.

### Paths §6: Example story: Portable Alice

The paths system is designed to be simple in its rules but flexible under strain — the same mechanics that handle a single-machine install with one download folder also handle a portable drive moving between platforms with machine-specific download locations, without special cases or user intervention. Consider this example: Alice carries ftorrent on a 2TB USB drive. She uses a Windows desktop at home and a Mac at the office.

She unzips `ftorrent.zip` to her USB drive and launches `ftorrent.exe` on her Windows desktop, where the drive mounts as `E:\`. She adds a few torrents — a Linux ISO and a documentary. Because `portable/ftorrent.json` contains `{"download_folders": ["./downloads"]}`, both go to `E:\ftorrent\downloads\`. Her drive looks like:

```
E:\ftorrent\ftorrent.exe
E:\ftorrent\portable\ftorrent.json
E:\ftorrent\portable\state
E:\ftorrent\downloads\ubuntu-24.04\
E:\ftorrent\downloads\some-documentary\
E:\ftorrent\downloads\.ftorrent\v1.aabb...\
E:\ftorrent\downloads\.ftorrent\v1.ccdd...\
```

A friend sends her a magnet link for a large Windows game he's developing, a new build for her to play-test. She wants that on her PC, not on the stick, so when adding the magnet she chooses a custom download location: `C:\Games`. ftorrent adds this folder to `download_folders` automatically — the folder doesn't need to be empty or ftorrent-specific. ftorrent only reads and writes inside the `.ftorrent` subdirectory it creates there, and only looks for files and folders it has resume data for. Everything else in `C:\Games` is untouched. Her `ftorrent.json` is now:

```
{
  "download_folders": [
    "./downloads",
    "C:/Games"
  ]
}
```

The game downloads to `C:\Games\moondrop-mountain-nightly\`, with resume data in `C:\Games\.ftorrent\`. She seeds it overnight.

Next morning, she ejects the drive and takes it to work. She plugs it into her Mac, where it mounts at `/Volumes/ALICE2TB/`. She double-clicks `ftorrent.app`. On startup:

1. ftorrent finds `portable/ftorrent.json`, enters portable mode.
2. `./downloads` resolves to `/Volumes/ALICE2TB/ftorrent/downloads/`. The `.ftorrent` directory is there — the Linux ISO and documentary load normally.
3. `C:/Games` doesn't exist on this Mac. ftorrent can't reach the directory, can't read its `.ftorrent`, and has no information about what's there — no names, no hashes, nothing. Those torrents simply don't appear. There's no error, no gray row, no dialog. The torrent list shows the two torrents on the stick and that's it.
4. She adds a new torrent at work. It goes to the default `./downloads` on the stick.

She takes the drive home. Her desktop already has a thumb drive plugged in, so the USB drive mounts as `F:\` instead of `E:\`. She launches `ftorrent.exe`. On startup:

1. `./downloads` resolves to `F:\ftorrent\downloads\`. The Linux ISO, documentary, and the new torrent from work all load — the drive letter change doesn't matter.
2. `C:/Games` exists on this machine. ftorrent scans its `.ftorrent`, finds the game torrent's resume data, sets `save_path` to `C:\Games\`, and the game resumes seeding exactly where it left off. Nothing about the trip to the Mac disturbed it — the resume data has been sitting on `C:\` the whole time, untouched.
3. Everything is back. No paths were edited. No dialogs were shown.

Her `download_folders` list grows over time as she adds locations on different machines. Entries that don't resolve on the current machine are silently skipped. There are no ghost entries in the torrent list, no error states, no prompts — torrents from unreachable folders simply aren't there, and come back when the folder is reachable again.

### Paths §7: Example story: Three users who install





**Bill** uses Windows 10. **Steve** has a Mac. **Linus** runs Ubuntu Desktop. Each installs ftorrent and downloads the same two torrents: Big Buck Bunny (`dd8255ec...6d1c`), a single `.mp4` file, and The WIRED CD - Rip. Sample. Mash. Share (`a88fda59...dad3`), a folder of MP3 tracks. Both are Creative Commons, listed on WebTorrent's free torrents page.

**Bill (Windows)**

```
# installer downloaded and double-clicked
C:\Users\Bill\Downloads\ftorrent.exe

# application binary (installed by the NSIS setup)
C:\Users\Bill\AppData\Local\ftorrent\ftorrent.exe

# settings (created on first launch)
C:\Users\Bill\AppData\Roaming\com.ftorrent\ftorrent.json

# global libtorrent state
C:\Users\Bill\AppData\Roaming\com.ftorrent\state

# downloaded content
C:\Users\Bill\Downloads\ftorrent\Big Buck Bunny.mp4
C:\Users\Bill\Downloads\ftorrent\The WIRED CD - Rip. Sample. Mash. Share\
C:\Users\Bill\Downloads\ftorrent\The WIRED CD - Rip. Sample. Mash. Share\01 - Beastie Boys - Now Get Busy.mp3
C:\Users\Bill\Downloads\ftorrent\The WIRED CD - Rip. Sample. Mash. Share\02 - David Byrne - My Fair Lady.mp3
C:\Users\Bill\Downloads\ftorrent\The WIRED CD - Rip. Sample. Mash. Share\...

# per-torrent session data
C:\Users\Bill\Downloads\ftorrent\.ftorrent\v1.dd8255ec...6d1c\resume
C:\Users\Bill\Downloads\ftorrent\.ftorrent\v1.dd8255ec...6d1c\metadata.torrent
C:\Users\Bill\Downloads\ftorrent\.ftorrent\v1.a88fda59...dad3\resume
C:\Users\Bill\Downloads\ftorrent\.ftorrent\v1.a88fda59...dad3\metadata.torrent
```

**Steve (macOS)**

```
# installer downloaded and double-clicked
/Users/Steve/Downloads/ftorrent.dmg

# application binary
/Applications/ftorrent.app/
/Applications/ftorrent.app/Contents/MacOS/ftorrent

# settings (created on first launch)
/Users/Steve/Library/Application Support/com.ftorrent/ftorrent.json

# global libtorrent state
/Users/Steve/Library/Application Support/com.ftorrent/state

# downloaded content
/Users/Steve/Downloads/ftorrent/Big Buck Bunny.mp4
/Users/Steve/Downloads/ftorrent/The WIRED CD - Rip. Sample. Mash. Share/
/Users/Steve/Downloads/ftorrent/The WIRED CD - Rip. Sample. Mash. Share/01 - Beastie Boys - Now Get Busy.mp3
/Users/Steve/Downloads/ftorrent/The WIRED CD - Rip. Sample. Mash. Share/02 - David Byrne - My Fair Lady.mp3
/Users/Steve/Downloads/ftorrent/The WIRED CD - Rip. Sample. Mash. Share/...

# per-torrent session data
/Users/Steve/Downloads/ftorrent/.ftorrent/v1.dd8255ec...6d1c/resume
/Users/Steve/Downloads/ftorrent/.ftorrent/v1.dd8255ec...6d1c/metadata.torrent
/Users/Steve/Downloads/ftorrent/.ftorrent/v1.a88fda59...dad3/resume
/Users/Steve/Downloads/ftorrent/.ftorrent/v1.a88fda59...dad3/metadata.torrent
```

**Linus (Ubuntu Desktop)**

```
# installer downloaded and double-clicked (or sudo dpkg -i)
/home/linus/Downloads/ftorrent.deb

# application binary
/usr/bin/ftorrent

# settings (created on first launch)
/home/linus/.local/share/com.ftorrent/ftorrent.json

# global libtorrent state
/home/linus/.local/share/com.ftorrent/state

# downloaded content
/home/linus/Downloads/ftorrent/Big Buck Bunny.mp4
/home/linus/Downloads/ftorrent/The WIRED CD - Rip. Sample. Mash. Share/
/home/linus/Downloads/ftorrent/The WIRED CD - Rip. Sample. Mash. Share/01 - Beastie Boys - Now Get Busy.mp3
/home/linus/Downloads/ftorrent/The WIRED CD - Rip. Sample. Mash. Share/02 - David Byrne - My Fair Lady.mp3
/home/linus/Downloads/ftorrent/The WIRED CD - Rip. Sample. Mash. Share/...

# per-torrent session data
/home/linus/Downloads/ftorrent/.ftorrent/v1.dd8255ec...6d1c/resume
/home/linus/Downloads/ftorrent/.ftorrent/v1.dd8255ec...6d1c/metadata.torrent
/home/linus/Downloads/ftorrent/.ftorrent/v1.a88fda59...dad3/resume
/home/linus/Downloads/ftorrent/.ftorrent/v1.a88fda59...dad3/metadata.torrent
```

All three users' `ftorrent.json` contains the same thing:

```
{"download_folders": ["~/Downloads/ftorrent"]}
```

This was created by ftorrent on first launch. The `~` resolves to `C:\Users\Bill\`, `/Users/Steve/`, and `/home/linus/` respectively. Every other path follows from the decisions in §1–§4 — nothing was configured, nothing was customized, everything is factory defaults.

## Automatic Update

ftorrent does not use Microsoft Authenticode or Apple notarization for signing. BitTorrent users are experienced enough — or adventurous enough — to click through SmartScreen warnings on Windows and the Gatekeeper bypass on macOS. Platform signing can be added later without changing anything about the update system itself.

Update signing uses Tauri's built-in Ed25519 keypair. The private key lives on the build machine and signs every release artifact. The public key is embedded in `tauri.conf.json` and ships with every copy of the app. It never changes across versions. If the app can't verify an update's signature against the embedded public key, the update is rejected — no partial install, no corrupted state.

**Update endpoint.** Each release, the build produces signed artifacts and a static JSON manifest per platform, hosted at the apex domain:

```
https://ftorrent.com/update/x86_64-pc-windows-msvc
https://ftorrent.com/update/aarch64-apple-darwin
https://ftorrent.com/update/x86_64-unknown-linux-gnu
```

Each URL serves a JSON response:

```
{
  "version": "0.3.0",
  "url": "https://ftorrent.com/releases/ftorrent_0.3.0_x64-setup.exe",
  "signature": "dGhpcyBpcyBhIHNpZ25hdHVyZQ==",
  "notes": "Bug fixes and DHT improvements"
}
```

These are static files, overwritten on each release. The server does no version comparison — the app compares the response's `version` field against its own running version and ignores the response if it's not newer. This means a user on v0.1.0 who misses v0.2.0 goes straight to v0.3.0. There is no sequential upgrade chain; every release is a full artifact, not a delta.

**Check cadence.** The app checks on launch and once every 24 hours while running. Each check is a single small HTTP GET. If the network is unreachable, the check fails silently and retries next cycle. The server sees each check in its access logs — IP address, platform, current version (via user-agent or query parameter). This provides approximate active install counts and version distribution without any analytics SDK or tracking code.

**User experience.** When a newer version is detected, the app downloads the artifact and verifies the signature in the background. Only after the download is complete and verified does a small, non-modal text indicator appear at the bottom of the window — something like "ftorrent 0.3.0 is available." This is not a popup and does not interrupt the user. It can be ignored indefinitely.

When the user clicks, a brief confirmation prompt appears. On confirmation, the app closes, the update applies silently — no installer wizard, no progress bar — and the new version launches. The window disappears and is replaced by the updated one within a few seconds.

This seamless apply-and-restart works on macOS and Windows. On macOS, Tauri replaces the `.app` bundle and relaunches. On Windows, Tauri runs the NSIS installer silently in per-user mode and relaunches. Neither requires admin privileges or platform signing.

On Linux and in portable mode, the same check and notification happen, but clicking the indicator opens `ftorrent.com` in the user's browser rather than applying the update in place. Linux updates require package manager privileges that the app shouldn't silently acquire, and portable installs can't replace a running binary on a USB stick. The user downloads the new `.deb` or `ftorrent.zip` manually — this matches platform expectations.

**Setting.** `ftorrent.json` has an `"update_check"` key, defaulting to `true`. If the user sets it to `false`, the app makes no HTTP requests to the update endpoint, no download occurs, no notification appears, and no install base data reaches the server.

## Crash capture

ftorrent does not have a logging system. During development, diagnostic output is visible in the terminal via `tauri dev`. In production builds, the Rust core captures the sidecar's stderr to a ring buffer in memory — the last 100 lines. During normal operation, nothing is written to disk.

If the sidecar exits with a non-zero exit code, the Rust core writes the buffer contents to `crash.log` in the application data directory (or `portable/` in portable mode):

```
C:\Users\username\AppData\Roaming\com.ftorrent\crash.log        # Windows
~/Library/Application Support/com.ftorrent/crash.log             # macOS
~/.local/share/com.ftorrent/crash.log                            # Linux
```

The file is overwritten on each crash, not appended. It contains only what the sidecar printed to stderr in its final moments — Python tracebacks, libtorrent alert messages, or native library segfault output. A user reporting a crash can attach this file to a GitHub issue. Without it, every crash report is "it broke."

## Sleep and wake

ftorrent has two separate layers for dealing with system sleep: detection and prevention.

**Detection.** The Rust core writes a timestamp to memory every 60 seconds. When the system sleeps, the timer stops. When it wakes, the timer resumes and the next tick sees a gap — an hour of missing timestamps means an hour of sleep. The first tick after the gap triggers the sidecar to re-announce to trackers and refresh peer connections rather than waiting for libtorrent's internal timers to expire naturally. No platform-specific sleep/wake notifications are involved — this works identically on macOS, Windows, and Linux.

**Prevention.** An optional setting in `ftorrent.json`, `"prevent_sleep"`, defaults to `false`. When the user enables it, ftorrent holds a power assertion that prevents the system from sleeping due to inactivity. On macOS, this is an `IOPMAssertion` with `PreventUserIdleSystemSleep`. On Windows, this is `SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED)`. The assertion is held as long as ftorrent is running, regardless of whether any transfers are active — the user may want to keep the machine awake for DHT participation alone. The assertion is released when ftorrent exits.

This only prevents idle sleep. It cannot prevent lid close, user-initiated sleep, critical battery shutdown, or forced OS restarts. These are correct OS boundaries — no unprivileged app can override them. When they happen, the timestamp gap detects the wake and the sidecar recovers.

Linux is excluded from the prevention layer for now. The mechanism (`systemd-inhibit` or D-Bus to `org.freedesktop.login1`) is straightforward but outside the active test matrix.

## Start at login

An optional `"start_at_login"` setting in `ftorrent.json`, defaulting to `false`, registers ftorrent to launch at OS login via Tauri's `tauri-plugin-autostart`. ftorrent calls the plugin's `enable()` and `disable()` API — it doesn't manage the underlying OS registration directly. On Windows, the plugin writes to `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, which is reliable on Windows 10 and 11. On macOS, the plugin registers a LaunchAgent, which on Ventura and later may prompt the user to approve in System Settings → Login Items. Because ftorrent ships unsigned, macOS may require this manual approval before the registration takes effect. On Linux, the plugin creates a `.desktop` file in `~/.config/autostart/`, which is the XDG autostart standard followed by Ubuntu Desktop and its derivatives.

In portable mode, this setting is grayed out in the UI — a portable app on a USB stick does not register itself on the host system's login sequence, as the portable promise is no change to the running system at all!

## Instance lock

**Instance lock.** ftorrent enforces a single running instance per settings file. On startup, after resolving which `ftorrent.json` to use (either `portable/ftorrent.json` alongside the executable or the platform-standard location per §4), the app claims an OS-level lock tied to that settings file. If the lock is already held, the app signals the running instance to focus its window and exits.

On Windows, the lock is a named mutex via `CreateMutex`. The mutex name is a fixed prefix plus a SHA-256 hash of the absolute path to `ftorrent.json` — something like `ftorrent-a1b2c3d4...`. Raw filesystem paths aren't valid mutex names, so the hash produces a fixed-length, namespace-safe identifier. If the mutex already exists, `CreateMutex` returns `ERROR_ALREADY_EXISTS` and the second launch knows to hand off and exit. The kernel destroys the mutex automatically when the owning process exits for any reason — clean exit, crash, kill, or power loss. There are no stale locks.

On macOS and Linux, the lock is `flock()` on a dedicated file, `ftorrent.lock`, in the same directory as `ftorrent.json`. The first launch ever creates this file empty via `open(O_CREAT)` and immediately acquires an exclusive non-blocking lock with `flock(fd, LOCK_EX | LOCK_NB)`. If the lock is already held by another process, `flock` fails with `EWOULDBLOCK` and the second launch knows to hand off and exit. The lock is tied to the open file descriptor — the OS releases it automatically when the process exits for any reason. There are no stale locks. No hashing is needed on these platforms because the directory path already distinguishes instances — `~/Library/Application Support/com.ftorrent/ftorrent.lock` and `portable/ftorrent.lock` are different files with different inodes. `ftorrent.lock` exists solely to be locked. It is never written to, never replaced, never renamed — a separate file avoids the inode-replacement problem that would arise from locking `ftorrent.json` itself, which is atomically replaced on every settings save.

When the second launch detects the lock, it signals the running instance to bring its window to the front. On Windows, it finds the window by a registered window class name and sends a focus message via the Windows API. On macOS, it uses `NSRunningApplication` to activate the existing process. If the focus step fails — the running instance is hung, or the window lookup doesn't match — the second launch exits anyway. It never forces past the lock. The user can kill the stuck process manually and relaunch; the OS will have already cleaned up the lock.

This replaces Tauri's built-in single-instance plugin, which keys on the bundle identifier. An installed copy and a portable copy share the same bundle ID, so the plugin would block the second from launching — exactly the case ftorrent needs to allow.

**No collision between OS users.** Alice and Bob share a Mac or PC, each signed in to their own OS account. Their settings files are in separate per-user directories — Alice's `ftorrent.json` and `ftorrent.lock` are under her home directory, Bob's under his. The locks are on different files entirely. Both can start, run, and stop their own ftorrent instances independently, even when both accounts are active simultaneously (fast user switching on macOS, switching signed-in users on Windows).

**No collision between installed and portable.** Alice can run her installed ftorrent and any number of portable ftorrent instances side by side on the same desktop. Each uses a different `ftorrent.json` — one in the platform application data directory, each portable copy in its own `portable/` directory — so each has its own lock. Multiple ftorrent windows appear on screen simultaneously, each managing its own torrents.

**Common case: user relaunches without realizing it's running.** Alice already has ftorrent running. She clicks the Start menu shortcut, the Dock icon, or the desktop launcher again. The second launch resolves to the same `ftorrent.json`, finds the lock held, signals the running instance to show and focus its window, and exits. Alice sees her existing window come to the front. She never sees a second window, an error dialog, or a delay.

## Appearance and dark mode

Full dark mode support in a Tauri + Vue app involves three layers. The Tauri window has a `theme` property — set to `"dark"`, `"light"`, or omitted to follow the system preference. This controls the native title bar and window chrome appearance, and sets the webview's `prefers-color-scheme` media query so CSS can respond. On the Vue side, styles branch on that media query or on a class applied to the root element (Tailwind's `dark:` variant uses the class approach). A complete implementation would detect the OS setting, let the user override it with a three-way toggle (system / light / dark), persist the choice in `ftorrent.json`, and apply it both to the Tauri window via the runtime API and to the Vue root element so all three layers — native chrome, CSS media query, and class-based styles — agree.

For v0.1, ftorrent ships a single dark appearance. The window theme is pinned to `"dark"` in `tauri.conf.json`, and the UI is designed dark from the start — there are no light-mode styles, no media queries, no toggle. This is a design constraint, not a technical limitation. Adding light mode and a system-follow toggle later means building out the CSS for a second palette and wiring the three-way preference through to the Tauri window API, but nothing about the v0.1 architecture prevents it.

## Icons and graphics

ftorrent needs a small set of source assets. The main app icon appears in two forms — one for macOS with Apple's required padding, one full-bleed for Windows and Linux — plus a monochrome tray icon, a DMG background, and a website favicon.

**Application icon.** The main icon is a color square at 1024×1024 pixels. The designer places the source file in the repo:

```
ftorrent/desktop/assets/app-icon.png
```

Tauri's CLI generates all platform-specific formats from it:

```
cd ftorrent/desktop
cargo tauri icon assets/app-icon.png
```

This populates the required files that Tauri reads at build time:

```
ftorrent/desktop/src-tauri/icons/icon.ico
ftorrent/desktop/src-tauri/icons/icon.icns
ftorrent/desktop/src-tauri/icons/icon.png
ftorrent/desktop/src-tauri/icons/32x32.png
ftorrent/desktop/src-tauri/icons/128x128.png
ftorrent/desktop/src-tauri/icons/128x128@2x.png
```

These are referenced in `tauri.conf.json`:

```
{
  "bundle": {
    "icon": [
      "icons/32x32.png",
      "icons/128x128.png",
      "icons/128x128@2x.png",
      "icons/icon.icns",
      "icons/icon.ico"
    ]
  }
}
```

This icon appears in the Dock, taskbar, app switcher, Start menu, Finder, File Explorer, Spotlight, and application launchers on all three platforms.

**macOS icon padding.** macOS requires app icons to include transparent padding within the 1024×1024 canvas — the artwork sits within roughly an 824×824 rounded rectangle, centered. If the source image fills edge to edge, the icon appears oversized in the Dock compared to every other app. This is a known issue with Tauri's icon generator, which does not add the padding automatically. The designer provides a second source:

```
ftorrent/desktop/assets/app-icon-macos.png
```

Same artwork but with transparent padding and rounded corners per Apple HIG. The build script automates the swap: run `tauri icon` with the full-bleed source, then replace only `icon.icns` with the macOS-padded version:

```
cd ftorrent/desktop
cargo tauri icon assets/app-icon.png
cargo tauri icon assets/app-icon-macos.png --output /tmp/macos-icons
cp /tmp/macos-icons/icon.icns src-tauri/icons/icon.icns
```

Only `icon.icns` differs — the `.ico` and `.png` files stay full-bleed, which is correct for Windows and Linux.

**Tray and menu bar icons.** Separate from the application icon, a small icon appears in the system tray (Windows), menu bar (macOS), or status notifier area (Linux) when ftorrent is running with its window hidden. All tray icons should be simple and legible at tiny sizes — the main app icon scaled down will be an unreadable blob.

On macOS, this must be a monochrome "template image" — a single-color shape on a transparent background. The system applies the appropriate color for light mode, dark mode, and the translucent menu bar. On Windows and Linux, a small color icon is used instead.

The designer provides these directly:

```
ftorrent/desktop/src-tauri/icons/tray-template.png       # 22×22, monochrome black on transparent, macOS
ftorrent/desktop/src-tauri/icons/tray-color.png           # 32×32, color on transparent, Windows and Linux
```

The tray icon is configured in `tauri.conf.json`:

```
{
  "app": {
    "trayIcon": {
      "iconPath": "icons/tray-color.png",
      "iconAsTemplate": false
    }
  }
}
```

On macOS, the Rust code overrides this at runtime to use the template icon:

```rust
let tray = TrayIconBuilder::new()
    .icon_as_template(true)
    .icon(Image::from_path("icons/tray-template.png")?)
    .build(app)?;
```

On Windows and Linux, `tray-color.png` from the config is used as-is.

**DMG background.** The macOS DMG window displays a decorative background image behind the app icon and the Applications folder icon. This image typically includes a visual arrow or cue indicating the user should drag the icon to Applications.

The designer provides this directly:

```
ftorrent/desktop/assets/dmg-background.png                # 660×400, decorative with drag arrow
```

The DMG layout is configured in `tauri.conf.json`:

```
{
  "bundle": {
    "macOS": {
      "dmg": {
        "background": "assets/dmg-background.png",
        "windowSize": {
          "width": 660,
          "height": 400
        },
        "appPosition": {
          "x": 180,
          "y": 220
        },
        "applicationFolderPosition": {
          "x": 480,
          "y": 220
        }
      }
    }
  }
}
```

The `appPosition` and `applicationFolderPosition` values position the ftorrent icon and the Applications folder alias within the window. The designer should place the drag arrow in the background image to connect these two points.

**Windows installer.** The NSIS installer runs silently — no wizard pages, no UI. The user double-clicks `ftorrent.exe`, files are extracted to `AppData\Local\ftorrent\`, and the app launches. No installer graphics are needed. This is configured in `tauri.conf.json`:

```
{
  "bundle": {
    "windows": {
      "nsis": {
        "installerIcon": "icons/icon.ico",
        "installMode": "currentUser"
      }
    }
  }
}
```

**Website favicon.** A standard favicon for ftorrent.com. Not part of the Tauri build — this goes in the website repo.

```
ftorrent/website/assets/favicon.png                       # 512×512, color, square
```

Converted to `.ico` and multiple `.png` sizes for web use with any favicon generator such as realfavicongenerator.net.

### Windows 10 Start

That's the Windows 10 `VisualElementsManifest`. For any desktop `.exe`, Windows looks for an XML file next to it — for `ftorrent.exe`, the Start menu looks for `ftorrent.VisualElementsManifest.xml` in the same folder. The manifest specifies a larger purpose-designed PNG for the medium tile, a background color, and whether to show the app name. Without it, Windows just extracts the tiny icon from the `.ico` and displays it on a generic theme-colored tile.

The manifest looks like this:

```xml
<Application xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <VisualElements
    BackgroundColor="#1a1a2e"
    ShowNameOnSquare150x150Logo="on"
    ForegroundText="light"
    Square150x150Logo="Assets\tile-150.png"
    Square70x70Logo="Assets\tile-70.png"/>
</Application>
```

The NSIS installer would need to place this file and the PNG assets next to `ftorrent.exe` in `AppData\Local\ftorrent\`:

```
C:\Users\username\AppData\Local\ftorrent\ftorrent.exe
C:\Users\username\AppData\Local\ftorrent\ftorrent.VisualElementsManifest.xml
C:\Users\username\AppData\Local\ftorrent\Assets\tile-150.png
C:\Users\username\AppData\Local\ftorrent\Assets\tile-70.png
```

That gives you a large clean icon on a branded background in the Start menu, like Firefox.

One caveat: Windows 11 dropped Live Tiles. The Start menu there just shows the icon from `.ico`, and the manifest is ignored. So this is Windows 10 polish only. Still worth doing — Windows 10 is in the support matrix — but it won't show on 11.
