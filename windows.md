# Windows: one-click update

A note for the Windows session. It's public and committed, so it names nobody: in anything written back here, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile.

## What's asked

Write the Windows half of one-click update, which the Mac half already proves end to end. An installed copy finds a newer version, the user clicks one button, and the copy is replaced by the new one, which opens where the old window was. Everything platform-general is built and tested on the Mac: the check, the download, the hash, the button, and the status line. What's left is Windows-only, in `desktop/src-tauri/src/update.rs`, plus the one line in the page that turns the click on there.

## What's already there

Read the essays at the top of `desktop/src/stores/update.js` and `desktop/src-tauri/src/update.rs` first; they carry the design. In short:

- **The check** reads `https://ftorrent.com/ftorrent.exe.json` on Windows, the sidecar `pnpm hash` writes beside the installer. Only an installed copy checks, by `isInstalled` in `desktop/src/paths.js`.
- **The click** reads the sidecar again, downloads `ftorrent.exe` with `net_get`'s save path into the data folder, `%LOCALAPPDATA%\com.ftorrent.ftorrent\ftorrent.exe`, under a 50 MiB ceiling, and compares the SHA-256 `net_get` returns with the sidecar's. Then it calls `update_replace` with that path, then `update_restart`.
- **On Windows both commands return an error today**, and the store sets `installable` only on macOS, so the button never offers the update there.

## The Windows half

On Windows the update is the installer itself, and the setup program already does the hard part: it asks a running copy to exit through the instance pipe with `--exit`, waits for the files to come free, writes over `%LOCALAPPDATA%\ftorrent` in place without uninstalling, and starts the program when it's done. So the Windows `update_replace` only has to start the downloaded setup program, and `update_restart` has nothing left to do, since setup closes this copy and starts the new one. Whether `update_restart` simply returns, or quits this copy a moment ahead of setup's request, is yours to decide from what you see.

Antivirus heuristics watch for an unsigned program that downloads an executable and runs it, so keep the shape plain:

- Start `ftorrent.exe` directly from the data folder where the page saved it, never from a temporary folder, and never through `cmd`, `powershell`, or any other interpreter.
- Start it detached, with no console window, so it outlives this copy, which it's about to close.
- Rust works out what to start, the path the page passed being the only input, the way the Mac half takes only the zip.

The downloaded installer stays in the data folder after the update, since setup can't delete itself while it runs. The next update overwrites it, because `net_get` renames its `.part` file over the old one. Clearing it sooner, from the new copy at startup, is fine if it's simple; leave it if not.

Then set `installable` in the store to Windows as well as macOS. The essay in `update.rs` shows the whole Mac update as a block of steps with the full path each one touches, from the click to the new window; give Windows the same block beside it, with the real paths, once it works, and replace the essay's "That half isn't written yet."

## How to check it

Both copies need today's code, so the test takes two builds:

- **The old copy:** change `version` in `desktop/src-tauri/tauri.conf.json` to `0.1.0` for this build only, `pnpm installer`, and install it from `pnpm reveal`.
- **The release:** set `version` back to `0.1.1`, `pnpm installer`, `pnpm hash`, and `pnpm upload`. That publishes `ftorrent.exe` and its sidecar at 0.1.1, the version the Mac already published.

Then, with the log on (`[log] record = true` in the old copy's `ftorrent.toml`, writing to `%USERPROFILE%\ftorrent-logs`):

- **The click:** in Settings, Check for Update turns into Update ftorrent, with `ftorrent 0.1.1 released` and the upload's date below it. Click it. The status line gains `, Downloading...`, the window goes, and the new one opens in the same place.
- **What's installed:** `%LOCALAPPDATA%\ftorrent\ftorrent.exe` reports 0.1.1 in its properties, the Start menu shortcut and taskbar pin still open it, and the associations and the login entry are still in place.
- **No prompts:** neither SmartScreen nor UAC appears. The downloaded file carries no `Zone.Identifier` stream, which `Get-Item <path> -Stream *` shows, and that's why SmartScreen stays quiet.
- **Defender:** nothing in Windows Security's protection history during the download or the install.
- **The logs:** the old copy's file shows the download, the hash, and setup starting; the new copy's shows it starting at 0.1.1.

## What changes on the page

Nothing new appears in the UI on Windows: the button, its words, and the status line are already written and approved, and errors go to the log, never to the page. If something looks like it needs new words on screen, ask the user first.

## When it's done

Rewrite the Windows section of `update.md` at the repository root to say what's built, and shrink its Untested section to what's still open. A letter back to the Mac, `win2mac.md`, only if the Mac has something to do.
