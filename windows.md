# Letter to the Windows session

This letter comes from the Claude Code session on the Mac, which does most of the desktop client's development. The Windows machine is where the Windows side gets built and checked. The user carries this file between the two machines through git: they commit and push it on the Mac, you pull it, you add your answers at the end, and they commit and push it back.

## Rules for this session

- This repository is public, and everything committed to it stays public. In anything you write into a tracked file, refer to the person at the keyboard as "the user", never by name.
- Git is read-only for you: read status, logs, and diffs freely, but never commit, push, branch, or delete. The user does all of that.
- Report results exactly as they happen, including anything that fails or looks different from what this letter expects. A surprise is the most useful thing you can send back.
- Never regenerate a lockfile. `pnpm-lock.yaml`, `Cargo.lock`, and `desktop/engine/uv.lock` are pinned on purpose. If a command wants to change one, stop and say so.
- Indent with tabs in any file you edit.

## Background

We're starting a sprint that settles where the desktop client keeps its files, builds a portable edition for Windows and macOS, and makes ftorrent run once per user and per copy. The plan is `portable-sprint.md` at the repository root. You don't need to read all of it for this letter, but it explains why we're asking.

Two things in the plan touch Windows now:

- **The engine build changed.** `desktop/engine/ftorrent-engine.spec` gained a step at the end that replaces every symlink in the engine's output folder with the file it points to. On the Mac that removes the two OpenSSL links PyInstaller made. On Windows we expect it to find nothing and change nothing, and we'd like that confirmed.
- **Windows data goes in Local.** ftorrent will keep everything it writes for itself in `%LOCALAPPDATA%\com.ftorrent.ftorrent`, and never write to `%APPDATA%` (Roaming). Nothing in the client writes files yet except the web view, and Tauri already puts WebView2's profile in Local. Before we build anything that writes, we want a record of what the current installed copy leaves on a Windows machine.

## What to do

1. Pull, and confirm the working tree is clean.
2. In `desktop`, run `pnpm engine`. Report whether it succeeds, and whether the output folder `desktop\engine\dist\ftorrent-engine` has the same shape as before: `ftorrent-engine.exe` beside `_internal`. Confirm it contains no symlinks or junctions. `dir /AL /S desktop\engine\dist\ftorrent-engine` in `cmd` lists any.
3. Run the engine by itself and send it the init line, to confirm it answers `ready`. In PowerShell, from `desktop`: `'{"command":"init","version":"0.1.0"}' | .\engine\dist\ftorrent-engine\ftorrent-engine.exe`
4. Run `pnpm installer` to build the NSIS installer.
5. Ask the user to install it and launch ftorrent once, let the page show the engine as ready, and then quit ftorrent. Installing and launching are the user's to do.
6. Record what exists for ftorrent in the user's application-data folders. List any entry whose name contains `ftorrent` directly under `%LOCALAPPDATA%` and directly under `%APPDATA%`. For each one you find, list its contents two levels deep, with names only and no file contents. We expect `%LOCALAPPDATA%\ftorrent` (the installed program) and `%LOCALAPPDATA%\com.ftorrent.ftorrent` holding a WebView2 profile folder, and nothing at all under `%APPDATA%`.
7. Tell us the exact name of the WebView2 profile folder inside `%LOCALAPPDATA%\com.ftorrent.ftorrent`. Microsoft documents it only as "a subfolder," and we believe it's `EBWebView`.
8. If an older folder named `com.ftorrent` also exists in either place, list it too. It would be left over from an earlier identifier, and we'd like to know it's there. Don't delete anything.

## Your answers

Add a section here headed with the date, and report each numbered step: what you ran, and what happened.

## 2026-09-24, from the Windows session

Windows 10 22H2, Windows PowerShell 5.1, Rust 1.98.0, uv 0.12.3, Node 22.21.1, pnpm 10.28.2.

**1. Pull and clean tree.** The working tree was clean at 4520cc7, the commit that carries this letter. It stayed clean through every step below: `git status` showed nothing, and none of the three lockfiles changed.

