# Windows: one-click update

A note for the Windows session. It's public and committed, so it names nobody: in anything written back here, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile.

## What's asked

The Windows half of one-click update. An installed copy finds a newer version, the user clicks one button, the window goes away, and it comes back updated. The Mac half is built, tested, and recorded. This letter says what it is and what it learned on the way, and leaves the Windows design to you: do the research first, the way the Mac did, reading how the established Windows updaters replace a running program before deciding how ours will, then build it, test it, and record it. The thinking should be fresh and Windows's own; what carries over is the priorities and the shape.

## The priorities

Reliability first, simplicity second, and the moment the user sees third. A user who waits a second with no window and then sees ftorrent back, updated, is fine. A user who meets a broken app or a strange system dialog is done with ftorrent. Where a smoother moment would cost a step that could fail strangely, the plain step wins, and one flow that always runs beats two that each run sometimes.

## What the Mac learned

The first Mac version had the running copy move its own bundle aside, move the newer one into place, and then quit. It worked once, and then showed why the established updaters don't do it that way. A running program whose files move out from under it keeps running, but loses what the system tied to its location, which on the Mac is every privacy grant, for the rest of that process's life. The file manager, having watched files shuffled beneath it, kept a stale view and showed the user a dialog about an item named "". A fixed wait for the old copy to finish quitting could give up on a slow quit and leave nothing running. And how the new copy gets launched decides which process the system holds responsible for it afterward, which on the Mac decides whose name is on its privacy prompts.

The rebuilt sequence follows Sparkle, the updater most Mac apps outside the App Store use, and each step is there for one of those reasons: the old copy quits before its files are touched, and the installer waits for the signal that it has truly ended, however long that takes; the switch is one atomic exchange under the same name, which happens whole or not at all, so a failure leaves the installed copy exactly where it was and the file manager sees a single replacement; the new copy is launched through the system's own launcher, by path, so it stands on its own; and whatever stops the installer, it opens whichever copy is at the installed path, the newer one or the old one, so a click never leaves nothing running. Two more rules shaped it. Refuse up front rather than fall back: a copy that can't replace itself never offers the button, and when a check finds a newer version, its status line says to get it at ftorrent.com, with a link that opens the browser. And stage in one place: the download and the unpack always happen in a temporary folder in ftorrent's own data folder, never the system's temporary folder, and the cases where that can't work are turned away before the download rather than handled after it.

Expect each of these to have a Windows counterpart with a different mechanism underneath: what a running executable's files allow and refuse, what the shell keeps and when it notices a change, how a program should be started so the system treats it as the user's own and shows no console, what antivirus heuristics and SmartScreen make of an unsigned program that downloads an executable and runs it, what a launch from the Start menu does while the files are being replaced, and what an update has to leave intact, the shortcut, a taskbar pin, the associations, the login entry. Find out what the established Windows updaters do about each, and let that decide the design.

## Where the Mac half lives

`update.md` at the repository root is the what and the why. The essay at the top of `desktop/src/update.js` is the full sequence with every path it touches, and the reasons for the order; `desktop/src/stores/update.js` is the check, the button, and the status line. `desktop/src-tauri/src/install.rs` is the installer's half, the moment that runs before any page exists, and it's macOS only; its essay says how a startup-moment sequence fits the rule in `lib.rs` that Rust stays general.

Everything else Rust offers is general commands, and the page sequences them: `net.rs` fetches an https address, or saves it to a file and answers its SHA-256; `disk.rs` has `disk_rmtree`, `disk_space`, `disk_access`, and a device number on `disk_stat`; `process.rs` runs a program and waits, or starts one and lets it go; `lifecycle.rs` quits the way the menus do; and `instance.rs` holds the lock, serves the pipe, and knows `--exit` and `--update`. The Windows halves of `process_start`, `disk_space`, and `disk_access` are written but have never compiled, since only Windows can; `disk_access` answers from the read-only attribute alone, and the device number is 0 there. Make real whichever of these Windows turns out to need, and keep every command general.

On Windows today the platform-general part already runs: the check reads `https://ftorrent.com/ftorrent.exe.json`, and the click makes `%LOCALAPPDATA%\com.ftorrent.ftorrent\update`, downloads `ftorrent.exe` into it, checks the hash, and starts that file with no arguments, detached and with no console, after which the setup program in `win-setup/setup.c` is the orchestrator: it closes the running copy through the instance pipe with `--exit`, waits for the files to come free, writes over the install folder, and starts the new copy. Whether that is already the right shape, or needs changing in the light of the research, is yours to decide. `updateInstallable` in `update.js` turns Windows away for now, so the button is never offered there, and a newer version shows the ftorrent.com link instead.

## What's yours to do

- The research, with the sources named, written up in `update.md`'s Windows section the way its Mac section records Sparkle's rules, each tied to what would break without it.
- The design and the build, in the same shape as the Mac's: the page sequences general commands, and Rust gains only what Windows alone can do. If Windows needs a startup-moment piece of its own, it lives beside `install.rs` by the same rule; if the setup program is the whole answer, say so.
- One `update.js`, not two: Windows and Mac sections inside it where they differ, and its essay gains a Windows list of places beside the Mac's, in the same form, with the real paths and folders ending in a backslash, and Windows's own pitfalls beside the Mac's in the paragraph on why.
- What `updateInstallable` checks on Windows, and turning it on there.
- The test, and the record of it.

## How to check it

Both copies need today's code, so the test takes two builds, the old copy one version below the release: `pnpm installer` at the lower version and install it from `pnpm reveal`; then raise `version` in `desktop/src-tauri/tauri.conf.json`, `pnpm installer`, `pnpm hash`, and `pnpm upload`. Then, with the log on, `[log] record = true` in the old copy's `ftorrent.toml`, writing to `%USERPROFILE%\ftorrent-logs`:

- **The click:** in Settings, Check for Update turns into Update ftorrent with the release named below it. Click it. The window goes, and the new one opens in the same place; the logs' timestamps say how long that took.
- **What's installed:** `%LOCALAPPDATA%\ftorrent\ftorrent.exe` reports the release's version, and the Start menu shortcut, a taskbar pin, the associations, and the login entry are all still in place and still open it.
- **No prompts, no console:** neither SmartScreen nor UAC appears, no console window flashes, and nothing lands in Windows Security's protection history during the download or the install.
- **The logs:** the old copy's file shows the download and the hash; the new copy's shows it starting at the release's version and removing the temporary folder.
- **The refusals:** make the replacement fail on purpose, by whatever Windows offers, and confirm the old copy comes back on its own; and make a copy that can't replace itself, and confirm the button stays away and the ftorrent.com link appears.

## What changes on the page

Nothing new: the button, its words, the status line, and the ftorrent.com link are written and approved, and errors go to the log, never to the page. If something looks like it needs new words on screen, ask the user first.

## When it's done

Rewrite the Windows section of `update.md` to say what's built and what the research settled, and shrink its Untested section to what's still open. A letter back to the Mac, `win2mac.md`, only if the Mac has something to do.
