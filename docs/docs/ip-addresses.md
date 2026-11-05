---
title: Reading IP Addresses
description: How IPv4 and IPv6 addresses are written, what the slash, the port, and the brackets mean, and the addresses a person meets most often, from the router's own to the ranges set aside for examples.
---

# Reading IP Addresses

An address on the internet is a number, and the ways of writing it down make a small language of their own: dots and colons, base 10 and base 16, a slash, a port, a pair of square brackets. This page writes addresses out a few at a time and says what each one is, starting with the most familiar and building to the rest, so that afterward an address written any of the usual ways reads plainly. Apart from two well-known servers, the addresses that stand for public machines come from the ranges set aside for documentation, which belong to no one.

## An IPv4 address

```
192.168.1.1
```

The address of a great many home routers, typed into a browser to reach the router's settings page. It's an IPv4 address, written in dotted decimal: four numbers separated by dots, each from 0 to 255. Each number is one byte, eight bits, so the whole address is four bytes, 32 bits.

```
192.168.1.1
11000000.10101000.00000001.00000001
```

Underneath, the address is just those 32 bits, and the dots fall between the bytes. Dotted decimal is a way of writing the bits that a person can read and say aloud, and the computer only ever handles the bits themselves. Its 32 bits allow 4,294,967,296 addresses, which is the whole of IPv4.

## The IPv4 addresses you'll meet

```
192.168.1.1
192.168.1.23
10.0.0.1
172.16.0.1
```

Private addresses, the kind a home or office router hands out to the devices on its network, from the three ranges set aside for it: everything starting `10.`, everything from `172.16.` through `172.31.`, and everything starting `192.168.`. The router usually takes the first address in its range, ending in `.1`, and the devices get the others, so a router at `192.168.1.1` and a laptop at `192.168.1.23` are a typical pair. Millions of networks use these same addresses at once, and none of them can be reached from the internet at these addresses.

```
127.0.0.1
0.0.0.0
```

Two addresses that never leave the machine. `127.0.0.1` is the loopback address, the machine itself, called localhost: a program that connects there reaches another program on the same computer. `0.0.0.0` means no particular address. A program listening on `0.0.0.0` listens on every address the machine has, and a device that hasn't been given an address yet sends from it while it asks for one.

```
169.254.83.20
100.64.12.7
```

Two that say something about the network. An address starting `169.254.` is link-local: a device gave it to itself because nothing on the network answered when it asked for an address, which usually means something is wrong with the connection. An address from `100.64.` through `100.127.` is from the shared address space, the addresses a provider running carrier-grade NAT gives its customers' routers, on the stretch between the home router's translation and the provider's own.

```
1.1.1.1
8.8.8.8
192.0.2.1
198.51.100.7
203.0.113.42
```

Public addresses. The first two are well known: public DNS servers run by Cloudflare and by Google, chosen to be easy to type. The last three are from the ranges set aside for documentation, `192.0.2.`, `198.51.100.`, and `203.0.113.`, which no machine on the internet is given, so a guide can show a public address without showing anyone's. Read one of these as "a public address goes here."

## The slash

```
192.168.1.0/24
255.255.255.0
```

A slash after an address turns it into a range. The number after the slash says how many of the address's leading bits are fixed, and the rest can be anything. `/24` fixes the first 24 bits, the first three numbers, so `192.168.1.0/24` is every address from `192.168.1.0` to `192.168.1.255`, 256 of them, which is one typical home network. Network settings still show the same thing the older way, as a subnet mask: `255.255.255.0` is 24 one bits followed by 8 zero bits, a `/24`. In a range like this one, the first address names the network and the last, ending in `.255`, is its broadcast address, which reaches every device on it, so the router and the devices take the 254 between.

```
172.16.0.0      10101100.00010000.00000000.00000000
172.31.255.255  10101100.00011111.11111111.11111111
```

The slash counts bits, not numbers, so it needn't fall on a dot. The private range `172.16.0.0/12` fixes 12 bits, all of the first byte and half of the second, which is why it runs from `172.16.` through `172.31.`: in binary, the range's first and last addresses share their first 12 bits and differ in every bit after.

