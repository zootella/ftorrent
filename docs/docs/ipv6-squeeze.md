---
title: The IPv6 Squeeze
description: IPv6 was built with enough addresses for every grain of sand, and its designers expected every home to be a network of its own. Then the provider fixed the top half of the address, the device claimed the bottom half, and many homes, phones, and servers were left with no room in the middle. How the numbers work, who holds them, and how 128 bits were spent.
---

# The IPv6 Squeeze

> Prepared by [Claude Code](https://claude.ai/code) using Opus 5.5
> <br>Published: 2026 October 31
> <br>Revised: 2026 October 31
> <br>Cite as: ftorrent. *The IPv6 Squeeze.* October 2026. https://docs.ftorrent.com/ipv6-squeeze.html

In 1994 Robert Hinden, one of the two authors of IPv6, wrote out the number of addresses the new protocol would have, all thirty-nine digits of it, and divided by the surface of the Earth. The answer was 665,570,793,348,866,943,898,599 addresses for every square meter of the planet. The engineers choosing the protocol wrote down their expectation that every home in the world would be a network of its own, and a few years later the Internet Architecture Board and the IETF's steering group recommended that every home receive room for 65,536 networks. The internet had run short of numbers, and the answer was to make numbers so plentiful that no one would ever count them again.

Thirty years later the numbers are here, and in many places there is no room. A phone on a cellular network receives exactly one network of 64 bits, and its own identifier takes the other 64. Many hosts hand a server rented by the month a single address. A quarter to a third of the world's home internet providers give the home one network, which cannot be divided, so the guest Wi-Fi, the smart thermostats, and the laptop share it or go without. The provider fixed the top half of the address, the device claimed the bottom half, and the space in between, where a home or a server was meant to arrange its own networks, was gone before it arrived. The companion article [Waiting for IPv6](/waiting-for-ipv6) asks why the new protocol took so long. This one asks a narrower question, the one that occurs to anyone who has set up a home network or rented a server and gone looking for a second subnet: with 128 bits, why isn't there any room left in the middle?

What follows is the record, with the sources beside it: the promise, a field guide to how addresses are written and who holds them, the protocol that took the bottom half, the warnings about the middle, what the home and phone providers actually hand out, what the server hosts do, and how a space this large comes back around to one.

## The promise

The scale was set before the protocol was chosen. In December 1993 the two area directors running the search for a successor to IPv4, Scott Bradner and Allison Mankin, asked for white papers, and their scaling question recorded the assumption of the day: "The current common wisdom is that IPng should be able to deal with 10 to the 12th nodes." A trillion machines. Time Warner Cable's answer worked it out by households, a hundred million in the United States and a hundred times that worldwide, and the technical criteria published in December 1994, [RFC 1726](https://www.rfc-editor.org/rfc/rfc1726.txt) by Craig Partridge and Frank Kastenholz, corrected the cable industry on one point that this whole article turns on:

> However, this white paper treats each home in the world as an end-node of a world-wide Internet. We believe that each home in the world will in fact be a network of the world-wide Internet.
>
> *Craig Partridge and Frank Kastenholz, [RFC 1726](https://www.rfc-editor.org/rfc/rfc1726.txt), Technical Criteria for Choosing IP The Next Generation, December 1994*

Not a host with an address, but a network with addresses of its own to arrange. The requirement that followed was at least a trillion end systems and at least a billion networks, "and preferably more."

The size of the address that would carry all this was argued for most of 1994. One camp held that 8 bytes, 64 bits, was plenty; [RFC 1752](https://www.rfc-editor.org/rfc/rfc1752.txt), the recommendation that chose IPv6 in January 1995, describes that view as "squaring the size of the IP address space," which it was, since 2^64 is 2^32 multiplied by itself. The proposal that won, SIPP, had itself been a 64-bit design until the middle of that year, when two of its working-group co-chairs, Steve Deering and Paul Francis, proposed changes in one message, the first two of them recorded in the recommendation:

> * Change address size from 8 bytes to 16 bytes (fixed-length).
>
> * Specify optional use of serverless autoconfiguration of the 16-byte address by using IEEE 802 address as the low-order ("node ID") part.
>
> *Steve Deering and Paul Francis, as recorded in [RFC 1752](https://www.rfc-editor.org/rfc/rfc1752.txt), The Recommendation for the IP Next Generation Protocol, January 1995*

The address doubled, and in the same breath the proposal promised the bottom of it to the device's own hardware identifier. The abundance and the division arrived together. Nobody minded, because the space left over was beyond counting. Christian Huitema, who had worked out how densely real numbering plans fill their space, put it plainly in [RFC 1715](https://www.rfc-editor.org/rfc/rfc1715.txt): "there is no question, in my view, that 128 bits is 'more than enough'," and "128 bits is entirely safe for the next 30 year." Thirty years from November 1994 is November 2024.

Hinden's version is the one with the planet in it. He first wrote it in an Internet-Draft in October 1994, then in a paper for the INET conference in May 1995, and then in *Communications of the ACM* in June 1996:

> This works out to be: 340,282,366,920,938,463,463,374,607,431,768,211,456 This is an extremely large address space. In a theoretical sense this is approximately 665,570,793,348,866,943,898,599 addresses per square meter of the surface of the planet Earth (assuming the earth surface is 511,263,971,197,990 square meters).
>
> *Robert Hinden, [IP Next Generation Overview](https://web.archive.org/web/19970502110430/http://playground.sun.com:80/pub/ipng/html/INET-IPng-Paper.html), INET'95, May 1995*

The arithmetic holds; Hinden truncated rather than rounded. Even under Huitema's most pessimistic assumption about how wastefully addresses would be handed out, Hinden went on, the space would still provide 1,564 addresses for each square meter of the Earth. And he named the markets the room was for: "every television set will become an Internet host," and the lighting, heating, and motors "currently controlled via analog switches." The specification itself, [RFC 1883](https://www.rfc-editor.org/rfc/rfc1883.txt), dated December 1995, states the purpose in one sentence that survived, but for a hyphen, into the Internet Standard of 2017: "IPv6 increases the IP address size from 32 bits to 128 bits, to support more levels of addressing hierarchy, a much greater number of addressable nodes, and simpler auto-configuration of addresses."

Even the jokes assumed room inside the home. On 1994 April 1, while the choice was still being made, an RFC written as a look back on a fictional IPv9 described providers that "issue a range of 1 billion addresses to each house," divided "into subnet hierarchies" by room and floor, so that "the light bulb requests an address block from the light switch, the light switch in turn from the electrical system." Its punch line is that IPv9 comes to the end of its useful life "once again due to address space exhaustion." [RFC 1606](https://www.rfc-editor.org/rfc/rfc1606.txt) closes, "Those who do not study history, are doomed to repeat it."

The comparison everyone remembers came later and from no one in particular. In the online textbook that became Jim Kurose and Keith Ross's *Computer Networking*, the version captured in February 1999 said that with IPv6 "every toaster on the planet can be IP-addressable"; [the revision dated December 1999](http://www2.ic.uff.br/~michael/kr1999/4-network/4_07-IPv6.htm) changed the toaster to "every grain of sand on the planet." Nokia's IPv6 pages had "one for every grain of sand on the planet" by January 2001, and by 2008 a European Commissioner, Viviane Reding, was telling the press that IPv6 "provides more addresses in cyberspace than there are grains of sand on the world's beaches," in a release headed "An unlimited source of Internet addresses" that promised it would "make it much easier for home users to build their own private networks." This research found no founder of the protocol on record with the sand; it was folklore by the time it reached print. The founders had their own ways of saying it. Vint Cerf, as honorary chairman of the IPv6 Forum, told Network World in October 1999 that the 128-bit field "allows for 10 to the 38th power possible addressable devices," and added, "We can't afford to send trained teams of people out to install the IP-enabled washing machine." On World IPv6 Day in June 2011 he said it on national television, in a report narrated by correspondent Hari Sreenivasan:

> HARI SREENIVASAN: ... Going forward, could IPv6 run out of addresses, too? That's unlikely. The number of addresses that IPv6 allows for is equivalent to 3.4 times 10 to the 38th.
>
> VINT CERF: Three hundred and forty trillion trillion trillion addresses, which is enough to last until after I'm dead. Then it's somebody else's problem.
>
> HARI SREENIVASAN: The amount is so big that experts are calling it unlimited.
>
> *[PBS NewsHour](https://www.pbs.org/newshour/show/world-tests-ipv6-why-4-2-billion-internet-addresses-just-weren-t-enough), World Tests IPv6, 2011 June 8*

The policy written for the space matched the rhetoric. In September 2001 the Internet Architecture Board and the IETF's steering group recommended, in [RFC 3177](https://www.rfc-editor.org/rfc/rfc3177.txt), that every site receive a `/48`, a block with room for 65,536 separate networks inside it, and they meant every home too: "Home network subscribers, connecting through on-demand or always-on connections should receive a `/48`." They had done the sums. There were about 35 trillion `/48`s in the part of the space then open, against a projected ten billion people in 2050, and the decision could be taken "in the knowledge that there is objectively no shortage of `/48`s, and the expectation that personal, home networks will become the norm." Their scaling goal, they wrote, amounted to "an 80 bit address space *per person*."

The numbers still hold, read the way they were written. Divided among the 8.3 billion people the United Nations projects for 2026, 2^128 is about 41 billion billion billion addresses each. A human body has about 37 trillion cells, by the 2013 count of Bianconi and colleagues at the University of Bologna; IPv6 has about a quadrillion addresses for every cell in every person alive. Against the 7.5 billion billion grains of sand usually estimated for the Earth's beaches and deserts, it has about 45 billion billion addresses per grain. Every one of those comparisons is true. Every one of them counts addresses, and as the rest of this article shows, what a home, a phone, or a server receives is a network, or a single address, and the count of those comes out very differently.

## A field guide to addresses

Before the story of where the room went, a reader needs to be able to read an address, and to tell the addresses that reach the internet from the local ones that never leave the room.

### Two sizes of address

An IPv4 address is 32 bits, written as four decimal numbers from 0 to 255 separated by dots, one number per byte: `192.0.2.1`. There are 4,294,967,296 of them. The dotted form is so familiar that it is surprising to find that [RFC 791](https://www.rfc-editor.org/rfc/rfc791.html), the September 1981 specification of IPv4, never writes an address that way; the Assigned Numbers list published the same month calls the notation "commonly used," and it was another eight years before [RFC 1123](https://www.rfc-editor.org/rfc/rfc1123.html) gave it the name dotted-decimal.

An IPv6 address is 128 bits, written as eight groups of four hexadecimal digits separated by colons. Hexadecimal counts in sixteens, using the digits 0 to 9 and then the letters a to f, so each digit stands for exactly four bits and each group of four digits for sixteen. Written out in full, an address looks like this:

```
2001:0db8:0000:0000:0000:0000:0000:0001
```

Two shortcuts make that bearable. Leading zeros in a group may be dropped, and one run of all-zero groups may be replaced by a double colon, so the same address is usually written `2001:db8::1`. The flexibility caused enough confusion that [RFC 5952](https://www.rfc-editor.org/rfc/rfc5952.html), in 2010, opened by printing eight different spellings of one address and then set a single canonical form: leading zeros always dropped, the double colon used for the longest run of zeros and never for a single group, and letters always lowercase, because "Capital case D and 0 can be quite often misread." The colons themselves go back to Steve Deering's draft of November 1992, where they kept an address from being mistaken for a domain name or a dotted IPv4 address.

Hexadecimal earns its place in the boundaries. Because each digit is four bits, a prefix whose length is a multiple of four ends exactly on a digit, and the sizes that matter in this article all do. In the address above, the first twelve digits are a `/48`, the first fourteen a `/56`, and the first sixteen a `/64`, the point where, in nearly every address in use, the network ends and the device begins:

```
2001:0db8:1234:5600 : 0000:0000:0000:0001
network, 64 bits      device, 64 bits
```

### Why there is no IPv5

The version number is the first four bits of every packet, so there are sixteen possible values, and IANA keeps the list. An experimental Internet Stream Protocol for voice and video took version 5 in 1979, before IPv4's own specification was published, when James Forgie of MIT's Lincoln Laboratory proposed it in [IEN 119](https://history.rfc-editor.org/ien/ien119.txt): "It is therefore proposed to use one of the 16 possible IP versions to distinguish ST envelopes from IP datagrams." Its successors, ST-II in 1990 and ST2+ in [RFC 1819](https://www.rfc-editor.org/rfc/rfc1819.html) in 1995, kept the number and shipped real implementations. When the candidates to replace IPv4 lined up in the early 1990s, they took the next numbers in turn: 6 for Deering's SIP, 7 for Robert Ullmann's TP/IX, 8 for Paul Francis's PIP, and 9 for TUBA. Ullmann explained his choice in [RFC 1475](https://www.rfc-editor.org/rfc/rfc1475.html):

> there wasn't anything magic about the number 7, I made it up. Version 4 is the familiar current version of IP. Version 5 is the experimental ST (Stream) protocol. ST-II, a newer version of ST, uses the same version number, something I was not aware of until recently; I suspected it might have been allocated 6. Besides, I liked 7.
>
> *Robert Ullmann, [RFC 1475](https://www.rfc-editor.org/rfc/rfc1475.html), TP/IX: The Next Internet, June 1993*

SIP grew into SIPP, SIPP was chosen, and in January 1995 [RFC 1752](https://www.rfc-editor.org/rfc/rfc1752) recorded the result: "The IANA has assigned version number 6 to IPng. The protocol itself will be called IPv6." In 2016 the IETF's steering group moved versions 5, 8, and 9 to historic, and IANA now lists 5, 7, 8, and 9 as reserved. Of version 7, an RFC from 2012 says simply, "IPv7 was never widely deployed."

### The slash

A prefix is the leading part of an address that names a network, and the number after the slash says how many bits long it is. Everything after the prefix is room inside that network. The notation came with classless routing in 1993, which let blocks be any size instead of the three fixed classes IPv4 started with; [RFC 1519](https://www.rfc-editor.org/rfc/rfc1519.txt) still wrote prefixes as pairs of network and mask, the slash was standard usage by 1995, and [RFC 4632](https://www.rfc-editor.org/rfc/rfc4632.html) wrote it down formally in 2006, noting that classless routing, meant as a stopgap of a few years, had far outlasted its expected life.

In IPv4, `192.0.2.0/24` is a block whose first 24 bits are fixed, leaving 8 bits, 256 addresses, inside; a `/16` holds 65,536 addresses and a `/8`, the size of the old Class A blocks, holds 16.7 million. In IPv6 the same arithmetic runs at a scale where the interesting unit is no longer the address but the `/64`, the standard size of a single network, which the next section explains. Table 1 gives the sizes this article uses.

<figure>
<figcaption>Table 1. Common IPv6 prefix sizes, what each usually is, and how many <code>/64</code> networks it holds. Sizes from <a href="https://www.rfc-editor.org/rfc/rfc4291">RFC 4291</a>, <a href="https://www.rfc-editor.org/rfc/rfc3177">RFC 3177</a>, <a href="https://www.rfc-editor.org/rfc/rfc6177">RFC 6177</a>, and <a href="https://www.ripe.net/publications/docs/ripe-690/">RIPE-690</a>; counts derived.</figcaption>

| Prefix | Usually | `/64` networks inside |
|---|---|---:|
| `/29`, `/32` | A provider's allocation from a regional registry | 34 billion, 4.3 billion |
| `/48` | A site: [RFC 3177](https://www.rfc-editor.org/rfc/rfc3177)'s size for every home and office, and the size RIPE-690 calls the most practical choice | 65,536 |
| `/56` | A home, at the many providers that follow RIPE-690's residential option | 256 |
| `/60` | A home, at providers that give a little | 16 |
| `/64` | One network: a phone, a home at some providers, a server at some hosts | 1 |
| `/128` | One address | none |


</figure>

### The local addresses

Some addresses never leave the device, the cable, or the building, and every device carries a few of them. They are worth recognizing, because they turn up in every list of a machine's addresses and are easy to mistake for the real thing.

**Loopback.** A machine talking to itself uses `127.0.0.1` in IPv4, from a block of sixteen million reserved for the purpose, and `::1` in IPv6, a single address.

**Private IPv4.** [RFC 1918](https://www.rfc-editor.org/rfc/rfc1918.html) set aside the ranges `10.0.0.0/8`, `172.16.0.0/12`, and `192.168.0.0/16` in 1996 for networks that would not be reached from outside. They are what a home router hands to the devices behind it, translating them all to its one public address on the way out. [RFC 6598](https://www.rfc-editor.org/rfc/rfc6598.html) set aside a fourth range, `100.64.0.0/10`, in 2012, for carriers that translate a second time, behind the home router, which is called carrier-grade NAT.

**Link-local.** Every IPv6 interface is required to have an address starting `fe80`, and [RFC 4291](https://www.rfc-editor.org/rfc/rfc4291.html) is plain about how far it reaches: "Routers must not forward any packets with Link-Local source or destination addresses to other links." It is a real address, but only on the wire or the radio between a device and its immediate neighbors, which is why IPv6's own housekeeping runs on it, and why it never works as an address to give anyone. Since every link uses the same `fe80` prefix, an operating system has to be told which link is meant, as in `fe80::1%en0`. IPv4 has the same idea in `169.254.0.0/16`, the address a machine gives itself when no one answers its request for one.

**Unique local.** IPv6's equivalent of the private ranges, defined in [RFC 4193](https://www.rfc-editor.org/rfc/rfc4193.html) in 2005, starts `fd`. A site picks 40 random bits to follow it, so that two networks that later merge almost never collide, and the result is a `/48` of its own, 65,536 networks of private space, free, without asking anyone. The local addresses, in other words, come with the room the global ones were promised. They replaced an earlier attempt, the site-local prefix `fec0::/10`, which [RFC 3879](https://www.rfc-editor.org/rfc/rfc3879.html) deprecated in 2004 after years of failing to agree what a site was. It concluded, "experts have been arguing about the definition of sites for years and have reached no sort of consensus. That suggests that there is in fact no consensus to be reached."

**Multicast and documentation.** Addresses starting `ff` are multicast, one packet to a group of listeners; IPv6 has no broadcast at all. And IANA reserves `2001:db8::/32`, joined in 2024 by `3fff::/20`, for examples in books and manuals, as `192.0.2.0/24` is in IPv4, so that an example never points at someone's real machine. The addresses in this article's diagrams come from those ranges.

### The global addresses

Everything else that a device uses to reach the internet comes from one block: `2000::/3`, every address whose first digit is 2 or 3. It is one eighth of the whole space, and IANA, which hands out the top level, allocates from nothing else; the registry still carries the 2003 instruction that the other seven eighths, "approximately 85% of the IPv6 address space," is "reserved for future definition and use." IANA's grants go to the five regional internet registries, which allocate to providers and large organizations in their parts of the world, which in turn assign to customers. The first grants, on 1999 July 1, were small blocks inside `2001::/16` for Asia-Pacific, North America, and Europe. On 2006 October 3 each registry received a `/12` of its own, and those blocks give real addresses their recognizable first digits:

<figure>
<figcaption>Table 2. The regional registries' blocks from IANA, and the first digits they give an address. Source: IANA IPv6 Global Unicast Address Space registry.</figcaption>

| Registry | Region | Blocks | Addresses start |
|---|---|---|---|
| APNIC | Asia and the Pacific | 2400::/12 (2006), 2410::/12 (2024) | 2400 to 241f |
| ARIN | North America | 2600::/12 (2006), 2630::/12 (2019) | 2600 to 260f, 2630 to 263f |
| LACNIC | Latin America and the Caribbean | 2800::/12 (2006) | 2800 to 280f |
| RIPE NCC | Europe, the Middle East, Central Asia | 2003::/18 (2005), 2a00::/12 (2006), 2a10::/12 (2019) | 2003, 2a00 to 2a1f |
| AFRINIC | Africa | 2c00::/12 (2006) | 2c00 to 2c0f |


</figure>

That is why so many North American addresses start with 2600: it is the first number of ARIN's block, and the block's first allocation, in December 2006, went to Sprint at `2600::/28`. Networks that received space before 2006 still carry older `2001:` prefixes: Hurricane Electric, an early IPv6 carrier, has had `2001:470::/32` since 2001, and Google's public DNS resolver answers at `2001:4860:4860::8888` from a block allocated in 2005.

### The map

The registries' records give the address space a shape. Comcast holds `2601::/20` and, across all its blocks, more than eight thousand `/32`s, and announces more IPv6 space to the rest of the internet than any other network. Deutsche Telekom has held `2003::/19` since January 2005, the day after IANA gave that block to Europe's registry. China Telecom's single record is `240e::/18`, assembled between 2010 and 2018, and it announces a quarter of it. China Mobile and China Unicom hold a `/20` each, and so, in a list that runs to about two dozen, do Veterans Affairs, the National Oceanic and Atmospheric Administration, Telstra, Telecom Italia, the carmaker Geely, the oil company Sinopec, and the Church of Jesus Christ of Latter-day Saints. Cloudflare sits at `2606:4700::/32`, where its resolver answers at `2606:4700:4700::1111`. Amazon's main block is `2600:1f00::/24`, from North America's registry, and its published ranges use it in thirty regions on every inhabited continent, including São Paulo, Cape Town, and Tokyo.

The universities are where IPv4's old map and IPv6's new one differ most. In IPv4, the institutions that built the network received the largest blocks there were: MIT held `18.0.0.0/8`, one 256th of all IPv4. In IPv6, MIT holds `2603:4000::/24`, allocated in 2016, which is the same size as Amazon's or Microsoft's largest single block from ARIN, one sixty-fourth of China Telecom's, and one two-millionth of `2000::/3`. Stanford, which once held `36.0.0.0/8`, has an ordinary `/32`. The largest academic holder anywhere is China's research network, CERNET.

The largest single block ever allocated went to a bank. On 2023 October 20 ARIN allocated `2630::/16` to Capital One, the first allocation from its newest `/12`. A `/16` holds 2^32 `/48`s, as many site-sized networks as IPv4 has addresses; it is 13% of all the IPv6 space the five registries have allocated, and as of this writing Capital One announces none of it to the internet. When an operator asked on ARIN's technical list whether the record could be accurate, ARIN's John Sweeting replied, "Yes, it is accurate and was allocated per policy." ARIN rounds every allocation up to a four-bit boundary, so a network that justifies a `/19` receives a `/16`. On the European registry's IPv6 list, Eric Vyncke wrote that if it were not a mistake, "I will start to fear an IPv6 addresses exhaustion in the future (only 2**13 of `/16` out of 2000::/3)." The second largest block, `2410::/17`, went to Huawei International in Singapore in November 2024, a week after APNIC received the `/12` it came from.

And almost all of it is empty. Of `2000::/3`, IANA has handed about 1.6% to the registries, the registries have allocated about 0.09% onward to networks, and the networks announce about 0.03% in the global routing table, by our count from the registries' files and the routing data on 2026 October 31. IPv4, by contrast, was gone from IANA's pool in February 2011, and about 85% of what is allocated is announced. At the top, the promise of abundance is plainly kept.

How far the first digits work as a map of the world is a question with a precise answer. They name the managing registry almost perfectly: checking every IPv6 record in all five registries' files against IANA's blocks turns up only two small exceptions, and IPv6 space, unlike IPv4, does not move between regions. Beyond that the reading weakens. A registry's region is not a continent; Europe's registry serves the Middle East and Central Asia. The country in a record is the holder's, not the user's, and the registries' own file format warns that it "can therefore not be reliably used to map IP addresses to countries." A cloud uses one block worldwide, and an anycast address, like Cloudflare's resolver, is answered from hundreds of places at once. The plan once had geography in it: the first addressing architecture, [RFC 1884](https://www.rfc-editor.org/rfc/rfc1884.txt) in 1995, reserved an eighth of the space for "Geographic-Based Unicast Addresses," and the 1998 revision dropped it. In 2004 the ITU proposed blocks reserved for national governments, and the registries answered that addresses "intrinsically have no national attributes." The map is of who holds the numbers, not where they are.

## The device's half

The bottom 64 bits of nearly every IPv6 address in use belong to the device, and the device fills them in itself. The mechanism is stateless address autoconfiguration, SLAAC. A router on the network announces a prefix, a `/64`, in a message called a router advertisement. Each device that hears it picks a 64-bit interface identifier, joins it to the prefix, checks that no neighbor already has the result, and starts using it. No server hands out addresses and nothing keeps a record. It is the IP-enabled washing machine Cerf described: plug it in and it is on the internet, with no trained team sent to configure it.

It was not always 64 bits. The first draft of SLAAC, by Susan Thomson at Bellcore in January 1995, formed an address "by concatenating an 80-bit subnet prefix with the 48-bit IEEE 802 address of the interface," the hardware address burned into every Ethernet card. The first addressing architecture, [RFC 1884](https://www.rfc-editor.org/rfc/rfc1884) in December 1995, left the length open, gave the 80-and-48 split as its common example, and went the other way from where the standards would end up: "It may be desirable to use an interface ID smaller than a 48-bit IEEE 802 MAC address to allow more space for the additional layers of internal hierarchy." The first SLAAC standard, [RFC 1971](https://www.rfc-editor.org/rfc/rfc1971.txt) in August 1996, left the length to each kind of link, and for sites that needed more room it pointed at the other method, a server handing out addresses: "In those cases where a site requires the use of longer prefixes than can be accommodated by the interface token, stateful autoconfiguration can be used." As late as January 1997, [RFC 2073](https://www.rfc-editor.org/rfc/rfc2073.txt) gave each subscriber a 64-bit prefix and left the other 64 bits local, 16 for the subscriber's own subnets and 48 for the hardware address: "65,535 subnets."

The boundary moved because of a proposal about routing. In October 1996 Mike O'Dell of UUNET posted a draft called 8+8, which split the address into two 8-byte halves: "Routing Goop" on top, which the routers at a site's border would rewrite as the site changed providers, and an "End System Designator" on the bottom, identifying the machine wherever it went. A revision in February 1997, renamed GSE, justified the 8 bytes by the hardware coming to market: "with the impending onslaught of the IEEE-1394 technology," the FireWire standard, "8-byte IEEE MAC addresses are simply fait accompli." At an interim meeting in Palo Alto at the end of that month the working group concluded that "IEEE Mac addresses are unique enough to serve as globally unique ESD's," and then took GSE apart. O'Dell himself, the minutes record, thought it "unreasonable to move forward with the current proposal." The group rejected router rewriting. But Steve Deering polled the room on widening the device's identifier from 6 bytes to 8, and most were for it.

The decision came at the IETF meeting in Memphis in April 1997. Thomas Narten listed the costs of globally unique identifiers in the bottom half, among them "Disallows small subnets," and John Stewart said what that meant for routing: "If global ESD's, mask lengths will be either up to 64 or 128. Nothing in between." On Friday, April 11, Bob Hinden proposed a compromise: every interface identifier would be 64 bits, built in the IEEE's new EUI-64 format, with one bit saying whether it was globally unique. His own minutes record the vote:

> This proposal would "keep the door open" for global ESDs but limit the impact now. The only cost now is requiring 64-bit interface ID and setting the global scope bit correctly.
>
> Considerable discussion resulted. Chairs polled the working group: 0 voted for global ESDs in IPv6 7 voted for no global ESD in IPv6 (e.g., keeping current interface ID behavior) 33 voted for proposal to use EUI-64 global/local bit to identify if interface ID is global. This is a rough consensus to adopt this approach.
>
> *Bob Hinden, reporter, [Minutes of the IPng Working Group](https://www.ietf.org/proceedings/38/97apr-final/xrtftr47.htm), IETF 38, Memphis, April 1997*

That vote is when the bottom half became the device's. The 1998 architecture wrote it in, and its successor, [RFC 4291](https://www.rfc-editor.org/rfc/rfc4291.txt) in 2006, still in force, states it as a rule: "For all unicast addresses, except those that start with the binary value 000, Interface IDs are required to be 64 bits long and to be constructed in Modified EUI-64 format." With the bottom fixed at 64, the subscriber's boundary moved up from `/64` to `/48`, and the 16 bits of room for a site's own networks sat between the two. In 1997 the cost of the trade looked small, because 64 bits for the network still squared IPv4.

No one walked through the door kept open. The globally unique identifier the 64 bits were sized for turned out to be a privacy problem, as the next part of this story tells, and devices stopped using it. In 2014 [RFC 7136](https://www.rfc-editor.org/rfc/rfc7136.txt), by Brian Carpenter and Sheng Jiang, said of the hope that a universally unique identifier would prove useful, "so far, this has not proved to be the case," declared the identifier an opaque string of bits, and removed the sentence that kept the door open. It kept the length. Ten years later Geoff Huston of APNIC measured that some 96% of IPv6 hosts use random identifiers for their public traffic, and asked why the boundary stays:

> The question is why do we persist with this 64/64 bit boundary in the IPv6 address architecture between the network and the host identifier? Why did we not just go all the way and emulate IPv4's address architecture and allow the network operator to select their own address length for the network? I have no rational answer to this question. I'm left with the observation that IPv6 is not in fact a 128-bit address protocol in the same way as IPv4 is a 32-bit address protocol. It's a "64-bits plus a few extra interface identifier bits" address protocol.
>
> *Geoff Huston, [IPv6 Prefix Lengths](https://ispcol.potaroo.net/2024-04/ipv6-prefixes.html), The ISP Column, April 2024*

The boundary stays because so much is built on it. [RFC 7421](https://www.rfc-editor.org/rfc/rfc7421.txt), the IETF's 2015 analysis of the 64-bit boundary, tested seven operating systems and found that every one ignores the flag that turns on SLAAC unless the prefix is exactly 64 bits long: "deployments in which longer subnet prefixes are used cannot make use of SLAAC-configured addresses and require either manually configured addresses or DHCPv6." Hand a network a `/65` and the phones on it simply do not configure. A group of senior engineers has argued since 2017, in a draft called "IPv6 is Classless" that reached its fifteenth revision in September 2026, that "The last remnant of IPv6 classful addressing is a rigid network interface identifier boundary at `/64`," and that "As most wireless operators give a single `/64` to wireless clients, subnetting beyond `/64` is a real world requirement, and its absence is an incentive to deploy network address translation for IPv6." It remains a draft. The working group's own rewrite of the architecture is parked.

### The address that follows you

Putting the hardware address into the IP address meant that a device carried the same 48 bits into every network it joined, readable in every packet by anyone along the path, even when the contents were encrypted. The IETF raised the concern itself, first. On 1999 January 26, days after Intel announced a serial number in its Pentium III processor, an Internet-Draft edited by Keith Moore for the IETF's steering group grouped "48-bit IEEE 802 Media Access Control (MAC) addresses, 64-bit Extended Unique Identifier (EUI-64) addresses, and the serial numbers which Intel Corporation has proposed" as one class of risk, named IPv6 autoconfiguration as a protocol that exposed them, and recommended that internet protocols "SHOULD NOT depend on the inclusion of hardware serial numbers." The press found it that autumn. Bill Frezza's column in InternetWeek in October 1999 asked where the outrage was:

> Surprise! Every packet you send out onto the public Internet using IPv6 has your fingerprints on it. And unlike your IP address under IPv4, which you can change, this address is embedded in your hardware. Permanently.
>
> *Bill Frezza, "Where's All The Outrage About The IPv6 Privacy Threat?", InternetWeek, October 1999, [as reposted](https://diswww.mit.edu/menelaus/cpunks/118779) that week*

"Permanently" overstated it; Thomas Narten had posted the working-group draft that answered the problem four months earlier. It became [RFC 3041](https://www.rfc-editor.org/rfc/rfc3041) in January 2001, by Thomas Narten and Richard Draves, and its motivation is the phone or laptop moving from home to work to a café:

> Even when higher layers encrypt their payloads, addresses in packet headers appear in the clear. Consequently, if a mobile host (e.g., laptop) accessed the network from several different locations, an eavesdropper might be able to track the movement of that mobile host from place to place, even if the upper layer payloads were encrypted [SERIALNUM].
>
> *Thomas Narten and Richard Draves, [RFC 3041](https://www.rfc-editor.org/rfc/rfc3041), Privacy Extensions for Stateless Address Autoconfiguration in IPv6, January 2001*

The fix was temporary addresses: random identifiers, replaced every day or so, used for outgoing connections. The standards and the operating systems then disagreed for fourteen years about whether to turn them on. The 2007 revision, [RFC 4941](https://www.rfc-editor.org/rfc/rfc4941), said they "SHOULD be disabled by default"; Windows turned them on for clients from XP onward, and Apple turned them on in Mac OS X Lion in 2011, in a line of kernel source that still carries its comment: `int ip6_use_tempaddr = 1; /* use temp addr by default for testing now */`. In 2016 macOS Sierra stopped building its stable addresses from the hardware address at all. Today Apple's security guide describes stable addresses generated cryptographically, a different one on each network, and says that "temporary addresses are created with a preferred lifetime of 24 hours, and these are used by default for any new connections." Since iOS 14 the hardware address itself is private, a different one for each Wi-Fi network, and since iOS 18 it can rotate every two weeks. The IETF caught up in steps: [RFC 8064](https://www.rfc-editor.org/rfc/rfc8064) in 2017 retired hardware-derived identifiers as the default, and [RFC 8981](https://www.rfc-editor.org/rfc/rfc8981) in 2021 dropped the advice to leave temporary addresses off.

So the device's half now changes daily and reveals nothing, and the identifier has moved up into the half the device does not control. RFC 3041 said so in 2001, in a paragraph that reads differently now:

> it should be noted that the network prefix portion of an address also serves as a constant identifier. All nodes at (say) a home, would have the same network prefix, which identifies the topological location of those nodes. This has implications for privacy, though not at the same granularity as the concern that this document addresses. Specifically, all nodes within a home would be grouped together for the purposes of collecting information.
>
> *Thomas Narten and Richard Draves, [RFC 3041](https://www.rfc-editor.org/rfc/rfc3041), section 2.2, January 2001*

Its 2021 successor states the limit flatly: "If the network contains a very small number of hosts -- say, just one -- changing just the IID will not enhance privacy, since the prefix serves as a constant identifier." A phone alone on its carrier's `/64`, a server alone on its host's, a household on its provider's: each rotates the bottom half and is recognized by the top. Measurement confirms that the top half holds still. A study of six years of data from RIPE's measurement probes and a large content network, [DynamIPs](https://www.caida.org/catalog/papers/2020_dynamips/dynamips.pdf) in 2020, found that IPv6 assignments often stay "stable for months," so that "a `/64` can be used to identify a subscriber over several months, and sometimes even years." In 2021 Erik Rye, Robert Beverly, and kc claffy showed in [Follow the Scent](https://arxiv.org/abs/2102.00542) that more than nine million home routers still put their own hardware address in their outside address, which let them follow households even across providers that change the prefix daily; in [IPvSeeYou](https://arxiv.org/abs/2208.06767), published in 2023, Rye and Beverly matched the same router identifiers against public databases of Wi-Fi access points and placed more than twelve million routers in 146 countries and territories, with a median error of 39 meters where they could check. A 2022 study of a large provider's traffic, [One Bad Apple](https://arxiv.org/abs/2203.08946), found that a single device in a home still using its hardware address marks the whole home's prefix, and that large third-party platforms "can track up to 17% of subscriber lines." These papers demonstrate that the tracking works; none documents a particular advertiser doing it. Google's own explainer for the IP protection in Chrome's private browsing says the general point plainly: IP addresses are essential to the web, "However, like third-party cookies, they can also be used for tracking."

Some countries answered by making the provider's half move too. Germany's federal and state data protection commissioners resolved in November 2012 that providers must as a rule assign customers' prefixes dynamically, that privacy extensions should be on by default, that IPv6 addresses are personal data, and, in a sentence any peer-to-peer project would underline, that peer-to-peer applications can contribute to a robust, privacy-friendly internet, one that cannot be disrupted or monitored at any single point. German providers renumbered their customers every 24 hours for years afterward. The operators' own best-practice document, RIPE-690 in 2017, answered the other way: "Non-persistent prefixes are considered harmful in IPv6 as you can't avoid issues that may be caused by simple end-user power outages." A prefix that changes every night protects privacy and breaks every server, port mapping, and firewall rule in the home that named the old one. Either way, the privacy of the whole home rests on the top half, and the top half is the provider's to set.

### More addresses, not fewer

The device's half has one more chapter, and in it the device fights for more room, not less. In June 2012 a network engineer filed a request asking Android to support DHCPv6, the protocol in which a server hands each device a single address and keeps a record of it, which enterprises and universities wanted for control and for matching addresses to users. In December 2014 Google's Lorenzo Colitti declined: a single assigned address would break tethering and the translation that lets an IPv6-only phone reach IPv4 sites, both of which need more than one address. One operator answered within the day: "It isn't for you to decide which is better to meet the needs of your users. It is for you to simply meet our needs." When the argument flared again in June 2015, Colitti wrote the reply that the next decade of standards followed:

> But it *is* harmful.
>
> It is harmful because it will force device operating systems to implement IPv6 NAT in order to provide feature parity with today's IPv4 functionality. NAT increases application development cost due to the necessity for developers to implement NAT traversal, impacts battery life due to the need for NAT keepalives, and makes applications flaky due to the huge variance in NAT behaviour, timeouts, statefulness (e.g., if the NAT crashes), and so on.
>
> We have these problems in IPv4 today, but that's unavoidable due to lack of address space. Implementing "one address per device IPv6" is a missed opportunity to remove all those problems.
>
> *Lorenzo Colitti, [Android issue 32621](https://web.archive.org/web/20170324213950/https://code.google.com/p/android/issues/detail?id=32621), comment 97, 2015 June 8*

The replies were not gentle. "Sorry Lorenzo, I call BS. Your religion is showing," one commenter wrote; another said the position "appears to hinge on political fervour." That same day Colitti offered to co-write an IETF document recommending that when a network must assign addresses, it delegate "an IPv6 prefix of sufficient size" instead of a single address. Four weeks later Colitti, Vint Cerf, and Apple's Stuart Cheshire posted the draft that became [RFC 7934](https://www.rfc-editor.org/rfc/rfc7934.txt), a Best Current Practice published in July 2016 with David Schinazi of Apple:

> In IPv6, a single link provides over four billion times more address space than the whole IPv4 Internet [RFC7421]. Thus, unlike IPv4, IPv6 networks are not forced by address scarcity concerns to provide only one address per host.
>
> *Lorenzo Colitti, Vint Cerf, Stuart Cheshire, and David Schinazi, [RFC 7934](https://www.rfc-editor.org/rfc/rfc7934.txt), Host Address Availability Recommendations, July 2016*

Its recommendation is two words in capitals: "it is NOT RECOMMENDED to limit a host to only one IPv6 address per prefix." A device, it says, should either form as many addresses as it needs on its own, or be given "a dedicated `/64` prefix." One of the authors of TCP/IP is on the byline, writing in 2016 that a computer should not be handed a single address, which is exactly what much of the server market, as a later section shows, still does.

## The middle

A subnet is a network inside a network. Devices on the same subnet reach each other directly, finding one another with neighbor discovery, IPv6's way of asking the local link who has an address. Devices on different subnets reach each other only through a router, and a router is where rules can be applied: this network may reach the internet but not that one, these devices may talk to that printer and nothing else. That is why subnets are the basic unit of security in a home or an office. The guest Wi-Fi goes on one, the smart television and the thermostats on another, the family's laptops and phones on a third, a home server or a work computer on a fourth, each with a border at the router. The IETF's home networking architecture, [RFC 7368](https://www.rfc-editor.org/rfc/rfc7368) in 2014, expects exactly this:

> While simple Layer 3 topologies involving as few subnets as possible are preferred in home networks, the incorporation of dedicated (routed) subnets remains necessary for a variety of reasons. For instance, an increasingly common feature in modern home routers is the ability to support both guest and private network subnets. Likewise, there may be a need to separate home automation or corporate extension LANs (whereby a home worker can have their corporate network extended into the home using a virtual private network, commonly presented as one port on an Ethernet device) from the main Internet access network, or different subnets may in general be associated with parts of the homenet that have different routing and security policies.
>
> *Tim Chown, editor, [RFC 7368](https://www.rfc-editor.org/rfc/rfc7368), IPv6 Home Networking Architecture Principles, October 2014*

The FBI's field office in Oregon put the same advice in one line: "Your fridge and your laptop should not be on the same network." The reason is what happens after something on the network is compromised. NIST's IPv6 guidelines note that an attacker who reaches one machine on a subnet "will be able to leverage IPv6 neighbor discovery to identify hosts on the local subnet for exploitation," and neighbor discovery has no authentication at all; any device on a link can claim to be any other. A border stops that at the edge of the subnet. John Brzozowski of Comcast and Gunter Van de Velde of Nokia, writing from Comcast's shared Wi-Fi networks in [RFC 8273](https://www.rfc-editor.org/rfc/rfc8273) in 2017, described what separate prefixes buy: devices "cannot send packets to each other except through the first-hop router," which "will automatically provide robust protection against attacks between devices that rely on link-local ICMPv6 packets."

A public address does not make a device exposed. A home router's firewall decides what may come in, and the standard default for IPv6 home routers, set out in [RFC 6092](https://www.rfc-editor.org/rfc/rfc6092) in 2011, admits only traffic answering a connection made from inside. RFC 7368 draws the line: "It is thus important to distinguish between addressability and reachability." Address translation never provided that protection; [RFC 4864](https://www.rfc-editor.org/rfc/rfc4864) said in 2007 that "IPv4 NAT was not developed as a security mechanism. Despite marketing messages to the contrary, it is not a security mechanism." The firewall works the same on every subnet. What a home cannot do without more than one subnet is put a border between its own devices.

And in IPv6 every subnet is a `/64`, because SLAAC works on nothing else. So the number of subnets a home can have is the number of `/64`s its provider gives it, and the room for them is whatever lies between the provider's prefix and the device's 64 bits. RFC 7368 named the consequence: "the growth inhibitor for the home network shifts from the number of addresses to the number of prefixes offered by the provider."

### The field in the middle

The 1998 address format gave that room a name and a size. [RFC 2374](https://www.rfc-editor.org/rfc/rfc2374.txt) fixed every site's prefix at 48 bits, followed by a 16-bit "Site-Level Aggregation Identifier," and said "The 16 bit SLA ID field support 65,535 individual subnets." (Sixteen bits give 65,536; the field was one larger than its own description.) The size was not an estimate of what a home would need. When the IAB and IESG sent their recommendation to the registries in September 2000, they said so: "Note that none of these reasons imply an expectation that homes, vehicles, etc. will intrinsically require 16 bits of subnet space." The `/48` was uniform so that every site looked the same, could change providers without renumbering its insides, and would never have a reason to translate its own addresses. That statement also records that experts from the IETF and the registries had suggested a `/56` for homes at the IETF meeting in Adelaide that March, and then set it aside, because "subsequent analysis has revealed significant advantages in using `/48` uniformly," and it warned that scarce space for customers would bring back "v6/v6 NATs, which would defeat the ability of IPv6 to restore address transparency." [RFC 3177](https://www.rfc-editor.org/rfc/rfc3177) published it the next year and answered the obvious objection in advance: "The question naturally arises whether giving a `/48` to every subscriber represents a profligate waste of address space. Objective analysis shows that this is not the case."

The objection came back in 2005, from the people who run the registries. In July Geoff Huston, then at APNIC, published a column that took the grains of sand at their word and then did the subtraction:

> So within this address plan there are 48 bits for the global routing identifier. This is the number of remaining bits once the 64 bit device identifier and the 16 bit subnet identifier are stripped out of the 128 bit address field. There are 281,474,976,710,656 global identifiers in this 48 bit space.
>
> Now if 128 bits encompasses 300 million planets the size of the earth, then one planet requires only 103 bits. By the time you get down to 48 bits then you are looking at the number of grains of sand on a large beach!
>
> *Geoff Huston, [Just how big is IPv6? - or Where did all those addresses go?](https://www.potaroo.net/ispcol/2005-07/ipv6size.html), The ISP Column, July 2005*

His sidebar sized the beach at 60 kilometers long, 30 meters wide, and 30 centimeters deep. David Conrad put the same point on ARIN's policy list that September: "I don't think people understand that IPv6 is not really 2^128. Currently, it is between 2^48 and 2^64." Measured in what is actually handed out, the sand was finite after all. Huston proposed `/56` for homes, which he estimated would cut the space consumed by two orders of magnitude, and his argument was as much about fairness as arithmetic. On ARIN's list he warned that a generous plan, changed only after the early networks had their space, would repeat IPv4's history, "visibly creating an early adopter reward and a corresponding late adopter set of barriers and penalties," at a moment when the ITU was already proposing an address system of its own on exactly that complaint. The proposal he wrote with Kurtis Lindqvist for Europe's registry, 2005-08, promised that the changes "would increase the anticipated useful lifetime of IPv6 to encompass a period in excess of 100 years," and made the size of each customer's block the provider's own decision. At the RIPE meeting that October, the minutes record, Huston explained that "the proposal allows for product differentiation."

Not everyone agreed. Tony Hain asked at the next meeting, in the minutes' words, "why are we pushing the assignment boundary to `/56`? Conservation is not an issue with IPv6." Randy Bush objected to fixed sizes of any kind as "classful addresses all over again." An unnamed participant at ARIN's October 2005 meeting said it most bluntly: "Fear is the real issue here. We just pushed it from 50 years to 500 years with the HD Ratio change and we're talking about taking this out to 50,000 years. `/56` is not on the right track. The fear is we're going to waste space and we're going to waste it anyway. Get over it."

What the operators said in those months shows where the habits came from. On the day 2005-08 was announced, Gert Doering asked on the RIPE list why anyone would give a customer ten IPv6 addresses and not a full `/64`, which "would be very much against the spirit of IPv6 - 'have enough addresses, and no questions asked'." Jørgen Hovland, an operator in Norway, answered with a list drawn straight from IPv4 practice:

> * Internal marketing and/or policy reasons.
> * Limit the amount of abuse.
> [...]
> * Limitations in the contract making it reasonable to limit the amount of IP addresses you get, like "you are only allowed to connect 10 cameras to the internet".
> * We might want to sell a "cheaper" version of a better product.
>
> *Jørgen Hovland, [address-policy-wg](https://www.ripe.net/ripe/mail/archives/address-policy-wg/2005-October/001327.html), RIPE NCC, 2005 October 5*

Doering replied to the list: "Limiting the number of IPv6 address is just going to break things, but not making the product cheaper." At APNIC's meeting that September an operator asked the room to keep "the same conservative IP policies that have charactered RIR IPv4 allocations," and wondered why a home with "one fridge, one telephone, one PDA, one PC, one laptop" needed so much. Lee Howard wrote to ARIN's list that he did not "think we need to give them 65,000 times what they need (and nevermind those last 64 bits)." One attendee at ARIN's October meeting, in the minutes' paraphrase, took the outcome for granted: "ISPs are going to allocate 64s to end home users and they won't have a choice," and asked how a home would then get more. On the NANOG list at Christmas 2007, Joe Greco wrote down the conversation a home user asking for more would have:

> Answer 1: "We don't provide more space on your Cableco Cable Modem Service. If you would like, we do offer Cableco Business Class Service."
>
> *Joe Greco, [NANOG](https://diswww.mit.edu/charon/nanog/101263), 2007 December 25*

RIPE accepted proposal 2005-08 in November 2007, and the policy that carried it, RIPE-421, said what the smallest customer would get:

> End Users are assigned an End Site assignment from their LIR or ISP. The size of the assignment is a local decision for the LIR or ISP to make, using a minimum value of a /64 (only one subnet is anticipated for the End Site).
>
> *APNIC, ARIN, and RIPE NCC, [RIPE-421](https://www.ripe.net/documents/2318/ripe-421.pdf), IPv6 Address Allocation and Assignment Policy, November 2007*

That sentence stayed in Europe's address policy until 2020. Here is what it, and each of the sizes before it, leaves a home. In each line, P is the provider's prefix, S is the home's own room for subnets, and D is the device's half:

<div class="language-text"><pre><code>/48  PPPP:PPPP:PPPP:<span class="bits-home">SSSS</span>:DDDD:DDDD:DDDD:DDDD   16 bits: 65,536 networks
/56  PPPP:PPPP:PPPP:PP<span class="bits-home">SS</span>:DDDD:DDDD:DDDD:DDDD    8 bits: 256 networks
/60  PPPP:PPPP:PPPP:PPP<span class="bits-home">S</span>:DDDD:DDDD:DDDD:DDDD    4 bits: 16 networks
/64  PPPP:PPPP:PPPP:PPPP:DDDD:DDDD:DDDD:DDDD   no bits: one network
</code></pre></div>

At `/64` the provider's half meets the device's half, and the middle is gone. The provider's half alone is 64 bits, as wide as two IPv4 addresses laid end to end, enough for every IPv4 address to have four billion networks of its own, and the home's share of the 128 bits is nothing.

> <img class="quote-portrait" src="/wonka.jpg" alt="Gene Wilder as Willy Wonka">It's all there, black and white, clear as crystal! You stole fizzy lifting drinks! You bumped into the ceiling which now has to be washed and sterilized, so you get nothing! You lose! Good day, sir!
>
> *Gene Wilder as Willy Wonka, [Willy Wonka & the Chocolate Factory](https://web.archive.org/web/20210617164244/https://catalog.afi.com/Catalog/moviedetails/54174), screenplay by Roald Dahl, 1971*

### The warnings

The people who wrote the standards saw it happening, and for the next decade they wrote warning after warning, each a little more urgent than the last.

In December 2008 [RFC 5375](https://www.rfc-editor.org/rfc/rfc5375.txt) said that subnets longer than `/64` on links with ordinary devices "would be an especially bad idea." In March 2011 [RFC 6177](https://www.rfc-editor.org/rfc/rfc6177.txt), by Thomas Narten, Geoff Huston, and Lea Roberts, replaced RFC 3177 as a Best Current Practice. It dropped the `/48` as the single answer, it stopped recommending single addresses, "It is no longer recommended that `/128`s be given out," and it set a floor of "at least one `/64`, and in most cases significantly more." It named the temptation directly:

> At the same time, it might be tempting to give home sites a single /64, since that is already significantly more address space compared with today's IPv4 practice. However, this precludes the expectation that even home sites will grow to support multiple subnets going forward. Hence, it is strongly intended that even home sites be given multiple subnets worth of space, by default.
>
> *Thomas Narten, Geoff Huston, and Lea Roberts, [RFC 6177](https://www.rfc-editor.org/rfc/rfc6177.txt), IPv6 Address Assignment to End Sites, March 2011*

It added that the charges for extra addresses familiar from IPv4 were "not due to the RIR fee structures, but to business choices ISPs make," and that "An important goal in IPv6 is to significantly change the default and minimal end site assignment, from 'a single address' to 'multiple networks'." But it left the number itself to "the operational community." The requirements for home routers, [RFC 7084](https://www.rfc-editor.org/rfc/rfc7084.txt) in 2013, told a router to ask for enough room for one `/64` on each of its interfaces, and, if handed less, to "log a system management error." RFC 7368 went further the next year: "if, for example, only a `/64` is offered by the ISP, the homenet may be severely constrained or even unable to function." Rather than improvise with longer prefixes or translation, it recommended "that the receiving router instead enters an error state and issues appropriate warnings," and it set a number: "it is not recommended that an ISP offers less than a `/60` prefix, and it is highly preferable that the ISP offers at least a `/56`." The guidelines for enterprises, [RFC 7381](https://www.rfc-editor.org/rfc/rfc7381.txt) that same October, put the diagnosis in its first sentence on the subject: "The most common problem encountered in IPv6 networking is in applying the same principles of conservation that are so important in IPv4." And in 2016 [RFC 7934](https://www.rfc-editor.org/rfc/rfc7934) said a host should never be limited to one address.

The operators wrote their own number down in October 2017. RIPE-690, a best-practice document from the European registry's IPv6 working group with eleven authors from operators, vendors, and the registry community, opens like an appeal:

> IPv6 is not the same as IPv4. In IPv6 you assign a short prefix to each end-user site, so they are able to have as many subnets (/64s) as they need. You should not be concerned with exhausting the IPv6 addressing space, and you should think big when planning future requirements.
>
> *[RIPE-690](https://www.ripe.net/publications/docs/ripe-690/), Best Current Operational Practice for Operators: IPv6 prefix assignment for end-users, October 2017*

It recommends a `/48` for everyone, or a `/48` for businesses and a `/56` for homes, and "Assigning prefixes longer than `/56` is strongly discouraged." Of the bottom of the range it says, "`/64` is not sustainable, it doesn't allow customer subnetting," and that with a single `/64` "it will not be possible to subnet, assign VLANs, alternative SSIDs, or have several chained routers in the same customer network." Of the split between businesses and homes it says that "This rationale is understood to be mainly coming from sales and marketing departments where they wish to create some distinction in services between different types of customer." And it describes the habit underneath all of it: "numbering decisions that were based on IPv4 scarcity need to be reconsidered because the abundance of IPv6 addresses enables numbering decisions to be taken differently." In 2020 Europe's address policy finally replaced "a minimum value of a `/64` (only one subnet is anticipated for the End Site)" with "a value of 'n' x `/64`," and a pointer to RIPE-690. The other registries now say much the same: ARIN's policy manual still calls a `/48` the recommended unit for a site, LACNIC's and AFRINIC's cite RIPE-690 and recommend a `/48`, and APNIC's counts in multiples of `/64`.

The warnings are guidance, and none of them can compel a provider to change the size it hands out. What the providers actually hand out is a matter of measurement, and the measurements are the next part of the story.

## Homes and phones

What the providers hand out is better than the history so far suggests, for wired homes, and exactly as tight as it sounds for phones. The typical wired home today receives a `/56`, room for 256 networks. Between a quarter and a third of the world's home providers give a single `/64`, and a smaller share give a `/60`. Almost none give the `/48` that [RFC 3177](https://www.rfc-editor.org/rfc/rfc3177) assumed in 2001. And every phone receives exactly one `/64`.

No source counts this by subscriber, so this picture draws on several that count networks, operators, or addresses, and they agree on its shape. The only worldwide survey of operators, run by Jordi Palet Martínez from 2016 and presented in January 2018, drew 1,559 responses from 105 countries; of the 460 that gave a home prefix size, 35% gave homes a `/56`, 33% a single `/64`, and 23% a `/48`. The `/64` answers came "mainly in LACNIC, some countries in APNIC," and the `/56`s from North America and Europe. A draft Palet co-wrote that June called the `/64` third "a possible misinterpretation of [RFC6177] by at least 1/3rd of the operational community." Measurement says the same from the outside. In 2020 [DynamIPs](https://www.caida.org/catalog/papers/2020_dynamips/dynamips.pdf) read the delegation size from the zeros that home routers leave at the end of their prefixes, across six years of RIPE's probes and 32.7 billion address pairs from a content network, and found a spike at `/56`, confirmed for Orange, Deutsche Telekom, and Sky; in North America 30% of home `/64`s pointed to a `/60` and 27% to a `/56`, and in Europe and Africa over 60% pointed to a `/56`. Follow the Scent, probing routers in 2020, found the `/56` the most common size and "about one-quarter of ASes allocate a `/64` to customers." Counted by address instead, the `/64` wins. APNIC Labs' 2024 count put 57% of the addresses it sees in `/64`s, 21% in `/60`s, 20% in `/56`s, and 2% in `/48`s, and Geoff Huston, who ran it, read the result plainly: "the use of a `/64` prefix is prevalent in mobile services, where individual devices are assigned a `/64` prefix by the mobile service provider, while the use of a `/60` and `/56` appears to have become the default setting in broadband deployments." The `/64` share had fallen from 72% in twenty-eight months.

Table 3 names names. It mixes the providers' own statements with regulators' tables, measurements, and, where nothing better exists, reports from customers on operators' mailing lists and forums, and says which is which.

<figure>
<figcaption>Table 3. What a selection of home and mobile providers delegate, by the best source available. Sources are listed in the references.</figcaption>

| Provider | Where | Delegates | Source |
|---|---|---|---|
| [Init7](https://www.init7.net/) | 🇨🇭 Switzerland | `/48` per DSL customer (2010); `/56` on its mass-market plan (2024) | Provider |
| [XS4ALL](https://www.xs4all.nl/) | 🇳🇱 Netherlands | `/48`, static, at no extra cost (2016) | Provider |
| [NetCologne](https://www.netcologne.de/) | 🇩🇪 Germany | `/48` | Measurement (2020) |
| [Aussie Broadband](https://www.aussiebroadband.com.au/) | 🇦🇺 Australia | `/48` | Provider (2024) |
| [Sky](https://www.sky.com/) | 🇬🇧 United Kingdom | `/56`, "up to 256 networks within the home" (2016) | Provider |
| [Deutsche Telekom](https://www.telekom.de/) | 🇩🇪 Germany | `/56`, renumbered daily for years | Provider (2010), measurement (2020) |
| [Orange](https://www.orange.fr/), [SFR](https://www.sfr.fr/) | 🇫🇷 France | `/56` | Regulator (2025) |
| [Sonic](https://www.sonic.com/) | 🇺🇸 United States | `/56` | Provider (2026) |
| [Starlink](https://www.starlink.com/) | 🌐 Worldwide | `/56` | Measurement (2024) |
| [Verizon Fios](https://www.verizon.com/) | 🇺🇸 United States | `/56` | Customer reports (2022) |
| [Bouygues Telecom](https://www.bouyguestelecom.fr/) | 🇫🇷 France | `/60` | Regulator (2025) |
| [Free](https://www.free.fr/) | 🇫🇷 France | `/61`; a `/64` per home in 2007 | Regulator (2025), [RFC 5569](https://www.rfc-editor.org/rfc/rfc5569) |
| [Comcast](https://www.xfinity.com/) | 🇺🇸 United States | `/64` by default, `/60` if the router asks, `/56` only on business service | Customer and operator reports (2021–2026) |
| [AT&T](https://www.att.com/) | 🇺🇸 United States | Up to eight separate `/64`s from a `/60` the gateway keeps | Customer reports (2022) |
| [Frontier](https://frontier.com/) | 🇺🇸 United States | `/64` | Customer report (2026) |
| [NTT FLET'S Hikari](https://flets.com/) | 🇯🇵 Japan | `/56` with the Hikari Denwa phone service, `/64` without (2012) | Operator presentation to the regulator |
| [T-Mobile Home Internet](https://www.t-mobile.com/home-internet) | 🇺🇸 United States | One `/64`, no delegation | Customer reports (2022–2024) |
| Every mobile carrier | 🌐 Worldwide | One `/64` per phone connection | 3GPP, [RFC 6459](https://www.rfc-editor.org/rfc/rfc6459) |


</figure>

The generous end says so in plain words. Sky's announcement in September 2016 that over 90% of its broadband customers had IPv6 explained, "Each customer is provided with a `/56` prefix, that can support up to 256 networks within the home." Init7 told a German operators' meeting in 2010 that it had "decided for a `/48` per xDSL customer, but it works with a `/56` too … make your choice. Either way the customer has plenty of space which lasts forever." Sonic, a California provider, opens its IPv6 help page with the sand, every grain on Earth standing for a whole planet of grains and still only "about 1/4 the number of available IPv6 addresses," and then gives the home its share of it: "A `/56` prefix provides 256 `/64` subnets to a customer, which is typically more than enough for most households or small businesses." France's telecom regulator publishes each operator's prefix size in its annual IPv6 barometer, with the reason it matters: "La taille du préfixe IPv6 conditionne la possibilité de déléguer un préfixe (64 bits habituellement) sur un autre réseau pour des usages avancés," the size of the prefix decides whether a home can delegate a network of its own onward.

The largest American cable provider shows the path from the bottom. When Comcast launched residential IPv6 in 2012, a computer plugged straight into the cable modem received one address, and its FAQ explained why: "We will allocate a single IPv6 address (`/128`), since we know that only a single device is connecting, with no additional need to sub-net." In April, home routers received a `/64` by default, with a stated plan "to provide shorter prefixes based on the type of service and/or devices in use by our subscribers, as well as the prefix size their device requests." Today, by the reports of customers and operators, a router that asks receives a `/60`, and a `/56` requires business service, much as Joe Greco predicted in 2007. On NANOG in August 2026, in a thread that began with a Frontier fiber customer reporting that Frontier was "only handing out `/64`s," an operator summed up the American residential market:

> For residential, it seems most ISPs believe RFC6177 is more of an idea than a firm suggestion[0], and if you want a /56, you need to upgrade to being what they consider a business site, and, of course, pay more.
>
> *Gary Buhrmaster, [NANOG](https://seclists.org/nanog/2026/Aug/36), 2026 August 6*

What a single `/64` costs a household is spelled out on Google's help page for its own Wi-Fi routers: "If the ISP's prefix is 64 bits, IPv6 won't be available on the guest network." Japan made the cost explicit in a different way. On NTT's fiber network, a household with NTT's Hikari Denwa phone service received a `/56`, and one without received a `/64`; an operator showed the ministry's study group in 2012 that the IPv6 space a customer could use differed depending on whether they subscribed to Hikari Denwa, and the group's minutes in April 2013 record the problem that, on NTT's network, a home could not divide its network into segments without a Hikari Denwa contract. The room for a home's own networks came bundled with a telephone line.

### One network per phone

A phone on a cellular network receives exactly one `/64` for each data connection, by design. The 3GPP standards that define mobile networks call the connection a PDP context, and [RFC 6459](https://www.rfc-editor.org/rfc/rfc6459.txt), the IETF's 2012 description of IPv6 on those networks, defines it in one line: "A PDP (Packet Data Protocol) context is an association between a UE represented by one IPv4 address and/or one `/64` IPv6 prefix, and a PDN represented by an APN." The phone owns the whole `/64` and uses its bottom half for its own addresses, a new temporary one each day. Prefix delegation, the mechanism that would give a phone room to hand networks to the devices around it, has been an optional part of the mobile standards since 3GPP's Release 10 in 2011, and no carrier found in this research states that it turns it on for handsets. The 5G standards specify delegation for home gateways on fixed wireless service, but customers of the two large American fixed-wireless home services report a single `/64` and no delegation, and France's regulator lists the 4G and 5G home access of every operator that offers IPv6 on it at 64 bits.

So when a phone shares its connection with a laptop, it has no second network to give. The IETF documented the workaround in 2014, in [RFC 7278](https://www.rfc-editor.org/rfc/rfc7278.txt), written by Cameron Byrne of T-Mobile USA with two Deutsche Telekom engineers "to facilitate the use of IPv6 in a LAN prior to the deployment of DHCPv6 Prefix Delegation in 3GPP networks": the phone stretches its own `/64` across the tethered link and answers neighbor discovery on the laptop's behalf. The document says of itself that "DHCPv6 is the best way to delegate a prefix to a LAN link," and that its methods should be used only when that "is not achievable." Twelve years later it is how phones tether. Android's tethering code, added in 2016 and still carrying the same comment in 2026, gives the reasoning:

> In lieu of Prefix Delegation and other evaluation of whether a prefix may or may not be dedicated to this device, for now just check whether the upstream is TRANSPORT_CELLULAR. This works because "[t]he 3GPP network allocates each default bearer a unique /64 prefix", per RFC 6459, Section 5.2.
>
> *Android Open Source Project, [IPv6TetheringCoordinator.java](https://android.googlesource.com/platform/frameworks/base/+/refs/tags/android-7.1.0_r1/services/core/java/com/android/server/connectivity/tethering/IPv6TetheringCoordinator.java), Android 7.1, 2016*

The same file notes, "we only support tethering IPv6 toward the oldest (first requested) active downstream." One network arrives, and it goes to the first device that asks. The engineers arguing that the `/64` boundary should be movable put the phone's position in a sentence: "In the case where a device is given a `/64` (e.g. mobile phone running SLAAC only, not DHCP), there is no protocol allowing them to provide downstream routed layer 3 subnets, because all they have is a `/64`."

The direction of travel is mixed. Comcast went from a single address to a `/64` to a `/60` on request. Free, the French provider, made native IPv6 available to more than 1.5 million homes in five weeks at the end of 2007, for any that switched it on, using 6rd, a tunnel Rémi Després designed and presented to it. At first it could give each home only a `/64`, because 6rd appended the customer's 32-bit IPv4 address to a 32-bit prefix and left nothing else; a few months later, with a larger block from the registry, "up to 16 IPv6 links per customer became possible." APNIC's `/64` share is falling. Others moved the other way: Japan's IIJ cut its consumer prefix from `/48` to `/56` by 2012. And a new bottleneck has turned up inside the home. Many home routers receive a `/56` and then cannot pass any of it on to a second router behind them, which is why in July 2025 the IETF published [RFC 9818](https://www.rfc-editor.org/rfc/rfc9818.txt), requiring them to: "Many service providers assign larger address blocks than `/64` to CE routers, as recommended in [RFC6177]. If an IPv6 CE router does not support the Identity Association for Prefix Delegation (IA_PD) Prefix Option ... it will not be able to assign any prefixes beyond its local interfaces."

## Servers

A rented server, a virtual machine in a cloud or a virtual private server from a hosting company, is the plainest case of what [RFC 7934](https://www.rfc-editor.org/rfc/rfc7934) meant by a host: one computer, fully under its renter's control, running whatever its renter chooses, often containers or virtual machines of its own that each want an address. The standards since 2011 say it should not be limited to one address, and since 2016 that it should have either the freedom to form addresses on its own or a `/64` of its own. The server market, taken as a whole, hands out addresses, not prefixes.

The largest clouds show the pattern most clearly, because they build everything around a network the customer defines. When Amazon Web Services added IPv6 in 2016, each customer network received a `/56` and each subnet in it a `/64`, but an individual machine receives addresses one at a time from its subnet, capped by its size, two per network interface on the smallest types. Since 2021 an interface can be given a `/80`, which AWS's own pricing describes as "approximately 300 trillion IPv6 addresses" and its address-management service bills as "a single active address attachment." Google Cloud gives each virtual machine's interface a `/96` and configures one address from it. Microsoft's Azure is the strictest: "Each network interface may have at most one IPv6 private address," and "IPv6-only Virtual Machines or Virtual Machines Scale Sets aren't supported. Each NIC must include at least one IPv4 IP configuration."

The smaller hosts split into two camps. In October 2026 we read the published IPv6 documentation of twelve hosting companies that sell small virtual servers in Europe and North America, and on October 13 and 14 we created one small server at three of them and looked at what arrived. Table 4 gives what one ordinary server receives at each, with the clouds above them for comparison.

<figure>
<figcaption>Table 4. What one ordinary rented server receives, by each provider's own documentation, fetched October 2026, and our own tests where noted. Sources are listed in the references.</figcaption>

| Provider | One ordinary server receives | More |
|---|---|---|
| [AWS EC2](https://aws.amazon.com/ec2/) | Single addresses from the subnet's `/64`, a few per interface | A `/80` per interface, counted as one address by AWS's address manager |
| [Google Cloud](https://cloud.google.com/) | A `/96` per interface, one address configured | None documented |
| [Microsoft Azure](https://azure.microsoft.com/) | At most one IPv6 address per interface, and IPv4 required | None |
| [Hetzner](https://www.hetzner.com/) | A `/64` | An extra `/56` for dedicated servers, for a one-time fee |
| [Scaleway](https://www.scaleway.com/) | A `/64` by router advertisement, on by default (tested 2026 October 13) | Up to five `/64`s |
| [netcup](https://www.netcup.com/) | A `/64` | Extra `/64`s for a monthly fee; servers sold with no IPv4 at all |
| [Vultr](https://www.vultr.com/) | A `/64`, by its documentation's examples | — |
| [TransIP](https://www.transip.eu/) | A `/64`, configured on a shared link with a `/48` mask | — |
| [Linode](https://www.linode.com/) (Akamai) | One address on a shared `/64` | A routed `/64` and a `/56` on request, one of each per account in each data center |
| [DigitalOcean](https://www.digitalocean.com/) | 16 addresses, a `/124`, behind a checkbox at creation that is off by default (tested 2026 October 14) | None: "Additional addresses are not available" |
| [UpCloud](https://upcloud.com/) | One address | Up to five |
| [Infomaniak](https://www.infomaniak.com/) | One address | None |
| [OVHcloud](https://www.ovhcloud.com/) | One address on a VPS or cloud instance | None |
| [Leaseweb](https://www.leaseweb.com/) | One address, a `/128`, on a VPS | None |
| [Greenhost](https://greenhost.net/) | Nothing published; one address inside a shared `/64`, set up by support on request (tested 2026 October 14) | — |


</figure>

Of the twelve hosts, five give a whole `/64` by default and seven give less, from sixteen addresses down to none. None gives anything larger than a `/64` by default. The one-address hosts say so without apology. OVHcloud's guide for its virtual servers tells the reader there is no need to configure more "as you only have one IPv6 with your VPS server." Leaseweb assigns its virtual servers "a single IPv6 address with a `/128` prefix," gives its dedicated servers 65,536 addresses "out of the shared `/64` range," and will grant a full `/64` only on a justification, adding that wanting the standard way of configuring addresses is not one: "SLAAC and/or DHCPv6 do not qualify as technical needs as we do not allow automatic configuration protocols in a shared environment." Customers have asked. On OVHcloud's community forum a thread opened in January 2018 asking for more than one IPv6 address per server, and a customer wrote that May, "I have 17 (!) IPv4 addresses on my VPS instance but only 1 (!) IPv6 address in a shared `/64` subnet." A representative replied in June that "at the moment we do not have an ETA on this subject," and a post in April 2026 says the limit still stands. At the other end, BitFolk, a small British host, moved in October 2024 from a `/64` per virtual machine to a `/48` per virtual machine, and took up the obvious question on its own wiki, under the heading "Isn't a `/48` a ridiculously wasteful amount of address space to give to each VM." Its answer: "BitFolk doesn't think so and neither does RIPE, which considers a `/48` per end user site to be fine and a VM as an end user site."

The prices run the other way from the supply. Every host we looked at gives IPv6 free, and nearly all charge for IPv4, the scarce address that is almost exhausted. AWS, Google Cloud, and Azure each charge half a cent an hour for a public IPv4 address. At Scaleway the IPv4 address on the server we tested cost €3.65 a month, the virtual machine itself €0.44, and its storage €0.36, while its `/64` cost nothing: the address costs eight times the computer. Hetzner sells IPv4 as a €0.50 add-on, netcup sells its servers without IPv4 for less, and AWS's simpler Lightsail service sells IPv6-only plans for less. The free resource is the rationed one, so cost is not what rations it.

### Sharing a `/64`

A server that receives a few addresses inside a `/64` shared with other customers inherits the reputation of everyone in that `/64`, because the rest of the internet counts a `/64` as one user. Spamhaus, whose blocklists most mail servers consult, says so in its documentation:

> A "/64" is the industry standard for the smallest IPv6 allocation to individual customers, even for home-uses like cable, DSL or wireless.
>
> Customers of providers that assign different customers within the same /64 block should contact their provider's support, ask for a dedicated /64 assignment, and move mail service to a non-shared /64 range.
>
> NOTE: Linode customers should read this document, then open a ticket to get their own /64.
>
> *Spamhaus, [FAQ: Combined Spam Sources](https://www.spamhaus.org/faqs/combined-spam-sources-css/), fetched October 2026*

Docker Hub applies its pull limits "per IPv4 address or per IPv6 `/64` subnet," and warns that pulls from several users sharing one "may cause abuse rate limiting"; a customer in that OVHcloud thread reported "constant '429 rate limit exceeded' errors" in 2025. MediaWiki's help page on range blocks, for the software Wikipedia runs on, says that "IPv6 `/64` rangeblocks are akin to blocks of single IPv4 addresses," and that longer prefixes are irrelevant "since no end-users will have addresses of this sort." On much of the server market, they do.

### How the server market got here

The operators' own words show the reasoning. In November 2012 Jon Lewis, a network engineer at a hosting company, explained on NANOG why he planned to give each virtual machine a tiny block on a shared network instead of a `/64`:

> Sure, IPv6 is big enough that we could give out /64s from that /48 and not run out of numbers, but I'm concerned about what happens when an abusive customer turns up 2^64 addresses and overloads the neighbor discovery cache on our gear. What's anyone really going to do with more than a few IP addresses on a VPS anyway? Just as we do with additional v4 IPs, if someone really has a need for additional v6 subnets, those could be provided, likely for a fee.
>
> *Jon Lewis, [NANOG](https://seclists.org/nanog/2012/Nov/344), 2012 November 18*

His first worry is real, and it is the honest technical reason behind much of the rationing. A router serving a `/64` on a shared network has to resolve every address someone sends traffic to, and a `/64` has eighteen billion billion of them. [RFC 6583](https://www.rfc-editor.org/rfc/rfc6583), in 2012, described how scanning a mostly empty `/64` can fill a router's neighbor table and lock out real machines; the Linux kernel, by default, keeps at most 1,024 such entries. But the same night William Herrin replied with the fix, and it needs no rationing at all: "Why not assign a single IPv6 address to each VM and then for those folks who need more, *route* a `/64` to the original address?" A routed `/64` costs the router one route and no neighbor entries, however many addresses the server uses behind it. That is what Linode offers on request today. The other half of the reason is the design underneath. Scaleway's documentation is candid about its own history: "Before routed IPs, IPv6 addresses were statically routed to the hypervisors, which means your IP changed depending on which hypervisor your Instance was running on." Many clouds built in the IPv4 years gave each virtual machine one address on a shared network, filtered by address, and added IPv6 to that shape, one address at a time. RFC 7934 listed the same three reasons in 2016, "hardware limitations (e.g., Ternary Content-Addressable Memory (TCAM) size or size constraints of the Neighbor Cache table), business models (e.g., a desire to charge the network's users on a per-device basis), or operational consistency with IPv4 (e.g., an IP address management system that only supports one address per host)," and answered that hardware limits "are expected to ease over time."

## Back to one address

Told fairly, the squeeze has no villain. The designers chose abundance, and gave the bottom half of every address to the device so that plugging in a washing machine would be enough; in 1997 that looked like a small price, because the half that remained still squared IPv4. The devices then learned to change that half every day for privacy, and the reason it was 64 bits long went away, but the length stayed, because by then everything from home routers to phone software was built on it. The registries, worried about fairness to the regions that arrived late, let each provider choose its customers' size, and kept "a minimum value of a `/64` (only one subnet is anticipated for the End Site)" in Europe's policy for more than twelve years. The providers chose with the systems and the habits they had. A cable company that had always given a home one IPv4 address gave it one network; a host whose virtual machines had always had one IPv4 address on a shared network gave each one IPv6 address on a shared network; a sales department that had always sold a business tier found a new line to draw between tiers. Each choice made sense where it was made. Stacked together, they spend the 128 bits in two cuts and leave the one party that needs to subdivide, the home or the server, with nothing in between. Geoff Huston, who argued for the `/56` in 2005, looked back on the address plan in January 2026:

> However, before we go too far down this path it is also useful to bear in mind that the 128 bits of address space in IPv6 has become largely a myth. We sliced off 64 bits in the address plan for no particularly good reason, as it turns out. We then sliced off a further 16 bits for again no particularly good reason.
>
> *Geoff Huston, [IP Addresses through 2025](https://www.potaroo.net/ispcol/2026-01/addr2025.html), The ISP Column, January 2026*

The record is not only a list of losses. Most wired homes in Europe, and many in North America, receive a `/56` or a `/60`, and a `/56` is all the room [RFC 7368](https://www.rfc-editor.org/rfc/rfc7368) asked for and more than most households will use. Comcast went from one address to a `/60` on request, Free from one network to sixteen, and APNIC's share of addresses in single `/64`s is falling. Scaleway has rebuilt its network to give each server a `/64` of its own, citing RIPE-690 and the fact that "SLAAC has been designed to work exclusively with `/64` prefixes." And the standards have found a way to give every device room without anyone having to ask. [RFC 8273](https://www.rfc-editor.org/rfc/rfc8273) in 2017 described a unique prefix for each host on a shared network. [RFC 9663](https://www.rfc-editor.org/rfc/rfc9663.txt), in 2024, by Lorenzo Colitti, Jen Linkova, and Xiao Ma, carries the mobile model of one network per device onto Wi-Fi and office networks, delivered by prefix delegation:

> Providing every host with one prefix allows the network to maintain only one entry per device, while still providing the device the ability to use an arbitrary number of addresses.
>
> *Lorenzo Colitti, Jen Linkova, and Xiao Ma, [RFC 9663](https://www.rfc-editor.org/rfc/rfc9663.txt), Using DHCPv6 Prefix Delegation (DHCPv6-PD) to Allocate Unique IPv6 Prefixes per Client in Large Broadcast Networks, October 2024*

That answers the operator's neighbor-table worry and the device's need for many addresses in one design, and it is the arrangement Colitti offered to write down in the Android thread in 2015. In June 2025 [RFC 9762](https://www.rfc-editor.org/rfc/rfc9762) gave networks a flag to tell devices to ask for a prefix, and that September Android announced prefix delegation for most of its devices running Android 11 and later, to arrive by system update. The announcement, by Colitti and Patrick Rohr, conceded that "Until now, Android only supported SLAAC," and described the result in the language of the original promise: "devices maintain the flexibility of SLAAC, such as the ability to use a nearly unlimited number of addresses, and the network maintains the manageability and accountability of a traditional DHCPv6 setup." It is the right design, nine years after the Best Current Practice that asked for it, and it will reach a network only when the network turns it on.

Meanwhile the top of the address space is almost untouched. Of the one eighth that IANA hands out, about a thirtieth of one percent is routed. The registries hold about seventeen times what they have allocated, a bank holds a block with room for as many networks as IPv4 has addresses and announces none of it, and Hinden's 665 sextillion addresses per square meter are all still there. The squeeze is entirely at the bottom, in the last few bits between a provider's prefix and a device's identifier, where a home arranges its own networks and a server gives its containers addresses of their own. That is where the room was promised, and that is where much of it never arrived. On World IPv6 Day in 2011, the same broadcast that carried Cerf's 340 trillion trillion trillion had this exchange about the protocol the world was leaving behind:

> GWEN IFILL: Right now, each person has one address.
>
> LESLIE DAIGLE: At most.
>
> GWEN IFILL: At most.
>
> LESLIE DAIGLE: Typically.
>
> GWEN IFILL: So -- and, sometimes, it's an address that they share with other people or other institutions or within an institution.
>
> LESLIE DAIGLE: Yes.
>
> *[PBS NewsHour](https://www.pbs.org/newshour/show/world-tests-ipv6-why-4-2-billion-internet-addresses-just-weren-t-enough), World Tests IPv6, 2011 June 8*

Fifteen years later a phone has one network, a household on a stingy provider has one network, and a server rented from many hosts has one address. For anyone building software meant to reach a computer directly, a peer, a game, a call, a service on a home server, the count sets the design, as the companion report [Open Address, Closed Device](/open-address-closed-device) found from the other side, counting the devices that could run a peer: the far end may have very little room of its own, for a long time yet. The address space is the largest ever built, and the room in the middle of it is the part worth winning back, one prefix delegation at a time.

## References

1. Bradner, S., and Mankin, A. *IP: Next Generation (IPng) White Paper Solicitation.* RFC 1550, December 1993. https://www.rfc-editor.org/rfc/rfc1550.txt
2. Vecchi, M. *IPng Requirements: A Cable Television Industry Viewpoint.* RFC 1686, August 1994. https://www.rfc-editor.org/rfc/rfc1686.txt
3. Partridge, C., and Kastenholz, F. *Technical Criteria for Choosing IP The Next Generation (IPng).* RFC 1726, December 1994. https://www.rfc-editor.org/rfc/rfc1726.txt
4. Bradner, S., and Mankin, A. *The Recommendation for the IP Next Generation Protocol.* RFC 1752, January 1995. https://www.rfc-editor.org/rfc/rfc1752.txt
5. Huitema, C. *The H Ratio for Address Assignment Efficiency.* RFC 1715, November 1994. https://www.rfc-editor.org/rfc/rfc1715.txt
6. Hinden, R. *IP Next Generation Overview.* draft-hinden-ipng-overview-00, October 1994, https://www.sobco.com/ipng/internet-drafts/draft-hinden-ipng-overview-00.txt; INET'95 paper, May 1995, archived copy, https://web.archive.org/web/19970502110430/http://playground.sun.com:80/pub/ipng/html/INET-IPng-Paper.html; Communications of the ACM 39(6):61–71, June 1996, doi:10.1145/228503.228517.
7. Deering, S., and Hinden, R. *Internet Protocol, Version 6 (IPv6) Specification.* RFC 1883, December 1995; RFC 8200, July 2017. https://www.rfc-editor.org/rfc/rfc1883.txt
8. Onions, J. *A Historical Perspective On The Usage Of IP Version 9.* RFC 1606, April 1994. https://www.rfc-editor.org/rfc/rfc1606.txt
9. Kurose, J., and Ross, K. *Computer Networking: A Top-Down Approach Featuring the Internet,* section 4.7, IPv6, online draft. February 1999 capture, https://web.archive.org/web/19990225162243/http://gaia.cs.umass.edu:80/kurose/network/IPv6/IPv6.htm; December 1999 revision, http://www2.ic.uff.br/~michael/kr1999/4-network/4_07-IPv6.htm
10. Nokia Networks. *IPv6: Benefits.* Archived 2001 January 28. https://web.archive.org/web/20010128013600id_/http://www.nokia.com/ipv6/benefits.html
11. European Commission. *An unlimited source of Internet addresses to be on stream in Europe by 2010.* Press release IP/08/803, 2008 May 27. https://ec.europa.eu/commission/presscorner/detail/en/IP_08_803
12. Barney, D., and Marsan, C. D. *Internet pioneer urges transition to new Net protocol.* IDG, on CNN.com, 1999 October 27. https://us.cnn.com/TECH/computing/9910/27/cerf.ipv6.idg/cerf.ipv6.html
13. PBS NewsHour. *World Tests IPv6: Why 4.2 Billion Internet Addresses Just Weren't Enough.* Broadcast transcript, 2011 June 8. https://www.pbs.org/newshour/show/world-tests-ipv6-why-4-2-billion-internet-addresses-just-weren-t-enough
14. IAB and IESG. *IAB/IESG Recommendations on IPv6 Address Allocations to Sites.* RFC 3177, September 2001. https://www.rfc-editor.org/rfc/rfc3177.txt
15. United Nations, Department of Economic and Social Affairs, Population Division. *World Population Prospects 2024.* July 2024. https://population.un.org/wpp/
16. Bianconi, E., et al. *An estimation of the number of cells in the human body.* Annals of Human Biology 40(6):463–471, 2013. https://pubmed.ncbi.nlm.nih.gov/23829164/
17. Krulwich, R. *Which Is Greater, The Number Of Sand Grains On Earth Or Stars In The Sky?* NPR, 2012 September 17. https://www.npr.org/sections/krulwich/2012/09/17/161096233/which-is-greater-the-number-of-sand-grains-on-earth-or-stars-in-the-sky
18. Postel, J. *Assigned Numbers.* RFC 790, September 1981. https://www.rfc-editor.org/rfc/rfc790.html
19. Postel, J., ed. *Internet Protocol.* RFC 791, September 1981. https://www.rfc-editor.org/rfc/rfc791.html
20. Braden, R., ed. *Requirements for Internet Hosts -- Application and Support.* RFC 1123, October 1989. https://www.rfc-editor.org/rfc/rfc1123.html
21. Kawamura, S., and Kawashima, M. *A Recommendation for IPv6 Address Text Representation.* RFC 5952, August 2010. https://www.rfc-editor.org/rfc/rfc5952.html
22. Forgie, J. W. *ST - A Proposed Internet Stream Protocol.* IEN 119, 1979 September 7. https://history.rfc-editor.org/ien/ien119.txt
23. Delgrossi, L., and Berger, L., eds. *Internet Stream Protocol Version 2 (ST2) Protocol Specification - Version ST2+.* RFC 1819, August 1995. https://www.rfc-editor.org/rfc/rfc1819.html
24. Ullmann, R. *TP/IX: The Next Internet.* RFC 1475, June 1993. https://www.rfc-editor.org/rfc/rfc1475.html
25. IESG. *Moving IP versions 5, 8, and 9 to Historic.* 2016 May 24. https://datatracker.ietf.org/doc/status-change-ip-versions-5-8-9-to-historic/
26. Pignataro, C., and Gont, F. *Formally Deprecating Some IPv4 Options.* RFC 6814, November 2012. https://www.rfc-editor.org/rfc/rfc6814.html
27. Fuller, V., Li, T., Yu, J., and Varadhan, K. *Classless Inter-Domain Routing (CIDR).* RFC 1519, September 1993. https://www.rfc-editor.org/rfc/rfc1519.txt
28. Fuller, V., and Li, T. *Classless Inter-domain Routing (CIDR): The Internet Address Assignment and Aggregation Plan.* RFC 4632, August 2006. https://www.rfc-editor.org/rfc/rfc4632.html
29. Rekhter, Y., Moskowitz, B., Karrenberg, D., de Groot, G. J., and Lear, E. *Address Allocation for Private Internets.* RFC 1918, February 1996. https://www.rfc-editor.org/rfc/rfc1918.html
30. Weil, J., Kuarsingh, V., Donley, C., Liljenstolpe, C., and Azinger, M. *IANA-Reserved IPv4 Prefix for Shared Address Space.* RFC 6598, April 2012. https://www.rfc-editor.org/rfc/rfc6598.html
31. Hinden, R., and Deering, S. *IP Version 6 Addressing Architecture.* RFC 4291, February 2006. https://www.rfc-editor.org/rfc/rfc4291.html
32. Hinden, R., and Haberman, B. *Unique Local IPv6 Unicast Addresses.* RFC 4193, October 2005. https://www.rfc-editor.org/rfc/rfc4193.html
33. Huitema, C., and Carpenter, B. *Deprecating Site Local Addresses.* RFC 3879, September 2004. https://www.rfc-editor.org/rfc/rfc3879.html
34. IANA. *IPv6 Address Space* and *IPv6 Global Unicast Address Space* registries, October 2025. https://www.iana.org/assignments/ipv6-address-space/ and https://www.iana.org/assignments/ipv6-unicast-address-assignments/
35. AFRINIC, APNIC, ARIN, LACNIC, and RIPE NCC. *Delegated address statistics, extended format,* and RDAP records, 2026 October 31. For example https://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest
36. Huston, G. *AS131072 IPv6 BGP Table Statistics.* potaroo.net, 2026 October 31. https://bgp.potaroo.net/v6/as2.0/index.html
37. Marks, P., and Sweeting, J. *2630::/16 allocated to Capital One.* arin-tech-discuss mailing list, November 2023. https://lists.arin.net/pipermail/arin-tech-discuss/2023-November/001112.html
38. Vyncke, E. *Allocating a /16 to a large enterprise ?* RIPE ipv6-wg mailing list, December 2023. https://www.ripe.net/ripe/mail/archives/ipv6-wg/2023-December/003932.html
39. Amazon Web Services. *AWS IP address ranges* (ip-ranges.json), 2026 October 31. https://ip-ranges.amazonaws.com/ip-ranges.json
40. Hinden, R., and Deering, S. *IP Version 6 Addressing Architecture.* RFC 1884, December 1995. https://www.rfc-editor.org/rfc/rfc1884.txt
41. Zhao, H. *ITU and Internet Governance.* ITU, October 2004, https://www.itu.int/ITU-T/tsb-director/itut-wsis/files/zhao-netgov01.doc; and Number Resource Organization, *Summary of NRO Response to ITU Comments on the Management of Internet Protocol (IP) Addresses,* November 2004, https://www.nro.net/summary-of-nro-response-to-itu-comments-on-the-management-of-internet-protocol-ip-addresses
42. Thomson, S. *IPv6 Address Autoconfiguration.* draft-ietf-addrconf-ipv6-auto-00, January 1995. https://www.ietf.org/archive/id/draft-ietf-addrconf-ipv6-auto-00.txt
43. Thomson, S., and Narten, T. *IPv6 Stateless Address Autoconfiguration.* RFC 1971, August 1996. https://www.rfc-editor.org/rfc/rfc1971.txt
44. Rekhter, Y., Lothberg, P., Hinden, R., Deering, S., and Postel, J. *An IPv6 Provider-Based Unicast Address Format.* RFC 2073, January 1997. https://www.rfc-editor.org/rfc/rfc2073.txt
45. O'Dell, M. *8+8 - An Alternate Addressing Architecture for IPv6.* draft-odell-8+8-00, October 1996, https://www.ietf.org/archive/id/draft-odell-8+8-00.txt; and *GSE - An Alternate Addressing Architecture for IPv6,* draft-ietf-ipngwg-gseaddr-00, February 1997, https://www.ietf.org/archive/id/draft-ietf-ipngwg-gseaddr-00.txt
46. Hinden, R., reporter. *Interim Minutes of the IPng Working Group, February 27 & 28, 1997,* and *Current Minutes of the IPng Working Group,* IETF 38 Proceedings, April 1997. https://www.ietf.org/proceedings/38/97apr-final/xrtftr47.htm
47. Carpenter, B., and Jiang, S. *Significance of IPv6 Interface Identifiers.* RFC 7136, February 2014. https://www.rfc-editor.org/rfc/rfc7136.txt
48. Huston, G. *IPv6 Prefix Lengths.* The ISP Column, April 2024, https://ispcol.potaroo.net/2024-04/ipv6-prefixes.html; also, with Ward, N., APNIC Blog, 2024 April 25, https://blog.apnic.net/2024/04/25/ipv6-prefix-lengths/
49. Carpenter, B., ed., Chown, T., Gont, F., Jiang, S., Petrescu, A., and Yourtchenko, A. *Analysis of the 64-bit Boundary in IPv6 Addressing.* RFC 7421, January 2015. https://www.rfc-editor.org/rfc/rfc7421.txt
50. Bourbaki, N. (Bush, R., Carpenter, B., Gont, F., Hilliard, N., Jaeggli, J., Huston, G., Morrow, C., and Snijders, J.). *IPv6 is Classless.* draft-bourbaki-6man-classless-ipv6-15, September 2026. https://www.ietf.org/archive/id/draft-bourbaki-6man-classless-ipv6-15.txt
51. Moore, K., ed. *Privacy Considerations for the Use of Hardware Serial Numbers in End-to-End Network Protocols.* draft-iesg-serno-privacy-00, 1999 January 26. https://www.ietf.org/archive/id/draft-iesg-serno-privacy-00.txt
52. Frezza, B. *Where's All The Outrage About The IPv6 Privacy Threat?* InternetWeek, October 1999, as reposted to the cypherpunks list, 1999 October 8. https://diswww.mit.edu/menelaus/cpunks/118779
53. Narten, T., and Draves, R. *Privacy Extensions for Stateless Address Autoconfiguration in IPv6.* RFC 3041, January 2001. https://www.rfc-editor.org/rfc/rfc3041
54. Narten, T., Draves, R., and Krishnan, S. *Privacy Extensions for Stateless Address Autoconfiguration in IPv6.* RFC 4941, September 2007. https://www.rfc-editor.org/rfc/rfc4941
55. Apple. XNU kernel source, `bsd/netinet6/nd6_rtr.c`, xnu-1699.22.73 (Mac OS X 10.7), 2011. https://github.com/apple-oss-distributions/xnu/blob/xnu-1699.22.73/bsd/netinet6/nd6_rtr.c
56. Apple. *Apple Platform Security: IPv6 security.* December 2024. https://support.apple.com/guide/security/ipv6-security-seccb625dcd9/web
57. Apple. *Use private Wi-Fi addresses on Apple devices.* December 2025. https://support.apple.com/en-us/102509
58. Gont, F., Cooper, A., Thaler, D., and Liu, W. *Recommendation on Stable IPv6 Interface Identifiers.* RFC 8064, February 2017. https://www.rfc-editor.org/rfc/rfc8064
59. Gont, F., Krishnan, S., Narten, T., and Draves, R. *Temporary Address Extensions for Stateless Address Autoconfiguration in IPv6.* RFC 8981, February 2021. https://www.rfc-editor.org/rfc/rfc8981
60. Padmanabhan, R., Rula, J. P., Richter, P., Strowes, S. D., and Dainotti, A. *DynamIPs: Analyzing address assignment practices in IPv4 and IPv6.* ACM CoNEXT, December 2020. https://www.caida.org/catalog/papers/2020_dynamips/dynamips.pdf
61. Rye, E., Beverly, R., and claffy, k. *Follow the Scent: Defeating IPv6 Prefix Rotation Privacy.* ACM Internet Measurement Conference, November 2021. https://arxiv.org/abs/2102.00542
62. Rye, E., and Beverly, R. *IPvSeeYou: Exploiting Leaked Identifiers in IPv6 for Street-Level Geolocation.* IEEE Symposium on Security and Privacy, 2023. https://arxiv.org/abs/2208.06767
63. Saidi, S. J., Gasser, O., and Smaragdakis, G. *One Bad Apple Can Spoil Your IPv6 Privacy.* ACM SIGCOMM Computer Communication Review, April 2022. https://arxiv.org/abs/2203.08946
64. Google Chrome. *IP Protection.* Explainer, November 2025. https://github.com/GoogleChrome/ip-protection
65. Konferenz der Datenschutzbeauftragten des Bundes und der Länder. *Einführung von IPv6 – Hinweise für Provider im Privatkundengeschäft und Hersteller.* Resolution of the 84th conference, 2012 November 7–8. https://www.datenschutz-bayern.de/dsbk-ent/DSK_84-IPv6.html
66. Žorž, J., Steffann, S., Dražumerič, P., Townsley, M., Alston, A., Doering, G., Palet, J., Linkova, J., Balbinot, L., Meynell, K., and Howard, L. *Best Current Operational Practice for Operators: IPv6 prefix assignment for end-users - persistent vs non-persistent, and what size to choose.* RIPE-690, October 2017. https://www.ripe.net/publications/docs/ripe-690/
67. Android Open Source Project. *Issue 32621: Support for DHCPv6 (RFC 3315).* June 2012 onward, archived copy of March 2017. https://web.archive.org/web/20170324213950/https://code.google.com/p/android/issues/detail?id=32621
68. Colitti, L., Cerf, V., Cheshire, S., and Schinazi, D. *Host Address Availability Recommendations.* RFC 7934, July 2016. https://www.rfc-editor.org/rfc/rfc7934.txt
69. Chown, T., ed., Arkko, J., Brandt, A., Troan, O., and Weil, J. *IPv6 Home Networking Architecture Principles.* RFC 7368, October 2014. https://www.rfc-editor.org/rfc/rfc7368
70. FBI Oregon. *Oregon FBI Tech Tuesday: Building A Digital Defense With Internet Of Things (IoT) Devices.* 2020 December 22. https://flashalert.net/id/FBIOregon/141073
71. Frankel, S., Graveman, R., Pearce, J., and Rooks, M. *Guidelines for the Secure Deployment of IPv6.* NIST Special Publication 800-119, December 2010. https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-119.pdf
72. Brzozowski, J., and Van de Velde, G. *Unique IPv6 Prefix per Host.* RFC 8273, December 2017. https://www.rfc-editor.org/rfc/rfc8273
73. Van de Velde, G., Hain, T., Droms, R., Carpenter, B., and Klein, E. *Local Network Protection for IPv6.* RFC 4864, May 2007. https://www.rfc-editor.org/rfc/rfc4864
74. Woodyatt, J., ed. *Recommended Simple Security Capabilities in Customer Premises Equipment (CPE) for Providing Residential IPv6 Internet Service.* RFC 6092, January 2011. https://www.rfc-editor.org/rfc/rfc6092
75. Hinden, R., O'Dell, M., and Deering, S. *An IPv6 Aggregatable Global Unicast Address Format.* RFC 2374, July 1998. https://www.rfc-editor.org/rfc/rfc2374.txt
76. Baker, F., for the IAB and IESG. *IAB/IESG Recommendations on IPv6 Address Allocations, September 1, 2000.* RIPE lir-wg mailing list, 2000 September 1. https://www.ripe.net/ripe/mail/archives/lir-wg/2000-September/002504.html
77. Huston, G. *Just how big is IPv6? - or Where did all those addresses go?* The ISP Column, July 2005. https://www.potaroo.net/ispcol/2005-07/ipv6size.html
78. ARIN Public Policy Mailing List. Thread on the proposal to amend ARIN's IPv6 assignment and utilisation requirement, September 2005: Huston, G., September 1, https://lists.arin.net/pipermail/arin-ppml/2005-September/004009.html; Conrad, D., September 8, https://lists.arin.net/pipermail/arin-ppml/2005-September/004016.html; Howard, W. L., September 8, https://lists.arin.net/pipermail/arin-ppml/2005-September/004018.html; Bush, R., September 13, https://lists.arin.net/pipermail/arin-ppml/2005-September/004024.html
79. Lindqvist, K., and Huston, G. *Proposal to Amend the IPv6 Assignment and Utilisation Requirement Policy.* RIPE policy proposal 2005-08, October 2005 to November 2007. https://www.ripe.net/community/policies/proposals/2005-08/
80. RIPE Address Policy Working Group. *Minutes from RIPE 51,* October 2005, https://www.ripe.net/community/wg/active-wg/ap/minutes/minutes-from-ripe-51/; and *Minutes from RIPE 52,* April 2006, https://www.ripe.net/community/wg/active-wg/ap/minutes/minutes-from-ripe-52/
81. ARIN. *ARIN XVI Public Policy Meeting Minutes, Day 1.* 2005 October 26. https://www.arin.net/vault/participate/meetings/reports/ARIN_XVI/ppm_minutes_day1/
82. RIPE address-policy-wg mailing list, *2005-08 New Policy Proposal,* 2005 October 5: Hovland, J., https://www.ripe.net/ripe/mail/archives/address-policy-wg/2005-October/001327.html; Doering, G., https://www.ripe.net/ripe/mail/archives/address-policy-wg/2005-October/001326.html and https://www.ripe.net/ripe/mail/archives/address-policy-wg/2005-October/001329.html
83. APNIC. *APNIC 20 Policy SIG,* draft transcript, 2005 September 8. https://conference.apnic.net/20/docs/transcripts/policy-sig.txt
84. Greco, J. *Re: v6 subnet size for DSL & leased line customers.* NANOG mailing list, 2007 December 25. https://diswww.mit.edu/charon/nanog/101263
85. APNIC, ARIN, and RIPE NCC. *IPv6 Address Allocation and Assignment Policy.* RIPE-421, November 2007. https://www.ripe.net/documents/2318/ripe-421.pdf
86. *Willy Wonka & the Chocolate Factory.* Directed by Mel Stuart, Paramount, 1971. AFI Catalog of Feature Films, archived June 2021, https://web.archive.org/web/20210617164244/https://catalog.afi.com/Catalog/moviedetails/54174; dialogue as transcribed at IMDb, archived May 2023, https://web.archive.org/web/20230503004802/https://www.imdb.com/title/tt0067992/quotes
87. Van de Velde, G., Popoviciu, C., Chown, T., Bonness, O., and Hahn, C. *IPv6 Unicast Address Assignment Considerations.* RFC 5375, December 2008. https://www.rfc-editor.org/rfc/rfc5375.txt
88. Narten, T., Huston, G., and Roberts, L. *IPv6 Address Assignment to End Sites.* RFC 6177, BCP 157, March 2011. https://www.rfc-editor.org/rfc/rfc6177.txt
89. Singh, H., Beebee, W., Donley, C., and Stark, B. *Basic Requirements for IPv6 Customer Edge Routers.* RFC 7084, November 2013. https://www.rfc-editor.org/rfc/rfc7084.txt
90. Chittimaneni, K., Chown, T., Howard, L., Kuarsingh, V., Pouffary, Y., and Vyncke, E. *Enterprise IPv6 Deployment Guidelines.* RFC 7381, October 2014. https://www.rfc-editor.org/rfc/rfc7381.txt
91. RIPE NCC. *IPv6 Address Allocation and Assignment Policy.* RIPE-738, March 2020. https://www.ripe.net/publications/docs/ripe-738/
92. ARIN, *Number Resource Policy Manual,* version 2025.1, March 2026, https://www.arin.net/participate/policy/nrpm/; APNIC, *APNIC Internet Number Resource Policies,* APNIC-127 version 015, February 2025, https://www.apnic.net/community/policy/resources/; LACNIC, *LACNIC Policy Manual,* version 2.21, https://innova.lacnic.net/innovaportal/file/680/1/manual-politicas-en-2-21.pdf; AFRINIC, *Consolidated Policy Manual,* version 1.6, November 2020, https://www.afrinic.net/consolidated-policy-manual
93. Palet Martínez, J. *IPv6 Deployment Survey (Residential/Household Services).* UKNOF 39, January 2018, https://indico.uknof.org.uk/event/41/contributions/542/attachments/712/866/bcop-ipv6-prefix-v9.pdf; and Roberts, L., and Palet Martínez, J., *IPv6 Address Assignment to End-Sites,* draft-palet-v6ops-rfc6177-bis-00, June 2018, https://www.ietf.org/archive/id/draft-palet-v6ops-rfc6177-bis-00.txt
94. Sky Group. *Sky completes roll out of IPv6 becoming the first major UK Internet provider to future-proof its service for customers.* September 2016. https://skygroup.sky/article/sky-completes-roll-out-of-ipv6-becoming-the-first-major-uk-internet-provider-to-futureproof-its-service-for-customers
95. Künzler, F. *Native IPv6 via xDSL – how to tweak your LNS.* DENOG #2, November 2010. https://www.denog.de/media/DENOG2/002-Kuenzler-Native_IPv6_via_xdsl.pdf
96. Hilbrink, T. (XS4ALL). Reply in *IPv6 residential service: What prefix, static, dynamic, extra cost ?* RIPE ipv6-wg mailing list, May 2016. https://lists.ripe.net/pipermail/ipv6-wg/2016-May/002860.html
97. Aussie Broadband. *Does Aussie Broadband support IPv6?* February 2024. https://www.aussiebroadband.com.au/help-centre/internet/does-aussie-broadband-support-ipv6/
98. Sonic. *IPv6.* Sonic Help Center, June 2026. https://help.sonic.com/hc/en-us/articles/15676915256343-IPv6
99. Arcep. *Baromètre annuel de la transition vers IPv6.* July 2025. https://www.arcep.fr/fileadmin/reprise/observatoire/ipv6/Arcep_Barometre_2025_de_la_transition_vers_IPv6.pdf
100. Comcast. *IPv6 Deployment FAQs.* March 2012, archived copy, http://web.archive.org/web/20151222044752/http://www.comcast6.net/index.php/ipv6-deployment-faq; and Brzozowski, J. J., *IPv6 Home Networking Pilot Market Deployment Technical Details,* Comcast Voices, April 2012, https://corporate.comcast.com/comcast-voices/ipv6-deployment-technology-2
101. NANOG mailing list. *Frontier IPv6 Prefix Delegation - AS5650,* August 2026: Riling, C., https://seclists.org/nanog/2026/Aug/30; Buhrmaster, G., https://seclists.org/nanog/2026/Aug/36; Beecher, T., https://seclists.org/nanog/2026/Aug/38
102. Customer reports on operators' forums: Netgate forum, *Comcast Residential /64 Delegation,* August 2021, https://forum.netgate.com/topic/165929/comcast-residential-64-delegation, and *Verizon Fios and IPV6, Which Settings Work?,* 2020–2022, https://forum.netgate.com/topic/155534/verizon-fios-and-ipv6-which-settings-work; *att-pfsense-ipv6,* GitHub, March 2022, https://github.com/jakedevans/att-pfsense-ipv6; MikroTik forum, *IPv6 NAT T-Mobile Home Internet,* February 2022, https://forum.mikrotik.com/t/ipv6-nat-t-mobile-home-internet/156194
103. Google. *IPv6.* Google Home and Nest Help, fetched October 2026. https://support.google.com/googlenest/answer/6361450?hl=en
104. BBIX. *IPv6の普及に向けて.* Presentation to the Ministry of Internal Affairs and Communications study group on IPv6, May 2012, https://www.soumu.go.jp/main_content/000161560.pdf; and Ministry of Internal Affairs and Communications, minutes of the study group's 23rd meeting, April 2013, https://www.soumu.go.jp/main_content/000228031.pdf
105. Matsuzaki, Y. (IIJ). *addressing planning on ISP – IIJ/AS2497 case.* APNIC 34, August 2012. https://conference.apnic.net/34/pdf/apnic-2012-ipv6adressing_1346149409.pdf
106. Korhonen, J., ed., et al. *IPv6 in 3rd Generation Partnership Project (3GPP) Evolved Packet System (EPS).* RFC 6459, January 2012. https://www.rfc-editor.org/rfc/rfc6459.txt
107. Byrne, C., Drown, D., and Vizdal, A. *Extending an IPv6 /64 Prefix from a Third Generation Partnership Project (3GPP) Mobile Interface to a LAN Link.* RFC 7278, June 2014. https://www.rfc-editor.org/rfc/rfc7278.txt
108. Android Open Source Project. *IPv6TetheringCoordinator.java,* android-7.1.0_r1, 2016, https://android.googlesource.com/platform/frameworks/base/+/refs/tags/android-7.1.0_r1/services/core/java/com/android/server/connectivity/tethering/IPv6TetheringCoordinator.java; and main branch, 2026, https://android.googlesource.com/platform/packages/modules/Connectivity/+/refs/heads/main/Tethering/src/com/android/networkstack/tethering/IPv6TetheringCoordinator.java
109. Després, R. *IPv6 Rapid Deployment on IPv4 Infrastructures (6rd).* RFC 5569, January 2010. https://www.rfc-editor.org/rfc/rfc5569
110. Winters, T. *DHCPv6 Prefix Delegation on IPv6 Customer Edge (CE) Routers in LANs.* RFC 9818, July 2025. https://www.rfc-editor.org/rfc/rfc9818.txt
111. Amazon Web Services. *New – IPv6 Support for EC2 Instances in Virtual Private Clouds,* December 2016, https://aws.amazon.com/blogs/aws/new-ipv6-support-for-ec2-instances-in-virtual-private-clouds/; *Amazon EC2 instance IP addressing,* https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/using-instance-addressing.html; *Prefix delegation for Amazon EC2 network interfaces,* https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/ec2-prefix-eni.html; *Amazon VPC pricing,* https://aws.amazon.com/vpc/pricing/; *Amazon Lightsail pricing,* https://aws.amazon.com/lightsail/pricing/; all fetched October 2026.
112. Google Cloud. *Configure IPv6 addresses for instances and instance templates,* https://cloud.google.com/compute/docs/ip-addresses/configure-ipv6-address; and *VPC network pricing,* https://cloud.google.com/vpc/network-pricing; October 2026.
113. Microsoft. *What is IPv6 for Azure Virtual Network?,* July 2026, https://learn.microsoft.com/en-us/azure/virtual-network/ip-services/ipv6-overview; and *Configure IP addresses for an Azure network interface,* October 2025, https://learn.microsoft.com/en-us/azure/virtual-network/ip-services/virtual-network-network-interface-addresses
114. Hosting providers' IPv6 documentation, fetched October 2026: Hetzner, https://docs.hetzner.com/cloud/servers/getting-started/connecting-to-the-server/ and https://docs.hetzner.com/general/infrastructure-and-availability/ipv4-pricing/; Scaleway, https://www.scaleway.com/en/docs/instances/how-to/use-flexips/ and https://www.scaleway.com/en/pricing/network/; netcup, https://www.netcup.com/en/blog/root-server/extended-network-configuration; Vultr, https://docs.vultr.com/configuring-ipv6-on-your-vps; TransIP, https://www.transip.eu/knowledgebase/215-how-many-ips-included-with; Akamai (Linode), https://techdocs.akamai.com/cloud-computing/docs/managing-ip-addresses-on-a-compute-instance; DigitalOcean, https://docs.digitalocean.com/products/networking/ipv6/details/limits/; UpCloud, https://upcloud.com/docs/products/networking/public-network/; Infomaniak, https://www.infomaniak.com/en/hosting/vps-cloud; OVHcloud, https://docs.ovhcloud.com/en/guides/bare-metal-cloud/virtual-private-servers/configure-ipv6 and https://docs.ovhcloud.com/en/guides/public-cloud/network-services/ipv6-configuration; Leaseweb, https://kb.leaseweb.com/kb/ips/ipv6-address-assignment-and-usage-guidelines-overview/ and https://kb.leaseweb.com/kb/ips/network-ipv6-address-assignment-and-usage-guidelines-adding-an-ipv6-address-to-your-server/; Greenhost, https://greenhost.net/infrastructure/
115. BitFolk. *New /48 assignments, October 2024.* BitFolk wiki, July 2026. https://tools.bitfolk.com/wiki/New_/48_assignments,_October_2024
116. OVHcloud Community. *IPv6 subnet/extra IPv6 IPs on cloud instances/VPS.* January 2018 to April 2026. https://community.ovhcloud.com/t/ipv6-subnet-extra-ipv6-ips-on-cloud-instances-vps/48208
117. Spamhaus. *FAQ: Combined Spam Sources (CSS).* Fetched October 2026. https://www.spamhaus.org/faqs/combined-spam-sources-css/
118. Docker. *Docker Hub usage and limits* and *Docker Hub pull usage and limits.* Fetched October 2026. https://docs.docker.com/docker-hub/usage/
119. MediaWiki. *Help:Range blocks/IPv6.* October 2026. https://www.mediawiki.org/wiki/Help:Range_blocks/IPv6
120. Lewis, J., Herrin, W., and DeLong, O. *Re: Long and unabbreviatable IPv6 addresses with random overloaded bits, vs. tunnelbroker.* NANOG mailing list, 2012 November 18. https://seclists.org/nanog/2012/Nov/344 and https://seclists.org/nanog/2012/Nov/346
121. Gashinsky, I., Jaeggli, J., and Kumari, W. *Operational Neighbor Discovery Problems.* RFC 6583, March 2012. https://www.rfc-editor.org/rfc/rfc6583
122. The Linux kernel documentation. *IP Sysctl.* Fetched October 2026. https://docs.kernel.org/networking/ip-sysctl.html
123. Huston, G. *IP Addresses through 2025.* The ISP Column, January 2026. https://www.potaroo.net/ispcol/2026-01/addr2025.html
124. Colitti, L., Linkova, J., ed., and Ma, X., ed. *Using DHCPv6 Prefix Delegation (DHCPv6-PD) to Allocate Unique IPv6 Prefixes per Client in Large Broadcast Networks.* RFC 9663, October 2024. https://www.rfc-editor.org/rfc/rfc9663.txt
125. Colitti, L., Linkova, J., Ma, X., ed., and Lamparter, D. *Using Router Advertisements to Signal the Availability of DHCPv6 Prefix Delegation to Clients.* RFC 9762, June 2025. https://www.rfc-editor.org/rfc/rfc9762
126. Colitti, L., and Rohr, P. *Simplifying advanced networking with DHCPv6 Prefix Delegation.* Android Developers Blog, September 2025. https://android-developers.googleblog.com/2025/09/simplifying-advanced-networking-with.html
127. Kuenzler, F. (Init7). *Re: Residential FTTH IPv6.* SwiNOG mailing list, December 2024. https://lists.swinog.ch/hyperkitty/list/swinog@lists.swinog.ch/message/UI363NIVOXEFUAWCQE37SYBZIMHKENP2/
128. WinFuture. *Telekom & IPv6: DSL-Zwangstrennung fällt weg.* October 2010, reporting Deutsche Telekom's statement to heise Netze. https://winfuture.de/news,58633.html
129. Number Resource Organization. *RIR Statistics Exchange Format.* https://ftp.ripe.net/pub/stats/ripencc/RIR-Statistics-Exchange-Format.txt
130. ftorrent. *Waiting for IPv6.* October 2026. https://docs.ftorrent.com/waiting-for-ipv6.html
131. ftorrent. *Open Address, Closed Device.* October 2026. https://docs.ftorrent.com/open-address-closed-device.html
