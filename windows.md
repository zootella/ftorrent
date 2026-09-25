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

## 2026-09-25, from the Windows session

Windows 10 22H2, Windows PowerShell 5.1, Rust 1.98.0. Steps 1 through 8 passed, most of them exactly as predicted. Step 9 found a real bug: after a cold-start race, the winner has no window and can't get one back. Steps 10 and 11 aren't done, and step 9 stopped after its first run so the bug could be understood; the details are below, and the rest waits for the next letter. One correction to how we tested along the way: partway through, this session piled measurement on top of the tests, polling window state and enumerating window handles while launches flew, and mistook its own noise for a tray failure. That "failure" is retracted below. Everything reported here was checked again with the harness stripped back to the letter's own commands.

**1. Pull and clean tree.** Clean at d611b99, which carries this letter and the one new line in Cargo.lock. The tree stayed clean through every build below.

**2. `cargo build`.** Finished in 42 seconds with no warnings and no errors. The lockfile didn't change. (A release build with `pnpm compile` later also finished clean, in 77 seconds, and was used for steps 8 and 9.)

**3. Cold launch with an argument.** With `pnpm dev` running, `target\debug\ftorrent.exe "magnet:?xt=urn:btih:one"` opened the window with the engine as its child. The page, by its Copy button:

```
engine: libtorrent 2.1.1.0, WebTorrent on, Python 3.13.15, pid 11124
ftorrent is installed, and the engine has its paths
program: C:\Documents\code\ftorrent\desktop\src-tauri\target\debug
data: C:\Users\username\AppData\Local\com.ftorrent.ftorrent
downloads: ~/Downloads/ftorrent → C:\Users\username\Downloads\ftorrent
lock: held, C:\Users\username\AppData\Local\com.ftorrent.ftorrent\ftorrent.lock
handoff: named pipe \\.\pipe\ftorrent-0270cda3d3f095d9
1. launch: magnet:?xt=urn:btih:one
```

The pipe directory listed exactly one ftorrent entry, `\\.\pipe\ftorrent-0270cda3d3f095d9`, the same sixteen hex digits as the page. `ftorrent.lock` existed at 0 bytes, and a second open of it from PowerShell was refused while ftorrent ran, and succeeded after it exited.

**4. A second launch.** `target\debug\ftorrent.exe "magnet:?xt=urn:btih:two"` exited with code 0 in 216 ms under `Measure-Command`, no second window, one `ftorrent.exe` and one engine left, and the page added `2. handoff: magnet:?xt=urn:btih:two`. The second process wrote one line to stderr on its way out:

```
[0924/235105.613:ERROR:ui\gfx\win\window_impl.cc:172] Failed to unregister class Chrome_WidgetWin_0. Error = 1412
```

That line is WebView2's, and it's the visible trace of what step 9 turns on: Tauri creates the configured window, webview inside, before it runs the setup hook, so a second launch builds a whole WebView2 and then `process::exit`s from inside setup. Whether the window came to the front is answered under step 7, where it was measured.

**5. Several arguments.** `target\debug\ftorrent.exe a b c` exited in 196 ms and the page added one line, `3. handoff: a b c`.

**6. Ten at once.** The ten `Start-Process` launches started within 297 ms. Six seconds later there was one `ftorrent.exe`, with one window titled ftorrent, one engine, and the same six WebView2 helpers as before. The page listed all ten, none dropped, in the order they won: burst3, burst2, burst5, burst4, burst9, burst1, burst10, burst8, burst6, burst7.

**7. The window's three states.** Each case sent a second launch with a fresh argument, and each argument arrived on the page. Whether the window took the foreground was read with `GetForegroundWindow` before and after, one case at a time:

- **Hidden by its X.** Both `ftorrent.exe` and `ftorrent-engine.exe` stayed running with the window gone. The handoff brought the window back visible and in the foreground, second process gone in 206 ms. An earlier, unmeasured try of the same case brought the window back without focus, so the answer to the letter's question is: the window comes forward, and once in five tries it came back without taking focus. Windows decides who may take the foreground, and the difference was likely where the last click had been.
- **Minimized.** Restored, and in the foreground, in 220 ms.
- **Behind another window.** The terminal held the foreground before; ftorrent held it after, in 183 ms.

The page after all of this ran to 17 lines: the 13 from steps 3 to 6, then `front`, `minimized`, `behind`, `hidden`, all as handoffs.

**8. The tray.** The tooltip says ftorrent. Left-click brings the hidden window back. The right-click menu has **Show ftorrent** and **Exit**; Show ftorrent brings the window back, and Exit ends everything: no `ftorrent.exe`, no `ftorrent-engine.exe`, zero `msedgewebview2.exe` naming `com.ftorrent.ftorrent`, the pipe gone from the pipe directory, and the lock file free and still 0 bytes. This session first reported the left-click as not working. That was wrong: at the time the screen was full of this session's own consoles and polling windows, and the click was being tested on a copy from step 9 that, it turned out, had no window to show. The user then ran the release binary by double-clicking it, with nothing else going on, and the tray did everything the letter expects.

Two Windows-side changes came out of this step, both in `lifecycle.rs` and wired in `lib.rs`, built and checked here since the platform with the APIs is where its integration is written:

- **A File menu with Exit**, the way every Windows client's File menu ends (µTorrent, qBittorrent, Deluge). It matters more once the X hides rather than quits, because otherwise the tray icon is the only way out, and Windows 11 tucks new tray icons into the overflow. It has no shortcut beside it, as a Windows Exit doesn't: the system's own key is Alt+F4, which is a close, and a close now hides. The item shares the tray's `exit` id, and `app.on_menu_event` sends it to `app.exit(0)`, the same run event the tray reaches. Windows only; the Mac's app menu and Dock are the Mac's.
- **The tray icon is removed in the Exit run event.** The user caught this one: after File, Exit the icon stayed in the notification area until the mouse touched it, the classic ghost of a process that ended without sending the shell its remove message. `app.exit` ends the process while Tauri still holds the tray, so `tray_remove` drops it by id from `RunEvent::Exit`, which every quit path reaches. The tray's own Exit had the same gap. Checked on the release build after the change: File, Exit and the tray's Exit each made the icon vanish at once with the mouse kept away, and left no `ftorrent.exe`, no engine, no WebView2 helper, and no pipe.

**9. A click during a cold start: a real bug.** The two `Start-Process` launches started within 89 ms (release build; 101 ms on an earlier debug run). Six seconds later exactly one `ftorrent.exe` and one engine remained, as expected. But no window ever appeared, and the tray couldn't raise one: neither left-click nor **Show ftorrent** did anything. The process holds the lock, serves the pipe (the pipe entry is present), runs the engine, and shows its tray icon, and it has no window. **Exit** from the tray menu still works in that state: the icon went away, and with it the process, the engine, and the pipe.

Repeated in isolation, release binary, the letter's own two-line command and nothing else running, with each process's stderr redirected to a file:

| gap between the two launches | result | winner's WebView2 helpers |
|---|---|---|
| none, 89 ms | no window | 0 |
| none, 39 ms | no window | 0 |
| none, 101 ms, debug build | no window | 0 |
| none, 67 ms, after the File menu was added | no window | 0 |
| 500 ms | window, `1. launch: half1`, `2. handoff: half2` | 6 |
| 2 s | window, `1. launch: gap1`, `2. handoff: gap2` | 6 |

In every run the loser's stderr had the one `Failed to unregister class Chrome_WidgetWin_0` line, and the winner's stderr was empty, including the three winners whose webview died. WebView2 said nothing about it. So the handoff itself is fine, and the fault is the startup overlap: two processes building a WebView2 on the same profile within a few tens of milliseconds of each other, one of which then exits abruptly.

What the surviving process looks like, from outside:

- `EnumWindows` for its pid finds only three top-level windows: the debug build's console, the tray's hidden helper (class `tray_icon_app`), and `Tao Thread Event Target`. The ftorrent window that a healthy copy has is not there, hidden or otherwise.
- It has **zero** `msedgewebview2.exe` helpers. A healthy running copy has six. So its webview is dead, and the window went with it.
- Nothing in the Application event log; no crash was recorded.

Why, as far as this side can tell from outside the process. Tauri's `setup` in `app.rs` creates every window from `tauri.conf.json`, with the webview inside, and only then calls the app's own setup hook. So in a race, both processes build a WebView2 on the same `EBWebView` profile at the same moment, and WebView2 processes sharing a profile share one browser process. The loser then finds the lock held and calls `std::process::exit(0)` from inside setup, with its webview still initializing. That abrupt exit appears to take the shared browser process down, and the winner's webview with it. The step 4 case never hit this because the running copy's browser process was already up and stable when the second launch came and went.

The mechanism inside WebView2 is inferred, since it logs nothing; what's measured is the overlap and the dead webview. Either way the shape of a fix is the same: decide the lock before any window exists, so the losing launch leaves before it has built a webview. That means trying the lock in `main` before the Tauri builder runs, or taking the window out of `tauri.conf.json` and creating it in code after the lock is known. The sprint plan already moves the window into code for the portable WebView2 folder, so the second may be the same change. The step 4 stderr line would go away with it too, since a loser would never build a webview at all.

**Steps 3 through 7 again, on the final binary, ending with File, Exit.** With the two changes above compiled in, the whole sequence was run once more on the release build with the tray along for the ride and never touched: cold launch with `one`; a second launch with `two` (176 ms) that brought the window to the front; then a launch against the window hidden by its X (210 ms, both processes alive throughout), minimized (171 ms), and covered by another window (213 ms), each of which brought it to the front. The page ended at five lines, `one` as the launch and `two`, `hidden`, `minimized`, `behind` as handoffs, and File, Exit left no process, helper, or pipe. The half-second race after that came up healthy, `half1` as the launch and `half2` as a handoff, and File, Exit ended it the same way. The simultaneous race after that was the fourth row in the table, and the tray was the only way out of it.

**10. No stale lock.** `Stop-Process -Name ftorrent -Force` on a healthy running copy: `ftorrent-engine.exe` exited by itself in 151 ms, the pipe entry was gone, and the lock file opened freely. A launch right after came up normally, and the page showed `lock: held` with its one `launch` line. The same thing happened the two earlier times the app was killed during cleanup, with the engine gone within three seconds each time.

**11. The installed copy.** Not done; this session stopped after the race investigation and the two lifecycle changes above. The uncommitted tree holds `lifecycle.rs`, `lib.rs`, and this reply.