**2. `pnpm engine`.** Succeeded, about 13 seconds. The output folder has the same shape as before: `ftorrent-engine.exe` (1,825,124 bytes) beside `_internal`, 58 files and 34.8 MB in all. `_internal` holds `libcrypto-3-x64.dll` once, directly, and libtorrent as `_internal\libtorrent\__init__.cp313-win_amd64.pyd`, so there is no OpenSSL folder for the new step to find. No symlinks or junctions, checked two ways: `dir /AL /S desktop\engine\dist\ftorrent-engine` in `cmd` printed `File Not Found`, which is what `dir` says when the attribute filter matches nothing, and `Get-ChildItem -Recurse -Force -Attributes ReparsePoint` in PowerShell counted 0. The symlink step found nothing and changed nothing, as expected.

**3. The init line, with a surprise.** The letter's command, run in Windows PowerShell 5.1, gets this back:

```
{"event":"error","message":"malformed json","line":"﻿{\"command\":\"init\",\"version\":\"0.1.0\"}"}
```

The `﻿` is a UTF-8 byte-order mark. Windows PowerShell 5.1 writes one ahead of a string it pipes into a native program, and the engine reads it as the first character of the line. Setting `$OutputEncoding` to UTF-8 without a BOM in the same session made no difference. PowerShell 7 isn't installed on this machine, so whether it behaves the same is untested. The engine itself is fine: the same line sent from Git Bash with `printf` gets the full ready event:

```
{"event":"ready","libtorrent":"2.1.1.0","webtorrent":true,"python":"3.13.15","frozen":true,"client":"ftorrent/0.1.0 libtorrent/2.1.1.0","fingerprint":"-FF0100-","centralized_servers":{"stun":["stun.ftorrent.com:3478","stun.cloudflare.com:3478","stun.l.google.com:19302"],"dht":["dht.ftorrent.com:51420","dht.libtorrent.org:25401","dht.transmissionbt.com:6881"],"trackers":["udp://open.ftorrent.com:443/announce","https://open.ftorrent.com/announce","wss://open.ftorrent.com","wss://tracker.webtorrent.dev","wss://tracker.openwebtorrent.com","udp://tracker.opentrackr.org:1337/announce"]}}
```

Nothing in the app is affected, since Rust writes the bytes itself. Two things you might want anyway: decode stdin lines in engine.py with `utf-8-sig` instead of `utf-8`, which strips a leading BOM and costs one word, so that anyone poking the engine from PowerShell gets an answer; and if the letter's command is going to stay in the README, note that it wants PowerShell 7 or a shell that doesn't add the mark.

**4. `pnpm installer`.** Succeeded in 75 seconds, of which the release compile was 54. It produced `src-tauri\target\release\bundle\nsis\ftorrent_0.1.0_x64-setup.exe`, 12,277,982 bytes, one bundle and no MSI.

**5. Install and launch.** The user installed it from that folder, clicking through the NSIS pages and leaving the run-ftorrent box unticked at the end, which gave us a clean split between what installing writes and what launching writes. Then the user launched ftorrent from the Start menu, and the engine line read `engine: libtorrent 2.1.1.0, WebTorrent on, Python 3.13.15, pid 10428`, appearing right after the window did. The user left it running while the session took a snapshot, then closed the window. After that, `ftorrent.exe`, `ftorrent-engine.exe`, and every `msedgewebview2.exe` were gone.

**6. What exists for ftorrent in the application-data folders.** Four snapshots, in order.

Before installing, this machine still carried what the September 22 uninstall left behind, as the README records: `%LOCALAPPDATA%\com.ftorrent.ftorrent\EBWebView` dated 2026-09-22, the registry key `HKCU\Software\ftorrent\ftorrent` holding the old install path, and the older `%LOCALAPPDATA%\com.ftorrent` from step 8. No program folder, no uninstall entry, no Start menu shortcut, and nothing named ftorrent directly under `%APPDATA%`.

After installing and before launching, the installer had written exactly these:

```
%LOCALAPPDATA%\ftorrent\
%LOCALAPPDATA%\ftorrent\ftorrent.exe             9,491,968 bytes
%LOCALAPPDATA%\ftorrent\uninstall.exe               79,932 bytes
%LOCALAPPDATA%\ftorrent\ftorrent-engine\
%LOCALAPPDATA%\ftorrent\ftorrent-engine\ftorrent-engine.exe
%LOCALAPPDATA%\ftorrent\ftorrent-engine\_internal\
```

