# UPnP, NAT-PMP, and PCP

The three protocols a program uses to ask a router to let traffic in from the internet: where each came from, what each looks like on the wire, where each stands today, and what ftorrent does with each. Started 2026-10-23 during Good5, which asks a router all three, as notes that may become a glossary article.

## The three

- **UPnP IGD**, 2001: an industry forum's standard, led by Microsoft. A multicast search finds the router, then XML over HTTP asks it. IPv4 port mappings, and IPv6 pinholes from version 2 in 2010. The most widely supported of the three.
- **NAT-PMP**, 2005: Apple's. Small binary messages over UDP to the router's port 5351. IPv4 only. Documented as RFC 6886 in 2013, the same month its successor was published.
- **PCP**, 2013: the IETF's standard, RFC 6887, NAT-PMP's successor on the same port. IPv4 and IPv6, and designed to reach a carrier's NAT as well as the home router.

All three ask the same thing in different words. On IPv4, the request is a port mapping: forward this public port to my private address and port. On IPv6, where nothing translates, it's a pinhole: let unasked packets through the firewall to my global address and port. Either way the request names a lifetime, and the program renews it while it runs.

## UPnP IGD

Universal Plug and Play came from the UPnP Forum, an industry group Microsoft led from 1999, as a way for devices on a home network to find each other and describe what they can do. The Internet Gateway Device profile, IGD, is the part that describes a router: its public address, and a list of port mappings a program can read, add to, and remove. Windows XP shipped with UPnP in 2001, and the programs that needed connections in were early users: instant messengers for calls and file transfers, game consoles, and BitTorrent clients. Within a few years nearly every consumer router answered it, many with it on by default.

On the wire it takes three exchanges.

- **The search.** A program sends a few lines of text over UDP to the multicast address 239.255.255.250, port 1900: `M-SEARCH * HTTP/1.1`, with an `ST` line naming what it's looking for, like `urn:schemas-upnp-org:device:InternetGatewayDevice:1`. A router with UPnP on answers with a reply in the same text form, whose `LOCATION` line is the address of its device description and whose `SERVER` line names its operating system and UPnP software, with their versions.
- **The description.** An HTTP fetch of that address returns an XML document: the router's friendly name, manufacturer, model name and number, serial number, and the address of its own settings page, then its services, each with a control address. The service a program wants sits a few devices deep: a WAN device, inside it a WAN connection device, and inside that WANIPConnection, or WANPPPConnection on a line that dials in. WANCommonInterfaceConfig sits beside them in the WAN device.
- **The calls.** Each request is an HTTP POST to a service's control address, with a `SOAPAction` header naming the action and an XML envelope carrying its arguments. WANIPConnection answers GetExternalIPAddress, GetGenericPortMappingEntry for each mapping by index until the index runs past the end, AddPortMapping, DeletePortMapping, and GetStatusInfo, whose answer includes how long the internet connection has been up. WANCommonInterfaceConfig answers GetCommonLinkProperties: the kind of link, like Cable, DSL, or Ethernet, and the upstream and downstream maximum rates the router reports.

Version 2 of the profile, adopted in September 2010, added IPv6 firewall control in a service of its own, WANIPv6FirewallControl, whose AddPinhole, UpdatePinhole, and DeletePinhole manage time-limited pinholes to a global address. The Open Connectivity Foundation lists version 1's descriptions as deprecated since 2015, though routers still answer as version 1. The UPnP Forum's specifications now live with the Open Connectivity Foundation.

UPnP has no authentication. Any program on the local network can ask for any mapping, which was the design, since the local network was trusted. The trouble came from routers whose firmware answered UPnP from the internet side as well: in January 2013, Rapid7 reported over 81 million addresses answering a UPnP discovery request from the internet, an estimated 40 to 50 million of them vulnerable to at least one remote attack, and over 23 million open to running an attacker's code from a single UDP packet. Since then some providers ship their routers with UPnP turned off.

## NAT-PMP

The NAT Port Mapping Protocol came from Apple, by Stuart Cheshire and Marc Krochmal, and was in Apple's products from 2005 on: Mac OS X, Bonjour for Windows, and the AirPort base stations. Where UPnP is a discovery system with XML and HTTP, NAT-PMP is a few bytes over UDP sent straight to the gateway, the router the machine already knows from its default route, at port 5351.

