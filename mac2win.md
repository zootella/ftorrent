# Mac to Windows

## Build the Windows firewall rule

This is Good9e's Windows half, the box's to build and prove as Good5's firewall reads were. The Mac half is a Firewall… button that opens the Network pane of System Settings, and a pointer in the Firewall list row. What's decided for Windows:

- An installed copy only, as `isInstalled` in `paths.js` tells, with one UAC prompt the user approves for the whole change.
- First delete every rule naming this copy's `ftorrent.exe` or engine by path, since the security alert's block rules override an allow, then add inbound allow rules for both programs over TCP and UDP.
- Every profile, since Windows starts a new network as Public, a home network included, as `windows-block.md` says.
- A button on the panel that says in the box what it's about to do before the prompt comes.
- Rust stays general, per the essay in `lib.rs`: the command elevates a program, and the page decides what runs.

Prove it on an installed copy with the alert's block rules standing. Start and Check, where the inbound rows should name the block rule. Then the button and the prompt, then back to the window, where Good9f's re-read should put a line in the box saying the firewall changed, then Check again, where the firewall rows should list the new allow rules and the inbound rows lose their note.
