# MacBook: the Connection tab on Tahoe

A note for the session on a Mac running macOS 26, Tahoe. Good4 ran on macOS 15, and ftorrent supports the current macOS and the two before it, so a few things Good4 found there need checking here. It's public and committed, so it names nobody: in anything written back, say "the user." Git is read-only, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile.

## What changed

The Connection tab now runs a libtorrent session, which is Good4 in `docs/docs/good-service-planning.md`; read that section first. Open… puts up the open box through `dialog_open` in `dialog.rs`, which on a Mac calls AppKit's open panel directly and slides it down from the window as a sheet. Start opens a session in the engine at a random port, on every address in both families, and joins the chosen torrent's swarm in upload mode. Stop tears it down.

While a session runs, the page writes the whole panel to `~/ftorrent-logs/stick.txt` a second after anything changes. Read that file rather than asking the user to paste.

## What to do

- **Build it.** Run `pnpm install` in `desktop`, then `pnpm engine`, then `pnpm local`.
- **The open sheet.** Have the user press Open… and choose a well-seeded torrent file of their own. Confirm it comes down as a sheet attached to ftorrent's window, not a separate window, and that the path and info-hash appear in the table. Press Open… again and Cancel, and confirm nothing changes. Never write the torrent's name or info-hash into any file.
- **The firewall, as it stands.** Run `/usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate` and `--listapps`, without sudo, and `defaults read /Library/Preferences/com.apple.alf globalstate`. Record what each answered, or that it asked for privilege.
- **The firewall prompt.** This needs the firewall on. If it's off, turning it on for the test is the user's call, in System Settings, Network, Firewall. With it on, have the user press Start and leave any prompt unanswered for a while. Record what the prompt names and how many seconds after Start it appeared, and read `stick.txt` while it's still up: say whether `incoming_connection` lines arrived before the user answered. Then have the user answer Allow, and run `--listapps` again.
- **Stop, then Start again.** Confirm no second prompt comes for the same build.

## When it's done

Write `macbook2mac.md` with what each step showed. Before pasting anything into it, leave out the torrent's name, its file name, its info-hash, and the info-hash's first six characters, which the box prints on many lines. Rewrite every public address into the documentation ranges, 192.0.2.0/24, 198.51.100.0/24, 203.0.113.0/24, and 2001:db8::/32, and leave out the host name and the user's folder names. If the user turned the firewall on for the test, ask whether they want it off again.