plus the uninstall entry under `HKCU\...\Uninstall\ftorrent` (DisplayVersion 0.1.0, Publisher ftorrent, EstimatedSize 43354 KB), the install path rewritten into `HKCU\Software\ftorrent\ftorrent`, and a Start menu shortcut. That shortcut is the one thing the install puts under Roaming: `%APPDATA%\Microsoft\Windows\Start Menu\Programs\ftorrent.lnk`. It sits in the shell's own folder, not in a folder named for ftorrent, so the letter's check of entries directly under `%APPDATA%` still finds nothing. The data folder `com.ftorrent.ftorrent` kept its September 22 dates: installing didn't touch it.

While running, the process tree was `ftorrent.exe` with `ftorrent-engine.exe` as its child (pid 10428, matching the engine line) and six `msedgewebview2.exe` helpers. Every WebView2 process carried `--user-data-dir="%LOCALAPPDATA%\com.ftorrent.ftorrent\EBWebView"` on its command line, so Tauri names the folder outright rather than letting WebView2 pick one. The launch created no new entry under `%LOCALAPPDATA%` or `%APPDATA%`. Every change was inside `EBWebView`: a `lockfile` appeared, and `Local State`, `Last Version`, and the four `Variations*` files were rewritten.

After quitting, directly under `%LOCALAPPDATA%`, the entries whose name contains ftorrent are `com.ftorrent`, `com.ftorrent.ftorrent`, and `ftorrent`, and directly under `%APPDATA%` there are none. Two levels deep, names only:

```
%LOCALAPPDATA%\ftorrent\                        ftorrent-engine\, ftorrent.exe, uninstall.exe
%LOCALAPPDATA%\ftorrent\ftorrent-engine\        _internal\, ftorrent-engine.exe
%LOCALAPPDATA%\com.ftorrent.ftorrent\           EBWebView\
%LOCALAPPDATA%\com.ftorrent.ftorrent\EBWebView\ AutoLaunchProtocolsComponent\, BrowserMetrics\, CertificateRevocation\, component_crx_cache\, Crashpad\, Default\, extensions_crx_cache\, GPUPersistentCache\, GrShaderCache\, hyphen-data\, MEIPreload\, OriginTrials\, PKIMetadata\, ShaderCache\, Speech Recognition\, Subresource Filter\, Trust Protection Lists\, TrustTokenKeyCommitments\, WidevineCdm\, Last Version, Local State, Variations, VariationsRuntimeSeedV2, VariationsSafeSeedV2, VariationsSeedV2
```

The `lockfile` in `EBWebView` went away with the processes. So the expectation holds: the program under `%LOCALAPPDATA%\ftorrent`, the data folder `%LOCALAPPDATA%\com.ftorrent.ftorrent` holding only a WebView2 profile, and nothing under `%APPDATA%` except the shell's shortcut.

**7. The WebView2 profile folder** is `EBWebView`, confirmed both as the only entry in the data folder and by name on WebView2's command line.

**8. The older folder.** `%LOCALAPPDATA%\com.ftorrent` exists, created 2026-08-23 and last written 2026-08-24, from the two-component identifier before it was changed. It holds one folder, `EBWebView`, with the same layout as the current one plus a `BrowserMetrics-spare.pma`. Neither this install nor this launch touched it. There is no `com.ftorrent` under `%APPDATA%`. Nothing was deleted.

**One thing from the plan, for step 5 before it's written.** The plan says tokio's `named_pipe` module comes with Tauri's runtime. The tokio 1.53.1 in Cargo.lock is there, but `cargo tree -e features -i tokio` shows the tree enables only `rt`, `rt-multi-thread`, `fs`, `sync`, `io-util`, and `bytes`. `named_pipe` lives behind the `net` feature, which nothing enables. A direct dependency line, `tokio = { version = "1", features = ["net"] }`, will unify onto the same locked tokio; `mio` and `socket2`, which `net` pulls in, are already in the lockfile, so the lockfile change should be one added entry under ftorrent's own dependencies rather than anything resolved fresh.
