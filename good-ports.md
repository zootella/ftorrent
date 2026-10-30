# Good and the router's ports

This note holds good's aim and stance on port mapping: what good tells a person about their router's ports, how it finds out, and how that measurement sits beside ftorrent's engine, which maps ports of its own. We wrote it on 2026-10-29 at the start of Good9, after the first run of libtorrent's two mappers, and it is meant to last past the spike, into the time when ftorrent's engine maps for real and libtorrent is working alongside.

## The question

- A router decides whether a peer on the internet can open a connection to a machine behind it. Over IPv4 it translates, and forwards nothing in unless told to. Over IPv6 nothing translates, and its firewall drops what nobody inside asked for.
- Three protocols let software on the network tell the router to let peers in: UPnP, NAT-PMP, and PCP. Over IPv4 each asks for a mapping, a port forwarded to this machine for a stated time. Over IPv6 the same kind of request asks for a pinhole in the firewall.
- NAT-PMP has no IPv6 at all, so there are five combinations of protocol and family to look for, not six.
- All three are deployed today, unequally. UPnP is the common one, and NAT-PMP and PCP are much rarer on home routers.
- Many routers belong to the provider rather than the person, run on the provider's firmware and administered from elsewhere, and the person may never have seen its settings page.

Good answers, at the person's own desktop, which of the five their router supports, and supports effectively. How many routers in the world are the person's own and how many are their provider's, and how many speak each protocol over each family, are hard to find out from outside, and we found no published measure of the second. Good doesn't need one, since it answers the question definitively for the router in front of the person. A person whose router is administered by their provider has the same question as anyone: whether software on their own machine can ask it to let peers in.

## Our stance

Good maps the router itself, protocol by protocol and family by family, and proves each mapping with a packet from moon. libtorrent's mappers are a second subject: whether ftorrent's engine gets what the router allows.

Good's answer to "what does my router allow?" can't depend on another program's bugs and choices. The first run of libtorrent's mappers, libtorrent 2.1.1 on macOS 27, showed three ways it would:

- On a Mac, libtorrent's NAT-PMP mapper never sends PCP over IPv6. Its route reading steps through the addresses in each routing message at eight-byte alignment where macOS packs them at four, so every IPv6 route's gateway is misread and the route dropped, and every IPv6 mapper logs that it found no default route and closes. Two more problems sit behind that one: macOS writes the interface number into the middle of a link-local gateway's address, and lists the Mac's own link-local address as the route's source, which libtorrent's lookup compares against each global address and never matches.
- libtorrent's UPnP mapper works over IPv4 alone, so it never asks for a UPnP pinhole.
- When a router stays silent, the NAT-PMP mapper sends nine times over about eleven seconds, then waits two hours to try again, and posts no alert; only its log says so.

So libtorrent's results describe libtorrent as much as the router. The gap between the two is itself a finding, one good can state plainly: "your router allows an IPv6 pinhole over PCP, and ftorrent's engine on a Mac can't ask for one yet." The macOS route reading is libtorrent's to fix, and worth reporting upstream, since a client that downloads wants every mapping the router allows.

## What definitive takes

A router can fail at any step between speaking a protocol and letting a peer in, so each combination takes four answers:

- **Speaks:** does it answer the protocol at all? Good5's read-only asks already say this: NAT-PMP's address request, PCP's ANNOUNCE, and UPnP's search and reads.
- **Grants:** what does it answer a mapping request? Success, with the lease and the external port it gave, which can differ from what was asked; or a refusal in the protocol's own words, PCP's "not authorized," or a UPnP error code.
- **Forwards:** does a packet from moon actually arrive through the mapping? This is the answer the other three serve, and it also catches a carrier's translator above the router, where the router grants a mapping that's useless, since the internet never reaches the router's external address.
- **Releases:** does the delete succeed, so the router is left as it was found?

## The five combinations

| Family | Protocol | Good's own request |
|---|---|---|
| IPv4 | UPnP | AddPortMapping through igd-next, which has `add_port` and `remove_port` |
| IPv4 | NAT-PMP | the natpmp crate, which sends mapping requests too |
| IPv4 | PCP | MAP written by hand, extending our ANNOUNCE code |
| IPv6 | UPnP | AddPinhole through igd-next |
| IPv6 | PCP | MAP by hand, to the link-local gateway on its interface |

For the proof, the panel opens a UDP socket of its own, maps that socket's port, and asks moon to poke the external address and port. For IPv4 that's the router's external address, and for IPv6 it's the machine's own global address, since nothing translates there. moon pokes only a public address, so a router whose external address is in the shared address space or a private range answers the forwards question by itself: the mapping exists, and a translator above it keeps the internet from reaching it. None of this needs the engine or a session.

Good's own requests go to the gateway the panel finds itself, through the system's tools on a Mac and Linux and `GetAdaptersAddresses` on Windows, so libtorrent's route reading never touches them. The one piece of the macOS trouble good handles on its own is the interface: a PCP request to the IPv6 gateway goes to a link-local address, which means nothing without the interface it's on, and the system's route tool names the gateway with it.

