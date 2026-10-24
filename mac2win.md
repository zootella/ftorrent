# Mac to Windows

A letter from the Mac session to the Windows session, written 2026-10-24 at the end of Good5's Mac side on the good roadmap, `docs/docs/good-service-planning.md`. The rules for this box: public files say "the user", never a name; git is read-only for a session here, the user commits; report results exactly as they came, including what failed; never regenerate a lockfile. Only what touches Windows is asked for, since everything platform-general in Good5 has run on the Mac.

## Build Good5b

Good5b is the one box in Good5 still open: read the Windows firewall through its COM interface, INetFwPolicy2, from a Windows-only module with two commands, firewall_profiles and firewall_rules, answering each profile's state, the current network's profile, and every allow or block rule naming ftorrent or the engine by path. The Good5 section's Windows paragraph says what the reads are for, and `windows-block.md` describes the rules the security alert writes. Build it here, where it can run.

A draft exists and has never run: `desktop/src-tauri/src/firewall.rs`, `desktop/src/firewall.js`, four features added to the windows crate's line in `desktop/src-tauri/Cargo.toml`, the registration in `lib.rs`, and the panel's Windows half in `desktop/src/connection.js`, the checks named firewall-profiles and firewall-rules and the Windows branches of the functions under _firewall. Treat it as a sketch of the shape: keep it, fix it, or rewrite it, and keep the rule from the essay in `lib.rs` that Rust reads and the page decides. The windows crate's names and signatures in the draft came from reading the crate's source on the Mac, not from compiling, and the read of the rule's IDispatch out of the VARIANT union in `rules` is the part most likely to be wrong.

## Compare with Windows Security

Run `pnpm local` in `desktop`, open the Connection tab, and press Check. The table's Firewall and Firewall rules rows and the box's Firewall section should match Windows Security, Firewall & network protection: which profile the current network is on, and whether each profile's firewall is on. And they should match Advanced settings, Inbound Rules, filtered for ftorrent and ftorrent-engine: each rule's action, direction, profiles, and protocol, the block rules the alert wrote included. A development build's rules are named after the program's description and an installed copy's after the file, so the panel finds rules by path; a rule whose profiles are set to all should show as all three names.

## Then

Check the Good5b box in the plan, rewrite the Windows paragraph of the Good5 section as what was built, in the voice the rest of the section uses, and put what Windows Security showed beside what the panel showed in `win2mac.md`, with the panel's Firewall rows and section pasted as Copy gives them.
