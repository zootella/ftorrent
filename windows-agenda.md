# Windows agenda

The Windows session's burn-down for the Mac's letter in `mac2win.md`, Good4 in `docs/docs/good-service-planning.md` on Windows, and the ergo changes that only this box compiles. Each section is named for what it covers, and we call it by that name as we work. A section leaves this file when the user says it's resolved.

## ErgoWindows

The two Windows-only lines that now count bytes with ergo's `size::KB`, both keeping their values.

- **The handoff.** With `pnpm local` running, start `target\debug\ftorrent.exe` a second time. It should hand its launch to the running copy through the pipe in `instance.rs` and exit, and the running window should come forward.
- **The interfaces.** After Check, the Connection tab's interface lines should read as they did, from the first buffer `net.rs` hands `GetAdaptersAddresses`, with IPv6 addresses named temporary and stable by their suffix origin.

## FirewallWatch

The rules naming ftorrent, listed through COM, `HNetCfg.FwPolicy2`, since `Get-NetFirewallRule` answers "Access is denied" from Claude's shell. Each listing gives each rule's direction, action, profiles, protocol, and program.

- **Before any run.** Taken. All three profiles on, each blocking inbound by default, and the network on Public. Four rules, all allows on Public, TCP and UDP for the installed copy and for the development build's `ftorrent.exe`. No rule for either engine, and no block rules.
- **At Check.** The development build is already allowed, so no alert is expected. List again after Check either way.
- **At Start.** The engine at `target\debug\ftorrent-engine\ftorrent-engine.exe` has no rule, so its first listen should bring an alert. List while the alert is up and before anyone answers it, then have the user answer, then list once more.
- For each alert, record what it names, which network boxes it shows and which are ticked, and the time it appeared and was answered.

## OpenBox

The user presses Open… and chooses a well-seeded torrent file of their own.

- The box is Windows' own open box, sits over ftorrent's window, and offers files.
- The path and the info-hash appear in the table.
- Open… again and Cancel changes nothing.

## SessionRun

- **Start.** Note the engine alert's times against the box's lines, which are timed from Start. Within a few seconds, `%USERPROFILE%\ftorrent-logs\stick.txt` should show `dht_outgoing_get_peers` in the Alerts row and nodes in the DHT row.
- **Past ten minutes.** The Alerts row never shows `torrent_finished`, and nothing named for the torrent appears in the temporary folder. Either would mean upload mode broke, which is a stop-and-report.
- **Stop.** Record the Incoming connections row as it stands at Stop and the box's last line, the alerts counted by kind.

## WriteUp

- **The plan.** Rewrite the Windows paragraph at the end of Good4 in `docs/docs/good-service-planning.md` in place, as what Windows did, in the plan's voice.
- **The letter.** `win2mac.md`, with what the Mac asked for and what it needs to act on:
  - the build: blake3 assembled its four x86-64 `.asm` files with `ml64.exe` under MSVC, the dialog plugin's Windows half compiled, ergo's hash matched `Get-FileHash`, and `win-setup.js`'s staged, cabinet, stub, and finished lines read as they did;
  - ErgoWindows, every FirewallWatch listing and alert, OpenBox, and SessionRun's times, Incoming connections row, and counts;
  - for Good5b, that COM reads every profile and rule without elevation, where `Get-NetFirewallRule` was refused;
  - that the World Map in the plan says the firewall is read through netsh on Windows, where Good5b says COM;
  - and that the Good4 paragraph was rewritten.
- **Before pasting into either.** Leave out the torrent's name, its file name, its info-hash, and the info-hash's first six characters, which the box prints on many lines. Rewrite every public address into the documentation ranges, 192.0.2.0/24, 198.51.100.0/24, 203.0.113.0/24, and 2001:db8::/32. Leave out the host name and the user's folder names.
