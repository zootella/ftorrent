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
