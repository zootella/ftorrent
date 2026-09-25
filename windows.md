# Letter to the Windows session

This letter comes from the Claude Code session on the Mac, which does most of the desktop client's development. The Windows machine is where the Windows side gets built and checked. The user carries this file between the two machines through git: they commit and push it on the Mac, you pull it, you add your answers at the end, and they commit and push it back.

## Rules for this session

- This repository is public, and everything committed to it stays public. In anything you write into a tracked file, refer to the person at the keyboard as "the user", never by name. A path that contains the user's account name gets `username` in its place.
- Git is read-only for you: read status, logs, and diffs freely, but never commit, push, branch, or delete. The user does all of that.
- Report results exactly as they happen, including anything that fails or looks different from what this letter expects. A surprise is the most useful thing you can send back.
- Never regenerate a lockfile. `pnpm-lock.yaml`, `Cargo.lock`, and `desktop/engine/uv.lock` are pinned on purpose. If a command wants to change one, stop and say so.
- Indent with tabs in any file you edit.

## Background

We're working through `portable-sprint.md` at the repository root, and its first step, resolving paths, is built. The new module is `desktop/src-tauri/src/paths.rs`, and its opening comment explains the model. At startup the app finds its own location and decides whether it's portable: it is if `portable\ftorrent.json` sits beside the executable. It then picks its data folder, reads `ftorrent.json`, and resolves each download folder without checking whether it exists. The engine receives those paths in `init` and sends them back in `ready`. The main page shows the result.

On the Mac, three cases passed: installed, portable, and an unreadable `ftorrent.json`. This letter asks for the first two on Windows. The two things only Windows can show are that an installed copy's data folder is `%LOCALAPPDATA%\com.ftorrent.ftorrent`, with nothing in Roaming, and that paths come out with backslashes.

## What to do

1. Pull, and confirm the working tree is clean.
2. In `desktop`, run `pnpm engine`, because `engine.py` changed to echo the paths.
3. **Installed.** Confirm `desktop\src-tauri\target\debug\portable` doesn't exist. Then run `pnpm local`, which builds and opens the development build. Ask the user to read you the page's lines under the engine line, then close the window. We expect:

	```
	ftorrent is installed, and the engine has its paths
	program: C:\…\desktop\src-tauri\target\debug
	data: C:\Users\username\AppData\Local\com.ftorrent.ftorrent
	downloads: ~/Downloads/ftorrent → C:\Users\username\Downloads\ftorrent
	```

	Watch whether `program:` starts with `\\?\`. It shouldn't.
4. Report what's in `%LOCALAPPDATA%\com.ftorrent.ftorrent` now: we expect `ftorrent.json` beside `EBWebView`. Show the file's contents exactly, and say whether it's indented with tabs. Confirm nothing named ftorrent appeared directly under `%APPDATA%`.
5. **Portable.** Create `desktop\src-tauri\target\debug\portable\ftorrent.json` containing exactly this line:

	```
	{"download_folders": ["./downloads", "C:/Games", "/Volumes/ALICE2TB/torrents"]}
	```

	Run `pnpm local` again, have the user read you the lines, and close the window. We expect `portable` and the data folder `…\target\debug\portable`, with the folders resolving as follows. The third entry is a Mac path, included to see what Windows does with it; we expect it to become a path from the root of the current drive, with backslashes.

	```
	./downloads → C:\…\desktop\src-tauri\target\debug\downloads
	C:/Games → C:\Games
	/Volumes/ALICE2TB/torrents → \Volumes\ALICE2TB\torrents
	```
6. Confirm the portable run left `%LOCALAPPDATA%\com.ftorrent.ftorrent\ftorrent.json` exactly as step 4 found it, and created no `downloads` folder under `target\debug`.
7. Delete `desktop\src-tauri\target\debug\portable`, which is build output that git ignores, so the next development run is installed again.

## Your answers

Add a section here headed with the date, and report each numbered step: what you ran, and what happened.
