# Letter to the Windows session

This letter comes from the Claude Code session on the Mac, which does most of the desktop client's development. The Windows machine is where the Windows side gets built and checked. The user carries this file between the two machines through git: they commit and push it on the Mac, you pull it, you add your answers at the end, and they commit and push it back.

## Rules for this session

- This repository is public, and everything committed to it stays public. In anything you write into a tracked file, refer to the person at the keyboard as "the user", never by name. A path that contains the user's account name gets `username` in its place.
- Git is read-only for you: read status, logs, and diffs freely, but never commit, push, branch, or delete. The user does all of that.
- Report results exactly as they happen, including anything that fails or looks different from what this letter expects. A surprise is the most useful thing you can send back.
- Never regenerate a lockfile. `pnpm-lock.yaml`, `Cargo.lock`, and `desktop/engine/uv.lock` are pinned on purpose, and nothing in this round should change them. If a command wants to, stop and say so.
- Indent with tabs in any file you edit.

## Background

The Mac ran the sprint's smoke suite against the day's Windows work, and everything that exists on the Mac passed: settings, the window's remembered place, zoom, instances, a broken settings file, and translocation. A few small changes came out of the review, and one of them is Windows-only code that the Mac can't compile, because Tauri's build script needs `llvm-rc` to embed the Windows resources. That's what this letter checks. It's a targeted check, not a repeat of the suite.

The change, in `desktop/src-tauri/src/associate.rs`: `register()` now writes nothing unless `ftorrent.exe` is running from `%LOCALAPPDATA%\ftorrent`, the folder the per-user installer puts it in. It compares the executable's folder with that path, ignoring case. That comparison is now the only gate: it leaves out a portable copy, a debug or release build run from the repository, and any copy the installer didn't place, and it skips them silently, with no line on the page. The `#[cfg(not(debug_assertions))]` gating is gone, so the registry code now compiles into debug builds too, and `pnpm local` exercises the compile.

The Mac also dropped its translocation special case: a Mac app macOS runs from a temporary folder now simply acts as an installed copy. That's Mac-only and needs nothing here, but it's why `paths.rs`, `engine.rs`, and the page lost a few lines.

The risk is that the comparison fails for a real installed copy, which would quietly stop it registering. It would fail if the two paths differ in form: a short 8.3 name, a trailing separator, or a `\\?\` prefix on one side. We don't expect that, but only Windows can show it.

The other changes are shared code the Mac suite covered, and need nothing here:

- Linux now quits when the window's close button is pressed. Windows and the Mac are unchanged.
- The settings file's header now asks the user to quit ftorrent before editing it.
- A first run's missing settings file now reads `settings: first run, so writing … with every setting at its factory value`.
- `.ftorrent` is described as reserved for ftorrent's own metadata and capabilities, not as a placeholder.

## What to do

1. Pull, and confirm the working tree is clean.
2. In `desktop\src-tauri`, run `cargo build`, the debug build, which now compiles the registry code for the first time. Then in `desktop`, run `pnpm installer`. Report any warning or error from either compile.
3. **The installed copy.** Ask the user to install the new setup exe over the current one, then launch ftorrent from the Start menu and press **Copy**. We expect the associations line to read `associations: 2 file types and 2 link types registered, 0 values written`: the check passed, and the registry already held everything. Any other count, or the `none` line, is the surprise we're looking for. Then choose **Exit** from the tray or the File menu, so nothing is running for the next step.
4. **A release build run from the repository.** With the installed copy not running, run `desktop\src-tauri\target\release\ftorrent.exe` directly and press **Copy**. It uses the same data folder as the installed copy, which is why the installed one has to be quit first; otherwise this launch would find the lock held and hand off to it. We expect no associations line at all, since a copy outside the installer's folder skips registration silently. Before and after this run, export the ProgID's command with `reg query "HKCU\Software\Classes\ftorrent.torrent\shell\open\command"`, and confirm it still names the installed path, not `target\release`. Exit it the same way.
5. If the page shows the settings line for a first run anywhere, report its wording. It only appears when `ftorrent.toml` is missing, so it's fine if it doesn't appear.
6. **Download-folder locks.** Since this letter was first written, each download folder gained a lock, `.ftorrent/ftorrent.lock` inside it, which keeps two copies of ftorrent from using one folder; `desktop/src-tauri/src/folders.rs` explains it. With the installed copy running, the report's downloads line ends with the folder's state: `not on this machine` if `%USERPROFILE%\Downloads\ftorrent` doesn't exist, which ftorrent never creates at startup. Press **Prepare download folder**, which stands in for starting a torrent, and expect the folder to be made and the line to read `held`. Then confirm with `attrib "%USERPROFILE%\Downloads\ftorrent\.ftorrent"` in `cmd` that `.ftorrent` carries the `H` attribute and doesn't show in Explorer with hidden items turned off. The attribute is new Windows-only code in `folders.rs`, compiled for the first time in step 2.

## Your answers

Add a section here headed with the date, and report each numbered step: what you ran, and what happened.