- **The public address request** is two bytes, `00 00`: version 0, opcode 0.
- **Its reply** is twelve bytes: version 0, opcode 128, a two-byte result code, a four-byte epoch, and the four-byte external address. The result code is 0 for success, then 1 unsupported version, 2 refused, 3 network failure, 4 out of resources, and 5 unsupported opcode.
- **A mapping request** is twelve bytes: version, opcode 1 for UDP or 2 for TCP, two reserved bytes, the internal port, the suggested external port, and the lifetime asked for. Its reply is sixteen bytes, with the external port and lifetime the router granted. A lifetime of 0 deletes the mapping.
- **The epoch** in every reply counts the seconds since the router's mapping table started, so a program that sees it fall back knows the router restarted and its mappings are gone.
- **Sending again** follows the RFC's schedule: a first wait of 250 milliseconds, doubling each time.
- **Announcements.** A router also sends its external address unasked, to the multicast address 224.0.0.1 at port 5350, when it starts or the address changes.

NAT-PMP covers IPv4 only. It was published as RFC 6886, informational, in April 2013, recording a protocol already in wide use, in the same month as the standard that replaced it. Apple stopped making AirPort routers in 2018.

## PCP

The Port Control Protocol is the IETF's standard, RFC 6887, published in April 2013, with Stuart Cheshire among its authors. It keeps NAT-PMP's shape, small binary messages to port 5351, and widens what they can say.

- **The header** is 24 bytes: the version, 2; one bit marking a request or a response, and a seven-bit opcode; two reserved bytes; the lifetime asked for, in seconds; and the client's own address in sixteen bytes, an IPv4 address written as an IPv4-mapped IPv6 address, like `::ffff:192.0.2.23`.
- **ANNOUNCE** is the header alone, opcode 0. It asks for nothing, and a router that speaks PCP answers it.
- **MAP**, opcode 1, adds 36 bytes: a 12-byte random nonce that names the mapping, the protocol, 6 for TCP or 17 for UDP, three reserved bytes, the internal port, and a suggested external port and address. On IPv4 a MAP asks for a port mapping, and on IPv6 for a firewall pinhole, in the same form.
- **The response** carries a result code, the lifetime granted, an epoch like NAT-PMP's, and the MAP's fields with the external port and address assigned.
- **Renewing** is the same MAP with the same nonce, sent about halfway through the lifetime. Deleting is the same MAP with a lifetime of 0. An epoch that jumps backward means the router lost its state, so the client maps again.
- **Sending again**, by the RFC, starts with a wait of three seconds.
- **Telling the two apart.** NAT-PMP's messages start with version 0 and PCP's with version 2, so one router answers both on one port, and a router that speaks only NAT-PMP answers a PCP message with NAT-PMP's unsupported version, which itself says what the router speaks.

PCP was designed for a world with a carrier's NAT above the home router, which neither NAT-PMP nor UPnP can reach. A carrier can tell the home router where its PCP server is through a DHCP option, RFC 7291, and the home router asks it for mappings on the customer's behalf.

### Through a carrier's NAT, in practice

Rarely. There's no measurement of how often, only provider documents and forum reports, and they point one way.

- **The equipment supports it.** Juniper's carrier-grade NAT implements PCP per RFC 6887, including for DS-Lite, the IPv6-only access design common in Germany, where the carrier translates IPv4 at its end.
- **A few providers turn it on, mostly in Germany.** 1&1 is reported to use PCP, and a Drillisch DSL customer reports 20 ports for incoming IPv4 through it. The FRITZ!Box, AVM's home router and a common one on German lines, has spoken PCP to its provider since FRITZ!OS 6.50 and shows the provider's PCP server in its monitor. A program inside still asks only the home router, which does the PCP talking upstream.
- **Elsewhere it mostly isn't there.** A Comcast customer behind carrier NAT found the provider didn't answer PCP. An OpenWrt user on a carrier-NAT line saw the router accept PCP mappings while the ports stayed blocked upstream. Nothing found shows a mobile carrier offering it.
- **What providers offer instead** is a way out of the carrier NAT: an Australian provider opts customers out on request, for $2 a month for a dynamic public IPv4 address or $5 for a static one. Others have no IPv4 to give, and IPv6 is the way in.

## On the router

Most home routers don't implement these protocols one by one. miniupnpd, from the MiniUPnP project Thomas Bernard started in the mid-2000s, is built into a great many router firmwares, OpenWrt's among them, and speaks all three. So a router that answers one often answers all three, and a program that wants to know what a router supports asks all three at once. Tailscale's netcheck does exactly that, from one IPv4 socket: NAT-PMP's public address request and PCP's ANNOUNCE to the gateway's port 5351, and UPnP's search to port 1900, first to the gateway and then to the multicast address, giving the answers 250 milliseconds and sorting them by the port they came from.

