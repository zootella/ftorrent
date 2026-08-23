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

## Addendum: the Windows session's reply

Written from the Windows machine on August 23, 2026, after working through everything above. Kevin cloned the repo fresh from GitHub and ran every install, build, and git command himself; this section records what happened and what turned up.

### The line-endings setting

`core.autocrlf` was `true`, planted exactly where the letter predicted — the system config that Git for Windows installs at `C:/Program Files/Git/etc/gitconfig`. A global `input` now overrides it, since global outranks system:

```
file:C:/Program Files/Git/etc/gitconfig	true
file:C:/Users/Kevin/.gitconfig	input
```

`git ls-files --eol desktop` reads `i/lf w/lf` across all seventeen tracked text files. The workspace is clean LF on both machines, as designed.

One correction to the reasoning above, though. `input` does no conversion at checkout, so it neither converts files already on disk nor makes the rest of this repo LF in the working tree — eighty-six tracked text files outside `desktop` and `classic` are still `i/lf w/crlf` here, and stayed that way after the setting changed. That is harmless, because `input` and `true` both normalize CRLF to LF on the way into a commit, so the repository stays LF either way. But the letter's picture of one setting keeping both working trees LF holds only for `desktop`, and only because `desktop/.gitattributes` is there enforcing it. Everywhere else the policy lives in machine config, not in the repo.

### What we tested

Everything the letter asked for, against a fresh clone:

- `pnpm install` at the root, with no flags — resolved against the committed lockfile and left it byte-identical. A bare install uses an existing lockfile and rewrites it only when some `package.json` disagrees; `--frozen-lockfile` does not change the resolution, it only turns that disagreement into an error instead of a rewrite.
- `pnpm build` in `desktop` — release build, a few minutes, producing both Windows installers, `bundle/msi/ftorrent_0.1.0_x64_en-US.msi` and `bundle/nsis/ftorrent_0.1.0_x64-setup.exe`, alongside an 8.96 MB `release/ftorrent.exe`. The config's `"targets": "all"` resolved correctly for this platform.
- `pnpm local` — the debug profile compiled in 2m 08s, the window opened, and the greet field round-tripped text into Rust and back.
- `pnpm win` — the built release exe launched immediately. Worth noting this script had never run anywhere before, being the Windows-only sibling of `app`.
- No version-mismatch warnings. The crate graph resolved to `tauri v2.11.5` against npm's `@tauri-apps/cli ^2.11.4` and `@tauri-apps/api ^2.11.1` — all within 2.11.x, and the CLI said nothing.
- Both lockfiles untouched throughout. `git status` stayed empty across the clone, the install, a release build, and a debug build. The entire Windows verification modified nothing in the repository.

One thing worth knowing next time: `tauri dev` and `tauri build` compile into separate profile directories, `target/debug` and `target/release`, which share no artifacts. Running one after the other means a second full compile from scratch, and that is expected rather than a symptom of anything wrong.

### What we skipped

HMR, deliberately. Kevin had confirmed hot module replacement the day before on a sister project running the identical stack, so we waived the test rather than spend the cycle. It is the one item in the letter this session did not observe firsthand.

### Finding: `upgrade-wash` deletes the wrong lockfile

`desktop/package.json` carries this pair of scripts:

```
"upgrade-wash": "pnpm wash && rimraf pnpm-lock.yaml src-tauri/Cargo.lock",
"wash": "rimraf dist node_modules src-tauri/target src-tauri/gen"
```

Both run with the working directory set to `desktop`, so `rimraf pnpm-lock.yaml` resolves to `desktop/pnpm-lock.yaml`. No such file exists — this monorepo keeps one lockfile at the root — and rimraf treats a missing path as nothing to do, so that argument passes in silence while `src-tauri/Cargo.lock` really is deleted. The script came over from Fuji, where the app is the repository root and the lockfile sits beside `package.json`; moved into a workspace, the relative path stopped pointing at anything.

This is not a Windows problem. rimraf resolves relative paths against the process working directory identically on both platforms, and pnpm sets that directory the same way, so mac and Windows behave the same here. Either machine can make the fix, and it is one line of JSON.

The repair worth making is not to correct the path. Writing `rimraf ../pnpm-lock.yaml` would make the script work as literally intended and turn it into something more dangerous: a script inside one workspace that discards dependency resolution for all five, which is precisely the cross-machine churn the letter warns against. A workspace script should own only what the workspace owns:

```
"upgrade-wash": "pnpm wash && rimraf src-tauri/Cargo.lock",
```

`Cargo.lock` genuinely belongs to `desktop`, so deleting it to re-resolve the Rust graph is a legitimate thing for this script to do. If a monorepo-wide lockfile reset is ever wanted, it belongs in a script at the root, where its scope is visible from the place it gets invoked.

### The one gap left in the line-ending rules

Scoping `.gitattributes` to `desktop` was the right instinct — a repo-wide `eol=lf` really would renormalize `classic`. It leaves one gap. Those twenty-two files are `i/crlf`, meaning CRLF inside the repository itself, committed that way in 2010 and never renormalized since, with no attribute holding them there. `input` protects them at checkout, where it converts nothing, but not at commit, where it normalizes CRLF to LF exactly as `true` would. Editing and committing one of those files from Windows would flip the whole file to LF and produce a diff touching every line.

The risk is small but has a specific shape: `classic` is preserved Win32 source, unlikely to be edited at all, and if it ever is, it will be edited here rather than on the mac. Following the pattern `desktop` already sets, where each workspace states its own policy, the fix is a `classic/.gitattributes` holding one line:

```
* -text
```

`-text` means no conversion in either direction, preserving the index exactly as it stands. Marking those files `eol=crlf` instead would renormalize all twenty-two, which is the outcome the original scoping decision was made to avoid.

The eighty-six other files need nothing. They are the website and server-side workspaces, which are mac-only work by choice — the desktop client is what brings us to Windows — so nothing here will edit them, and `input` covers the case where something someday does.
