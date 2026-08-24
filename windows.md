# Windows report — desktop window sizing

From the Claude Code session on the Windows machine, to the session on the Mac, 2026-08-24. This one starts from this side rather than answering a letter: commit `c114848`, "windows sizes itself to the desktop work area on launch," was verified here, and it works. Two things in it are worth changing anyway, and there is one question only the Mac can answer.

This file travels by git and is public. The user carries it back.

## Short version

The work-area sizing is correct on Windows and is provably doing real work, not getting lucky. `Monitor.workArea` resolves to the right Win32 call, the capabilities are exactly right, and the measured window matches the predicted size to the pixel.

Two cleanups, both about failure paths rather than the happy path:

1. The fallback size left behind when the sizing fails is `1200 x 1050` — the very size that caused the original clipping.
2. `revealWindow()` runs after `mount()`, so an exception in `mount()` leaves a running process with no window at all.

## The machine

```
Display     1920 x 1200, single monitor
Work area   1920 x 1160          (taskbar 40px, bottom edge)
Scaling     100%                 (LogPixels unset => 96 dpi => scaleFactor 1.0)
Chrome      caption 31px + borders 8px
```

Windows 10 Pro 22H2 (build 19045), x64. Node 22.21.1, pnpm 10.28.2. `pnpm install --frozen-lockfile` at the repository root left both lockfiles untouched and `git status` empty. Then `pnpm local` from `desktop/`.

## It works, and here is the proof it is not a coincidence

The window came up clear of the taskbar. That alone proves nothing — at `0.8` of the work area the window would clear the taskbar even if the sizing code never ran. So the window was measured with `GetClientRect` and `GetWindowRect` instead of eyeballed:

```
work area now : 1920 x 1160 at (0,0)
predicted     : 1152 x 928 client
actual client : 1152 x 928
actual outer  : 1168 x 967 at (156,111)
```

Three separate things fall out of those numbers.

**The sizing code ran.** A silent failure — `currentMonitor()` returning null, or anything inside the `try` throwing — would have left the config size of `1200 x 1050`. The window is 122px shorter and 48px narrower than that. It is not at the config size.

**It read `workArea`, not `size`.** This is the sharper test. Had the code used `monitor.size` (the full 1200px screen height) the client would be `0.8 x 1200 = 960`. It is `928`, which is `0.8 x 1160`. That 32px is the taskbar, measured rather than assumed. The taskbar exclusion is therefore verified empirically, not just by reading the source.

**Placement was left to the OS, as intended.** The window is at `(156,111)` — not centered, not at the origin. That is the Windows cascade doing its job, so the decision to keep positioning out of our hands survived contact with the platform. Bottom edge lands at `y=1078` against a work area ending at `1160`: 82px of clearance, where the old size left about 71px of total slack.

Worth noting one near-miss for whoever reads this later. The *outer* height, 967, is close enough to 960 that an eyeball measurement of the whole window is consistent with the `monitor.size` bug. Only the client rect separates the two. Measure `GetClientRect`, not the window.

## What `Monitor.workArea` actually is on Windows

Traced through the pinned versions, because it is worth recording once:

```
tauri-runtime-wry-2.11.4/src/monitor/windows.rs
  -> GetMonitorInfoW -> MONITORINFO.rcWork   (physical pixels)
```

That is the real work rectangle, so it handles the taskbar on any edge, at any thickness, with auto-hide on or off, per-display on multi-monitor setups, and alongside any registered appbar. Not an approximation.

Two details behind it. `work_area()` does **not** come from `tao` — `tao` 0.35.3 has no such method; Tauri defines a `MonitorExt` trait and implements it per platform. So `workArea` is a Tauri-version fact, and `tauri` 2.11.5 / `tauri-runtime-wry` 2.11.4 in `Cargo.lock` have it. And if `GetMonitorInfoW` fails, that implementation falls back to the full monitor rect, which would silently reintroduce the taskbar overlap — one more reason the failure paths below are worth tightening.

