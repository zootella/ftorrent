# Simpler Rust

The desktop client splits its code across a boundary. The page, Vue in the web view, holds the application logic: what a setting is, what a download folder is for, when to lock one, what a request from a second launch means. The Rust core holds commands that are atomic and dumb, and that do one thing each: read a file, take a lock, write text at exit, pipe a line to the engine. Rust knows facts about the machine and performs operations on it. It doesn't know what ftorrent is trying to do. Logic kept on one side stays true; logic split across both sides drifts apart, which makes the whole less safe, not more. `disk.rs` says this about the disk, and it holds for every module.

This note plans two essays that put that design into the code where future sessions will read it, then lists the places where the Rust has picked up application logic and what the simpler shape of each is, with what we decided. It stays current as the work happens, and each item leaves it once it's done.

## Where it stands

All four items and both essays are built and tested on the Mac: essay one opens `lib.rs`, and essay two opens `engine.rs` with its counterpart in `engine.py`. Reading every module's essay against essay one turned up one more module that failed the test, `folders.rs`, which knew a download folder holds a `.ftorrent` folder with `ftorrent.lock` inside; it became `locks.rs`, `lock_take` and `lock_release` on any file, with `disk_hide` for the Windows hidden attribute, and the page now owns `.ftorrent`. The same batch took the product's name out of the code: `productName` in `tauri.conf.json` is the one place it's written. Rust reads it through its package info for the window title, the tray, the settings file's name, the lock file, the engine's folder, and the pipe; the page imports the same file at build time through `brand.js` for `.ftorrent`, the ProgIDs and link scheme, the default download folder, and its own text; and the engine gets it in `init` for the client name. Only the engine's server hostnames still say ftorrent, on purpose, since a fork points those at its own servers. What's left is the Windows letter: the Windows-only code compiling, the registry ending up key for key where the Rust version left it, the handoff arriving through the new queue, maximize without a flash, and `.ftorrent` hidden. Once that passes, this note has nothing left to track, and its essays live in the code.

## Essay one: Rust stays general

**Where it goes.** At the top of `lib.rs`, the first file anyone opens in the Rust core, as the essay that frames every module below it. Each module's own essay then says what that module does and needn't repeat the principle, only point at `lib.rs` when a module is tempted to cross the line. `disk.rs` already carries a local version of it for the disk, which stays, trimmed to what's particular to the disk.

**What it says.**

- **The Rust core reads like a general-purpose API.** Each command is atomic, does one thing, and could belong to any desktop application: read a file, write bytes, make a folder, take a lock, write text at exit, start a process, pass a line to it. Reading `lib.rs`'s list of commands should tell you what the machine can be asked to do, and nothing about what ftorrent is.
- **It carries out commands without knowing why.** The page above decides what a setting is, which folders to lock and when, what to tell the engine, and what an arrival means. Rust receives the request and performs it. Why the page wants it is above Rust's pay grade, and a Rust function whose comment has to explain the application's reasons is a sign the reasons leaked down.
- **Why that's safer, not just tidier.** Logic that lives in one place stays true. Logic split across the boundary, half in the page and half in Rust, drifts apart as each half is edited alone, and the gap between them is where bugs live. A guard or a rule in Rust that the page also knows is a second copy of the same knowledge.
- **When more Rust is right.** Rust grows when the operating system is the only one who can do the thing, or when the page can't be there yet. The instance lock and the handoff run before any page exists. Starting the engine happens at launch. The tray, the menus, and hiding the window are platform plumbing. Reading and writing the Windows registry needs native calls. And facts of the file system belong down here, like whether two paths name the same folder. In each case Rust offers the operation in general terms, the way `registry_get` and `registry_set` offer the registry without knowing a single key ftorrent uses, and the page, or the moment of startup, decides what to do with it. Names that belong to ftorrent, a registry key, a settings section, a command the engine understands, a folder's purpose, appearing in Rust is the sign that logic has leaked down.
- **The test for a new command.** Describe it without naming a ftorrent feature. "Lock this folder" passes; "lock the download folders in the settings and drop the ones no longer listed" fails, and the second half belongs to the page. When a command fails the test, split it: the general operation stays in Rust, and the decision goes up.
- **Where the security comes from.** The page runs only ftorrent's own code: untrusted text reaches it through Vue's escaping interpolation and never becomes script, and the Content-Security-Policy keeps foreign script out of the web view. That's what makes it right for Rust to follow the page's commands without second-guessing them, and it's why the Rust commands don't carry guards of their own.

## Essay two: the single roadway to the engine

**Where it goes.** At the top of `engine.rs`, replacing the part of its essay that describes a command per message, with a matching paragraph in `engine.py`'s opening, since the two halves of the roadway need to tell the same story from each end.