## Where each stands today

None of the three is extinct.

- **UPnP is the workhorse.** It's the most likely to be on, and with PCP one of the two ways to ask for an IPv6 pinhole. When a provider ships a router with it off, nothing on the computer can turn it back on.
- **NAT-PMP is replaced on paper and alive in practice.** PCP succeeded it and Apple's routers are gone, but miniupnpd answers it on a great many routers, and asking costs two bytes.
- **PCP is the current standard and the least common on home routers.** It's the only one designed to reach a carrier's NAT, which matters more as carrier-grade NAT spreads, and on the few providers that run it, the only way a program can open a port there.

## In ftorrent

The mechanics of all three are the world's, so they live low, in the engine's libtorrent and in Rust crates; when to ask and what an answer means for a person is the page's.

- **libtorrent maps with all three.** The engine's libtorrent 2.1.1 has two mappers, turned on by `enable_upnp` and `enable_natpmp`, which keep the listening port mapped and renew it themselves. The UPnP mapper runs on IPv4 sockets only, since its search is over IPv4, and calls GetExternalIPAddress, AddPortMapping, and DeletePortMapping, never AddPinhole. The NAT-PMP mapper sends PCP first, on every global socket of either family, and falls back to NAT-PMP on IPv4 when the router answers unsupported version; `portmap_transport` names it `natpmp` either way. So PCP is libtorrent's too, pinholes on IPv6 included.
- **libtorrent looks only while it maps.** It has no way to ask a router anything without asking for a mapping. The external address a mapping learns arrives as an `external_ip_alert`, the port-mapping log category shows each exchange line by line, full SOAP bodies included, and NAT-PMP's epoch is read and thrown away.
- **Reads without a mapping come from crates.** igd-next, which iroh's port mapper uses for its own UPnP, searches for the router and reads its external address and its port mappings by index, over IPv4, and over IPv6 can read the firewall's status and manage pinholes; it doesn't read the router's maker, model, or uptime. natpmp sends NAT-PMP's public address request alone and returns the address and the epoch. No crate sends PCP's ANNOUNCE, so a router's answer to PCP has to come from Rust written by hand, 24 bytes out and a header back.
- **Crates passed over.** portmapper, iroh's, keeps one IPv4 mapping alive for one port behind three yes-or-no answers, with its protocol code private. crab_nat maps over PCP and NAT-PMP, which libtorrent already does, reads NAT-PMP's address without the epoch, and has no ANNOUNCE.

## Sources

- [RFC 6886: NAT Port Mapping Protocol](https://datatracker.ietf.org/doc/rfc6886)
- [RFC 6887: Port Control Protocol](https://www.rfc-editor.org/rfc/rfc6887.html)
- [draft-cheshire-nat-pmp-07](https://datatracker.ietf.org/doc/html/draft-cheshire-nat-pmp)
- [Open Connectivity Foundation: Internet Gateway Device v2](https://openconnectivity.org/?p=2706)
- [Rapid7: Security Flaws in Universal Plug and Play, Unplug, Don't Play](https://www.rapid7.com/blog/post/2013/01/29/security-flaws-in-universal-plug-and-play-unplug-dont-play/)
- [Juniper: Port Control Protocol](https://www.juniper.net/documentation/us/en/software/junos/interfaces-adaptive-services/topics/topic-map/port-control-protocol.html)
- [administrator.de: DS-Lite, CGNat und Port Control Protocol](https://administrator.de/forum/ds-lite-portcontrolprotocol-pcp-unifi-678144.html)
- [heimnetz.de: Fritz!Box PCP, Portfreigaben beim Provider einrichten](https://forum.heimnetz.de/threads/fritz-box-pcp-portfreigaben-beim-provider-einrichten.203/)
- [IPFire community: Port Control Protocol](https://community.ipfire.org/t/port-control-protocol-pcp/11007)
- [OpenWrt forum: Help with CGNAT and PCP](https://forum.openwrt.org/t/help-with-cgnat-and-pcp/195382)
- [Spintel: port forwarding with CGNAT](https://articles.spintel.net.au/print-512.html)
- [crab_nat](https://docs.rs/crab_nat)
- [pcp](https://github.com/Rimpampa/pcp)
