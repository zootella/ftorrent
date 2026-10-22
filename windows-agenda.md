# Windows agenda

The Windows session's burn-down for the Mac's letter in `mac2win.md`, Good4 in `docs/docs/good-service-planning.md` on Windows, and the ergo changes that only this box compiles. Each section is named for what it covers, and we call it by that name as we work. A section leaves this file when the user says it's resolved, and a finished bullet leaves as it finishes; the red lines in each commit are the record, on both machines.

## FirewallWatch

The rules naming ftorrent, listed through COM, `HNetCfg.FwPolicy2`, since `Get-NetFirewallRule` answers "Access is denied" from Claude's shell. Each listing gives each rule's direction, action, profiles, protocol, and program.

- **At Start.** Neither the installed copy's engine nor the development build's has a rule, so the first listen of whichever runs should bring an alert. List while the alert is up and before anyone answers it, then have the user answer, then list once more.
- For each alert, note for the plan's paragraph what it names, which network boxes it shows and which are ticked, and the time it appeared and was answered.

## OpenBox

The user presses Open… on the Connection tab and chooses a well-seeded torrent file of their own.

- The path and the info-hash appear in the table.
- Open… again and Cancel changes nothing.

## SessionRun

- **Start.** Note the engine alert's times against the box's lines, which are timed from Start. Within a few seconds, `%USERPROFILE%\ftorrent-logs\stick.txt` should show `dht_outgoing_get_peers` in the Alerts row and nodes in the DHT row.
- **Past ten minutes.** The Alerts row never shows `torrent_finished`, and nothing named for the torrent appears in the temporary folder. Either would mean upload mode broke, which is a stop-and-report.
- **Stop.** Note for the plan's paragraph the Incoming connections row as it stands at Stop and the box's last line, the alerts counted by kind.

## WriteUp

- **The plan.** Rewrite the Windows paragraph at the end of Good4 in `docs/docs/good-service-planning.md` in place, as what Windows did, in the plan's voice. The World Map in the same file says the Windows firewall is read through netsh, where Good5b says COM; bring it into line.
- **The letter.** `win2mac.md` has its first sections; add one only for what needs a turn from the Mac: a question, or work for it to do.
- **Before pasting into either.** Leave out the torrent's name, its file name, its info-hash, and the info-hash's first six characters, which the box prints on many lines. Rewrite every public address into the documentation ranges, 192.0.2.0/24, 198.51.100.0/24, 203.0.113.0/24, and 2001:db8::/32. Leave out the host name and the user's folder names.
