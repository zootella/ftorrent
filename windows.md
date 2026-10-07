# Windows: one-click update

A note for the Windows session. It's public and committed, so it names nobody: in anything written back here, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile.

## What's asked

Finish and test the Windows half of one-click update, which the Mac already proves end to end. An installed copy finds a newer version, the user clicks one button, and the copy is replaced by the new one, which opens where the old window was. The check, the download, the hash, the button, and the status line are built and tested on the Mac. What's left touches Windows alone: the detached start in Rust, which only Windows can compile and run, and the test.

## What's already there

The essays at the top of `desktop/src/stores/update.js` and `desktop/src/update.js` carry the design, the second with the Mac's steps and paths from the click to the new window. Rust holds no update logic: it offers general commands, in `net.rs`, `disk.rs`, `process.rs`, and `lifecycle.rs`, and the page sequences them. The essay at the top of `desktop/src-tauri/src/lib.rs` is the rule for anything added down there.

On Windows:

- **The check** reads `https://ftorrent.com/ftorrent.exe.json`, the sidecar `pnpm hash` writes beside the installer. Only an installed copy checks, by `isInstalled` in `desktop/src/paths.js`.
- **The click** reads the sidecar again, makes a temporary folder in the data folder, `%LOCALAPPDATA%\com.ftorrent.ftorrent\update`, downloads `ftorrent.exe` into it with `net_get`'s save path under a 50 MiB ceiling, and compares the SHA-256 `net_get` returns with the sidecar's. A failure at any step removes the temporary folder.
- **`updateInstall`** in `update.js` then calls `process_start` on that file with no arguments, and that's all: the setup program closes the running copy through the instance pipe with `--exit`, waits for its files to come free, writes over `%LOCALAPPDATA%\ftorrent` in place without uninstalling, and starts the new copy.
- **`process_start`** in `process.rs` starts the program directly, never through a shell, with `DETACHED_PROCESS` and `CREATE_NO_WINDOW`, so setup outlives this copy and shows no console. That block is Windows-only, so it hasn't been compiled yet.
- **The store** sets `installable` on macOS alone, so the button doesn't offer the update on Windows until this is tested.

Antivirus heuristics watch for an unsigned program that downloads an executable and runs it, which is why the shape is kept plain: the installer runs from ftorrent's own data folder rather than the system's temporary folder, nothing goes through `cmd`, `powershell`, or any other interpreter, and nothing else is started. Keep it that way through any fix.

The temporary folder stays after the update, since setup runs from it, and the next update's first step removes it. Clearing it sooner, from the page at startup with `disk_rmtree`, is fine if it's simple; leave it if not.

## The changes

- Compile `process.rs`, and the Windows half of `disk_space` in `disk.rs`, which answers through `GetDiskFreeSpaceExW`, and fix what the Windows build needs, keeping the commands general.
- Set `installable` in `desktop/src/stores/update.js` to Windows as well as macOS.
- In `update.js`'s essay, give Windows its own list of places beside the Mac's, in the same form, with the real paths and folders ending in a slash, and replace its last paragraph to describe the steps as they turned out.

## How to check it

Both copies need today's code, so the test takes two builds:

- **The old copy:** change `version` in `desktop/src-tauri/tauri.conf.json` to `0.1.0` for this build only, `pnpm installer`, and install it from `pnpm reveal`.
- **The release:** set `version` back to `0.1.1`, `pnpm installer`, `pnpm hash`, and `pnpm upload`. That publishes `ftorrent.exe` and its sidecar at 0.1.1, the version the Mac already published.

Then, with the log on, `[log] record = true` in the old copy's `ftorrent.toml`, writing to `%USERPROFILE%\ftorrent-logs`:

- **The click:** in Settings, Check for Update turns into Update ftorrent, with `ftorrent 0.1.1 released` and the upload's date below it. Click it. The status line gains `, Downloading...`, the window goes, and the new one opens in the same place.
- **What's installed:** `%LOCALAPPDATA%\ftorrent\ftorrent.exe` reports 0.1.1 in its properties, the Start menu shortcut and a taskbar pin still open it, and the associations and the login entry are still in place.
- **No prompts:** neither SmartScreen nor UAC appears. The downloaded file carries no `Zone.Identifier` stream, which `Get-Item <path> -Stream *` shows, and that's why SmartScreen stays quiet.
- **No console:** no console window flashes as setup starts.
- **Defender:** nothing in Windows Security's protection history during the download or the install.
- **The logs:** the old copy's file shows the download and the hash; the new copy's shows it starting at 0.1.1.

## What changes on the page

Nothing new appears in the UI on Windows: the button, its words, and the status line are already written and approved, and errors go to the log, never to the page. If something looks like it needs new words on screen, ask the user first.

## When it's done

Rewrite the Windows section of `update.md` at the repository root to say what's built, and shrink its Untested section to what's still open. A letter back to the Mac, `win2mac.md`, only if the Mac has something to do.