```
192.0.2.1/32
0.0.0.0/0
```

The two ends of the scale. `/32` fixes every bit, a range of one address. `/0` fixes none, every IPv4 address there is, which is how routing tables and firewall rules say everywhere: a computer's default route is `0.0.0.0/0`, the route for every address no narrower route covers.

## Ports

```
192.0.2.1:6881
192.0.2.1:443
```

A colon after an IPv4 address adds a port. The port is a number of its own, from 0 to 65535, written in base 10, which is 16 bits, two bytes. It picks out one program's socket at that address, so a BitTorrent client listening at `6881`, the port BitTorrent clients traditionally used, and a web server at `443` can share one machine. An address and a port together name one end of a conversation.

```
192.168.1.23:52144  →  203.0.113.42:443
```

A connection is named by both its ends. Here a laptop on a home network, at port `52144`, an ephemeral port its system picked, has connected to a web server at `443`. Every packet of the connection carries all four numbers, and a router doing NAT rewrites the first pair to its own public address and a port of its choosing before the packet leaves.

## An IPv6 address

```
2001:0db8:4a2c:0100:8d3e:22f1:9b07:5c1e
```

An IPv6 address is 128 bits, sixteen bytes, four times the length of an IPv4 address. It's written in base 16, hexadecimal, where each digit stands for four bits and runs from `0` to `9` and then `a` to `f` for ten through fifteen. Its 32 hexadecimal digits are grouped by fours, each group 16 bits, two bytes, and the eight groups are separated by colons. Four times the length is far more than four times the addresses, since each added bit doubles the count, and IPv6 has 96 more bits than IPv4.

## Shortening

```
2001:0db8:0000:0000:0000:0000:0000:0001
2001:db8:0:0:0:0:0:1
2001:db8::1
```

Three ways of writing the same address, by two rules. Zeros at the start of a group can be dropped, so `0db8` becomes `db8` and `0000` becomes `0`. And one run of groups that are all zero can be replaced by a double colon, `::`, which stands for as many zero groups as it takes to make eight. The last line is the form to expect, which RFC 5952 makes the standard one: lowercase, leading zeros dropped, and `::` taking the longest run of zeros. Uppercase digits mean the same thing, and some tools still print them.

```
2001:db8:0:0:1:0:0:1
2001:db8::1:0:0:1
```

The double colon appears once at most. Written twice, as in `2001:db8::1::1`, nothing would say how many zero groups each one stands for. When an address has two runs of zeros, `::` takes the longer one, or the first when they're the same length, and the other run stays written out. A single zero group stays `0`.

## The slash in IPv6

```
2001:db8:4a2c:100:8d3e:22f1:9b07:5c1e
2001:db8:4a2c:100::/64
```

The slash means the same thing in IPv6, a count of fixed leading bits. Each group is 16 bits, so `/64` fixes the first four groups. In a typical address that half is the network, the prefix the router announced, and the other half, the last four groups, is the device's own, which it chose itself. So the first line is one device's address and the second its whole network, written as the prefix with `::` standing for everything after.

```
2001:db8:4a2c:1200::/56
2001:db8:4a2c:1200::/64
2001:db8:4a2c:12ff::/64
```

A provider often gives a home a `/56`, eight bits shorter than one network's prefix, and the router divides it. Fifty-six bits is fourteen hexadecimal digits, so this prefix fixes `2001:db8:4a2c:12` and leaves the last two digits of the fourth group free: 256 networks of a `/64` each, from `1200` to `12ff`.

```
18,446,744,073,709,551,616
```

The number of addresses in one `/64`, 2 to the 64th power. That's 4,294,967,296 times the whole IPv4 internet, in every home network that has IPv6. No network fills it. The size is there so a device can choose its own half of the address at random and never in practice collide with a neighbor's.

```
2630::/16
2600::/12
2001:db8::/32
```

The registries and the providers write their blocks with much shorter prefixes. The address before the slash is the block's first address, every bit past the prefix set to zero, and the prefix divided by four says how many hexadecimal digits are fixed. `2630::/16` fixes four: every address that starts `2630:`, the block North America's registry gave a bank in 2023. `2600::/12` fixes three, `260`, so it runs from `2600:` through `260f:`, the block that registry was given in 2006. `2001:db8::/32` fixes eight, `2001:0db8`, the range set aside for documentation.

