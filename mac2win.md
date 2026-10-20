# Windows: libtorrent wakes up

A note for the Windows session. It's public and committed, so it names nobody: in anything written back here, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile.

## What changed

The Connection tab now runs a libtorrent session, which is Good4 in `docs/docs/good-service-planning.md`; read that section first. Three new buttons sit beside Check:

- **Open…** puts up the system's open box through a new Rust command, `dialog_open` in `dialog.rs`. Off the Mac it calls the dialog plugin's Rust half, `tauri-plugin-dialog`, registered in `lib.rs` and granted nothing in `capabilities/default.json`. The engine reads the file chosen, and the table shows its path and info-hash.
- **Start** opens a session in the engine at a random port, listening on every address in both families, joining the DHT, and joining the chosen torrent's swarm in upload mode. The page polls the engine once a second, and the table and box fill with what libtorrent reports.
- **Stop** tears the session down.

`engine.py` has new commands, so the engine must be frozen again. `Cargo.toml` adds the dialog plugin and holds Tauri at `~2.11`, and `Cargo.lock` already carries the plugin's crates for Windows, among them rfd and windows-sys 0.60. The plugin's Windows half has never compiled.

While a session runs, the page also writes the whole panel, the table and then the box, to `%USERPROFILE%\ftorrent-logs\stick.txt` a second after anything changes, the first line saying when and which process. Read that file rather than asking the user to paste.

## Ergo, and what only this box compiles

The desktop workspace now takes ergo 0.1.3, the user's own helper library, from npm and crates.io, for its size and time constants, its SHA-256, and its base16. Before any other command, run `pnpm install --frozen-lockfile` at the repository root, which installs what the committed lockfile names and refuses to change it. These are the parts no machine has built yet:

- **blake3 under MSVC.** Ergo 0.1.3 brings blake3, sha1, and sha2 0.11 into `Cargo.lock`. Nothing here calls blake3 or sha1; they compile because the crate is there. When the target is MSVC, blake3's build script assembles its x86-64 code from `.asm` files with `ml64.exe`, which comes with the Visual Studio Build Tools that give Rust its linker. A failure shows in blake3's build script during `cargo check`: report it in full and stop there, since the fix may belong in ergo, and that is the user's call.
- **The Windows-only Rust.** Two lines behind `#[cfg(windows)]` now count bytes with ergo's `size::KB`: the longest request `instance.rs` reads from a second launch's pipe, and the first buffer `net.rs` hands `GetAdaptersAddresses`. Both keep their values, 64 KB and 15 KB. The Connection tab's interfaces should read as they did, and a second launch should still hand over to the running copy.
- **`scripts.js` and `win-setup.js`.** `pnpm hash` hashes through ergo now, with an async `main` that ends at `main().catch(...)`. After `pnpm installer` and `pnpm hash`, compare the hash it prints for `ftorrent.exe` with PowerShell's `Get-FileHash -Algorithm SHA256` of the staged file; PowerShell prints it in capitals, so compare without case. `win-setup.js` counts its megabytes, kilobytes, and seconds through ergo's `Size` and `Time`, and its staged, cabinet, stub, and finished lines should read as they did.

Put what each of these showed in `win2mac.md` with the rest.

## What to do

- **Make it build.** Run `pnpm engine`, then `cargo check` from `src-tauri`, then `pnpm compile`. Report any error in full before changing anything.
- **Watch the firewall from the start.** Before the first run, list the rules naming ftorrent from PowerShell, `Get-NetFirewallRule | Where-Object DisplayName -like '*ftorrent*'`, with each rule's direction, action, profile, and program path from `Get-NetFirewallApplicationFilter`. Then run `pnpm local`, open the Connection tab, and press Check. If a Windows Security alert appears, list the rules again before anyone answers it, then have the user answer it, and list them once more. Record what the alert names, which network boxes it shows, and what each listing held.
- **Open a torrent.** Have the user press Open… and choose a well-seeded torrent file of their own. Confirm the box is Windows' own open box, sits over ftorrent's window, and offers files; that the path and info-hash appear in the table; and that Cancel changes nothing. Never write the torrent's name or info-hash into any file.
- **Start a session.** Have the user press Start. The engine is a separate program, so its first listen should bring an alert of its own: list the rules before it's answered, and note the time it appeared and the time it was answered against the box's lines, which are timed from Start. Then have the user answer it. Within a few seconds of Start, `stick.txt` should show `dht_outgoing_get_peers` in the Alerts row and nodes in the DHT row.
- **Let it run past ten minutes.** The Alerts row must never show `torrent_finished`, and nothing named for the torrent may appear in the temporary folder. Either would mean upload mode broke, and that's a stop-and-report.
- **Stop.** The box's last line gives the alerts counted by kind.

## When it's done

Write `win2mac.md` with the build result, every rule listing and what each alert named, the alert times against the box, the Incoming connections row as it stood at Stop, and the last line's counts. The Windows paragraph at the end of Good4 in the plan says what Windows is expected to do. Rewrite it in place as what Windows did, in the plan's voice, and say in the letter that you did.

That letter and the plan are public and committed, so before pasting anything into either:
- leave out the torrent's name, its file name, and its info-hash, and the info-hash's first six characters, which the box prints on many lines;
- rewrite every public address into the documentation ranges: 192.0.2.0/24, 198.51.100.0/24, 203.0.113.0/24, and 2001:db8::/32;
- leave out the host name and the user's folder names.
