# Letter to the Windows session

This letter comes from the Claude Code session on the Mac, which does most of the desktop client's development. The Windows machine is where the Windows side gets built and checked. The user carries this file between the two machines through git: they commit and push it on the Mac, you pull it, you add your answers at the end, and they commit and push it back.

## Rules for this session

- This repository is public, and everything committed to it stays public. In anything you write into a tracked file, refer to the person at the keyboard as "the user", never by name. A path that contains the user's account name gets `username` in its place.
- Git is read-only for you: read status, logs, and diffs freely, but never commit, push, branch, or delete. The user does all of that.
- Report results exactly as they happen, including anything that fails or looks different from what this letter expects. A surprise is the most useful thing you can send back.
- Never regenerate a lockfile. `pnpm-lock.yaml`, `Cargo.lock`, and `desktop/engine/uv.lock` are pinned on purpose. This round's commit already added one line to `Cargo.lock`, `"tokio"` under ftorrent's own dependencies; if a build wants to change anything more, stop and say so.
- Indent with tabs in any file you edit.

## Background

The sprint's current step is instance management, described in `portable-sprint.md` at the repository root. ftorrent runs once per copy, with one window. A second launch of a copy that's already running hands over what it carried and exits, and closing the window hides it while ftorrent keeps running. Three files hold it, and each opens with a comment explaining it:

- `desktop/src-tauri/src/instance.rs`: the lock, an exclusive lock on `ftorrent.lock` in the data folder, and the handoff. On Windows the copy holding the lock serves a named pipe, `\\.\pipe\ftorrent-` followed by a hash of the lock file's path. A launch that finds the lock held writes its arguments into the pipe as one line of JSON and exits, retrying for up to five seconds if the pipe isn't there yet.
- `desktop/src-tauri/src/lifecycle.rs`: the close button hides the window, the tray icon brings it back and quits, and a handoff brings the window forward.
- `desktop/src/pages/MainPage.vue`: the page now shows everything in one read-only box with a **Copy** button under it. The last lines list each request as it arrives, numbered, as `launch` for the copy's own command line or `handoff` for one handed over by a second launch.

On the Mac, everything that exists there passed: the lock, a second launch exiting, closing and reopening from the Dock, quitting, and a `kill -9` leaving no stale lock. The pipe, the tray, and the compile itself exist only on Windows. The Mac couldn't cross-compile, because Tauri's build script needs `llvm-rc` to embed the Windows resources. So this letter is the first time any of the Windows code runs.

Windows limits which process may take the foreground. So when a second launch asks the running copy to come forward, the window may come to the front, or it may only flash in the taskbar. We don't know which yet, and we'd like to find out.

## What to do

The development build loads its page from the Vite dev server. So for the steps that run `target\debug\ftorrent.exe` directly, keep `pnpm dev` running in its own terminal in `desktop`. The page's **Copy** button gives you its text to report; ask the user to press it and paste.

1. Pull, and confirm the working tree is clean.
2. In `desktop\src-tauri`, run `cargo build`. Report every warning and error. If it fails, stop here and report.
3. **Cold launch with an argument.** Start `pnpm dev`, then from `desktop\src-tauri` run `target\debug\ftorrent.exe "magnet:?xt=urn:btih:one"`. We expect the page to show `lock: held, C:\Users\username\AppData\Local\com.ftorrent.ftorrent\ftorrent.lock`, then `handoff: named pipe \\.\pipe\ftorrent-` followed by sixteen hex digits, then `1. launch: magnet:?xt=urn:btih:one`. Also list the machine's pipes and confirm that name is among them: `[System.IO.Directory]::GetFiles("\\.\pipe\") | Select-String ftorrent`.
4. **A second launch.** With it running, run `target\debug\ftorrent.exe "magnet:?xt=urn:btih:two"`, and time it with `Measure-Command`. We expect it to exit quickly, with no second window, and the page to add `2. handoff: magnet:?xt=urn:btih:two`. Ask the user whether the window came to the front or only flashed in the taskbar.
5. **Several arguments in one launch.** Run `target\debug\ftorrent.exe a b c`, and expect one new line: `handoff: a b c`.
6. **Ten at once.** Run `1..10 | ForEach-Object { Start-Process .\target\debug\ftorrent.exe -ArgumentList "magnet:?xt=urn:btih:burst$_" }`. Wait a few seconds, then report the page's list. We expect all ten burst lines, in any order, with no second window, and exactly one `ftorrent.exe` running afterward: `Get-Process ftorrent -ErrorAction SilentlyContinue`.
7. **The window's three states.** Each time, run a second launch with a new argument, and report where the window ended up and whether the argument arrived:
	- Close the window with its X. First confirm `ftorrent.exe` and `ftorrent-engine.exe` are both still running and the window is gone.
	- Minimize the window.
	- Leave the window open behind another application's window.
8. **The tray.** With the window hidden by its X, find ftorrent's tray icon, which may be in the overflow behind the ^ arrow. Its tooltip should say ftorrent. Left-click it, and the window should come back. Right-click it, and the menu should have **Show ftorrent** and **Exit**. Try **Show ftorrent** with the window hidden, then choose **Exit**. Confirm `ftorrent.exe`, `ftorrent-engine.exe`, and every `msedgewebview2.exe` whose command line names `com.ftorrent.ftorrent` are gone.
9. **A click during a cold start.** With nothing running, start two launches as nearly together as PowerShell can, the second while the first is still starting: `Start-Process .\target\debug\ftorrent.exe -ArgumentList "magnet:?xt=urn:btih:cold1"; Start-Process .\target\debug\ftorrent.exe -ArgumentList "magnet:?xt=urn:btih:cold2"`. We expect one window listing both, one as `launch` and one as `handoff`, in whichever order they won. Repeat it three times, choosing **Exit** from the tray between runs, and report each run's list.
10. **No stale lock.** With it running, kill it outright: `Stop-Process -Name ftorrent -Force`. Report whether `ftorrent-engine.exe` exits by itself; it should, because its pipe closes. Then launch again, and confirm it comes up with its lock held.
11. **The installed copy.** Stop `pnpm dev`, run `pnpm installer`, and ask the user to install it and launch ftorrent from the Start menu. Then ask them to launch it from the Start menu a second time, and report whether one window came forward, or flashed, and no second window appeared. Then close it with the X, and confirm it's still running and the tray icon is there. Finish with **Exit** from the tray.

## Your answers

Add a section here headed with the date, and report each numbered step: what you ran, and what happened.