**What it says.**

- **libtorrent is commands in and alerts out.** In a plain Python program libtorrent is one process and function calls: calls on the session and on a torrent's handle post requests to libtorrent's own network thread and return at once, and libtorrent reports everything back as alerts, which the program collects in batches. ftorrent adds one process boundary, between the app and the engine, and that boundary is the only wire protocol we own: newline-delimited JSON over the engine's stdin and stdout.
- **One road down, one road up.** Going down, the page builds a command as a line of JSON and sends it through one generic command, `engine_send`, and Rust writes it to the engine without reading it. Coming up, the engine turns alerts and status into lines of JSON, and Rust holds them in a queue until the page takes them, again without reading them. Rust is the wire.
- **The engine is where JSON meets libtorrent.** `engine.py` keeps one dispatch table, one entry per command, and each entry does the translating: a magnet becomes `add_torrent_params`, a torrent's id becomes its handle, an alert becomes plain JSON. The list is also a boundary on purpose: it names exactly what the page may ask the engine to do, where a reflective "call any libtorrent method by name" would hand the page every method libtorrent has.
- **What a new libtorrent feature costs.** Using a libtorrent call ftorrent hasn't used before, say pausing a torrent, means two edits: the page, which decides to pause and sends `{"command":"pause",…}`, and one entry in `engine.py`, which calls `handle.pause()`. What libtorrent answers comes back as an alert the engine already turns into a line. Rust, `lib.rs`, and the JavaScript that talks to Rust don't change. Before this roadway, the same feature meant a new function at every layer: a JavaScript wrapper, a Rust command, its registration, the engine's branch, a Rust status field for the answer, and the page.
- **What stays special.** `init` is built in Rust, because the engine starts before the page exists and needs its version and data folder at once. Starting the engine, stopping it, and keeping its stderr stay in Rust as the process plumbing they are.

## The test

A Rust command passes when it could be described without mentioning ftorrent's features: "lock this folder", "write these bytes to this path when the process exits", "send this line to the engine". It fails when its description needs a setting's name, a policy, or a rule about when something should happen. A command that has to exist before the page does, like taking the instance lock or starting the engine, is allowed to know what it needs for that moment, and nothing more.

The download-folder locks show both shapes. The first version took a list of folders, kept the locks still wanted, released the ones no longer listed, and retried the ones another copy held: application logic in Rust. They became `folder_lock(path)` and `folder_unlock(path)`, one folder at a time, with the page deciding which folders to lock and when, and then, in the read-through, `lock_take(path)` and `lock_release(path)` in `locks.rs`, which lock any file and know nothing of download folders or `.ftorrent`. The one thing that stayed in Rust is deciding whether two paths name the same file, since that's a fact of the file system: asking for a second lock on a file this process already holds would find its own lock and answer busy.

What already passed when this plan was written: `disk.rs`, `paths.rs`, which reports the program's location, the home folder, and the data folder, `desktop.rs`, which writes text handed to it when the process exits, and `lifecycle.rs`, whose menus, tray, and hiding are platform plumbing that can only live in Rust.

## 1. Window placement

**Now.** `settings.rs` parses `ftorrent.toml` to read the `[window]` and `[screen]` sections: the saved position, the inner size, the maximized flag, and the size of the screen they were recorded on. `window.rs` then applies the placement rule. It replays the saved rectangle when the monitor under its middle still has the recorded size. Otherwise it recalculates: five eighths by one half of the primary screen, dropped at a random spot inside the centered three-quarters field. It builds the window there, maximized if it was. So Rust knows the settings schema, the fingerprint rule, and the first-run placement, and it carries the `toml` and `fastrand` crates for this alone.

**Why it moved into Rust.** Two reasons, from the Windows burst. The window had to leave `tauri.conf.json` for code, so that a launch that loses the lock leaves before any web view exists; that fixed the cold-start race, and it stays either way. And building the window at its final size meant it never appeared at one size and jumped to another.

**Decided: the page places the window, and shows it.** Rust builds the window hidden, at any size, with its data folder, from the `Ready` event, as now; the window stays out of `tauri.conf.json`, so a launch that loses the lock still leaves before any web view exists. The page does the rest. It already reads the settings file at startup, and the JavaScript window API has what the placement rule needs: `primaryMonitor`, `monitorFromPoint`, `setPosition`, `setSize`, `maximize`, `show`. So the rule moves into `window.js`, beside the recording it already does, and the whole window rule, recording and replaying, lives in one file in one language. The page places the window while it's hidden and then shows it.

What that changes about startup is ordering, not steps. The page already asks Rust for `ftorrent.toml` and parses it; today it shows the window first and reads the settings after, and afterward it reads first and shows once the window is placed. The added calls are a handful of small IPC round trips on a file under 2 KB, single-digit milliseconds, against the web view's own startup of hundreds, so the window appears no later that anyone could see. Checked by eye on each machine.

