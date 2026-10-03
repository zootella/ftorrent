# Portable Build

The foundation under the portable edition is in place. A `portable/ftorrent.toml` beside the program makes a copy portable, with its data in that folder, WebView2's profile included on Windows. A copy registers with Windows only when it runs from the installer's folder. Every copy holds a lock on its data folder and on each download folder it uses. What's left is making the thing a user downloads: a zip with Windows and macOS side by side, published and documented, and tested on both machines. This note burns down as the steps finish.

## The layout

The zip unpacks to one folder:

```
ftorrent/
ftorrent/ftorrent.exe                   # Windows
ftorrent/ftorrent-engine/               # the Windows engine, where an installed copy also keeps it
ftorrent/ftorrent.app/                  # macOS, identifier com.ftorrent.portable, engine inside
ftorrent/portable/ftorrent.toml         # its presence is what makes a copy portable
```

After a first run, the portable folder holds the rest, and downloads land beside it:

```
ftorrent/portable/ftorrent.lock
ftorrent/portable/state
ftorrent/portable/EBWebView/            # WebView2's profile, on Windows
ftorrent/downloads/
ftorrent/downloads/.ftorrent/
```

`portable/ftorrent.toml` ships holding only `folders = ["./downloads"]` under `[downloads]`, and the page writes out the rest of the file on the first run. Linux isn't in the zip; it keeps its four packages.

## Translocation

A quarantined Mac app opened where it arrived runs from a random read-only folder under `/private/var/folders/…/AppTranslocation/`, and **Open Anyway** doesn't stop it; only removing the mark does. A portable copy unpacked with Archive Utility can't see its `portable/` folder, so it acts installed, and works. The portable instructions clear the mark after unpacking, with `xattr -dr com.apple.quarantine ftorrent`, or fetch the zip with `curl`, which sets no mark. A zip unpacked on Windows onto an exFAT stick carries no mark at all.

## Host baselines

What an installed copy leaves, launched once and quit, to compare a portable session against:

- **Windows 10.** The installer writes the program to `%LOCALAPPDATA%\ftorrent`, an uninstall entry and the install path under `HKCU`, and the Start menu shortcut. Launching adds `%LOCALAPPDATA%\com.ftorrent.ftorrent` with `EBWebView`, `ftorrent.toml`, and `ftorrent.lock` inside, and the association registration under `HKCU`. Nothing ftorrent-named appears under `%APPDATA%`.
- **macOS 15.** Launching creates `~/Library/WebKit/com.ftorrent.ftorrent`, `~/Library/Caches/com.ftorrent.ftorrent`, and `$TMPDIR/com.ftorrent.ftorrent`, all the web view's, and `~/Library/Application Support/com.ftorrent.ftorrent` with `ftorrent.toml` and `ftorrent.lock`.

A portable session should add none of ftorrent's own files to the host. What the operating system keeps anyway, for any program that runs:

- **Windows.** SmartScreen's approval, the shell's recent and jump-list entries, and firewall rules if the first listen prompt is answered.
- **macOS.** WKWebView's folders under the bundle identifier, here `com.ftorrent.portable`, since no public API moves them; possibly `~/Library/Saved Application State/`; Launch Services' registration of the app; Gatekeeper's approval; any privacy grant for the removable volume. On an exFAT stick, macOS writes a `._` file beside every file it touches.

The installing page lists these plainly.

## Steps

1. **The portable Mac bundle.** `tauri.portable.conf.json` beside `tauri.conf.json`, applied with `tauri build --config tauri.portable.conf.json --bundles app` as a JSON merge patch: `identifier` becomes `com.ftorrent.portable`, and `bundle.macOS.infoPlist` set to `null` drops `src-tauri/macos/Info.plist`, so the portable bundle declares no document types or URL schemes, and Launch Services never hands it a click. `--bundles app` makes the `.app` and no disk image. A `pnpm` script names the command.
2. **The Windows half.** On Windows, a script packs the release `ftorrent.exe` and its `ftorrent-engine/` folder, the same ones the setup program carries, into `portable_win.zip` with `tar -a`, which writes forward slashes. `pnpm hash` writes its sidecar, `portable_win.zip.json`, which is committed and pushed, and `pnpm upload` puts the zip at `https://ftorrent.com/portable_win.zip`, linked from nowhere. The server is only the courier.
3. **Assembly, on the Mac.** A script downloads `portable_win.zip`, hashes it, and stops unless the hash matches the committed sidecar. It unpacks the Windows half, adds the portable `.app` and `portable/ftorrent.toml`, and zips the folder as `ftorrent.zip` with no `__MACOSX` folder, no `._` files, and no symlinks, since the exFAT stick it's meant for can't hold one. The Mac zips because Windows zip tools don't record the executable bit the `.app`'s binaries need.
4. **Publish.** `pnpm hash` and `pnpm upload` for `ftorrent.zip` and its sidecar, like the other packages.
5. **The installing page.** A portable section in `docs/docs/installing-ftorrent.md`: the download box, clearing the mark on a Mac before the first launch, the WebView2 runtime a Windows 10 machine without it needs (Tauri's own message names the download), and the host records above.
6. **Tests**, on both machines:
	- Run an installed copy and a portable copy side by side; each keeps its own folder, window, pipe, and lock.
	- Give both the same download folder; the second shows it as in use and loads nothing from it.
	- After a portable session, `%LOCALAPPDATA%\com.ftorrent.ftorrent\EBWebView` keeps its timestamp, and the host gained nothing beyond the list above.
	- Unzip on Windows onto an exFAT stick, carry it to the Mac, clear the mark, and run the `.app`; then the other way round.
	- Launch a portable Mac copy without clearing the mark and confirm it runs as installed; clear the mark and confirm it runs as portable.
	- On Windows, two signed-in users launch the same portable copy from one stick; the second launch leaves and the first keeps running.
	- Sign in as two users and run each user's installed copy at once, on Windows and on macOS.
	- Locks on a real USB stick on both platforms, and a download folder on an SMB share.
7. **The Mac's Downloads prompt.** An installed Mac build raises the privacy prompt at startup, checking `~/Downloads/ftorrent`, and the page fills in only after it's answered. ftorrent should warn first, as the planning doc's File access story says, and a portable copy on a stick meets the removable-volume prompt the same way.

## Also waiting, outside the portable build

- **Each folder to the engine once.** Two spellings of one download folder are held once but would both reach the engine. `lock_take` should answer the lock file's real path, and the page hand the engine each real folder once, before the engine loads torrents.