Each request changes the router, so none of them runs on an ordinary Check. They sit behind a button of their own on the panel, with the person's say-so, and each attempt is a run of its own in the box, with leases of a minute and a delete after, so every change has its before and its after.

## The first run

From the dual-stack connection we test from, on 2026-10-29, with libtorrent's mappers on and leases of a minute:

- The router answered UPnP's search as an Internet Gateway Device of version 1, and granted AddPortMapping for TCP and for UDP with the sixty-second lease asked, at the same external port as the listen port. It didn't answer error 725, so it takes leases, and libtorrent's fallback to a permanent mapping never ran.
- The search also found other devices on the network answering UPnP, which offered no port mapping; libtorrent read each one's description and moved on.
- Over IPv4 the router refuses port 5351, so it speaks neither NAT-PMP nor PCP there. The NAT-PMP mapper sent PCP's MAP nine times for TCP with no answer, since its socket never hears the refusal, then the same for UDP.
- The router offers no UPnP IPv6 firewall service, which version 1 of the profile doesn't carry.
- Over IPv6 the NAT-PMP mapper sent nothing, for the macOS reason above, so whether the router's IPv6 side speaks PCP waits for good's own ANNOUNCE to ask it.

The panel's own mapping run, the same day, answered all four questions for UPnP over IPv4. The router answered the search in 56 milliseconds and granted AddPortMapping of a UDP port for the sixty seconds asked. moon's poke at the router's external address and that port was held back before the request, arrived three packets of three within half a second after the grant, and was held back again after DeletePortMapping. Over IPv6, igd-next declined to send AddPinhole, since the router offers no IPv6 firewall control service, and moon's poke at the machine's own IPv6 address was held back, the router's IPv6 firewall dropping a stranger.

The run with all five combinations, later that day, repeated UPnP's result over IPv4, and the router refused port 5351 to NAT-PMP's mapping request and to PCP's MAP over IPv4 in a few milliseconds. Over IPv6, PCP's ANNOUNCE and its MAP to the router's link-local gateway on its interface both went unanswered for four seconds. That silence is the router's habit over IPv6 rather than anything about PCP: a UDP packet to its link-local address at any closed port, 5351, 9, or 33434, comes back with nothing, where over IPv4 the same packets come back refused. So over IPv6 good can't tell a router without PCP from one dropping the request, and infers the first, since a router that spoke PCP would answer ANNOUNCE.

So on this connection, software on the machine can open IPv4 ports on the router through UPnP, proven by a packet from outside, and very likely has no software way into IPv6 at all. That's one connection's answer. The point of the five combinations is that good gives every person their own.

## ftorrent's own mapping, and libtorrent

ftorrent's own code does the mapping for the session that downloads too, for now and the foreseeable future. It requests each mapping, proves it with moon, renews it, and releases it when ftorrent quits. libtorrent runs with its mappers off and listens on a port we already know is forwarded, with `announce_port` set when the router grants a different external port. Good's experiments are the first use of that code, and the mapping lifecycle becomes the shipping feature, while the panel's version stays separate measurement code.

It works because a mapping is the router's rule to forward an external port to an address and port on the network, and it doesn't matter which program asked for it: whatever listens on that address and port gets what arrives, so libtorrent's sockets take the connections that come through, the same as if libtorrent's mapper had asked. We chose it because we can write working mapping code of our own sooner than libtorrent's gaps close. It covers the macOS IPv6 problem and a protocol libtorrent doesn't speak, UPnP's pinhole, and it answers to good's measurements rather than to another program's choices. The fix to libtorrent's route reading is still worth sending upstream, for every other client built on it.

The line between the two is the one the panel already keeps. The Rust commands that speak each protocol are general and named by operation, so the shipping mapper calls the same ones the panel's run proves. What stays apart is the page's side: the panel's run asks once, proves, reports, and deletes, and the shipping mapper keeps a mapping alive for as long as ftorrent runs. What our code owns, which libtorrent's mapper handles for its own mappings:

- **Renewing and releasing.** Renew each lease before it runs out, and delete each mapping when ftorrent quits.
- **Router restarts.** NAT-PMP's and PCP's answers carry the router's epoch, the seconds since its mapping table started, which the panel already reads. An epoch that goes backward, or runs slower than the clock, means the router lost its mappings, and the mapper asks again.
- **Address changes.** A new local address, or a new IPv6 temporary address, leaves a mapping or a pinhole pointing at an address the machine no longer listens on, so the mapper maps again for the new one.
- **The external port.** UPnP usually grants the port asked for, and NAT-PMP and PCP may assign another. Peers have to be told the port that reaches the machine, and libtorrent's `announce_port` setting sends a given port to trackers and the DHT in place of the listen port, documented for exactly this case. It is one setting for the whole session, though, and an IPv6 pinhole keeps the listen port, so an IPv4 mapping at a different external port and an IPv6 pinhole want two ports announced, which one setting can't say.
- **libtorrent's mappers stay off.** Two programs asking for the same port at once contend, and UPnP answers a request that conflicts with another client's mapping with error 718, so the engine's session runs with `enable_upnp` and `enable_natpmp` off whenever ftorrent maps.
