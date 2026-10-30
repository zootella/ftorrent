# Mac to Windows

## Run Good8's hole punches

Check now punches through moon's three namespaces, a cone NAT and a symmetric NAT over IPv4 and a stateful firewall over IPv6, from sockets `udp.rs` holds open, and this box hasn't run them. Run `pnpm local` in `desktop` and press Check. From the Mac on the dual-stack connection, the cone and IPv6 firewall punches got through and the symmetric one didn't.

## See whether libtorrent's IPv6 mapper finds the gateway on Windows

Map and Unmap, beside Start, turn libtorrent's UPnP and NAT-PMP mappers on and off in the running session, with one-minute leases, every port-mapping alert and log line in the box, and a row for each mapper on each local address. On the Mac the NAT-PMP mapper never sends over IPv6: its macOS route reading drops every IPv6 default route, so each IPv6 mapper logs `failed to find default route` and closes, and `to-libtorrent.md` has the cause and its line about Windows. libtorrent reads routes on Windows through `GetIpForwardTable2`, which may find the gateway. Press Start, then Map, wait about fifteen seconds, then Check, Unmap, and Stop. From the Mac the UPnP mapper mapped the port and the inbound rows read reachable over IPv4.

## Run Try mapping

Try mapping, beside Check, asks the router itself for UPnP, NAT-PMP, and PCP over IPv4, and UPnP and PCP over IPv6, each with a lease of a minute, and has moon poke the granted port before the request, after the grant, and after the delete. Its Rust is new in `desktop/src-tauri/src/net.rs` and hasn't compiled here: four `net_upnp` commands that change the router, `net_natpmp_map`, `net_pcp_map`, and `net_pcp_announce` over either family, where an IPv6 link-local gateway takes the interface's number, here the `Ipv6IfIndex` net_interfaces answers. From the Mac the router granted UPnP over IPv4 and moon's poke came through, refused port 5351 to NAT-PMP and PCP, offered no UPnP pinhole service, and was silent to PCP over IPv6. Windows' firewall swallows the ICMP refusal for port 5351, Good5 found, so NAT-PMP and PCP over IPv4 should read no answer here.

## Build the Windows firewall rule

This is Good9e's Windows half, the box's to build and prove as Good5's firewall reads were. The Mac half is a Firewall… button that opens the Network pane of System Settings, and a pointer in the Firewall list row. What's decided for Windows:

- An installed copy only, as `isInstalled` in `paths.js` tells, with one UAC prompt the user approves for the whole change.
- First delete every rule naming this copy's `ftorrent.exe` or engine by path, since the security alert's block rules override an allow, then add inbound allow rules for both programs over TCP and UDP.
- Every profile, since Windows starts a new network as Public, a home network included, as `windows-block.md` says.
- A button on the panel that says in the box what it's about to do before the prompt comes.
- Rust stays general, per the essay in `lib.rs`: the command elevates a program, and the page decides what runs.

Prove it on an installed copy with the alert's block rules standing. Start and Check, where the inbound rows should name the block rule. Then the button and the prompt, then back to the window, where Good9f's re-read should put a line in the box saying the firewall changed, then Check again, where the firewall rows should list the new allow rules and the inbound rows lose their note.
