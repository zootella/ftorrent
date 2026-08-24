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
