# Simpler Rust

The desktop client splits its code across a boundary. The page, Vue in the web view, holds the application logic: what a setting is, what a download folder is for, when to lock one, what a request from a second launch means. The Rust core holds commands that are atomic and dumb, and that do one thing each: read a file, take a lock, write text at exit, pipe a line to the engine. Rust knows facts about the machine and performs operations on it. It doesn't know what ftorrent is trying to do. Logic kept on one side stays true; logic split across both sides drifts apart, which makes the whole less safe, not more. `disk.rs` says this about the disk, and it holds for every module.

This note lists the places where the Rust has picked up application logic, and what the simpler shape of each would be. It stays current as the work happens, and each item leaves it once it's done.

## The test

A Rust command passes when it could be described without mentioning ftorrent's features: "lock this folder", "write these bytes to this path when the process exits", "send this line to the engine". It fails when its description needs a setting's name, a policy, or a rule about when something should happen. A command that has to exist before the page does, like taking the instance lock or starting the engine, is allowed to know what it needs for that moment, and nothing more.

The download-folder locks show both shapes. The first version took a list of folders, kept the locks still wanted, released the ones no longer listed, and retried the ones another copy held: application logic in Rust. They became `folder_lock(path)` and `folder_unlock(path)`, one folder at a time, with the page deciding which folders to lock and when. The one thing that stayed in Rust is deciding whether two paths name the same folder, since that's a fact of the file system: asking for a second lock on a folder this copy already holds would find this copy's own lock and answer busy.

What already passes: `disk.rs`, `folders.rs`, `paths.rs`, which reports the program's location, the home folder, and the data folder, `desktop.rs`, which writes text handed to it when the process exits, and `lifecycle.rs`, whose menus, tray, and hiding are platform plumbing that can only live in Rust.

## 1. Window placement

**Now.** `settings.rs` parses `ftorrent.toml` to read the `[window]` and `[screen]` sections: the saved position, the inner size, the maximized flag, and the size of the screen they were recorded on. `window.rs` then applies the placement rule. It replays the saved rectangle when the monitor under its middle still has the recorded size. Otherwise it recalculates: five eighths by one half of the primary screen, dropped at a random spot inside the centered three-quarters field. It builds the window there, maximized if it was. So Rust knows the settings schema, the fingerprint rule, and the first-run placement, and it carries the `toml` and `fastrand` crates for this alone.

**Why it moved into Rust.** Two reasons, from the Windows burst. The window had to leave `tauri.conf.json` for code, so that a launch that loses the lock leaves before any web view exists; that fixed the cold-start race, and it stays either way. And building the window at its final size meant it never appeared at one size and jumped to another.

**The simpler shape.** Rust builds the window hidden, at any size, with its data folder, from the `Ready` event, as now. The page does the rest. It already reads the settings file, and the JavaScript window API has what the rule needs: `primaryMonitor`, `monitorFromPoint`, `setPosition`, `setSize`, `maximize`, `show`. So the placement rule moves into `window.js`, beside the recording it already does, and the window is moved and sized while hidden and then shown. A hidden window moved before it's shown looks the same to the user as one built in place.

**What leaves.** `settings.rs`, most of `window.rs`, and the `toml` and `fastrand` crates. `window.rs` keeps the builder call and its data folder.

**What to check.** Maximize on Windows. tao maximizes with `ShowWindow(SW_MAXIMIZE)`, which shows the window, so the page maximizes as its last step, together with showing, and the check is that nothing appears at the pre-maximized size first. On the Mac, zoom from a hidden window. And the Retina scale: the page records in CSS pixels already, so it places in CSS pixels too.

## 2. Engine messages

**Now.** `engine.rs` has a command for each message the page sends, `engine_folders` building `{"command":"folders",…}`. It reads the engine's stdout and keeps two particular events, `ready` and `folders`, in the status it hands the page. Each new message the engine learns means a new Rust command and a new field.

**The simpler shape.** One generic command, `engine_send(line)`, which writes a line of JSON the page built to the engine's stdin. And the engine's output kept as it comes: Rust holds the recent lines it read, uninterpreted, the way it already holds stderr, and the page reads them and makes sense of them. `init` stays in Rust, because the engine starts before the page exists and needs its version and data folder at once. After this, a new engine message needs no Rust change at all.

**What leaves.** `engine_folders`, and the `ready` and `folders` fields in the status. The page finds `ready` and `folders` in the lines itself.

**What to check.** That the page still shows the engine line and "the engine has its folders" exactly as before, and that a burst of engine output can't grow without bound: the kept lines are a ring, like stderr's.

## 3. Arrivals

**Now.** `instance.rs` keeps a history of the requests that reached this copy, the last hundred, each marked `launch` or `handoff`, and the page polls the whole history once a second to list it.

**The simpler shape.** Rust has to hold what arrives, since a magnet can land during a cold start before the page is up, and on macOS the associations work will add Apple Events to the same stream. But holding is a queue, not a history: `instance_take()` returns what's waiting and empties it. The page keeps its own list and decides what an arrival means, which is where adding a torrent will live.

**What leaves.** The history and its size limit in Rust. The status keeps the lock and the handoff, which are facts about this copy.

**When.** With adding torrents, which is when the page starts owning arrivals for real. Until then the history is a test fixture that works.

## 4. Associations

**Now.** `associate.rs` holds the whole Windows registration policy: which file types and link schemes ftorrent offers, what it writes under each ProgID, the rule that it claims a scheme's shared class only while it's empty or already runs ftorrent's command, and the one gate, that `ftorrent.exe` has to be in `%LOCALAPPDATA%\ftorrent`. That policy is application logic.

**The simpler shape, by the test.** The page would own the policy, on top of two dumb commands, `registry_read(key, name)` and `registry_write(key, name, value)`, and a third that tells the shell something changed.

**Why it may stay.** It's Windows only, carefully written and measured, and it runs at startup where the page isn't needed. Keeping it in one Rust file keeps the registry's specifics out of the page. The default-app panel, which will show who owns `.torrent` and `magnet:` and offer to claim them, needs the same knowledge from the page side, and that's the moment to decide: if the panel needs the policy too, the policy moves to the page and this file becomes the three commands.

## Order

Items 1 and 2 are the biggest simplifications and touch code already being tested, so they fit in the portable sprint. Item 3 goes with adding torrents. Item 4 waits for the default-app panel.
