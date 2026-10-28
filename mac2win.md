# Mac to Windows

A letter from the Mac session to the Windows session, written 2026-10-28 during Good7 on the good roadmap, `docs/docs/good-service-planning.md`. The rules for this box: public files say "the user", never a name; git is read-only for a session here, the user commits; report results exactly as they came, including what failed; never regenerate a lockfile. This letter and the one back are public files, so no public address of this network or of any of our machines goes in either, in either family: rewrite an address in the documentation ranges, 192.0.2.0/24, 198.51.100.0/24, 203.0.113.0/24, or 2001:db8::/32, or leave it out, and never name the router's maker or model or the kind of line, only "the router" and what it did. Only what touches Windows is asked for here.

## What's new since Good5

Good7 gives the Connection tab a second vantage, moon, and two checks that read the Windows firewall, which only this box can run.

- `desktop/src-tauri/src/udp.rs` is new: five commands around a UDP socket that stays open between them, udp_open, udp_stun, udp_send, udp_receive, and udp_close, with net_stun's body moved into `stun_ask` in `net.rs` so both share it. The Mac compiled it and ran its tests; this box hasn't compiled it. Its receive passes over the error Windows reports on the next read after an ICMP refusal, `ConnectionReset`, and reads a timeout as either `WouldBlock` or `TimedOut`, which is the part most worth seeing run here.
- The filtering check, Good7d: for each family, two sockets side by side, each asking one STUN server and then having moon poke its public port, sorted into the three kinds of RFC 4787. Its row adds a note when this machine's firewall may be what held a poke back, from the firewall reads Good5 built: the current profile on and blocking all, a rule blocking ftorrent coming in over UDP there, or none allowing it.
- The inbound proof, Good7f: while a session listens, moon probes the session's port in each family with a DHT ping, a uTP connection request, and a TCP dial with a BitTorrent handshake. Its row adds the same note for the engine, over both UDP and TCP.

## Run it

Run `pnpm local` in `desktop`, which compiles `udp.rs` here, and open the Connection tab.

Press Check with no session first. Each family has a filtering row, and the box has, after the router's lines, four lines of steps per test: the STUN kept open, the poke moon sent, and the receive, which should read nothing in four seconds on a home router that admits nothing unasked. On a dual-stack home connection the Mac read address-and-port-dependent in both families with no firewall note, since its firewall lists ftorrent as allowed. Set this box's rows beside Windows Security's inbound rules for ftorrent on the current network's profile, and say whether the note appeared and whether it was right to.

Then press Start, and Check again. Each family has an inbound row, and the box has three probe lines per family from moon. At Good5 the development build's engine had no firewall rules at all, so Start may bring the security alert for it. If it does, press Check while the alert is still standing, before answering it, then answer Allow access and press Check again. The first inbound rows should name the missing or blocking rule for the engine, and the second shouldn't. Report both runs' rows, and the engine's inbound rules in Advanced settings after.

A Check takes about five seconds with no session and up to ten with one. Anything slower, a receive that ends early or with an error, or a probe line that reads differently from the Mac's is the finding.

## Then

If the rows read correctly, check the Good7d and Good7f boxes in the plan. If `udp.rs` or `connection.js` needs a fix to run right here, make it and say what and why. Put the rows and the box's filtering, probe, and firewall lines as Copy gives them, addresses rewritten as above, in `win2mac.md` at the root, replacing the Good5 letter there, which the Mac has read.
