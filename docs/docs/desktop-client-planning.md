---
title: Desktop Client Planning
description: Design and planning notes for the ftorrent desktop client — stack, paths, lifecycle, and distribution.
---

# ftorrent Desktop Client

ftorrent is a desktop BitTorrent client for Windows, Mac, and Linux. The source lives at [github.com/zootella/ftorrent](https://github.com/zootella/ftorrent) in the `desktop` workspace — the project is "ftorrent," always lowercase, and the workspace is "desktop." The app participates in both traditional BitTorrent swarms (TCP/uTP) and browser-based WebTorrent swarms (WebRTC) through a single hybrid engine.

The Rust filesystem module is ported from a separate Tauri app, Fuji, where it was written as `io.rs`. In ftorrent it is `disk.rs`, renamed to avoid ambiguity with network I/O.

## Building from the outside in

We build ftorrent from the outside in. This document plans the shell around the torrent experience — distribution, installation, automatic update, the application lifecycle, OS integration, file and protocol associations, and the paths and locking that keep state correct across machines and instances. This is not paper planning, and the app is far from empty: a lot of real work happens here, much of it on the front end. Stories surface settings the user changes, a directory listing, a live value from the engine, a bar asking to be the default torrent app — each closing its own loop end to end, UI included.

What the plan intentionally leaves for later is the core feature itself: making a torrent, adding a torrent, downloading a torrent. Where a user story calls for it, a thin slice of adding a torrent appears — but only as much as that story specifies, just enough to confirm the shell is wired correctly, never the full experience. That deferral is the whole point of the ordering. This is outside-in, not a smallest-MVP-first march from a crude downloader toward a polished one. The usual instinct is to build "add a torrent and watch it download" first — it is the reason the app exists — then bolt on updates, portability, associations, and crash recovery afterward. We do the reverse: finish the surrounding platform machinery first, because it is the part that is hardest to change once people depend on it, and add the transfer experience last, onto a foundation already decided and trusted.

## Design principles

We wrote this plan to align design with reality. Where a feature touches the operating system, the framework, or the engine, we checked how those things actually behave — how Tauri exposes drag-and-drop, how macOS reverses (or can't reverse) an install, how libtorrent maps a router port — and shaped the design to fit. The mistake cuts both ways: it is wrong to design carelessly, and just as wrong to design something elegant that runs against the grain of what the platform allows. We would rather meet a constraint here, on paper, than collide with it in code. Implementation will still surface wrinkles the research missed; when it does, we bring it back to the plan and talk it through, rather than quietly bending the design until it breaks.

Good design here means a few specific things. It puts the user in control. It ships sensible defaults, so that someone with no opinion about a setting never has to form one — the default is the answer. And for the user who does care, who wants an outcome the default doesn't give, it opens the whole topic: every control, and enough explanation to understand it. We do not dumb settings down. A simplified surface that hides the real levers serves no one — the person who wanted simplicity already had the default, and the person who went looking for the lever deserves to find it in full, not a watered-down version. The same screen can carry someone from needing to know nothing to understanding everything about that corner of the app.

Design also favors observability before control: before ftorrent offers to change something, it shows what is currently true. The status surface is built on this. Rather than presenting a bare button to claim the `.torrent` file association, it first reports who holds that association now — named by path, or by whatever the system will tell us — and it says so honestly even when the answer is this installed copy of ftorrent. A control to change something the user cannot first see asks for blind trust; showing the current state first makes every change an informed one.

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

**Router and pages.** Add Vue Router to `package.json` and configure it for hash mode, which is the correct choice for Tauri since there's no server to handle `pushState` URLs. The app has at least two navigable pages — main and about — and the about page is written as a lazy route, so the build splits it into a chunk of its own and the router's code splitting shows up in the build output. The frontend dependency pipeline needs no separate proof: Vue and `@tauri-apps/api` already resolve out of `node_modules` and are bundled into the app that runs, which is the path any further package would take.

**Filesystem commands.** Port Fuji's `io.rs` into the Tauri command layer as `disk.rs` (renamed to avoid ambiguity with network I/O), bringing all commands from the start: `disk_readdir`, `disk_stat`, `disk_read`, `disk_copy`, `disk_write`, `disk_rename`, `disk_unlink`, `disk_rmdir`, `disk_mkdir`. All are callable from Vue through `invoke()`. These are the app's own commands rather than a plugin's, so they need no permission entries in Tauri v2's capability configuration — that system governs what a plugin exposes, not what the app writes for itself. The main view surfaces a directory listing — the user pastes a path, the app lists contents below using `disk_readdir` and `disk_stat` over IPC — as proof the full pipeline works. The remaining commands are ported and available but not individually surfaced in the UI at this stage. Smoke test: paste a directory path and confirm its contents appear; confirm all commands are registered and invocable from the frontend console. As built: seven commands are registered, `disk_readdir`, `disk_stat`, `disk_read`, `disk_write`, `disk_mkdir`, `disk_copy`, and `disk_hide`, which sets the hidden attribute on Windows and does nothing elsewhere; `disk_rename`, `disk_unlink`, and `disk_rmdir` are sketched at the bottom of `disk.rs` for the feature that needs them. They carry no guards, on purpose: the Rust core's commands stay general, and the page, which runs only its own code, decides what's right to do with them (§Security). The directory-listing page is not yet built.

**Take libtorrent (all platforms).** Written as two from-source build stories, one for macOS and one for Windows, and overtaken before either began: libtorrent 2.1 shipped in July 2026 with WebTorrent on by default, and its maintainers publish wheels for every platform we ship. So the story became a lockfile rather than a build. `desktop/engine/uv.lock` pins the libtorrent 2.1.1 wheel, the Python 3.13 interpreter, and PyInstaller for macOS, Windows, and Linux by SHA-256, and `uv sync --frozen` fetches and verifies them on any machine. The libtorrent provenance document on this site records the chain of custody and the hashes. Validated on macOS by importing libtorrent from the wheel, printing its version, confirming the WebTorrent settings are present, and announcing over `wss://open.ftorrent.com`; the same lock built and ran in Linux containers for both architectures. Windows followed on 2026-Sep-22: the same lock fetched the Windows wheel and interpreter, and the frozen engine there printed a ready line identical to the Mac's.

**Engine.** Tauri's word for a bundled helper process is sidecar; ours is the engine, and the process is named `ftorrent-engine`. As built: PyInstaller freezes the Python program, the interpreter, and libtorrent into a folder, an executable beside an `_internal` directory, rather than a single self-extracting binary, because the single-file shape is what antivirus heuristics on Windows most often flag. No custom hook was needed; the wheel installs like any package and PyInstaller collects it. The folder is carried as a resource directory in `tauri.conf.json` rather than through `externalBin`, so there is no target-triple suffix and one path works on every platform: inside the bundle on macOS, beside the executable on Windows, under `/usr/lib/ftorrent` on Linux. The Rust core starts it at setup with the standard library's process API, without the shell plugin, sends it `init`, carries every other line between the page and the engine without reading it, and stops it from the `ExitRequested` and `Exit` run events. The main page shows the engine's status. Smoke test, passed on macOS and on Windows: launch the app and confirm `ftorrent-engine` runs as its child; quit the app, through its own quit event or killed outright, and confirm no engine process survives. On Windows the engine also ran with no console window behind the app, from the development build and from the installed one, and Windows Defender raised nothing at any step. The Linux packages carry the same folder, and their engine answered on a bare Debian 12. The desktop architecture document on this site describes the processes, the files, and the road between the page and the engine as built.

**IPC end to end.** Wire up NDJSON communication between the Tauri Rust core and the Python sidecar over stdin/stdout. The Rust core's first message to the sidecar on startup is an `init` command containing the product name, the version, and the paths the sidecar needs to operate: the data folder and the state file location. The download folders are a setting, so the page sends them in a `folders` command once it has read `ftorrent.toml`. The sidecar loads session state, scans `.ftorrent` directories in each reachable download folder, loads resume data, starts the libtorrent session, and emits a `ready` event on stdout. From there, the page builds each command as a line of JSON, which the Rust core writes to the sidecar's stdin without reading it; the sidecar emits NDJSON events on stdout (progress, peer updates, alerts), which wait in a queue in the Rust core until the page takes them. The sidecar keeps one dispatch table, one entry per command, where JSON meets libtorrent, so a new libtorrent call is two edits, the page and the table, and no Rust. The main view displays a live libtorrent session value — like DHT node count or listen port — proving the full path works. Malformed JSON from either side is handled without crashing. Smoke test: launch the app, confirm the sidecar receives init, emits ready, and the UI shows a live session value; send a malformed JSON line and confirm no crash. Built so far: the road both ways, `engine_send` down and `engine_take` up, the `init` and `ready` exchange, the `folders` command and its echo, and the engine answering a malformed or unknown line with an `error` line; the session and the live value are still to come.

**Icons and graphics.** Produce all icon and graphic assets per §Icons and graphics and wire them into the build. One SVG drawing, seen through three viewBoxes: full-bleed for Windows/Linux, inset for the macOS Dock, and inset further for the Windows 10 Start tile, with the build taking only `icon.icns` from the second and the tile images from the third. Tray icons provided directly: monochrome template for macOS, small color for Windows/Linux, with the Rust core overriding to template mode at runtime on macOS. A document icon for `.torrent` files on Windows, wired to the file association; macOS and Linux draw a document icon themselves. DMG background. Windows 10 `VisualElementsManifest.xml` and tile PNGs placed beside the executable as bundle resources. Website favicon, an SVG, in each site. As built: the application icon, the document icon, the Start tile, and the DMG background; the tray uses the application icon until it has its own. Smoke test: confirm macOS Dock icon matches neighboring app sizing; confirm tray icon is legible in both light and dark menu bar; confirm a `.torrent` file shows the document icon in Finder and File Explorer; confirm DMG opens with background and correctly positioned icons; confirm Windows 10 shows the branded Start tile.

### Distribution

**Installers.** Configure Tauri to produce the packages ftorrent ships: a setup `.exe` for Windows, `.dmg` for macOS (Apple Silicon only), and for Linux a `.deb` for each of two architectures, an `.rpm`, and a Flatpak. Disable Tauri's default `.AppImage` output. This story first said we would ship `.deb` only, written when the Linux packages were expected to be built by hand on a Linux machine, the way the Windows package is built on a Windows one; the Docker pipeline in the repository's Linux guide made all four Linux packages as cheap as one, so that limit is retired. The Windows setup program runs completely silently with no wizard UI, per user, so no UAC prompt appears. The DMG uses the background image and icon positions from the icons story. Build artifacts are renamed from Tauri's versioned filenames to `ftorrent.exe`, `ftorrent.dmg`, `ftorrent.amd64.deb`, `ftorrent.arm64.deb`, `ftorrent.x86_64.rpm`, and `ftorrent.x86_64.flatpak` for website distribution. As built: the `.dmg` is made by [dmgbuild](https://github.com/dmgbuild/dmgbuild) from the `.app` Tauri leaves, rather than by Tauri, so no build drives Finder; the Linux packages are made on the Mac in Docker containers, the two `.deb` files and the `.rpm` by Tauri and the Flatpak wrapped from the x86-64 `.deb`, engine and all; every Linux name carries its architecture, in each ecosystem's own word for it; and the Windows `.exe` is made by ftorrent's own setup program and creator in the repository's `desktop/win-setup`, in place of Tauri's NSIS installer: a 64-bit C program with no interface that unpacks a cabinet Windows' own `makecab` packed from the end of its own file, asks a running copy to exit first, writes the Start menu shortcut and the uninstall entry, and is its own uninstaller; a Node script after `tauri build` stages, packs, compiles, and appends. The desktop entry the `.deb` and `.rpm` install declares the categories `Network;FileTransfer;P2P;`, which is what qBittorrent and Transmission declare, so launchers file ftorrent beside them. Smoke test: confirm the Windows setup program installs with no visible UI and the app launches from `AppData\Local\ftorrent\`; confirm the DMG opens with the drag-to-Applications layout; confirm the `.deb` installs and launches on Ubuntu, the `.rpm` on Fedora, and the Flatpak on SteamOS.

**Uninstall.** Remove ftorrent the way each platform expects, with the user in charge: the uninstaller on Windows (listed in Add or Remove Programs), dragging `ftorrent.app` to the Trash on macOS, `apt remove` or `dnf remove` for the Linux packages and `flatpak uninstall` for the Flatpak. Only the Windows uninstaller and the Linux package scripts run code, so only they actively reverse changes — clearing the `HKCU` autostart and handler entries, and offering to remove the machine-wide firewall exemption (which elevates, as adding it did). macOS drag-to-Trash runs nothing, so ftorrent's job is to never leave anything that misbehaves: the login item is registered through the OS's ServiceManagement framework so it does not orphan when the app is deleted (§Start at login), and everything else it leaves is small, standard, and inert. The user's data is always preserved — downloaded files, the `.ftorrent` session directories beside them, and `ftorrent.toml` and `state` all remain, so uninstalling never deletes a download and reinstalling resumes cleanly. A user who wants ftorrent's system changes gone first can flip each off in the status surface (§System status and permissions) before removing the app. A portable copy has nothing to uninstall — delete the folder; ftorrent's own code never wrote to the host. Smoke test: on Windows enable start-at-login, claim the default handler, add a firewall exemption, run the uninstaller, and confirm all three are reversed; on macOS enable start-at-login, drag the app to the Trash, and confirm no login item is left pointing at the missing app; on every platform confirm downloads, `.ftorrent` directories, settings, and state survive and a reinstall resumes; delete a portable folder and confirm the host is untouched.

**Portable mode.** Implement portable detection and path resolution per §4. On startup, look for `portable/ftorrent.toml` alongside the program before choosing the platform data directory. If found, all state — settings, lock, session state, crash log, and WebView2's profile on Windows — stays under `portable/`, and ftorrent's own code never writes the platform data directory. `./` paths resolve relative to the executable on Windows and Linux, and relative to the `.app` bundle on macOS. The build produces `ftorrent.zip` with Windows and macOS in one folder and `portable/ftorrent.toml` defaulting downloads to `./downloads`. `start_at_login` is grayed out in portable mode. As built: detection, path resolution, and the data folder with WebView2's profile inside it; the zip is not yet built. Smoke test: launch from a USB stick on both macOS and Windows, confirm all state stays on the stick and nothing is written to the host; add a torrent, move the stick to the other platform, confirm it resumes; confirm installed and portable run side by side without lock collision.

**Automatic update.** Implement Tauri's built-in updater with Ed25519 signing. The build generates a keypair once; the public key ships in `tauri.conf.json`. Each release publishes a static JSON manifest per platform at `ftorrent.com/update/{target-triple}`. The app checks on launch and every 24 hours, failing silently if offline. When a newer version is verified, a non-modal text indicator appears at the bottom of the window — no popup, no interruption. On click, confirm, close, apply silently, relaunch. On Linux, and for any copy not running from where the installer put it, the indicator opens `ftorrent.com` in the browser instead. An `update_check` setting in `ftorrent.toml` defaults to `true`; when `false`, no requests are made and no indicator appears. Smoke test: host a manifest locally with two versions, confirm detection, indicator, and successful apply-and-relaunch; confirm a bad signature is rejected; confirm `update_check: false` suppresses everything.

### Lifecycle

**System status and permissions.** ftorrent's standing with the operating system — default handler for `.torrent` and `magnet:`, external reachability and firewall, file-access permissions, start at login — is gathered into one status surface that always shows the current state of each in plain language ("you are externally contactable," "ftorrent is not your default torrent app"). These checks run on startup, never at install, and nothing about them blocks the app from running. ftorrent is never pushy: there are no demanding popups. Each item the user could change carries a short explanation and, where applicable, a button that asks ftorrent to make the change for them — and where that hands off to an OS confirmation, ftorrent warns first — and/or written instructions to do it by hand. For a copy not running from where the installer put it — a portable copy, a development build, a copy on the Desktop — the surface is read-only: it reports status but offers no changes and writes nothing to the host. Smoke test: launch installed and confirm the surface reports each integration's real current state; use one item's button and confirm ftorrent warns about the impending OS prompt before invoking it; confirm a fresh install has taken no system action on its own; confirm a portable copy shows status without offering to change anything.

**Start at login.** When the user enables `start_at_login` in settings, ftorrent registers for launch at OS login; disabling it unregisters. On Windows and Linux this is `tauri-plugin-autostart` (a `HKCU\Run` value, an XDG `.desktop` file), both per-user and cleanly removed on uninstall. On macOS, ftorrent registers through the system ServiceManagement framework (`SMAppService`) rather than dropping a raw LaunchAgent, so the login item is OS-managed, bound to the app, and does not orphan when the user later drags the app to the Trash (§Uninstall). The UI reflects the current state. For a copy not running from where the installer put it, the setting is grayed out and nothing is registered. On macOS the first enable may require approval in System Settings → Login Items — expected, and noted in the settings UI.

**File and protocol associations.** Register ftorrent as a handler for the `.torrent` and `.ftorrent` extensions and the `magnet:` and `ftorrent:` URI schemes, so after install ftorrent appears in the OS "Open with" menu without becoming anyone's default. When the OS routes a torrent to ftorrent, whether as a launch argument at cold start or to a running instance, it reaches the page, which will start the add-torrent flow; the instance handoff routes a magnet clicked while ftorrent is already open to the existing window. On Windows the installed copy registers itself under `HKCU` at startup, after every answer, and whenever its window comes back into focus, offering itself for all four, and claiming all four only on the user's yes; on macOS the types and schemes are declared in `Info.plist`, and opens arrive as Apple Events through `RunEvent::Opened`. As built, on Windows: the registration, the question of being the default, the uninstaller's cleanup, and a launch argument and a second launch's handoff both arriving in the page's request list; on macOS: the declarations, opens arriving in the same list at a cold start and while running, and the question of being the default, answered through Launch Services. Becoming the default is separate and never automatic, and one answer covers all four: unless the answer is no, a bar across the top of the window asks whenever the system opens any of the four with another program, with yes, no, and a close that keeps it down until the next launch, and the answer is one setting, `[associations] default`, which the Settings page changes any time. ftorrent writes only on the user's answer; at startup and on focus it only reads, apart from the first focus after a yes given again on Windows sent the user to Settings, which writes under that yes, so a type another program takes later, perhaps by the user's own choice, brings the question back rather than being taken back. A yes writes per-user state only, and where the system has saved the user's choice of another program, or on Windows where a yes is given again because another program took a type back after the first, sends the user to the system's own settings to finish; ftorrent reads what the system would open each type with and follows it, and a system that already opens all four with ftorrent leaves nothing to ask. For a copy not running from where the installer put it, the choice on the Settings page is grayed out and nothing is declared or claimed. The §File and protocol associations section covers the per-platform mechanics. Smoke test: confirm the bar appears at startup while the answer is ask; say yes and confirm the system opens all four with ftorrent, finishing in its own settings where it had saved another program; give one type to another program and confirm ftorrent doesn't take it back, and the bar asks again; say no and confirm ftorrent gives back what it claimed and leaves the system's saved choices as they are; on Windows, after a yes, let another program rewrite a type's fallback, say yes again, and confirm Settings opens showing that program on that type, then choose ftorrent there, come back, and confirm the system opens all four with ftorrent; close the bar and confirm it stays down until the next launch; double-click a `.torrent` file and click a magnet link and confirm both open ftorrent and start the add-torrent flow; launch cold from a magnet and confirm the torrent arrives at startup; click a second magnet while running and confirm the existing window receives it; confirm a portable copy declares and claims nothing.

**Drag and drop.** The user can drag a `.torrent` file from Finder, File Explorer, or a Linux file manager onto the ftorrent window and ftorrent adds it. Tauri's native drag-and-drop delivers the dropped file's path to the Rust core, which routes a `.torrent` to the sidecar as the same add-torrent command the open-event path uses (§File and protocol associations) — a double-click, a clicked magnet, and a file drop all converge on one entry point into the engine. The window shows a drop affordance while a drag is over it, and an unrelated payload is ignored without error. Dragging a *magnet link* (which is dropped text, not a file) is a deliberate open question: Tauri makes native file-drop and webview drop mutually exclusive, so supporting it could degrade the reliable file-drop we get for free — the trade-off and the condition for adding it are in §Drag and drop, and for now magnets arrive by click and paste. Smoke test: drag a `.torrent` from the file manager onto the window and confirm it is added; confirm the drop affordance appears during the drag and clears after; confirm an unrelated dropped file is ignored without error.

**Inbound connectivity and firewall.** ftorrent listens on a port for incoming peer connections. Outbound traffic always works; inbound depends on the host firewall, so ftorrent reports its state in the status surface — distinguishing what it can know locally (is a firewall exemption in place?) from true external reachability, which only an outside probe can confirm (§Inbound connectivity and firewall). Adding a firewall exemption is the one place ftorrent makes a system-wide rather than per-user change, and that is correct — the firewall is where this capability lives. On Windows, ftorrent offers a button that adds the exemption through an elevated call (a UAC prompt it warns about first) and instructions to do it by hand; a user with no administrator rights can't enable inbound at all and runs outbound-only. On macOS there is no supported API to add an application-firewall exemption — and that firewall is off for most users anyway — so ftorrent reports status and gives instructions rather than offering a button. It never forces the choice. The exemption is offered only for a copy running from where the installer put it; any other copy reports status but changes nothing on the host. Smoke test: on Windows behind a default firewall, confirm the surface reports no exemption; use the button, confirm the UAC warning precedes the prompt, accept it, and confirm incoming peers connect; decline and confirm downloads still proceed outbound-only; on macOS confirm status and instructions appear with no button; confirm a portable copy reports status without offering the change.

**Port mapping (UPnP and NAT-PMP/PCP).** ftorrent asks the router to forward its listen port inward so peers can reach it. libtorrent provides this through two mechanisms, each its own setting — `enable_upnp` and `enable_natpmp` (the NAT-PMP mapper also speaks PCP, NAT-PMP's successor; there is no separate PCP switch) — and ftorrent unifies them under one user-facing setting, `port_mapping` in `ftorrent.toml`, default `true`, which turns both on or both off. That matches the single "UPnP/NAT-PMP" control mainstream clients present, and gives a clean opt-out for users on managed networks, who consider it a risk, or who forward ports by hand. ftorrent reads libtorrent's port-mapping alerts — whose transport is one of two values, NAT-PMP or UPnP — and shows the result in the status surface (§System status and permissions): mapped, naming the mechanism that worked and the external port, or not mapped because the router declined or offers no such service. A successful mapping means the router cooperated — not that the internet can reach you, which is confirmed separately (§Inbound connectivity and firewall). Unlike the host-level integrations, a port mapping is a transient request the router expires on its own, so it is not gated like them — a portable copy maps its port too. Smoke test: behind a UPnP-capable router, launch and confirm the status names the mechanism (UPnP or NAT-PMP) and the external port; set `port_mapping` to `false` and confirm neither mechanism is attempted and the status reflects that; on a router with both off, confirm the failure is reported rather than hidden.

**File access (macOS).** On macOS Ventura and later, the system gates access to the Downloads, Desktop, and Documents folders and to removable and network volumes behind per-folder permission prompts, independent of code signing and declared with `NS…UsageDescription` strings in the app's `Info.plist`. ftorrent's default download folder sits in Downloads and portable copies run from removable USB volumes, so these prompts will appear. There is no API to query permission ahead of time — it is discovered by attempting access, and the attempt is what triggers the prompt — and since ftorrent must read its download folders at startup to resume torrents, that is when the prompt appears; ftorrent warns in its own UI just before. macOS will not re-prompt once denied, so on a prior denial ftorrent reports the block in the status surface and gives instructions to grant access in System Settings → Privacy & Security. On Windows and Linux this is a no-op for a user writing within their own profile: the surface reports full access. Smoke test: on a clean macOS account, point a download folder at Downloads and confirm ftorrent warns before the access attempt that triggers the system prompt; grant access and confirm writes succeed and status reads clear; deny it and confirm the surface explains the block and links to the Settings instructions; run a portable copy from a USB volume and confirm the removable-volume prompt is anticipated the same way.

**Instance lock.** Enforce single-instance per data folder. After resolving which data folder to use, take an exclusive lock on `ftorrent.lock` inside it, with the standard library's `File::try_lock` on every platform; the file stays empty and separate from `ftorrent.toml`, which is replaced whole on save. If the lock is already held, hand this launch's arguments to the running instance, which brings its window forward, and exit — on Windows through a named pipe the running instance serves, named from the lock file's path, and on macOS through Launch Services, which brings a running app forward and delivers opens to it itself. This replaces Tauri's single-instance plugin, which keys on bundle ID and would incorrectly block installed-plus-portable from running side by side. Each download folder is locked the same way, on `.ftorrent/ftorrent.lock`, so two copies never load the same torrents. Smoke test: launch ftorrent, launch it again from the same shortcut, confirm the existing window comes forward and no second instance appears; then launch a portable copy alongside the installed copy and confirm both run independently; then test two OS users on the same machine and confirm no collision. As built on macOS and Windows: the lock, the handoff, and the download folder locks.

**Sleep detection and prevention.** A timer in the Rust core writes a timestamp every 60 seconds. If the next tick sees a gap longer than two minutes, the system slept — the Rust core tells the sidecar to re-announce to trackers. An optional `prevent_sleep` setting, off by default, holds a platform power assertion while ftorrent is running to prevent idle sleep. macOS and Windows only.

**Periodic saves.** The sidecar periodically writes resume data while running, not only at shutdown. When libtorrent fires a `save_resume_data_alert` — triggered by state changes like piece completion, tracker replies, or peer list updates — the sidecar serializes the resume blob to the torrent's `resume` file in its `.ftorrent` directory. This ensures crash recovery has something recent to fall back on. Without it, a crash means full re-check of every torrent from launch. Smoke test: start a download, let it make progress, kill the sidecar with `SIGKILL` (or `taskkill /F` on Windows), relaunch, confirm the torrent resumes near where it was rather than re-checking from zero.

**Sidecar crash and recovery.** The Rust core captures the sidecar's stderr to a 100-line ring buffer in memory (built). During normal operation, nothing is written to disk. If the sidecar exits with a non-zero code, the Rust core writes the buffer to `crash.log` in the application data directory (or `portable/`), overwriting any previous file. The UI surfaces a message indicating the engine has stopped — not a silent failure. The user can restart the sidecar from the UI without relaunching the app; on restart, the sidecar reloads session state and the most recent resume data written by periodic saves, and resumes normally. Normal shutdown produces no `crash.log`. Smoke test: trigger a deliberate sidecar crash via a debug command, confirm `crash.log` appears with the traceback and the UI shows the failure state; click restart in the UI and confirm the sidecar comes back and torrents resume; crash again and confirm `crash.log` is overwritten not appended; confirm clean exit produces no `crash.log`.

**Close and exit.** Clicking the window's close button hides the window — the webview stays alive, JS keeps running, the sidecar keeps transferring. The window is created once at startup and destroyed once at shutdown; hiding and showing is the only transition during normal use. On Windows, the tray icon indicates ftorrent is still running, clicking it shows the window, and its menu has Show and Exit; the window has the classic menu bar, File with Exit, Tools with Options, and Help with About. On macOS, the app remains in the Dock, and clicking it brings the window back; on a fullscreen window, the red button first leaves fullscreen, since a hidden window would keep its Space, empty, and a second click hides it. On Linux, the app remains in the taskbar. Quitting from the tray menu, the File menu, the Dock menu, or `⌘Q` triggers graceful shutdown. A torrent client that disappears when you close the window is broken. As built on macOS and Windows; on Linux, closing still quits.

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

>window size and position
built, and the notes that were here are settled. the window is made hidden in code, and the page places it: the saved rectangle is replayed exactly when the screen it was recorded on is still there with the same position, size, and scale, and otherwise the window gets a fresh place, five eighths of the primary screen by half, at a random spot in the centered three-quarter field, which keeps it off any taskbar or dock without asking where they are and keeps an installed and a portable copy from stacking. a portable copy records no place and always opens fresh, never maximized. geometry lives in ftorrent.toml under [window] and [screen], in css pixels, so tauri-plugin-window-state stays out. still no minWidth or minHeight.

>first example features
magnet link maker and inspector
good.ftorrent.com client for connection nat cone cgnat ipv6 status checker
torrent maker

>webtorrent reach, and an apex demo
joining browser WebTorrent swarms over WebRTC is already core below — the hybrid `libtorrent webtorrent=on` engine puts desktop and browser peers in one swarm, not two separate networks. flagged here only as a forward thread, not a new requirement.
the apex (ftorrent.com) may also *demonstrate* WebTorrent in the browser itself, as one of several fragment-payload features alongside the magnet landing page and the magnet analyzer/editor (see roadmap-in-reverse). the prior art is Wormhole (wormhole.app) by Feross — WebTorrent's own creator: browser file-sharing over WebTorrent, the AES-GCM key carried in the URL `#fragment` so the server never sees it, files pulled from WebTorrent peers and a 24-hour backup super-seeder at once. the native desktop client and an apex browser demo would meet in the same WebTorrent swarm — the same span, native and browser alike.

## Stack

This is a desktop BitTorrent client that participates fully in both traditional BitTorrent swarms (TCP/uTP) and browser-based WebTorrent swarms (WebRTC). The architecture is Tauri + Vue on the frontend, with a Python sidecar wrapping libtorrent on the backend.

**Tauri** is the application framework. It provides a native webview, a Rust core for system-level operations, and a well-defined sidecar mechanism for embedding external binaries. Tauri was chosen over Electron because it uses the operating system's native webview rather than shipping a full Chromium instance, resulting in dramatically smaller binaries, lower memory usage, and a smaller attack surface. Its Rust core also provides a strong security model: Tauri v2's capabilities system requires explicit, scoped permission grants for everything the frontend can access, including sidecar execution and argument validation.

**Vue** is the UI framework running inside Tauri's webview. It was chosen as the team's strongest frontend framework and pairs naturally with Tauri, which is frontend-framework-agnostic. The UI communicates with the Tauri Rust core via Tauri's built-in IPC commands.

**Vue Router** is used for top-level view management even though the app has no visible location bar. This is standard practice in Tauri and Electron apps — a client-side router provides lazy-loaded route components, route guards for flows like first-run setup, and a single declarative file (`router/index.js`) that serves as a readable table of contents for the app's view structure. The alternative of swapping components via `v-if` or dynamic `<component :is>` works for trivial cases but degrades quickly, leading teams to reinvent history stacks, guard logic, and lazy-loading wrappers — a router in all but name. Vue Router runs in hash mode (`createWebHashHistory()`) since there is no server to handle `pushState` URLs; the hash fragment is purely an internal concern, invisible to the user in a chromeless Tauri window.

**libtorrent** (arvidn/libtorrent, aka libtorrent-rasterbar) is the BitTorrent engine. It is the most mature, full-featured open-source implementation of the BitTorrent protocol, with twenty years of development, and it powers major clients including qBittorrent and Deluge. It supports the full modern BitTorrent protocol suite: DHT, PEX, magnet links, uTP, protocol encryption, IPv6, and web seeds. Critically, libtorrent is the only C/C++ BitTorrent library with first-party WebTorrent support, allowing a single engine to serve both traditional TCP/uTP peers and WebRTC-based browser peers as a hybrid client with a unified piece store and unified swarms. Pure-Rust alternatives like librqbit were evaluated but lack WebTorrent support entirely.

**libtorrent comes from its maintainers' wheels, not from a build of our own.** When this plan was first written, WebTorrent support lived only on libtorrent's development branch behind a build flag, and the plan was to build from that branch with `webtorrent=on` on every platform. libtorrent 2.1.0, released 2026-Jul-09, made WebTorrent a release feature enabled by default, and the maintainers publish Python wheels for each release from a workflow in libtorrent's own repository. The client pins the 2.1.1 wheels by hash in `desktop/engine/uv.lock`, one per platform, and takes them as they are. What the project still carries is the watching: OpenSSL is compiled into or shipped beside those wheels at a version that differs by platform, so a new libtorrent release is worth taking promptly. The libtorrent provenance document records the versions, the hashes, and what we verified.

**Python** serves as the bridge between the Tauri Rust core and the libtorrent C++ library. libtorrent's C++ API uses templates, virtual classes, and Boost types that are impractical to call directly via Rust FFI, and the only existing Rust binding crate (libtorrent-sys) is unmaintained and covers a tiny fraction of the API. However, libtorrent maintains first-party Python bindings in its own repository, built as part of the same compilation step as the C++ library, exposing nearly the full API surface including WebTorrent features. No equivalent bindings exist for JavaScript or any other language. Python is not a preference — it is the only maintained, first-party bridge available.

The Python code, the interpreter, and libtorrent are frozen by **PyInstaller** into one folder, an executable beside an `_internal` directory, rather than a single self-extracting binary, for the reason the engine story gives. Tauri carries that folder as a **resource** directory rather than through its `externalBin` sidecar mechanism, which handles single files named with target-triple suffixes; a resource directory needs neither.

**Communication between Tauri and the sidecar uses stdin/stdout pipes**, not a local HTTP server. OS-level pipes between a parent and child process cannot be connected to or eavesdropped on by other processes on the same machine (absent root-level access). A localhost HTTP server, by contrast, binds a TCP port visible to any local process. The sidecar emits newline-delimited JSON (NDJSON) on stdout for events (progress, peer updates, alerts) and accepts JSON commands on stdin (add torrent, pause, configure). The Rust core spawns the engine with the standard library's process API and holds its three pipes; no shell plugin is registered, so nothing the page can call spawns a process.

The resulting architecture, from top to bottom:

```
Vue UI (Tauri webview)
  ↕  Tauri IPC (engine_send down, engine_take up, and general commands)
Tauri Rust core
  ↕  stdin / stdout (NDJSON)
Python engine (PyInstaller folder)
  ↕  Python bindings (in-process, Boost.Python)
libtorrent 2.1.1 C++ (the maintainers' wheel, WebTorrent on)
  ↕  TCP / uTP + WebRTC
Traditional and browser peers
```

## Security

The app exposes POSIX-style filesystem commands through Tauri IPC: `disk_readdir` (list directory contents), `disk_stat` (file metadata), `disk_read` (read file bytes), `disk_write` (write file bytes), `disk_mkdir` (make a directory), `disk_copy` (copy a file), and `disk_hide` (set the hidden attribute on Windows), with `disk_rename`, `disk_unlink`, and `disk_rmdir` to follow when a feature needs them. Together these give the webview effectively the same filesystem access as a native desktop application — arbitrary reads, writes, and directory manipulation across any path the user account can reach.

Tauri deliberately does not ship this kind of access as a built-in. Traditional desktop frameworks like Win32 or Qt grant full filesystem access because all running code is compiled by the developer — there is no mechanism for foreign code to appear at runtime. A webview is a browser engine, and if an application loads remote content or renders unsanitized HTML, injected script runs with whatever privileges the framework has exposed. Tauri protects against this by scoping its filesystem plugin to specific directories, so that apps built by teams who may not fully understand the threat model don't ship dangerous defaults. For our app, we bypass those guardrails intentionally by writing our own Rust commands and exposing them through IPC.

The entire security posture depends on preventing foreign script from executing in the webview, because IPC is trust-by-origin — any script running in the webview can call `invoke()` exactly as our own Vue components do, and the Rust core cannot distinguish between them. The threat is concrete: if a torrent were named `<img onerror="invoke('disk_unlink',{path:'/'})" src=x>` and that string were inserted into the DOM via `innerHTML` or `v-html`, the attack payload would execute with full filesystem access.

Our app closes this vector at two layers. First, the webview serves only our own bundled assets from Tauri's custom protocol (`tauri://` / `asset://`), never from `http://` or `https://` origins — there is no path for remote content to enter the renderer. Second, all untrusted data from trackers, peers, and the user is rendered through Vue's template interpolation (`{{ }}`), which calls `createTextNode()` under the hood and produces plain text nodes, never parsed HTML. The rule is simple: never use `v-html` with data that originated outside the app. CSP with compile-time nonces is the backstop — even if something somehow bypassed Vue's escaping, an injected script without a valid nonce would not execute.

This is also why the Rust commands carry no guards of their own. Every command the Rust core offers is general, like one in any desktop application's API, and the page holds all of ftorrent's logic about when and why to use it. A guard in Rust would be a second copy of that logic on the other side of the boundary, and it would protect against nothing the two layers above don't already stop.

The sidecar runs as a regular OS child process with the user's own filesystem permissions — Tauri's security model governs the webview, not child processes. libtorrent reads and writes freely without any special configuration, and this is secure because the trust boundary sits between the webview and the Rust core, not between the Rust core and its children.

A remaining theoretical vector is a compromised npm dependency importing `@tauri-apps/api` and calling `invoke()` from inside our own bundle, where it would pass CSP and origin checks. We set this aside by choosing a minimal, curated frontend dependency tree — Vue, Vue Router, and a small number of well-known, widely-audited packages. This is not a plugin system, not an extension platform, and not an app that loads third-party widgets. The attack surface is narrow by design.

## Paths

### Paths §1: Built packages and setup files

**Packages.** ftorrent ships four packages:

- `ftorrent.exe` — setup program for Windows 10 and later.
- `ftorrent.dmg` — macOS disk image, supporting Sequoia and Tahoe, Apple Silicon only.
- `ftorrent.amd64.deb` and `ftorrent.arm64.deb` — Debian packages for Ubuntu Desktop and its derivatives (Mint, Pop!_OS, Zorin, elementary), which together account for the majority of desktop Linux users, and for Raspberry Pi OS on ARM.
- `ftorrent.x86_64.rpm` — RPM package for Fedora, RHEL, Rocky, and AlmaLinux.
- `ftorrent.x86_64.flatpak` — Flatpak bundle for any distribution, sandboxed, and the only package that installs on SteamOS and Bazzite, whose root filesystems are read-only or atomic. Arch users install the Flatpak.
- `ftorrent.zip` — portable distribution for Windows and macOS, described separately below.

During development, macOS and Windows are the active test matrix — the platforms where daily work and testing happen. Linux is a first-class target but not a hot path. The expectation is that by keeping choices simple and standard, Tauri's Linux build will work at the end with little or no correction. As built, it did, and with no Linux machine: the four Linux packages come out of Docker containers on the Mac from one command, so Linux is built on every release rather than when someone sits down at a Linux box.

On Windows, the entire installation is per-user — binary, settings, and downloads all live under the user's home directory, with no UAC elevation prompt during setup. On macOS and Linux, the binary is shared system-wide but all state is per-user: settings, session data, and downloads are separated by OS account. This follows platform conventions on all three systems, and two users on the same machine never interact with each other's ftorrent data.

One setting in `tauri.conf.json` is written down rather than left to Tauri's default: the bundle targets are narrowed to two, `app` for the Mac and `deb` for a Linux checkout, where Tauri's default of `"all"` builds everything each platform can produce, which adds an `.AppImage` on Linux and an `.msi` and an NSIS installer on Windows. Windows names no target, because its installer is not Tauri's: `desktop/win-setup` makes it from the executable `tauri build` leaves, and the per-user install that is load-bearing for everything else in this document is the program's own fixed behavior rather than a setting. The Linux pipeline asks its own containers for the `.deb` and `.rpm` as an argument, and wraps the Flatpak from the `.deb`, so the list in the config file never has to name them. It is a one-line change in the config file. The `.dmg` is made from the `.app` by [dmgbuild](https://github.com/dmgbuild/dmgbuild), not by Tauri, as §Icons and graphics describes. Everything else in the distribution and filesystem layout follows Tauri's out-of-box defaults.

**Build output.** In the `ftorrent` monorepo's `desktop` workspace, the build produces installer artifacts for Windows, macOS, and Linux as follows: Tauri writes the `.exe`, `dmg.js` writes the `.dmg` beside it, and the containers write the Linux ones into the nested `linux` workspace's release folder:
```
ftorrent/desktop/src-tauri/target/release/bundle/win-setup/ftorrent_0.1.0_x64-setup.exe
ftorrent/desktop/src-tauri/target/release/bundle/dmg/ftorrent_0.1.0_aarch64.dmg
ftorrent/desktop/linux/release/ftorrent_0.1.0_amd64.deb
ftorrent/desktop/linux/release/ftorrent_0.1.0_arm64.deb
ftorrent/desktop/linux/release/ftorrent-0.1.0-1.x86_64.rpm
ftorrent/desktop/linux/release/ftorrent_0.1.0_x86_64.flatpak
```
The `.dmg` contains `ftorrent.app` for the user to drag into Applications. The `.exe` is the setup program, not the application binary itself — it extracts and installs the app into the program files location described below. These are renamed to `ftorrent.dmg`, `ftorrent.exe`, `ftorrent.amd64.deb`, `ftorrent.arm64.deb`, `ftorrent.x86_64.rpm`, and `ftorrent.x86_64.flatpak` for distribution on the website.

### Paths §2: Installed and running

**Program files.** Where the application binary lives after installation:
```
C:\Users\username\AppData\Local\ftorrent\       # Windows, per-user
/Applications/ftorrent.app/                     # macOS, system-wide
/usr/bin/ftorrent                               # Linux, system-wide
```
On macOS, the `.dmg` presents `ftorrent.app` for the user to drag into `/Applications/` in the standard way. On Windows, the setup program is per user and places files under `AppData\Local\ftorrent\` — no admin privileges required. On Linux, the `.deb` installs to `/usr/bin/`.

**Application settings.** ftorrent's own settings file, keyed by the Tauri bundle identifier:

```
C:\Users\username\AppData\Local\com.ftorrent.ftorrent\ftorrent.toml      # Windows
~/Library/Application Support/com.ftorrent.ftorrent/ftorrent.toml        # macOS
~/.local/share/com.ftorrent.ftorrent/ftorrent.toml                       # Linux
```

`ftorrent.toml` contains user-facing configuration: the ordered list of download folders, the window's place, UI preferences, and any other settings the user knows about and can change. It's TOML because it's meant to be opened in a text editor, and JSON can't carry comments: every setting is written out under a comment explaining it, so the file documents every knob ftorrent has. The page owns it, reading it at startup and writing it whole when a setting changes. On Windows everything ftorrent keeps is in Local rather than Roaming, because it belongs to this machine: Tauri already keeps WebView2's profile in Local, and the DHT routing table in `state` is this machine's view of the network. The same folder holds `ftorrent.lock` and, on Windows, WebView2's profile in `EBWebView`. The Windows registry is avoided except where a feature absolutely requires it, like file type associations for `.torrent` files. On Linux, the base path follows the XDG Base Directory Specification — `$XDG_DATA_HOME` defaults to `~/.local/share/` on Ubuntu Desktop, and virtually no one changes it.

**Global libtorrent state.** Separate from user settings, libtorrent maintains session-wide state that persists across restarts:

```
C:\Users\username\AppData\Local\com.ftorrent.ftorrent\state              # Windows
~/Library/Application Support/com.ftorrent.ftorrent/state                # macOS
~/.local/share/com.ftorrent.ftorrent/state                               # Linux
```

This file holds the DHT routing table and session-wide settings — global state that belongs to the running instance, not to any individual torrent. It is serialized via libtorrent's `write_session_params()` on shutdown and restored with `read_session_params()` on startup. Without it, every launch would bootstrap DHT from scratch. Per-torrent state — resume data, piece completion, tracker lists — lives alongside downloads in `.ftorrent` directories, described below.

**Downloads.** Downloaded content — torrent data and the potentially large files and folder trees it describes:
```
C:\Users\username\Downloads\ftorrent\                       # Windows
~/Downloads/ftorrent/                                       # macOS
~/Downloads/ftorrent/                                       # Linux
```
The user can change this in `ftorrent.toml`, but the out-of-box default puts everything in a clearly named subfolder of Downloads rather than scattering files into the user's home directory. On Linux, `~/Downloads/` is the default value of `$XDG_DOWNLOAD_DIR`, a system variable that distributions can override — Ubuntu Desktop leaves it at the default.

### Paths §3: Download locations and session data

ftorrent does not keep a central session store. Session data lives alongside the downloaded files it describes — each download folder carries its own `.ftorrent` directory. If you move a folder, copy it to a different drive, or carry it on a USB stick, the session information travels with it.

`ftorrent.toml` maintains an ordered list of download folder paths:

```
[downloads]
folders = ["~/Downloads/ftorrent", "E:/big torrents"]
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

**Startup.** ftorrent reads `ftorrent.toml` and walks `folders` in order. For each reachable folder, it takes an exclusive lock on `.ftorrent/ftorrent.lock`, and a folder another copy of ftorrent already holds shows as in use and loads nothing. For each folder it holds, it scans `.ftorrent/`, loads each `resume` file with `read_resume_data()`, overwrites `save_path` on the returned `add_torrent_params` with the absolute path of the download folder it's currently scanning, and passes it to `session.async_add_torrent()`. Unreachable folders — drive not mounted, path doesn't exist — are skipped, but not removed; a future session may be able to reach that folder, and list the torrents there. (For instance, the user might be running ftorrent installed, but not have plugged in a removable high capacity usb drive.)

**Adding a torrent.** ftorrent creates an info hash subfolder inside the target download folder's `.ftorrent` directory, writes the initial resume data, and passes `add_torrent_params` to libtorrent with `save_path` set to that download folder.

**Removing a torrent.** ftorrent deletes the info hash subfolder from `.ftorrent/`. The downloaded files remain. The torrent disappears from the list. Re-adding it later triggers a piece re-check against the files on disk.

**Graceful shutdown.** ftorrent calls `save_resume_data()` on every active torrent, waits for each corresponding `save_resume_data_alert`, and writes the serialized blob to the torrent's `resume` file. Global session state — DHT routing table and session-wide settings — is saved separately to the `state` file in the application data directory.

**Path resolution.** libtorrent bakes `save_path` into resume data as an absolute path. ftorrent ignores it. The path is determined by physical location: `.ftorrent` lives inside the download folder that contains the files, so ftorrent always knows the correct path from the folder it's scanning. This means a drive letter change — Windows assigns `E:\` one day and `F:\` the next — is a one-line fix in `folders`. On next launch, ftorrent finds `.ftorrent` at the new path, sets `save_path` accordingly, and all torrents recover without re-checking pieces.

### Paths §4: Portable ftorrent, startup, and path defaults

**Paths in settings.** Paths in `ftorrent.toml` are written with forward slashes. Three forms are supported:

```
./    relative to the application directory
~     user's home directory
      absolute paths (e.g. D:/torrents)
```

`./` resolves relative to the application directory on every platform. On Windows and Linux, this is the directory containing the executable. On macOS, the running binary is inside `ftorrent.app/Contents/MacOS/`, but `./` resolves to the directory containing the `.app` bundle — the code detects that it's inside a bundle and walks up to the bundle's parent. This means `./downloads` resolves to the same level as `ftorrent.exe` on Windows, `ftorrent` on Linux, and `ftorrent.app/` on macOS — which is what the user expects. `~` resolves to `/Users/username/` on macOS, `C:\Users\username\` on Windows, `/home/username/` on Linux. Absolute paths are also allowed — a user who points downloads at `D:\torrents` stores exactly that. A user may wish to keep ftorrent portable on a thumb drive plugged into `G:\` which downloads and seeds torrents to a desktop USB drive on `H:\`, for instance. Relative and `~`-prefixed paths are portable across machines; absolute paths are intentionally machine-specific.

**Startup flow.** When ftorrent launches, it computes the path to its own running executable and looks for `portable/ftorrent.toml` alongside it — on Windows, next to `ftorrent.exe`; on macOS, next to the `ftorrent.app` bundle. If that file exists, ftorrent is portable and `portable/` is its data folder. If it doesn't, ftorrent is installed and uses the platform-standard data folder described in §2, creating it if missing. Anything that isn't portable is installed, whatever the reason: a development build, a copy on the Desktop, or a Mac app macOS translocates, running it from a random read-only folder because it still carries a download's quarantine mark. A translocated portable copy can't see its `portable/` folder, so it acts installed, and works; the portable instructions clear the mark first.

A second rule, separate from the first, decides whether a copy may change the host: only a copy running from where the installer put it registers file types and link schemes, and later starts itself at login or updates itself in place. On Windows that's `%LOCALAPPDATA%\ftorrent`. A copy on the Desktop or one run from the repository is installed by the first rule, but it shouldn't point the registry at a file that will move.

Once the page is up, it reads `ftorrent.toml`. A missing key takes its factory value, a bad value is reported and its factory value kept, and the repaired file is written back; a missing file is written whole, every setting under its comment. A file that won't parse, or can't be read, is reported and left exactly as it is, since it may be one typo from right, and ftorrent runs on factory settings until it's fixed. On a first run the downloads section reads:

```
[downloads]
folders = ["~/Downloads/ftorrent"]
```

A setting a newer ftorrent adds is filled in with its factory value and written out on the next save. Because every value is written out, the file can't tell an old factory value from a choice the user made, so a newer ftorrent that changes a factory default carries it over with a step of its own. Each line shows its factory value in a comment, so a user can see where theirs differ.

**What ftorrent.zip contains.** The portable distribution unpacks to a self-contained directory for Windows and macOS:

```
ftorrent/
ftorrent/ftorrent.exe                                               # Windows binary
ftorrent/ftorrent-engine/                                           # Windows engine folder, where an installed copy keeps it too
ftorrent/ftorrent.app/                                              # macOS app bundle, identifier com.ftorrent.portable
ftorrent/ftorrent.app/Contents/MacOS/ftorrent                       # macOS binary
ftorrent/ftorrent.app/Contents/Resources/ftorrent-engine/           # macOS engine folder
ftorrent/portable/
ftorrent/portable/ftorrent.toml                                     # triggers portable mode
```

Linux isn't in the zip. A bare Tauri binary on Linux depends on the system's WebKitGTK, which not every distribution installs, Linux ships on two architectures, and it sits outside the active test matrix, so Linux keeps its four packages. Leaving Linux out also keeps each engine folder where its app looks for it: beside `ftorrent.exe` on Windows, as in an installed copy, and inside the bundle on macOS. The portable Mac bundle has its own identifier, `com.ftorrent.portable`, and declares no document types or URL schemes, so Launch Services never sends a clicked magnet to a portable copy that happens to be running.

`portable/ftorrent.toml` is set during the build that produces the zip and contains:

```
[downloads]
folders = ["./downloads"]
```

The `./downloads` path resolves relative to the executable on every platform, so the default portable download location works regardless of drive letter or mount point. This is a single zip that works on Windows and macOS. The user plugs in a USB stick, launches the binary for their platform, and the same `portable/ftorrent.toml` governs both. Downloaded files, `.ftorrent` directories, and the `state` file are all platform-agnostic. A user can download a torrent on a Mac, eject the stick, plug it into a Windows machine, and resume seeding without re-checking pieces. Most portable app distributions are single-platform. A cross-platform portable BitTorrent client that carries its state between operating systems is, as far as we know, novel.

**Portable ftorrent writes nothing to the host.** A portable copy's own code never writes the platform application data directory. Settings live at `portable/ftorrent.toml`, the lock at `portable/ftorrent.lock`, global libtorrent state at `portable/state`, WebView2's profile on Windows at `portable/EBWebView`, and the default download location is alongside the executable. A user can explicitly point downloads at a host drive, but out of the box, everything stays on the stick. The operating system still keeps its usual records of any program that runs: WKWebView's folders named for the bundle identifier and Launch Services' registration on macOS, SmartScreen's approval and the shell's recent items on Windows, and a firewall rule on either once the firewall has asked about incoming connections. The installing page's portable section lists them plainly.

### Paths §5: Multiple running instances

Single-instance is enforced *per data folder*, which is the key to how the paths model handles multiple copies on one machine: the data folder a launch resolves to (§4's startup flow) is its instance identity. The same data folder means the same instance; different data folders run independently. The lock that enforces this — an exclusive lock on `ftorrent.lock` in the data folder, the same mechanism on every platform — is detailed in §Instance lock; here is what it means in practice.

If Alice double-clicks `ftorrent.exe` while her installed copy is already running, the second launch resolves to the same data folder, finds the lock held, hands what it carried to the running instance, which brings its window forward, and exits. She never sees two windows from the same installation — no error dialog, no delay.

Multiple *independent* instances are expected and allowed, because they resolve to different settings files:

- **Two OS users.** Alice and Bob share a Mac. Alice is signed in running her installed ftorrent; she locks the screen, Bob signs in and runs his. Separate accounts mean separate application data directories, separate `ftorrent.toml` files, and separate locks — no conflict, even with both accounts active at once via fast user switching.
- **Installed plus portable.** Alice runs her installed ftorrent and a portable copy from her USB stick side by side. One data folder is the platform application data directory, the other is `portable/` on the stick — different files, different locks, two windows each managing its own torrents. This is exactly the case Tauri's single-instance plugin would wrongly block, which is why ftorrent uses its own lock (§Instance lock).

### Paths §6: Example story: Portable Alice

The paths system is designed to be simple in its rules but flexible under strain — the same mechanics that handle a single-machine install with one download folder also handle a portable drive moving between platforms with machine-specific download locations, without special cases or user intervention. Consider this example: Alice carries ftorrent on a 2TB USB drive. She uses a Windows desktop at home and a Mac at the office.

She unzips `ftorrent.zip` to her USB drive and launches `ftorrent.exe` on her Windows desktop, where the drive mounts as `E:\`. She adds a few torrents — a Linux ISO and a documentary. Because `portable/ftorrent.toml` lists `./downloads` as its download folder, both go to `E:\ftorrent\downloads\`. Her drive looks like:

```
E:\ftorrent\ftorrent.exe
E:\ftorrent\portable\ftorrent.toml
E:\ftorrent\portable\state
E:\ftorrent\downloads\ubuntu-24.04\
E:\ftorrent\downloads\some-documentary\
E:\ftorrent\downloads\.ftorrent\v1.aabb...\
E:\ftorrent\downloads\.ftorrent\v1.ccdd...\
```

A friend sends her a magnet link for a large Windows game he's developing, a new build for her to play-test. She wants that on her PC, not on the stick, so when adding the magnet she chooses a custom download location: `C:\Games`. ftorrent adds this folder to its download folders automatically — the folder doesn't need to be empty or ftorrent-specific. ftorrent only reads and writes inside the `.ftorrent` subdirectory it creates there, and only looks for files and folders it has resume data for. Everything else in `C:\Games` is untouched. Her `ftorrent.toml` now reads:

```
[downloads]
folders = ["./downloads", "C:/Games"]
```

The game downloads to `C:\Games\moondrop-mountain-nightly\`, with resume data in `C:\Games\.ftorrent\`. She seeds it overnight.

Next morning, she ejects the drive and takes it to work. She plugs it into her Mac, where it mounts at `/Volumes/ALICE2TB/`. She double-clicks `ftorrent.app`. On startup:

1. ftorrent finds `portable/ftorrent.toml`, enters portable mode.
2. `./downloads` resolves to `/Volumes/ALICE2TB/ftorrent/downloads/`. The `.ftorrent` directory is there — the Linux ISO and documentary load normally.
3. `C:/Games` doesn't exist on this Mac. ftorrent can't reach the directory, can't read its `.ftorrent`, and has no information about what's there — no names, no hashes, nothing. Those torrents simply don't appear. There's no error, no gray row, no dialog. The torrent list shows the two torrents on the stick and that's it.
4. She adds a new torrent at work. It goes to the default `./downloads` on the stick.

She takes the drive home. Her desktop already has a thumb drive plugged in, so the USB drive mounts as `F:\` instead of `E:\`. She launches `ftorrent.exe`. On startup:

1. `./downloads` resolves to `F:\ftorrent\downloads\`. The Linux ISO, documentary, and the new torrent from work all load — the drive letter change doesn't matter.
2. `C:/Games` exists on this machine. ftorrent scans its `.ftorrent`, finds the game torrent's resume data, sets `save_path` to `C:\Games\`, and the game resumes seeding exactly where it left off. Nothing about the trip to the Mac disturbed it — the resume data has been sitting on `C:\` the whole time, untouched.
3. Everything is back. No paths were edited. No dialogs were shown.

Her list of download folders grows over time as she adds locations on different machines. Entries that don't resolve on the current machine are silently skipped. There are no ghost entries in the torrent list, no error states, no prompts — torrents from unreachable folders simply aren't there, and come back when the folder is reachable again.

### Paths §7: Example story: Three users who install





**Bill** uses Windows 10. **Steve** has a Mac. **Linus** runs Ubuntu Desktop. Each installs ftorrent and downloads the same two torrents: Big Buck Bunny (`dd8255ec...6d1c`), a single `.mp4` file, and The WIRED CD - Rip. Sample. Mash. Share (`a88fda59...dad3`), a folder of MP3 tracks. Both are Creative Commons, listed on WebTorrent's free torrents page.

**Bill (Windows)**

```
# installer downloaded and double-clicked
C:\Users\Bill\Downloads\ftorrent.exe

# application binary (installed by the setup program)
C:\Users\Bill\AppData\Local\ftorrent\ftorrent.exe

# settings (created on first launch)
C:\Users\Bill\AppData\Local\com.ftorrent.ftorrent\ftorrent.toml

# global libtorrent state
C:\Users\Bill\AppData\Local\com.ftorrent.ftorrent\state

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
/Users/Steve/Library/Application Support/com.ftorrent.ftorrent/ftorrent.toml

# global libtorrent state
/Users/Steve/Library/Application Support/com.ftorrent.ftorrent/state

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
/home/linus/.local/share/com.ftorrent.ftorrent/ftorrent.toml

# global libtorrent state
/home/linus/.local/share/com.ftorrent.ftorrent/state

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

All three users' `ftorrent.toml` lists the same download folder:

```
[downloads]
folders = ["~/Downloads/ftorrent"]
```

This was created by ftorrent on first launch. The `~` resolves to `C:\Users\Bill\`, `/Users/Steve/`, and `/home/linus/` respectively. Every other path follows from the decisions in §1–§4 — nothing was configured, nothing was customized, everything is factory defaults.

## Uninstall

Uninstalling is the counterpart to the install flow in §1–§2, and ftorrent follows each platform's own convention rather than inventing one — no "are you sure you want to leave?" guilt screen, no self-deleting trick, the user stays in charge. On Windows that is the uninstaller the setup program wrote beside the app (registered so it appears in Add or Remove Programs); on macOS it is dragging `ftorrent.app` to the Trash, the gesture every Mac user already knows; on Linux it is `apt remove` / `dpkg -r`.

**Only some of these run code — so we design for the ones that don't.** A subtlety decides the whole approach: a Windows uninstaller and a Debian `postrm` script execute, so they *can* actively reverse changes, but macOS drag-to-Trash runs nothing at all. There is no uninstall hook on macOS, so anything ftorrent leaves behind it leaves for good until the user removes it by hand. The answer is not to fake an uninstaller but to make sure ftorrent never leaves anything that misbehaves.

- **Windows.** The uninstaller clears the per-user autostart and file/protocol class entries from `HKCU` (§Start at login, §File and protocol associations). It walks the list in `desktop/win-setup/win-setup.toml`, written in five instructions, deleting ftorrent's own keys, and a shared scheme class like `magnet` only while it still names ftorrent. An upgrade never runs it: the setup program writes over the files in place without uninstalling first, so the registrations, and the user's saved choice that points at them, survive every release. A firewall exemption is machine-wide, so removing it elevates just as adding it did (§Inbound connectivity and firewall); the uninstaller offers to do so.
- **macOS.** Nothing runs, so the design carries the weight. The login item is registered through `SMAppService`, not a raw LaunchAgent, so it is bound to the app and simply stops existing when the app is trashed — no orphaned plist still trying to launch a deleted binary (§Start at login). The default-handler registration in Launch Services goes stale and the system drops it on its own; any file-access (TCC) grants and a rarely-used firewall entry are inert references the user can clear in System Settings. What remains is small, standard, and harmless.
- **Linux.** `postrm` runs as root and reverses the system-level install. Per-user files — the autostart `.desktop`, `mimeapps.list`, the application-data directory — live in each user's home and are left in place, small and in standard locations.

**The user's data is always preserved.** Downloaded files, the `.ftorrent` session directories beside them, and `ftorrent.toml` and `state` in the application data directory all remain on every platform. Uninstalling never deletes a download, and a later reinstall finds its settings and resumes its torrents without re-checking. Deleting someone's downloads on the way out would be the wrong default; a user who wants a clean sweep removes the application data directory and the download folders by hand.

**Undo-first, for the tidy.** A user who wants ftorrent's system changes gone *before* removing the app can turn each one off in the status surface (§System status and permissions) — the same controls that enabled start-at-login, the default-handler claim, and the firewall exemption switch them back off. This is the honest, user-in-charge way to "fully uninstall," and it works identically on all three platforms. It also pairs with the install philosophy: install is deliberately minimal — close to xcopy, taking no active system action — so what little there is to undo is exactly what the user explicitly turned on.

**Portable has nothing to uninstall.** A portable copy's own code never wrote to the host — no binary in a program directory, no autostart entry, no handler claim, no firewall rule, no application-data files. Uninstalling is deleting the folder on the stick; what remains on the host is only the operating system's own records of a program that ran, which §4 lists. This is the portable promise seen from the other end.

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

This seamless apply-and-restart works on macOS and Windows. On macOS, Tauri replaces the `.app` bundle and relaunches. On Windows, the setup program is silent and per user, and starts the app itself when it is done. Neither requires admin privileges or platform signing.

On Linux, and for a copy not running from where the installer put it, the same check and notification happen, but clicking the indicator opens `ftorrent.com` in the user's browser rather than applying the update in place. Linux updates require package manager privileges that the app shouldn't silently acquire, portable installs can't replace a running binary on a USB stick, and a copy the installer didn't place isn't one the updater should replace. The user downloads the new `.deb` or `ftorrent.zip` manually — this matches platform expectations.

**Setting.** `ftorrent.toml` has an `update_check` key, defaulting to `true`. If the user sets it to `false`, the app makes no HTTP requests to the update endpoint, no download occurs, no notification appears, and no install base data reaches the server.

## Crash capture

ftorrent does not have a logging system. During development, diagnostic output is visible in the terminal via `tauri dev`. In production builds, the Rust core captures the sidecar's stderr to a ring buffer in memory — the last 100 lines. During normal operation, nothing is written to disk.

If the sidecar exits with a non-zero exit code, the Rust core writes the buffer contents to `crash.log` in the application data directory (or `portable/` in portable mode):

```
C:\Users\username\AppData\Local\com.ftorrent.ftorrent\crash.log          # Windows
~/Library/Application Support/com.ftorrent.ftorrent/crash.log             # macOS
~/.local/share/com.ftorrent.ftorrent/crash.log                            # Linux
```

The file is overwritten on each crash, not appended. It contains only what the sidecar printed to stderr in its final moments — Python tracebacks, libtorrent alert messages, or native library segfault output. A user reporting a crash can attach this file to a GitHub issue. Without it, every crash report is "it broke."

## Sleep and wake

ftorrent has two separate layers for dealing with system sleep: detection and prevention.

**Detection.** The Rust core writes a timestamp to memory every 60 seconds. When the system sleeps, the timer stops. When it wakes, the timer resumes and the next tick sees a gap — an hour of missing timestamps means an hour of sleep. The first tick after the gap triggers the sidecar to re-announce to trackers and refresh peer connections rather than waiting for libtorrent's internal timers to expire naturally. No platform-specific sleep/wake notifications are involved — this works identically on macOS, Windows, and Linux.

**Prevention.** An optional setting in `ftorrent.toml`, `prevent_sleep`, defaults to `false`. When the user enables it, ftorrent holds a power assertion that prevents the system from sleeping due to inactivity. On macOS, this is an `IOPMAssertion` with `PreventUserIdleSystemSleep`. On Windows, this is `SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED)`. The assertion is held as long as ftorrent is running, regardless of whether any transfers are active — the user may want to keep the machine awake for DHT participation alone. The assertion is released when ftorrent exits.

This only prevents idle sleep. It cannot prevent lid close, user-initiated sleep, critical battery shutdown, or forced OS restarts. These are correct OS boundaries — no unprivileged app can override them. When they happen, the timestamp gap detects the wake and the sidecar recovers.

Linux is excluded from the prevention layer for now. The mechanism (`systemd-inhibit` or D-Bus to `org.freedesktop.login1`) is straightforward but outside the active test matrix.

## System status and permissions

A desktop app sits inside an operating system that guards certain capabilities behind the user's consent: which app owns a file type or URL scheme, whether a program may accept incoming network connections, whether it may read a particular folder, whether it launches at login. ftorrent treats all of these the same way — through one surface and one philosophy.

**Checks run on startup, not at install.** Installation places files and little else, as close to xcopy as each platform allows. The packages carry only the inherent metadata that lets the OS list ftorrent as an option — the document types in the macOS `Info.plist` and the `MimeType` in the Linux `.desktop` file; on Windows, where an installer-placed app has no manifest, the installed copy writes its per-user class registration itself at startup — and beyond that take no active system action: no default claims, no firewall rules, no login registration. Each time it launches, ftorrent instead checks where it actually stands — am I the default for `.torrent` and `magnet:`? am I externally contactable? can I write my download folders? am I set to start at login? — and reports the answers.

**The surface is always honest and never pushy.** A single area in the app shows the current state of each integration in plain language: "you are externally contactable," "ftorrent is not your default torrent app," "incoming connections are blocked." There are no demanding popups and nothing the user must dismiss to use the app. Each item the user could change carries a short explanation of the situation and, where applicable, two ways forward: a button that asks ftorrent to make the change, and written instructions for doing it by hand. The user can ignore all of it and the app still works. Every change the surface can make it can also undo — the control that claims the default handler or adds the firewall exemption is the same one that gives it back — which is also how a user tidies up before removing the app (§Uninstall).

**ftorrent warns before the OS interrupts.** Several of these actions hand off to the operating system's own confirmation — Windows elevates to add a firewall rule, macOS shows a dialog when an app first touches a protected folder, macOS asks the user to approve a login item in System Settings. When ftorrent is about to trigger one of these, it says so first in its own UI, so the OS prompt is expected rather than a surprise. The OS dialog is the right place for the final yes — ftorrent asks for it, it does not try to route around it.

**Per-user by default, system-wide when the capability is.** ftorrent installs and stores its state per-user precisely so two people on one machine never collide (§1, §Instance lock), and most integrations are per-user too — the default-handler claim, autostart. A firewall exemption is inherently system-wide, and there it is correct for ftorrent to make a system-wide change, with the user's consent and through the OS's own elevation (§Inbound connectivity and firewall). Per-user is the default, not a vow never to touch the system.

**Notifications.** ftorrent prefers to tell the user things inside its own window — a banner or a line of text — rather than through OS notifications, the same way the updater shows a quiet in-window indicator instead of a popup (§Automatic Update). OS notifications are not banned; where one is genuinely the better tool, such as an event worth surfacing while the window is hidden, ftorrent may post one, and if that requires the OS notification permission, the permission is handled like any other — shown in the surface, explained, never forced.

**Only the installer's copy changes the host.** A copy not running from where the installer put it — a portable copy, a development build, a copy on the Desktop — runs the same checks and shows the same status read-outs, but offers no changes and writes nothing to the host — no claims, no rules, no login registration. For a portable copy, that's the portable promise; for the others, it keeps the host from pointing at a file that may move.

## Start at login

An optional `start_at_login` setting in `ftorrent.toml`, defaulting to `false`, registers ftorrent to launch at OS login. On Windows and Linux this goes through Tauri's `tauri-plugin-autostart`: on Windows it writes `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, reliable on Windows 10 and 11; on Linux it creates a `.desktop` file in `~/.config/autostart/`, the XDG autostart standard followed by Ubuntu Desktop and its derivatives. Both are per-user and are removed cleanly on uninstall.

macOS needs more care, because the obvious approach has a tail. The autostart plugin registers a LaunchAgent — a plist in `~/Library/LaunchAgents` — which is not tied to the app bundle: drag the app to the Trash and the plist stays behind, still trying to launch a binary that is gone, lingering in the system's background-items list with nothing to remove it. ftorrent instead registers the login item through the macOS ServiceManagement framework (`SMAppService`, macOS 13+), so the item is OS-managed and bound to the app — it presents a single, clearly-labeled approval when enabled and stops existing when the app is deleted, so nothing orphans (§Uninstall). Because ftorrent ships unsigned, macOS may still require the user to approve the item in System Settings → Login Items before it takes effect.

For a copy not running from where the installer put it, this setting is grayed out in the UI — a portable app on a USB stick does not register itself on the host system's login sequence, and a copy on the Desktop or in a development folder shouldn't start a file that may move.

## File and protocol associations

A desktop BitTorrent client is expected to open when you double-click a `.torrent` file or click a `magnet:` link in a browser. ftorrent registers for both, and for its own `.ftorrent` files and `ftorrent:` links, but splits the work into two steps that carry very different consent: being *eligible* to handle them, and being the user's *default*. This is outer-first work — the OS-level shell that delivers a torrent into the app is built and finished here so the in-app experience is later built on top of it, the same way the filesystem commands were ported whole before any view surfaced them.

**Eligibility costs the user nothing.** On macOS the types and schemes are declared in `Info.plist`, `CFBundleDocumentTypes` and `CFBundleURLTypes`, and on Linux in the `.desktop` file's `MimeType` and `x-scheme-handler/magnet` lines; the system reads them from the package. Windows is different: an app placed by an installer has no manifest, so its only channel is the registry, and the installed copy writes it itself, from `associate.js` on general registry commands that read a value, write one only if it would change, delete one, tell the shell, and ask it what opens a type. Under `HKCU` only, it writes a ProgID for each type and scheme, ftorrent's entry in each extension's `OpenWithProgids` list, and a `Capabilities` block named in `RegisteredApplications`, so ftorrent appears in Explorer's Open with menu and in Settings under Default apps. It never writes the sealed `UserChoice`. The fallbacks beneath it, which Windows uses where the user has saved no choice, an extension's own default value and a scheme's shared class like `magnet` itself, ftorrent writes for all four only once the user says yes, as the next paragraphs describe; `.ftorrent` and `ftorrent:` are its own and nobody else wants them today, but the answer sets ftorrent's assertiveness the same for every type, so a future in which another client reaches for them is already covered. Tauri's `bundle.fileAssociations` stays out of `tauri.conf.json` on purpose: its bundlers would seize each type's default at install. After install the OS knows ftorrent *can* open torrents and magnets — it shows up in the "Open with" menu — without changing any default the user already depends on.

**The open event is the plumbing that matters.** A torrent reaches ftorrent two ways: when ftorrent is opened cold by double-clicking a file or following a magnet link, or as a live event to an instance already running. The mechanics differ by platform — Windows and Linux pass the path or URI as a launch argument to a new process, while macOS delivers it as an Apple Event to the app whether or not it is already running. On Windows, this copy's own launch argument and a second launch's handoff through the instance pipe both land in one queue the page takes from, each marked as a launch or a handoff (§Instance lock). On macOS, `RunEvent::Opened` delivers files and URLs from Launch Services and feeds the same queue. No deep-link plugin is needed for either: the handoff is the role Tauri's single-instance plugin normally plays for deep links, and we provide it ourselves because we don't use that plugin. Finishing this plumbing on all three platforms, in both cold-start and already-running states, is the whole point of the story: the inner UI — the add-torrent dialog, the download-folder choice, duplicate detection — is later built on an event that already arrives reliably.

**Becoming the default is where ftorrent asks.** Setting ftorrent as the default handler overwrites a choice the user may have made deliberately, so it never happens on install and never happens silently. Unless the answer is no, a bar across the top of the window asks whether ftorrent should open `.torrent` files and magnet links whenever the system opens any of the four types with another program, with yes, no, and a close that keeps it down until the next launch; the answer is one setting, `[associations] default` in `ftorrent.toml`, which covers all four and which the Settings page changes any time. ftorrent writes only when the user answers, and otherwise only reads, so a type another program takes later, perhaps by the user's own choice, brings the question back rather than being taken back. ftorrent learns where things stand by asking the system what it would open each type with: on Windows the shell's own lookup, `AssocQueryString`, which follows the user's saved choice in `UserChoice` first and the fallbacks after, the same answer Explorer and Settings give; Launch Services on macOS; `xdg-mime query default` on Linux. It follows what it finds: a system that already opens all four with ftorrent leaves nothing to ask, and on Windows, where a saved choice of another program outranks ftorrent's fallbacks, a yes in the bar opens Windows' own settings, the one place that difference can be settled. The Settings page only records the answer, and a no means ftorrent stands down and leaves the saved choices as they are, for the user or another program to change.

The claim writes per-user state only, matching the per-user, no-UAC install model from §1 — `HKCU` rather than `HKLM` on Windows, the user's `mimeapps.list` and `~/.local/share/applications` rather than system paths on Linux, the per-user Launch Services database on macOS. Windows guards the default itself and ftorrent leans on that rather than fighting it: Windows 10 and 11 seal the `UserChoice` value with a per-user hash precisely to stop apps from silently seizing defaults, so on a yes ftorrent writes its fallbacks, which decide only where the user has saved no choice, and where Windows has saved another program, opens the native "Default apps" settings page, letting the final confirmation happen in the OS's own trusted UI. A yes given again, after another program has rewritten the fallbacks, goes straight to that page before writing anything, so the page still shows the other program on the types it took, and the user's choice there is saved above the fallbacks and ends the contest; ftorrent writes the fallbacks under that yes when the user comes back, which covers any type the user didn't set there. That saved choice holds against every program that plays by Windows' rules. An older client can delete it outright and write the fallback beneath, and where no choice is saved, Windows 10 prefers the program the user most recently picked through Open with, when it's still registered for the type, over the fallback; we measured both against uTorrent, and the measurements are in a note at the repository root, `utorrent-associations.md`. One link serves both versions: `ms-settings:defaultapps?registeredAppUser=ftorrent` opens ftorrent's own page, listing its types, on Windows 11 with the April 2023 update or later, and Windows 10, which doesn't know the parameter, opens Default apps itself, where Set defaults by app leads to the same list. macOS is the opposite: setting a non-browser handler succeeds quietly, with no system dialog, for file types and link schemes alike, so there a yes simply works. ftorrent calls `NSWorkspace`'s `setDefaultApplication`, the current form of the older Launch Services functions, through `launch.rs`, handing it its own copy's path, though macOS records the bundle identifier and, where several copies share it, chooses which opens; on macOS 15 we measured it silent, and Transmission's own set-default buttons behave the same way. That silence is also why ftorrent claims only on the user's yes: macOS records a user's choice made through another app's button exactly as it records that app's own claim, so a program that took types back on its own would be overriding the user. Either way the principle holds — ftorrent asks to become the default and never seizes it behind the user's back.

**Only the installer's copy registers.** On Windows, `associate.js` registers only when `ftorrent.exe` runs from `%LOCALAPPDATA%\ftorrent`, where the installer puts it, and skips silently everywhere else; the choice on the Settings page is grayed out for any other copy. A copy carried on a USB stick must leave the host as it found it, and registering as the machine's torrent handler would leave a dead association pointing at a path that no longer exists the moment the stick is ejected; a development build or a copy on the Desktop would point the registry at a file that moves. On macOS the portable bundle carries its own identifier, `com.ftorrent.portable`, and declares no types or schemes, so Launch Services never hands it a click. This is the same reasoning that grays out `start_at_login`: anything that writes the app into the host's persistent configuration belongs to the installer's copy.

## Drag and drop

Dragging is the other natural way a torrent enters ftorrent, alongside file and protocol associations, and it works whether or not ftorrent is anyone's default. Dropping a `.torrent` **file** is the part we ship and the part Tauri makes easy; dropping a **magnet link** is something we want but cannot have for free, and the difference comes down to how Tauri exposes drag-and-drop.

**File drops, the reliable path.** With Tauri's native drag-and-drop enabled (the default), a dropped file is delivered to the Rust core as a real filesystem path. ftorrent routes a `.torrent` to the sidecar as the same add-torrent command used by the open-event path (§File and protocol associations) — a double-click, a clicked magnet, and a file drop all converge on one entry point into the engine. While a drag hovers the window ftorrent shows a drop affordance, and an unrelated file is ignored without error. This is solid, out-of-the-box plumbing.

**Magnet drops, and why they aren't free.** A magnet link dragged from a browser is not a file — it is dragged *text* or a *URL*. Tauri's native handler only ever delivers file *paths*, and it intercepts drags at the native window layer before the webview's own HTML5 drop events can see them. The two modes are mutually exclusive: the moment we turn the native handler off (`dragDropEnabled: false`) to let the webview receive a dropped magnet string, native file drops stop firing — and files dropped onto a webview that way arrive as browser `File` objects with no real path, which is exactly the smooth behavior we would be giving up. So adopting magnet-drop today would mean reimplementing, less reliably, the file-drop Tauri already does well.

**The decision.** We want magnet-drop, but not at the cost of file-drop. So v1 ships file-drop via the native handler, and magnets continue to arrive by click (the protocol handler) and paste — which is how people actually use magnet links. We revisit magnet-drop only if it can coexist with reliable file-drop. The open question to test: whether a *non-file* drag (a link or text) still reaches the webview's HTML5 drop events while the native file-drop handler stays enabled — Tauri's interception is documented around *file* drags, so link/text drags may pass through. If they do, we can have both and we add it; if the interception is global, magnet-drop waits for Tauri to deliver text and URL drops alongside file paths. Either way, file-drop is never sacrificed to get there.

## Inbound connectivity and firewall

A BitTorrent client is most useful when other peers can reach it. ftorrent always makes outbound connections — to trackers, to the DHT, to peers it dials, and to the update endpoint — and these need no special permission. Incoming connections are different: ftorrent listens on a port, and whether peers can reach it depends on the host firewall and, beyond the machine, on the router and NAT.

Three layers stand between ftorrent and an incoming peer: the host firewall, the router's NAT, and the wider internet. ftorrent can act on the first two and can only observe the third.

**Router port mapping (UPnP and NAT-PMP/PCP).** Even with the host firewall open, a home router's NAT will not deliver incoming connections unless it forwards the listen port inward. libtorrent does this automatically, and it exposes exactly two mechanisms as two settings: `enable_upnp` and `enable_natpmp`. There is no third switch for PCP — libtorrent's NAT-PMP mapper also speaks PCP, NAT-PMP's successor, so PCP rides along inside NAT-PMP rather than being controlled separately. ftorrent unifies the pair behind a single user-facing `port_mapping` setting in `ftorrent.toml`, default `true`, which flips both libtorrent booleans together — the same single "UPnP/NAT-PMP" control mainstream clients present, on by default. libtorrent reports the outcome through port-mapping alerts whose transport is one of two values (NAT-PMP or UPnP) plus the external port, or a failure when the router refuses or offers no such service; ftorrent surfaces this in the status surface, naming the mechanism that won — "router port mapped via UPnP," or "no port mapping — your router declined or has UPnP/NAT-PMP turned off." Users on managed networks, who consider letting an app open a router port a risk, or who forward ports by hand can set `port_mapping` to `false`. A successful mapping is a good sign but not a guarantee: carrier-grade NAT or double-NAT can still sit above the router.

**What we can know, and what we can't.** Locally, ftorrent can tell whether it has bound its listen port, whether a firewall exemption is in place, and whether the router accepted a port mapping — all reported in the status surface (§System status and permissions). True *external reachability* is still a separate question: it depends on NAT, the router, and the ISP, and can only be confirmed by something outside the machine probing back in — the job intended for a future good.ftorrent.com reachability check (noted in the futures list above). So the status is honest about its own limits: it reports the local facts it knows and defers the stronger claim "peers on the internet can reach you" to the external check. Either way the user is not left guessing, because the failure is otherwise silent — an outbound-only client works, just less well.

**The exemption is the one system-wide change, and only Windows can automate it.** A firewall exemption changes the machine's firewall for every account, not just the current user. This is the deliberate exception to ftorrent's per-user default (§System status and permissions), and it is correct: a torrent client that cannot accept connections is hobbled, and the firewall is where that capability lives. On Windows, ftorrent can add an inbound allow-rule programmatically (the Windows Firewall COM API, or `netsh advfirewall`), but that requires administrator rights — so the button elevates and triggers a UAC prompt, which ftorrent warns about first. This is exactly why installation stays per-user and silent (§1) while this one runtime action is allowed to elevate: the clean install is preserved, and the system change happens later, explicitly, only if the user asks. A user with no administrator access cannot enable inbound at all and runs outbound-only.

**macOS is status-and-instructions, not a button.** macOS offers no supported API for an app to add itself to the application firewall, and that firewall is off by default for most users in any case. So on macOS ftorrent reports the state and, if the user has the firewall on and wants incoming connections, gives instructions — it does not shell out to undocumented tools to do it silently. When the macOS firewall is on, it shows its own allow-incoming prompt the first time ftorrent listens; the same warn-first treatment applies.

**Never required.** A user who declines runs outbound-only and ftorrent keeps working. The exemption is offered only for a copy running from where the installer put it; a portable copy reports its status but does not touch the host firewall, consistent with the portable promise (§4).

## File access on macOS

On macOS Ventura and later, the system places the user's Downloads, Desktop, and Documents folders, and all removable and network volumes, behind per-folder privacy permissions enforced by TCC. The first time an app tries to read or write one of these locations, macOS shows a prompt; if the user denies it, the app cannot see the files, and macOS will not ask again on its own. This applies regardless of code signing, so ftorrent's unsigned build (§Automatic Update) does not sidestep it. The app declares why it wants each location through `NS…UsageDescription` strings in its `Info.plist` (Downloads, Desktop, Documents, removable volumes), which is the text the prompt shows the user.

Two parts of ftorrent's design walk straight into this. The default download folder is `~/Downloads/ftorrent` (§2), inside the protected Downloads folder; and portable ftorrent is built to run from a removable USB volume (§4, §6), exactly the kind of volume macOS gates. These prompts are expected, not edge cases.

The key API reality is that there is **no way to ask macOS "do I have access?" without trying.** Permission is discovered by attempting a read or write and seeing whether it succeeds — and the attempt itself is what triggers the first-time prompt. ftorrent turns this to its advantage: it must read its download folders at startup anyway, to load resume data and resume torrents (§3's startup flow), so the prompt arrives at a moment that is genuinely necessary rather than gratuitous. Following the §System status and permissions rule, ftorrent warns in its own UI just before the access that will trigger the dialog, so the system prompt is anticipated.

If the user denied access on a previous run, macOS will not let ftorrent re-trigger the prompt. So instead the status surface reports the folder as blocked, explains the consequence, and gives instructions to grant access in System Settings → Privacy & Security → Files and Folders. (Opening that pane directly uses an `x-apple.systempreferences:` URL, which is undocumented and has shifted between macOS versions — worth verifying per release, with plain written steps as the reliable fallback.)

On Windows and Linux there is no equivalent gate for a user writing within their own profile, so this is a no-op: the surface simply reports full access to the configured folders. The aim is not to add a permission system ftorrent controls — it is to make the operating system's permission system legible, so a blocked download folder appears as a clear, fixable status line instead of a mysterious failure to save.

## Instance lock

**Instance lock.** ftorrent enforces a single running instance per data folder. On startup, after resolving the data folder (`portable/` beside the program, or the platform-standard location per §4), the app takes an exclusive lock on `ftorrent.lock` inside it. If the lock is already held, the app hands what it carried to the running instance, which brings its window forward, and exits. What this means in practice — relaunches, two OS users, installed-plus-portable side by side — is walked through in §5.

**One mechanism on every platform.** Rust's standard library has had file locking since 1.89: `File::try_lock` takes an exclusive lock with `flock()` on macOS and Linux and `LockFileEx` on Windows, and the operating system releases it however the process ends — clean exit, crash, kill, or power loss — so there are no stale locks. A file lock applies to everyone who opens the file, whatever account they're signed in as, so two people running the same portable copy from one stick can't both win it. A named mutex, the usual choice on Windows, lives in the per-session namespace and would let exactly that happen. `ftorrent.lock` stays empty and separate from `ftorrent.toml` for two reasons: the settings file is replaced whole on save, which would drop a lock held on the old file, and a lock on Windows is mandatory, so while it's held no other process could read the locked file at all. PID-based lock files are deliberately not used: a PID file can be left behind when a process is killed without cleanup, producing the worst case — nothing visibly running, but startup blocked by a stale lock. The lock holds on exFAT and FAT32, the formats a USB stick carries; a volume that can't lock at all runs without the lock and says so, rather than refusing to start.

**Forwarding an open request.** A second launch is sometimes not a stray double-click but the OS asking ftorrent to open a `.torrent` or a `magnet:` (§File and protocol associations). On Windows every launch starts a new process, so ftorrent carries the request itself: the running instance serves a named pipe, `\\.\pipe\ftorrent-` followed by a hash of the lock file's path, and a second launch that finds the lock held connects, writes its command-line arguments as one line of JSON, and exits. The pipe name comes from the lock path, so each copy only hears from launches of itself. The server end is inbound only, local clients only, and asks for the first instance of its name, so it never joins another process's pipe. A second launch that finds the lock held a moment before the pipe exists, during a cold start, retries for a few seconds rather than giving up. On macOS, Launch Services brings a running app forward rather than starting a second process for the same bundle and delivers opens to it as Apple Events, so there is no second process to forward from; the file lock remains the backstop for a launch that bypasses Launch Services, like `open -n`. When another account runs the same portable copy, the second person's launch can't write to the first person's pipe, and it leaves without a word.

**Focus.** The running instance brings its window forward only when the user asked: a second launch, the tray, or the Dock. It shows the window if hidden, restores it if minimized, and focuses it. Windows limits which process may take the foreground; a handoff brings the window to the front from hidden, minimized, or behind another window.

This replaces Tauri's built-in single-instance plugin, which keys on the bundle identifier — a named mutex in the per-session namespace on Windows, a socket in the shared `/tmp` on macOS. An installed copy and a portable copy share the same bundle ID on Windows, so the plugin would block the second from launching — exactly the case ftorrent needs to allow — and it can't tell two different OS users' installed copies apart. ftorrent's own per-data-folder lock handles both; the user-facing scenarios it enables are walked through in §5.

**Download folders are locked too.** The data folder lock keeps two launches of one copy apart, but two different copies can share a download folder. So each copy also takes an exclusive lock on `.ftorrent/ftorrent.lock` inside every download folder it loads, with the same mechanism, and a folder another copy already holds shows as in use and loads nothing. The lock is the Rust core's general `lock_take`, which locks any file; the page decides which folders to lock, makes each `.ftorrent`, and hides it on Windows. One folder listed under two spellings is held once. The engine receives each real folder once: `lock_take` answers the lock file's real path, and the page hands the engine each folder once.

## Appearance and dark mode

ftorrent is light or dark by one setting, `[appearance] mode` in `ftorrent.toml`: `"light"`, `"dark"`, or `"system"`, the factory value, which follows the operating system's own setting and changes when it does. The Settings page offers the three as Light, Dark, and System, the same words on every platform, and a choice takes effect at once.

One switch turns everything. The window's theme is the only thing the page sets, through Tauri's `setTheme`, with `null` for system: `window.js` calls it while the window is still hidden at startup, so it first appears in the right colors, and again whenever the setting changes. The window turns its own chrome, the title bar and on Windows the menu bar, and hands the theme to the web view, which reports it to the page as `prefers-color-scheme`. The page never branches on a class of its own: every color in `style.css` holds a light and a dark value in CSS's `light-dark()`, with `color-scheme: light dark` on the root, so the web view's own parts, scrollbars, radio buttons, and checkboxes, follow too. Following the system as the user changes it takes no code, since the system tells the window and the window tells the page. The palette is matched to the Windows menu bar, white in light and `#2b2b2b` in dark, so on Windows the page runs on from the menu without a seam; on macOS the theme is app-wide, and the same colors sit beside macOS's own window colors rather than matching them.

The typeface is the setting beside it, `[appearance] font`: `"system"`, the factory value, which is the face the platform sets its own menus in, Segoe UI on Windows and San Francisco on a Mac, with the platform's own fixed-width face, Consolas or SF Mono; `"inter"`, which ftorrent carries with IBM Plex Mono for fixed-width text, the same two faces everywhere; and on Windows only `"verdana"`, for the way Windows programs looked around 2000, also with Plex Mono, which descends from typewriter faces and so echoes the Courier New of that era without its thin strokes. So the choice is all the platform's faces or, for Inter and Verdana alike, ftorrent's own fixed-width face, and every font list is one face and a generic family, short enough that whatever draws a piece of text is never a guess. The root keeps the system font whatever the choice, so every size and space measures from the system's own and switching faces changes the letters and nothing else; `style.css` says how each face is sized.

## Icons and graphics

ftorrent needs a small set of source assets: the application icon, a document icon for `.torrent` files, the Windows 10 Start tile, a tray icon, a DMG background, and a website favicon.

**Application icon.** The mark is an orange pill, `#FF7900`, holding two white rings joined by a bar: two nodes, linked. It's drawn as an SVG on a sixteen-unit grid, so every coordinate lands on a pixel at the smallest sizes. Tauri's icon generator resizes a square SVG edge to edge into every platform's files at once, and it has no notion of a platform's safe area, which the platforms disagree about: Windows and Linux want an icon that fills its canvas, macOS draws an icon exactly as authored on a grid where every neighbor leaves a margin, and the Windows 10 Start tile wants the mark inset on its background. So `pnpm icons` feeds the generator three sources, the same drawing seen through three viewBoxes, full bleed, inset for the Dock, and inset further for the tile, and a script copies out only the wanted files: the Mac's `.icns` from the second run and the tile PNGs from the third. `bundle.icon` in `tauri.conf.json` names the full-bleed `.ico` and PNGs and the Mac `.icns`, and Tauri picks the `.icns` for the Mac by extension, so that one list is the whole per-platform switch. The setup program and uninstaller wear the same `.ico`. Generated icons are frozen at the CLI that made them, so `pnpm icons` runs again after a CLI upgrade as well as after an artwork change.

**Document icon.** A `.torrent` file wears a document icon, not the application icon, so a folder of torrents isn't a folder of identical pills: a blank page with a folded corner carrying one orange circle with a white dot at its center, one node waiting to join the others. It's drawn by hand on the same sixteen-unit grid, with its center and radii on whole units, so every edge sits on a pixel at 16. Only Windows needs one: macOS composes a document icon itself, a page with the application's icon on it, for any type an app declares, and Linux draws the desktop theme's own icon for the MIME type. On Windows, `bundle.resources` lands `torrent.ico` beside the executable, and the registration names it as the `.torrent` ProgID's `DefaultIcon` (§File and protocol associations). The icon studio in `desktop/icon-studio/`, Windows only and run by hand, is where the icons are made; its README has the flow.

**Windows 10 Start.** For any desktop `.exe`, Windows 10 looks for `ftorrent.VisualElementsManifest.xml` beside it, naming a larger tile image, a background color, and whether to show the app name; without one, Start extracts the small icon from the `.ico` onto a generic tile. `bundle.resources` places the manifest and its two tile PNGs beside `ftorrent.exe`, since on Windows the resource directory is the executable's own folder. The two logo attributes are all or nothing, and naming one without the other makes Windows silently ignore the file. Windows 10 paints the tile in the theme's own color behind the inset mark, milky white in light mode and near black in dark, as the major browsers' tiles do, whatever color the manifest declares. Windows 11 has no tiles and ignores the file.

**Tray and menu bar icons.** Separate from the application icon, a small icon appears in the system tray (Windows), menu bar (macOS), or status notifier area (Linux) when ftorrent is running with its window hidden. All tray icons should be simple and legible at tiny sizes — the main app icon scaled down will be an unreadable blob. On macOS, this must be a monochrome "template image" — a single-color shape on a transparent background, which the system colors for light mode, dark mode, and the translucent menu bar. On Windows and Linux, a small color icon is used instead. The designer provides these directly:

```
ftorrent/desktop/src-tauri/icons/tray-template.png       # 22×22, monochrome black on transparent, macOS
ftorrent/desktop/src-tauri/icons/tray-color.png           # 32×32, color on transparent, Windows and Linux
```

The tray is built in code, with Tauri's `TrayIconBuilder`, and on macOS it takes the template image with `icon_as_template(true)`.

**DMG background.** The macOS DMG window displays a background picture behind the app icon and the Applications folder icon. The designer provides it directly, `ftorrent/desktop/src-tauri/dmg/ceiling.jpg`, 1600 pixels square tagged at 144 dpi so it shows at 800 points on standard and Retina panels alike, with the two icons centered in its halves. Tauri's own dmg bundler lays out the window by driving Finder through AppleScript, which steals focus and stalls when Finder is busy, so `pnpm installer` on a Mac runs `dmg.js` after `tauri build` instead, and dmgbuild writes the window's `.DS_Store` directly into an image Finder never sees. The window opens 400 points from the left and bottom of the screen, which on most screens macOS moves up until it sits just beneath the menu bar.

**Windows installer.** The setup program runs silently — no wizard pages, no UI. The user double-clicks `ftorrent.exe`, files are extracted to `AppData\Local\ftorrent\`, and the app launches. It needs no graphics beyond its icon, which its creator compiles in from the `.ico` that `bundle.icon` names.

**Website favicon.** ftorrent.com and docs.ftorrent.com wear the brand mark; open.ftorrent.com and good.ftorrent.com wear an earth emoji, which suits pages about the planet's peers. Every favicon is an SVG inline in its site's head as a `data:` URI, so no site carries a favicon file. The brand's is `favicon.svg` from the icon studio, the brand mark as it is.
