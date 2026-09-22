# Letter to the Windows session — the libtorrent engine

From the Claude Code session on the Mac, to the session on the Windows machine, 2026-09-21. The desktop client gained its engine today: a second process holding libtorrent 2.1.1 with WebTorrent, frozen from Python by PyInstaller, started and stopped by the app's Rust core. It is built and verified on macOS and, through Docker on the Mac, for Linux on both architectures. Windows is the third platform, and the one with the most users. This letter asks you to build it there, answer the questions only Windows can answer, and append a report at the bottom.

This file travels by git and is public. The user carries it back.

## The rules, since a fresh session cannot know them

- Write "the user" in any file, never a name.
- Git is read-only for you: `status`, `log`, `diff`, `show`. The user commits and pushes. Never `checkout` or `restore` a file to undo something; use the editing tools.
- Never delete or regenerate a lockfile. `pnpm-lock.yaml`, `src-tauri/Cargo.lock`, and `desktop/engine/uv.lock` are committed and cross-platform on purpose. If one changes on your machine, that is a finding to report, not a problem to fix.
- Report exactly what happened, including what failed, with the exact text of any error. Never smooth over a result.
- Change code only if a step cannot pass without it. If you must, keep the change minimal, and describe it precisely in your report. No refactoring, no cleanup.
- `style.md` at the repository root governs any code you write: tabs, the comment style, no abbreviations.
- Do not edit `docs/` or any README. Your findings go in your report below, and the Mac session folds them into the public record.
- No Docker and nothing under `desktop/linux/` is for you. The Linux packages are built on the Mac.

## What changed, in one screen

`desktop/engine/` is new: `engine.py`, a small program that reads newline-delimited JSON on stdin and answers on stdout; `pyproject.toml`, its manifest; `uv.lock`, which pins the libtorrent wheel and PyInstaller for every platform by SHA-256; `.python-version`, which pins the interpreter to 3.13.15; and `ftorrent-engine.spec`, the PyInstaller recipe. `pnpm engine` in the desktop workspace runs `uv sync --frozen` and then PyInstaller, and the result is the folder `engine/dist/ftorrent-engine/`: the executable beside an `_internal` directory holding the interpreter, the standard library, and libtorrent.

Two decisions in that folder matter to you in particular. It is a folder rather than PyInstaller's single-file form, because a single file unpacks itself into a temporary directory on every launch, and that self-extracting shape is what antivirus heuristics on Windows most often flag. And the executable is named `ftorrent-engine` so that Task Manager says whose process it is.

`src-tauri/tauri.conf.json` names that folder under `bundle.resources` in the directory form, so Tauri copies it into the bundle and, during development, next to the debug binary. `src-tauri/src/engine.rs` starts the process from `setup`, writes it `{"command":"init"}`, keeps the `ready` line it answers with, and stops it from the `ExitRequested` and `Exit` run events. On Windows it starts the process with `CREATE_NO_WINDOW`, so no console window should appear behind the app. No shell plugin is registered; the only code that can spawn a process is that Rust. The main page shows one line about the engine, and `src/engine.js` wraps the one command behind it.

`src-tauri/rust-toolchain.toml` is also new and pins Rust 1.98.0, the version this machine and the Mac already run. The first Cargo command after pulling may download that toolchain once as a named version if rustup only holds it as "stable". That is expected; note whether it happened.

## What the machine needs

