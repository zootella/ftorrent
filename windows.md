# Windows check — desktop workspace

For the Claude Code session on the Windows machine, from the session on the Mac, 2026-08-24. The desktop client is developed on the Mac and verified here. This file travels by git; append your results at the bottom and the user will carry it back.

## House rules

- Git is read-only for you. The user makes every commit, push, and branch.
- In any file that gets committed, write "the user" — never a personal name.
- Report what actually happened, including output that surprised you. Don't tidy results.
- If a lockfile changes during install, that is a finding to report, not a problem to fix. Never delete or regenerate `pnpm-lock.yaml` or `Cargo.lock` — both are committed and cross-platform on purpose.

## What changed since your last pass

The last Windows pass built the bare scaffold. Since then the workspace gained Vue Router, a `disk.rs` module of filesystem commands, and Tauri's dialog plugin — which pulls in `rfd` and a family of `windows-targets` crates this machine has never compiled. The bundle targets were also narrowed, and the Rust sources were re-indented, which is whitespace only and needs no checking.

## The checks

Run from `desktop/` after `pnpm install` at the repo root.

- **`pnpm install`** — do `pnpm-lock.yaml` and `desktop/src-tauri/Cargo.lock` stay byte-identical? `git status` should be empty.
- **`pnpm build`** — this is the one that matters most. It should produce **only** an NSIS `.exe` and no `.msi`. Name every file that lands under `src-tauri/target/release/bundle/`.
- **`pnpm build-app`** — an open question, and a one-line answer either way. This runs `tauri build --bundles app`, and `app` is the macOS bundle format. Does the CLI accept it here, or reject it? Paste the error if there is one.
- **Install and launch** the NSIS installer. Confirm it installs without a UAC prompt and without asking whether to install for one user or the whole machine.
- **Click between Main and About** in the running app, and use the greet box on the main page.

## Report

Append below: what each check produced, the Tauri CLI's version-mismatch warnings if any appeared, and anything that differed from the above. Keep the raw output where it's short enough to paste.

---

## Results from the Windows machine, 2026-08-24

Windows 10 Pro 22H2 (build 19045), x64. Node 22.21.1, pnpm 10.28.2, rustc and cargo 1.98.0. Every check below was run from `desktop/` after `pnpm install` at the repository root, in that order.

Short version: everything passed except `pnpm build-app`, which the CLI refuses on this platform by design. One check went untested by choice, and it is marked untested rather than passed.

### `pnpm install` — clean

Both lockfiles came through untouched. Compared by SHA-256 before and after:

```
487cf6a1ed69a3962e8fec72c91f7c977587abc6cdaa88e2134aa44a9ecd3a7a  pnpm-lock.yaml
c67112e77411d05f18bc6da713d7592f9e168a6d951545bc7e0a70b009a5c5fc  desktop/src-tauri/Cargo.lock
```

Identical on both sides of the install, and `git status --porcelain` came back empty. pnpm resolved nothing — it printed `Lockfile is up to date, resolution step is skipped` and added the 9 packages it was missing.

Two notices appeared that belong to pnpm rather than to this workspace, reported here only so they are not a surprise next time: an update banner (`10.28.2 -> 11.23.0`), and `Ignored build scripts: @parcel/watcher, esbuild` with a suggestion to run `pnpm approve-builds`. Neither affected the result.

### `pnpm build` — one bundle, NSIS only

Finished in 1m13s. This machine compiled the new crates for the first time, as expected: `rfd v0.16.0`, `tauri-plugin-dialog v2.7.2`, `tauri-plugin-fs v2.5.1`, `windows-sys v0.60.2`, `windows-targets v0.53.5`, and `windows_x86_64_msvc v0.53.1`. `Cargo.lock` did not move.

The bundling tail, in full:

```
    Finished `release` profile [optimized] target(s) in 1m 13s
       Built application at: ...\target\release\ftorrent.exe
        Info Patching ...\ftorrent.exe with bundle type information: nsis
        Info Target: x64
     Running makensis to produce ...\bundle\nsis\ftorrent_0.1.0_x64-setup.exe
    Finished 1 bundle at:
        ...\bundle\nsis\ftorrent_0.1.0_x64-setup.exe
```

`Finished 1 bundle` is the direct answer: the narrowing works. Everything now under `src-tauri/target/release/bundle/`:

```
nsis/ftorrent_0.1.0_x64-setup.exe    2026-08-24 11:30    2,024,911 bytes
```

That is the complete list. There is no `.msi` anywhere under `target/`.