**Decided: one bit of state keeps bring-forward safe during startup.** A second launch's handoff, the tray's Show, and a Dock click all bring the window forward through `lifecycle::bring_forward`, and one of them can land in the first moment of startup, before the page has placed the window. Today that's harmless, because Rust built the window already in place. Afterward, showing it then would put it on screen at the builder's default spot, and the page would move it a moment later, a visible jump; the handoff tests fire launches during a cold start on purpose, so this would be seen. So Rust keeps one flag, whether the page has revealed the window yet: the page calls a one-line command once it has placed and shown the window, and until then `bring_forward` does nothing, since the page is about to show the window anyway. The alternative, sending the page an event and letting it do the showing, is purer by the test but comes out within ten lines, and it would route the handoff, the tray, and the Dock through the page, three paths already tested hard on both machines. The page can still show itself or learn that the window came forward from the window's own focus and visibility events, so neither shape closes a door, and ftorrent never comes forward on its own anyway: every case begins with the user.

**What leaves.** `settings.rs`, all of it; `window.rs` down from 80 lines to about 15, the builder call and a short essay; the `toml` and `fastrand` crates. About 100 lines of Rust out, about 40 lines of JavaScript in, and three lines in `capabilities/default.json` granting the page position, size, and maximize.

**What stays the same.** The window built in code from `Ready`, which is the cold-start race fix; the data folder handed to the builder, which is what keeps a portable copy's WebView2 profile in `portable/`; the recording in `window.js`; the lock, the handoff, the tray, the Dock, and quitting.

**What to check.** Maximize on Windows. tao maximizes with `ShowWindow(SW_MAXIMIZE)`, which shows the window, so the page maximizes as its last step, together with showing, and the check is that nothing appears at the pre-maximized size first. On the Mac, zoom from a hidden window, which showed no flash when Rust did it. A second launch during a cold start, which must bring the window forward in its placed spot and not jump. The Retina scale: the page already records in CSS pixels, so it places in CSS pixels too.

## 2. Engine messages

**Now.** `engine.rs` has a command for each message the page sends, `engine_folders` building `{"command":"folders",…}`. It reads the engine's stdout and keeps two particular events, `ready` and `folders`, in the status it hands the page. Each new message the engine learns means a new Rust command and a new field.

**Decided: one road down, one road up, and Rust is the wire.** Essay two above is the design, and this is its code.

- **Down.** `engine_send(line)` writes a line of JSON the page built to the engine's stdin. Rust checks only that the line holds no newline, since a newline would split it into two messages, and otherwise doesn't read it.
- **Up.** The stdout reader puts each line the engine writes, as text, on a drained queue, the shape described under "One shape for what arrives from below" below. `engine_take()` hands over everything waiting and empties it. The page keeps its own idea of what the engine has said: the `ready` event, the `folders` echo, and later everything a session reports.
- **Stays in Rust.** `init`, starting and stopping the engine, and the stderr ring.

**What leaves.** `engine_folders`, and the `ready` and `folders` fields in the status, with the reader's match on event names. The page finds `ready` and `folders` in the lines itself.

**What it costs.** Roughly even in Rust today: out go `engine_folders`, the two fields, and the event matching; in come `engine_send`, `engine_take`, and the shared queue. The page gains about 15 lines to read the lines and remember what they said. The value isn't in this diff. It's that every engine command after this one, and the protocol is about to grow by a lot, from creating the session to adding, pausing, and removing torrents to progress and alerts, costs no Rust at all.

**What to check.** The page still shows the engine line and "the engine has its folders" exactly as now; a `folders` sent from the page comes back; a line holding a newline is refused; and the queue stays bounded, with a dropped count the page shows if it ever isn't zero.

## 3. Arrivals

**Now.** `instance.rs` keeps a history of the requests that reached this copy, the last hundred, each marked `launch` or `handoff`, and the page polls the whole history once a second to list it.

**Decided: a drained queue, in this batch.** Rust has to hold what arrives, since a magnet can land during a cold start before the page is up, a handoff arrives on the pipe's thread rather than in the page, and on macOS the associations work will add Apple Events to the same stream. But holding is a queue, not a history. `instance_take()` hands over everything waiting and empties it, the same shared queue the engine's lines use. The page keeps its own list and decides what an arrival means, which is where adding a torrent will live.

Each arrival keeps its mark, `launch` for this copy's own command line or `handoff` for one carried in by a second launch, since where it came from is a fact Rust knows and the page can't learn. Bringing the window forward on a handoff stays in Rust, as item 1 decided. The status keeps the lock and the handoff, which are facts about this copy.