- Node 22 and pnpm through corepack, as before.
- Rust through rustup, as before.
- [uv](https://docs.astral.sh/uv/), version 0.12.3, which does not exist on the machine yet. Install it with Astral's own installer, pinned to that version, from PowerShell:

```
powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/0.12.3/install.ps1 | iex"
```

Open a new terminal afterward so `uv` is on the path. Do not install Python. uv installs its own copy of 3.13.15 under the user's AppData, in uv's own folder, the first time `pnpm engine` runs, and that copy is the one the engine is frozen against on every platform.

## The steps, and what to record at each

**1. Pull and install.** `git pull`, then `pnpm install --frozen-lockfile` at the repository root. Record that `git status` is empty and both lockfiles are untouched.

**2. Freeze the engine.** From `desktop/`, run `pnpm engine`. Record uv's output, which should show the interpreter download and `+ libtorrent==2.1.1` among the packages, and PyInstaller's last lines. Then record the folder: the total size of `engine/dist/ftorrent-engine/`, the names at its top level, the names at the top level of `_internal/`, and the names inside `_internal/libtorrent/`. Two things I want to know from that listing: whether any `libssl` or `libcrypto` DLL is present anywhere in the folder, and the size of the `.pyd` file.

**3. Run the frozen engine by hand.** In Git Bash:

```
printf '{"command":"init"}\n{"command":"quit"}\n' | engine/dist/ftorrent-engine/ftorrent-engine.exe
```

or in PowerShell:

```
'{"command":"init"}','{"command":"quit"}' | .\engine\dist\ftorrent-engine\ftorrent-engine.exe
```

Expected: one line, `{"event":"ready","libtorrent":"2.1.1.0","webtorrent":true,"python":"3.13.15","frozen":true}`, and a clean exit. Record the line exactly as printed.

**4. Which OpenSSL the Windows wheel carries.** The macOS wheel bundles OpenSSL 3.6.3 as separate libraries; the Linux wheels compile in 3.5.0. From `desktop/engine/`:

```
uv run --frozen python -c "import re,glob; p=glob.glob('.venv/Lib/site-packages/libtorrent/*.pyd')[0]; print(p); print(sorted(set(re.findall(rb'OpenSSL 3\.[0-9.]+ [0-9]+ [A-Z][a-z]+ [0-9]{4}', open(p,'rb').read()))))"
```

Record the output. Together with the DLL question in step 2, it says whether Windows compiles OpenSSL in and at what version.

**5. Type-check the Rust.** From `src-tauri/`, `cargo check`. It should pass now that the freeze exists; tauri-build copies the folder as it goes. Record whether `target/debug/ftorrent-engine/` appeared with `_internal` inside it.

**6. Development mode.** From `desktop/`, `pnpm local`. Record: whether the window opened; the engine line on the main page, exactly as shown; whether any console window flashed or stayed open behind the app; and what `Get-Process ftorrent-engine` in PowerShell shows while the app is up, including its parent if Task Manager's Details tab shows it. Then close the window and run `Get-Process ftorrent-engine` again. Expected: the process is gone.

**7. The installer.** From `desktop/`, `pnpm build`. Record the installer's size and, if you can list its contents, whether `ftorrent-engine\` is inside it. Then install it. The user may prefer to double-click the installer themselves; either way, record what the SmartScreen screen said, since the installer is unsigned and that screen is expected. After installing, record whether `%LOCALAPPDATA%\ftorrent\ftorrent-engine\ftorrent-engine.exe` exists with `_internal` beside it, then launch from the Start menu and repeat the checks from step 6: the engine line, no console window, the process present while the app is up and gone after the window closes.

**8. Uninstall.** Run the uninstaller from Add or Remove Programs and record whether `%LOCALAPPDATA%\ftorrent\` is gone entirely, including the `ftorrent-engine` folder. The client's plan says uninstall must leave nothing behind, and a resource folder is the first thing that might.

**9. Windows Defender, throughout.** This is the question the folder shape was chosen to answer, so watch for it at every step and report exactly. Before you start, note the current state of Windows Security's Protection history. After each of steps 2, 6, 7, and 8, check it again, and run `Get-MpThreatDetection` and `Get-MpThreat` in PowerShell. Record any detection with its name, the file it named, and what Defender did about it. If nothing was flagged, say so plainly for each step; that is the result we hope for and it needs to be stated, not implied. Distinguish SmartScreen on the installer, which is expected, from a Defender detection of a file, which is not.

**10. The tree at the end.** `git status`. Expected: only this file modified. If a lockfile changed, report the diff and leave it as it is.

## Your report

Append it below this line, under a heading `## Reply from the Windows session`, in your own words, with these parts: the machine (Windows version, Node, pnpm, Rust, uv, and whether rustup downloaded a toolchain); each step above with what happened and what was recorded; the Defender findings on their own; the sizes; anything you changed in code, precisely; and any question only the Mac can answer. Then stop, and tell the user the letter is ready to commit.