One thing worth reporting rather than tidying away: an `.msi` **was** on disk when the build finished, at `bundle/msi/ftorrent_0.1.0_x64_en-US.msi`, dated 2026-08-23 13:07 — the previous Windows pass, from before the targets were narrowed. This build did not produce it, as both the timestamp and `Finished 1 bundle` show. The user deleted it, so the listing above is now the whole truth. The empty `msi/` directory remains and is harmless.

**Version-mismatch warnings: none.** The CLI ran its check (`Info Looking up installed tauri packages to check mismatched versions...`) and reported nothing after it.

### `pnpm build-app` — rejected, and usefully so

The CLI refuses `app` on Windows and names what it will take instead:

```
> tauri build --bundles app

error: invalid value 'app' for '--bundles [<BUNDLES>...]'
  [possible values: msi, nsis]

For more information, try '--help'.
 ELIFECYCLE  Command failed with exit code 2.
```

So the answer is a hard error at argument parsing, before any build work starts, and Windows accepts exactly two bundle values: `msi` and `nsis`. `build-app` is a macOS-only script that happens to live in a shared `package.json`.

Related, and now confirmed rather than assumed: `tauri.conf.json` still lists `"targets": ["app", "dmg", "nsis", "deb"]`, and the bundler filters that list down to the host platform's subset **silently** — no warning that three of the four were dropped here. That filtering is exactly why the build produced NSIS alone, so the behavior is working, but it is quiet about it.

### Install and launch — no prompts of any kind

The installer ran without a UAC prompt and without asking whether to install for one user or the whole machine. Setup completed and ftorrent launched from the installer with no external confirmation at any step.

That result is mechanically explained rather than merely observed, which matters because a pass here could otherwise be an accident of one machine's settings:

- The installer's embedded manifest requests `requestedExecutionLevel level="asInvoker"`, so it never asks for an elevated token.
- The manifest matters for a second reason too. Windows auto-elevates legacy executables whose filename looks like an installer — and this file is named `ftorrent_0.1.0_x64-setup.exe`, which fits that pattern exactly. The heuristic applies only to executables carrying no embedded manifest, so having one takes the file out of its reach.
- UAC was verified enabled on this machine, at its default consent behavior, with installer detection also enabled. Nothing was switched off, so the absence of a prompt is a real result.
- `installMode: "currentUser"` did its half. The app installed under `%LOCALAPPDATA%\ftorrent`, the uninstall entry landed in `HKCU`, and there is no entry in `HKLM` and nothing in `Program Files`. No privileged write happens anywhere in the script, so there is nothing elevation would have been for.

One note about the account it ran under: the testing account is in the Administrators group, and under UAC that means it runs on a filtered token until something elevates. An elevation request would still have produced a consent dialog. A standard-user account would install this identically, since nothing in the path needs a privilege the filtered token lacks.

This is the per-user install working end to end, which is what lets the planned updater apply an update without ever raising a prompt.

### The running app

**Greet box: works.** Typing a name and submitting returns the greeting from the Rust side, so the webview-to-core round trip is proven on Windows.

**Main and About: not tested.** The user chose to skip it, on the grounds that routing which works on macOS will work here. Recorded as untested rather than passed, so the next pass knows it is still open.

### For the macOS side

Four things, all of them changes that belong on that side rather than this one:

1. **`build-app` is macOS-only.** Nothing to fix, but the shared `package.json` carries a script that hard-errors here, and the valid Windows values are `msi` and `nsis`.
2. **The bundle targets list is filtered silently.** `["app", "dmg", "nsis", "deb"]` works, and Windows correctly produced NSIS alone, but nothing warns that the other three were dropped.
3. **The default window size needs a decision.** `tauri.conf.json` sets the window to 1200 x 1050, which is very nearly square, and on this machine it frequently opens low enough that the bottom edge runs under the taskbar. The cause is not the size by itself: with the title bar and borders the window comes to roughly 1089 tall against 1160 of usable height on a 1920 x 1200 display at 100% scaling, leaving about 71 pixels of slack — and since the config sets no position, the OS cascades each new window down and to the right until that slack is gone. A more traditional default, something near 1280 x 800, restores enough slack that the cascade is harmless, and it does so without taking placement away from the OS, which is worth keeping: the plan supports an installed copy and a portable copy running side by side, and cascading is what keeps two windows visible instead of exactly stacked. This is shared configuration affecting both platforms, so it was left unchanged here.
4. **A note for whenever window geometry gets remembered.** `Monitor.workArea` is available in the pinned `@tauri-apps/api` 2.11.1 and reports the monitor area excluding taskbars and docks, so fitting a window to usable space needs no plugin. It is a runtime API rather than a config option, so it has to be a few lines at startup. One trap to design against: `workArea` is in physical pixels while window sizes and positions are in logical pixels, and a missing conversion cannot be caught on this machine, which runs at 100% scaling.

---