**What leaves.** The history and its hundred-entry limit inside the instance status; the page's once-a-second poll of the whole list becomes a take of what's new.

**When.** Moved into this batch from "with adding torrents", because it shares its queue with item 2, and building that once for both costs less than building it now and changing arrivals later.

## One shape for what arrives from below

The engine's lines and the arrivals are the same problem: something lands in Rust on a thread of its own, before the page is up or between the page's looks, and the page needs every piece of it, in order. Both use one small shared shape, a drained queue.

- **Push.** Whatever lands is appended. The queue has a cap, and a push that would pass it drops the oldest item and counts it, so a page that stops taking can't make Rust's memory grow without end.
- **Take.** One call hands over everything waiting, with the count of anything dropped since the last take, and leaves the queue empty. In Rust it's `std::mem::take` on the list.
- **The page owns the rest.** It appends what it takes to its own history, reads meaning into it, and shows the dropped count if it's ever not zero, which is how a busy engine outrunning a hidden web view's slowed timers would show up.

We weighed a numbered ring, where the page reads everything after the last number it saw and nothing is consumed. It survives a page reload and serves several readers, but the only reader is the page, and a reload only happens in development, where launches aren't what's being tested. The queue does the same job in less code on both sides, and its dropped count gives the same warning the ring's gap in numbers would.

About 15 lines of Rust, used twice.

## 4. Associations

**Now.** `associate.rs`, 164 lines, runs from setup on every launch and holds the whole Windows registration. Part of it is mechanism: read a registry value, write one only if it differs, tell the shell that associations changed. The rest is policy, and application logic: which file types and link schemes ftorrent offers and what they're called, the registry keys and values written for each, the rule that ftorrent claims a scheme's shared class only while it's empty or already runs ftorrent's command, the order the writes go in so ftorrent is published to Settings last, and the gate, that `ftorrent.exe` has to be in `%LOCALAPPDATA%\ftorrent`. Registry key names written into Rust are the clearest sign of the problem: Rust knowing that `Software\Classes\ftorrent.torrent\shell\open\command` is where ftorrent's open command goes is exactly the knowledge that belongs above it.

**Decided: the page owns registration, on general registry commands, in this batch.**

- **Rust offers the registry the way `disk.rs` offers the disk.** `registry_get(key, name)` reads a value, through `HKEY_CLASSES_ROOT`, the merged view Windows itself uses, or the current user's hive, and answers nothing when it isn't there. `registry_set(key, name, value)` writes a string under the current user only, reading first, and answers whether anything changed. `shell_notify_associations()` tells the shell to catch up. They're Windows-only, general, and know nothing about file types, schemes, or ftorrent; any Windows application could use them as they are.
- **The page holds the policy, in plain JavaScript.** The list of file types and link schemes with their names, what goes where for each, the claimable check on a shared scheme class, writing the Settings publication last, notifying the shell only when a write changed something, and the gate. It runs once at startup on Windows, after the settings are read, and shows the same one line it does now.
- **The gate is a fact plus a comparison.** Rust adds one fact to what `paths` reports, the folder the per-user installer puts ftorrent in, `%LOCALAPPDATA%\ftorrent`, and the page compares it with the program's location, ignoring case. A copy anywhere else skips registration silently, as now.
- **Security is `disk.rs`'s stance.** `registry_set` can write any string value under the current user, and the page's own code decides what; it never touches the machine-wide hive. The page runs only ftorrent's code, which is what makes that right.

**Why now.** About 100 lines of Rust leave and about 70 of JavaScript arrive. The default-app panel, which comes with the interface, has to read who owns `.torrent` and `magnet:` today, show it, and help the user choose, all from the page; with the policy on the page it reuses `registry_get`, where policy left in Rust would need ftorrent-specific commands like "query our associations" that fail the test. And registration has to be checked on Windows after any change, on a trip we're making anyway.

**What stays the same.** What gets written, key for key and value for value: offering everywhere, taking nothing, the shared class only while empty or already ftorrent's, `RegisteredApplications` last, the donut icon for files, and the silent gate. On macOS and Linux there's still nothing to run, since the declarations ship inside the package.

**What to check, on Windows.** An installed copy's first launch after the change writes nothing, since the registry already holds every value, and reports the same line as before with zero values written. A fresh user profile, or the keys deleted first, gets the same 26 or 27 values the Rust version wrote, compared key by key with `reg query`. A copy run from `target\release` writes nothing and says nothing. Explorer and Settings > Default apps look exactly as they do now.

## Order

All four items are decided, and they're coded together in the portable sprint, along with the two essays: they touch code already being tested, items 2 and 3 share the drained queue, and item 4 is checked on the Windows trip that follows.
