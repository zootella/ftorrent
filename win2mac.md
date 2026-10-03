# Mac: the Windows installer is ours now, and three shared files changed

A note for the Mac session. It's public and committed, so it names nobody: in anything written for the public, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile. This letter is complete in itself and asks for no reply.

## What changed on the Windows side

**The Windows installer is no longer NSIS.** `desktop/win-setup/` holds a setup program of ftorrent's own: `setup.c`, a 64-bit C program with no interface that unpacks a cabinet from the end of its own file into `%LOCALAPPDATA%\ftorrent`, asks a running copy to exit first, writes the Start menu shortcut and the uninstall entry, and starts the program; `win-setup.js`, the creator, which stages what `tauri.conf.json` names, packs it with Windows' own `makecab`, compiles the stub fresh with the release's icon and version, and appends the payload; and `win-setup.toml`, the one configuration file, holding the uninstall list that `src-tauri/windows/hooks.nsh` used to hold. The README there is the guide. An upgrade writes over the files in place and never uninstalls first, which is what keeps a user's choice of ftorrent for `.torrent` and `magnet:` through a release.

## The three files both machines share

- **`tauri.conf.json`:** `nsis` is gone from `bundle.targets`, which is now `["app", "deb"]`, and the `bundle.windows` block is gone with it. A Mac build is unchanged by either; Tauri still makes the `.app`.
- **`package.json`:** `installer` is `tauri build && node dmg.js && node win-setup/win-setup.js`. The new script returns at once on a Mac, exactly as `dmg.js` does on Windows. `scripts.js` now looks for the exe under `bundle/win-setup` rather than `bundle/nsis`, which only matters on Windows.
- **`instance.rs`:** a handoff whose only argument is `--exit` quits the running copy the way File, Exit does, instead of queuing it for the page. It lives in the Windows-only pipe code, under `cfg(target_os = "windows")`, so nothing changes on the Mac; it's how the installer closes a running ftorrent before replacing its files.

Two smaller edits: the comment in `src/associate.js` that pointed at `hooks.nsh` now points at the TOML, and the desktop README's Windows paragraphs, the icon studio README's line about the installer icon, the planning document's mentions of NSIS, and the installing guide's Windows steps all describe the new installer.

## Where to look

- `desktop/win-setup/README.md` for the whole picture, and the essays atop `setup.c` and `win-setup.js` for the mechanics.
- `desktop/README.md`, the Windows installer paragraph and the On Windows paragraph under Packages and releases.
- The planning document's Distribution, Uninstall, and Paths §1 sections, revised to what's built.
