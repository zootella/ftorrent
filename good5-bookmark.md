# Good5 bookmark

_A working note for Good5 in `docs/docs/good-service-planning.md`, the milestone that reads the machine's firewall and asks the router what it knows. It holds the design questions still open before building, the reading they rest on, and the parts of Good5 that wait for the development Mac to run macOS 26 Tahoe, which from now on is the oldest macOS ftorrent supports. Running Tahoe will likely settle some of these. Delete each section once its decision is in the plan's boxes and the code._

## What the reading found

On macOS 15.7.4 the application firewall's preferences file isn't where the plan expected it: `/Library/Preferences/com.apple.alf.plist` doesn't exist, and the file of that name in `/usr/libexec/ApplicationFirewall/` is the factory template, not the live settings. `socketfilterfw` answers every read without sudo: `--getglobalstate`, `--getblockall`, `--getstealthmode`, `--getallowsigned`, `--listapps`, and `--getappblocked` with a program's path. Block-all, when on, overrides every program's allowance, and stealth mode leaves probes to closed ports unanswered.

Tailscale's port-mapping probe asks the router three ways at once, from one IPv4 socket, and gives the answers 250 milliseconds. To the gateway's port 5351 it sends NAT-PMP's two-byte public address request and PCP's ANNOUNCE, 24 bytes: version 2, opcode 0, and the client's address written as an IPv4-mapped IPv6 address. To port 1900 it sends UPnP's SSDP search, unicast to the gateway first and then to the multicast address 239.255.255.250. It sorts the replies by the port they came from, and reads the external address from NAT-PMP's reply alone.

igd-next 0.18 is the current release. Its gateway offers typed methods, `get_external_ip`, `get_generic_port_mapping_entry` by index, `add_port`, `remove_port`, and `add_pinhole` among them, and no call that takes an action name. Its search sends to the multicast address unless given another, and the gateway it answers has public fields: its address, its description and control URLs, and its service type. It brings attohttpc for plain HTTP, and xmltree with xml-rs for the XML.

Apple's technote TN3179 sets the rules for local network privacy. Sending to an address on the local network, which includes the router, and any multicast, needs the Local Network privilege, except to a DNS server or proxy on that network. Command-line tools run from Terminal or over SSH are allowed automatically, along with every process they start. A helper a program starts counts as that program. The system may refuse the first packet before the user has answered the alert, so a first check can fail where a second succeeds. An app explains itself in the alert through `NSLocalNetworkUsageDescription` in its Info.plist. The privilege follows the program's code signature, which Apple says it tracks reliably only for code signed with an Apple-issued identity, and the button that resets an answer is new in macOS 27.2.

## The decisions

### NAT-PMP through a general command

The plan's Good5c names `net_natpmp`. A general command passes the lib.rs test better: `net_udp`, which sends the bytes it's given to an address from a new socket, sends them again at doubling intervals until a reply comes or the time runs out, and answers the reply in hex with the address it came from and the round trip. The panel builds NAT-PMP's two bytes and reads the twelve that come back: the result code, the epoch, which counts seconds since the router's mapping table started, and the external address. The same command carries PCP's ANNOUNCE now, and both protocols' mapping requests in Good9, which then needs no new Rust for them. Recommended.

### PCP's ANNOUNCE in Good5

The plan leaves PCP for Good9 because PCP has no request that reads the external address without making a mapping. ANNOUNCE asks for nothing and changes nothing: a router that speaks PCP answers it, and one that speaks only NAT-PMP is expected to answer unsupported version, which is itself a finding. With it, Good5 says which of the three protocols the router speaks. As its own box, Good5 reletters to NAT-PMP, PCP ANNOUNCE, UPnP, and the carrier-grade NAT check. Send ANNOUNCE over IPv4 for now; sending it to the IPv6 gateway means a link-local address and its interface scope, which can wait for Good9's pinhole. Recommended.

### UPnP's shape

The plan's Good5d puts igd-next behind a command that takes the action name, which igd-next doesn't offer. In its place: a discovery command that runs igd-next's search and answers what it found, the router's address, its description and control URLs, and its service type, which says whether the router speaks the first or second version of the gateway standard; and two reads that each take that gateway back, one for the external address and one for a port mapping by index, which the panel steps through until the router says the index is past the end. Discovery runs once and gets its own line in the box. Whether discovery sends to the gateway directly, to the multicast address, or to both as Tailscale does, is for the first run against a real router to show. Recommended.

### The local network alert's words

`NSLocalNetworkUsageDescription` is a sentence in a system alert, the one piece of Good5 a person reads outside the panel, so its words are a decision rather than a draft to ship. A first draft: "ftorrent asks your router about port mappings." Add it before the first test of an installed copy, since a report from another app's users says that without it macOS refuses silently, with no alert and no entry in System Settings.

### Windows' firewall

Good5b reads Windows' firewall over COM, through INetFwPolicy2, from a Windows-only module with two general commands: `firewall_profiles`, each profile's state and which profiles the current networks are on, and `firewall_rules`, every rule with its program path, direction, action, and profiles, from which the panel picks the ones naming ftorrent or the engine. The Mac can't compile the Windows target, so build it last within Good5, and check it whenever a Windows session can take it.

## Waiting for Tahoe

- Read the firewall again on Tahoe: whether the preferences file is back, and whether `socketfilterfw` still answers each read without sudo.
- Find what the Local Network privilege does to a development build run from iTerm, a third-party terminal the Terminal exemption may not cover, and then to an installed copy.
- Move the support floor from macOS 15 to 26: `minimumSystemVersion` in `desktop/src-tauri/tauri.conf.json`, the Mac paragraph of `docs/docs/installing-ftorrent.md`, and the wheel paragraph of `docs/docs/libtorrent-provenance.md`, which calls the wheel's macOS 15 tag the client's floor. Whether the floor then moves each September, to the current macOS and the one before it, is still open.
