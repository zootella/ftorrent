# Letter to the Windows session

This letter is for a Claude Code session on a Windows machine, working in a fresh pull of this repo. The `desktop` workspace was just scaffolded on macOS — Tauri 2.11, Vue 3.5, Vite 8, Tailwind 4, patterned after Fuji, the sister Tauri project — and verified there: workspace install, vite build, cargo check, dev window, release bundle. This session is the Windows half of that verification, plus one git setting first.

## First, the line-endings setting

Check what this machine has, and where it came from:

```
git config --show-origin --get core.autocrlf
```

Git for Windows usually plants `true` in the system config at install time. Suggest to the user setting `input` globally:

```
git config --global core.autocrlf input
```

`input` normalizes CRLF to LF at commit and does no conversion at checkout, so the repository and this working tree both stay LF, matching the mac. `desktop/.gitattributes` enforces the same policy from inside the repo (`* text=auto eol=lf`); the global setting extends the same behavior to every repo on this machine, including ones without granular settings. The desktop files are all new in this pull, so with the setting in place they arrive LF with nothing to renormalize. Verify:

```
git ls-files --eol desktop
```

Every line should read `i/lf w/lf`.

## Then, verify the scaffold

This machine already builds Fuji, so the toolchain — Node 22.12+, corepack-pinned pnpm, Rust with the MSVC target — is in place. From the repo root:

```
pnpm install
```

Corepack enforces the pinned pnpm, 10.28.2. Then from `desktop/`:

```
pnpm local   # tauri dev — first run compiles every crate, takes minutes; then a window opens
pnpm build   # release build — bundles land under src-tauri/target/release/bundle/
pnpm win     # start the built exe
```

What to watch for:

- No version-mismatch warnings from the Tauri CLI — npm packages and Rust crates are both on 2.11.x and should agree.
- The window renders the scaffold's Vue demo, and the greet field round-trips text into Rust and back.
- HMR works: with `pnpm local` running, edit a template string in `src/App.vue` and watch it hot-swap.
- Both lockfiles (`pnpm-lock.yaml`, `Cargo.lock`) are committed and cross-platform by design — pnpm records every platform's native binaries and selects at install time. Never delete or regenerate a lockfile to make a Windows problem go away; that's a finding to report.

## What not to chase

Fuji does real work detecting the display's physical and virtual resolution; ftorrent needs none of that. If any of those concerns look like they leaked into documents or code here, ignore them — this scaffold's Rust core carries only the stock greet command.

## Ground rules

- `style.md` at the repo root governs code, especially comments.
- Read-only git commands freely; every mutating git command — add, commit, push, pull — is the user's alone.
- Report results exactly as they happened: pass with output, or fail with output.