```
2000::/3
2000:0000:0000:0000:0000:0000:0000:0000
3fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff
```

A prefix that isn't a multiple of four ends partway through a digit. `/3` fixes the first three bits of the first digit, `001`, and leaves the fourth free, so the first digit is either `0010` or `0011`, a 2 or a 3. The block runs from the second line to the third: every address that starts with a 2 or a 3, one eighth of all IPv6, the part every public address today comes from. It's written `2000::/3` because `2000::` is its first address, and it doesn't mean only the addresses starting `2000`. Written `2000::/4`, the same address would name a block half the size, only the addresses starting with a 2.

## The IPv6 addresses you'll meet

```
::1
::
```

The loopback address and the unspecified address, IPv6's `127.0.0.1` and `0.0.0.0`. Written out, `::1` is 127 zero bits and a one, and `::` is all zeros, the shortening rule at its most extreme.

```
fe80::200:5eff:fe00:5301%en0
fe80::1%12
```

Link-local addresses, which every IPv6 interface gives itself, starting `fe80`. Every interface has one in the same range, so the part after the percent sign, the zone, names which interface is meant: a name like `en0` on a Mac or `wlan0` on Linux, and a number on Windows. A device's half with `ff:fe` in its middle, as in `5eff:fe00` here, was built from the network hardware's own address, the way many systems once made it and some still do. Routers often take a short one, like `fe80::1`.

```
2001:db8:4a2c:100:8d3e:22f1:9b07:5c1e
```

A global address, the kind that's public. Every global address handed out today starts with a `2` or a `3`, from the block `2000::/3`, so the first digit alone tells a global address from the others on this page. This one is from `2001:db8::/32`, IPv6's range for documentation, the counterpart of IPv4's three.

```
fd12:3456:789a::1
ff02::1
```

Two more that turn up now and then. An address starting `fd` is a unique local address, private to one network the way `192.168.` is, which some routers hand out beside the global ones. An address starting `ff` is multicast, a destination that reaches every device listening for it, and `ff02::1` reaches every device on the local network.

```
::ffff:192.0.2.1
```

An IPv4 address written as IPv6, which shows up in logs and in programs that handle both families through one socket. The first 80 bits are zero, the next 16 are ones, `ffff`, and the last 32 are the IPv4 address, written in its own dotted decimal, since those 32 bits read more easily that way.

## Brackets

```
[2001:db8::1]:6881
http://[2001:db8::1]:8080/
```

An IPv6 address already uses colons, so a colon can't also mark where a port starts: written as `2001:db8::1:6881`, it reads as an address whose last group is `6881`. So an IPv6 address with a port goes in square brackets, and the port follows the closing bracket. The same brackets put an IPv6 address in a web address.

## Not an IP address

```
00:00:5e:00:53:01
```

Hexadecimal with colons, and not an IP address at all: a MAC (media access control) address, the network hardware's own address on Ethernet or Wi-Fi, six bytes written as six pairs of digits, this one from the range set aside for documentation. It's used only within the local network and never travels past the router. It's also the hardware address the `fe80` example above was built from: split in half with `ff:fe` set in the middle, and one bit of its first byte flipped, which turns `00` into `02`, written `200` once the leading zero is dropped.

## Side by side

```
IPv4 address     32 bits    4 bytes   192.0.2.1
IPv6 address    128 bits   16 bytes   2001:db8::1
port             16 bits    2 bytes   6881
MAC address      48 bits    6 bytes   00:00:5e:00:53:01
```

The numbers this page has written, at their true sizes. An address and a port together come to six bytes over IPv4 and eighteen over IPv6, which is how BitTorrent trackers pack the peers they hand out, six or eighteen bytes to a peer, one after another with nothing between them.

```
4,294,967,296
340,282,366,920,938,463,463,374,607,431,768,211,456
```

Every IPv4 address there is, and every IPv6 address. The address is four times as long, and the count is the first number raised to the fourth power.