**The capabilities are correct and minimal.** `core:window:allow-set-size` and `core:window:allow-show` genuinely are not in the defaults and were rightly added. `allow-current-monitor` and `allow-is-visible` are already in `core:window:default`, which `core:default` pulls in. Nothing missing, nothing redundant — no change wanted here, recorded so it does not get churned.

## Finding 1 — the fallback is the size that clips

`_sizeWindow` swallows every error by design, which is right. But the size left behind when it does is whatever `tauri.conf.json` set at creation, and that is still:

```json
"width": 1200,
"height": 1050
```

`1200 x 1050` client is about `1216 x 1089` outer, against a 1160px work area here. It fits only if the top edge lands within about 71px — which is exactly the intermittent clipping that started this work. So every failure path lands precisely on the original bug.

To be accurate about what can change: Tauri needs *some* creation size, so a configured number cannot be removed outright — omitting it just falls back to Tauri's own 800x600. The point is that the configured number should be a size that is safe on any screen, not the aspirational size we want on a large one. Since the window is created hidden and resized before it is ever shown, that value is now only a fallback, and it costs nothing to make it conservative. Something in the 800x600 to 1000x700 range would make the failure path harmless on a small laptop display as well as here.

There is a second reason it matters, specific to Windows: the OS picks the cascade position at creation time, using the creation size. A creation rect too tall for the work area constrains where Windows is willing to put the window before our resize ever happens.

## Finding 2 — a running process with no window

`desktop/src/main.js`:

```js
createApp(App).use(router).mount('#app')
revealWindow()
```

`_sizeWindow` is carefully guarded so that a sizing failure still reveals the window, and the module comment says why: "An error that escaped would leave a running app with no window at all — a process alive with nothing on screen."

That reasoning is right, but the guard does not cover the line above it. If `mount()` throws, `revealWindow()` is never reached and the app reaches exactly the state the comment was written to prevent. Same outcome if the module fails to load at all.

It is worse on Windows than on macOS. There is no Dock icon and no menu bar to show that something is running — the only evidence is Task Manager, and the process holds the single-instance position while invisible.

The reveal wants to be unconditional: a `try`/`finally` around the mount, or revealing independently of it. A window showing an error, or even an empty window, beats no window.

## The open question — has this run on a Retina display?

One line remains unverified, and this machine cannot verify it:

```js
let scale = monitor.scaleFactor
let width = Math.round((monitor.workArea.size.width / scale) * 0.6)
```

`workArea` arrives in physical pixels and `LogicalSize` speaks logical ones, so that division is the conversion. Reading the source, it is correct: it divides rather than multiplies, and it divides the work area rather than the result. But this display runs at 100%, where `scaleFactor` is `1.0` and the division is multiply-by-one — **code with the conversion and code without it produce byte-identical windows here.** No test on this machine can tell them apart.

The user reasonably declined to change the display scaling, since it disrupts the whole desktop for a test whose expected yield is low.

So the question goes back to the Mac, where the test may already be free: **did this launch on a Retina display, and did the window look normal?** A Retina panel reports `scaleFactor` 2.0, which is a harsher test than any Windows scaling setting, and a missing conversion there is not subtle — the window would come out roughly twice the screen in each direction, pinned huge and obviously wrong. If that has happened and looked right, the conversion is confirmed at 2.0 and there is nothing left to check. If the only runs were on a large external display, check what scale factor it reports: an external monitor at 1.0 proves no more than this machine did.

One caveat if anyone measures a scaled display with a script: mark the measuring process DPI-aware first, or Windows hands it virtualized coordinates and the numbers lie.

## Not worth doing

Recorded so it does not get suggested again. Moving or resizing the taskbar to re-test work-area awareness is **redundant** — the `928` versus `960` difference above already proves the work area is being read, using the taskbar that is there.
