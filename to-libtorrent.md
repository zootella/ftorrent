# To libtorrent

A note for later, on a bug in how libtorrent reads the routing table on macOS, which keeps its NAT-PMP mapper from ever sending PCP over IPv6 on a Mac. We found it on 2026-10-29, the first time the Connection tab's Map turned libtorrent's mappers on, and this is what we'll confirm again and then take upstream.

## What happens

- libtorrent 2.1.1 from the maintainers' Python wheel, in the engine, on a Mac running macOS 27 with IPv4 and IPv6 on one Wi-Fi interface, `en1`.
- With `enable_natpmp` on, libtorrent starts a NAT-PMP mapper on each listen socket with a global address, one for IPv4 and one for each IPv6 address the Mac holds, nine of them that day, its stable address and its temporaries.
- The IPv4 mapper finds the gateway and sends PCP's MAP request to port 5351, then falls back to NAT-PMP if the router answers that the version is unsupported.
- Every IPv6 mapper logs that it found no default route and closes, so it sends nothing at all.
- So on a Mac, libtorrent never asks the router for an IPv6 pinhole over PCP, whatever the router speaks.
- The same code is in v2.1.1, v2.1.2, master, and RC_2_0, as of 2026-10-29, and a search of the repository's issues and pull requests found no report of it.

The port-mapping log from a session listening at port 60064, where a run of your own shows your machine's addresses in place of these:

```
NAT-PMP [192.168.1.2]: found gateway at: 192.168.1.1
NAT-PMP [192.168.1.2]: ==> port map [ mapping: 0 action: add transport: PCP proto: TCP local: 60064 external: 60064 ttl: 60 ]
NAT-PMP [2001:db8::2]: failed to find default route for "en1" 2001:db8::2
NAT-PMP [2001:db8::2]: closing
NAT-PMP [2001:db8::3]: failed to find default route for "en1" 2001:db8::3
NAT-PMP [2001:db8::3]: closing
```

The Mac has an IPv6 default route the whole time, and `route -n get -inet6 default` names it, through the router's link-local address on `en1`.

## Why

There are three problems, one behind another, all in `src/enum_net.cpp`. The first stops the route from being read at all, and the other two would stop it from being used once it is.

**The addresses in a route message are stepped through at the wrong alignment.** On macOS, `enum_routes` asks the kernel for the routing table through `sysctl` with `NET_RT_DUMP`, and `parse_route` walks the socket addresses that follow each `rt_msghdr`, rounding each one's `sa_len` up with a `ROUNDUP` macro that uses `sizeof(long)`. That's FreeBSD's alignment. macOS aligns them to four bytes, `sizeof(uint32_t)`, as Apple's own `route` and `netstat` do. On a 64-bit Mac, `long` is eight bytes, so the rounding differs whenever an address's length isn't a multiple of eight. An IPv6 address, `sockaddr_in6`, is 28 bytes, which rounds to 32 instead of 28. So in every IPv6 route the gateway that follows the destination is read four bytes late, its family comes back as 0, `valid_addr_family` fails, and `parse_route` drops the route. IPv4's `sockaddr_in` is 16 bytes, so the IPv4 default route keeps its destination and gateway. The netmask after them has an `sa_len` of 0, though, which rounds to 8 instead of 4, so the interface address after that is misread too. `get_gateway` tolerates that only because `sockaddr_to_address` returns an unspecified address for a family it doesn't know, and an unspecified source hint matches any interface address.

**A link-local gateway carries the interface number inside its address.** Read at the right alignment, the IPv6 default route's gateway comes back as `fe80:c::1`, where the address the router answers at is `fe80::1` on interface 12. macOS writes a link-local address's interface index into its second 16-bit word in routing messages, a habit inherited from the KAME IPv6 stack, and leaves `sin6_scope_id` at 0. `sockaddr_to_address` copies the address bytes as they are and the scope id as it is, so the mapper would send its request to `fe80:c::1` with no scope. The word needs clearing and the index moving to the scope id.

**The route's source is the link-local address.** The same route's interface address, `RTAX_IFA`, which `parse_route` keeps as the source hint, is the Mac's own link-local address on `en1`. `get_gateway` accepts a default route only when its source hint is unspecified or equals the interface address the mapper starts from, and no global address equals a link-local one, so every IPv6 mapper would still find no route.

The route dump, from the program at the end of this note, stepping through the addresses as libtorrent does, with your own addresses in place of these:

```
default fam 2 if en1 flags 0x40010803 addrs 0x27: dst=0.0.0.0(fam 2 len 16) gateway=192.168.1.1(fam 2 len 16) netmask=-(fam 0 len 0) ifa=-(fam 168 len 192)
default fam 30 if en1 flags 0x40010003 addrs 0x27: dst=::(fam 30 len 28) gateway=-(fam 0 len 0) netmask=-(fam 0 len 0) ifa=-(fam 8 len 254)
```

And at four-byte alignment:

```
default fam 2 if en1 flags 0x40010803 addrs 0x27: dst=0.0.0.0(fam 2 len 16) gateway=192.168.1.1(fam 2 len 16) netmask=-(fam 0 len 0) ifa=192.168.1.2(fam 2 len 16)
default fam 30 if en1 flags 0x40010003 addrs 0x27: dst=::(fam 30 len 28) gateway=fe80:c::1(fam 30 len 28) netmask=-(fam 0 len 0) ifa=fe80:c::2(fam 30 len 28)
```

The dump also lists a default route for each of the Mac's `utun` interfaces, which carry no traffic to the internet here; `get_gateway` matches on the interface name, so they don't confuse it.

## The fix

All three changes are in the `TORRENT_USE_SYSCTL` code in `src/enum_net.cpp`, for Apple's platforms:

- Round each socket address up to four bytes on Apple's platforms, and keep `sizeof(long)` on the BSDs.
- For a link-local IPv6 address read from a routing message, take the index from its second 16-bit word into the scope id and clear the word.
- In `get_gateway`, let an IPv6 default route whose source hint is link-local serve every address on the same interface.

How upstream would rather shape the second and third is the maintainer's call. Proving any of it means building libtorrent from source on a Mac, which the desktop client otherwise never does, since it runs the maintainers' wheels.

## Taking it upstream

- Confirm it again from a fresh session: the log lines, the dump, and the same results from libtorrent's own `test_enum_net` if it covers routes.
- Check whether the IPv6 mappers find a gateway on Windows and Linux, which read routes through `GetIpForwardTable2` and netlink, so the issue can say how far it reaches.
- Open an issue on arvidn/libtorrent, from zootella, with the measurement and the cause, offering a patch and leaving the design to the maintainer.
- If a patch is welcome, write it, build and test it on a Mac, and open the pull request.

The Connection tab picks the fix up only once the maintainers ship wheels with it. Until then, the Mac panel can measure the IPv6 side itself, which is a choice for the good roadmap.

## The reproducer

The program that printed the dumps above. It reads the routing table the way libtorrent does, and prints each default route with what its message carries. Change `sizeof(long)` to `sizeof(uint32_t)` in `ROUNDUP` for the four-byte reading.

```c
//dump the kernel's routing table the way libtorrent's enum_routes does on macOS, and print each default route with the addresses its message carries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ROUNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))//libtorrent's rounding

static void show(const char *label, struct sockaddr *sa) {
	char buf[64] = "-";
	if (!sa) { printf(" %s=absent", label); return; }
	if (sa->sa_family == AF_INET) inet_ntop(AF_INET, &((struct sockaddr_in*)sa)->sin_addr, buf, sizeof buf);
	else if (sa->sa_family == AF_INET6) inet_ntop(AF_INET6, &((struct sockaddr_in6*)sa)->sin6_addr, buf, sizeof buf);
	printf(" %s=%s(fam %d len %d)", label, buf, sa->sa_family, sa->sa_len);
}

int main(void) {
	int mib[6] = {CTL_NET, PF_ROUTE, 0, AF_UNSPEC, NET_RT_DUMP, 0};//the same request libtorrent makes
	size_t needed = 0;
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) { perror("sysctl"); return 1; }
	char *buf = malloc(needed);
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) { perror("sysctl"); return 1; }
	for (char *next = buf; next < buf + needed; ) {
		struct rt_msghdr *rtm = (struct rt_msghdr*)next;
		next += rtm->rtm_msglen;
		struct sockaddr *info[RTAX_MAX];
		struct sockaddr *sa = (struct sockaddr*)(rtm + 1);
		for (int i = 0; i < RTAX_MAX; i++) {//the addresses follow the header in order, each present when its bit is set in rtm_addrs
			if ((rtm->rtm_addrs & (1 << i)) == 0) { info[i] = NULL; continue; }
			info[i] = sa;
			sa = (struct sockaddr*)((char*)sa + ROUNDUP(sa->sa_len));
		}
		struct sockaddr *dst = info[RTAX_DST];
		if (!dst) continue;
		int unspecified = 0;
		if (dst->sa_family == AF_INET) unspecified = ((struct sockaddr_in*)dst)->sin_addr.s_addr == 0;
		else if (dst->sa_family == AF_INET6) unspecified = IN6_IS_ADDR_UNSPECIFIED(&((struct sockaddr_in6*)dst)->sin6_addr);
		if (!unspecified || !(rtm->rtm_flags & RTF_GATEWAY)) continue;//default routes only
		char name[IF_NAMESIZE] = "";
		if_indextoname(rtm->rtm_index, name);
		printf("default fam %d if %s flags 0x%x addrs 0x%x:", dst->sa_family, name, rtm->rtm_flags, rtm->rtm_addrs);
		show("dst", info[RTAX_DST]); show("gateway", info[RTAX_GATEWAY]); show("netmask", info[RTAX_NETMASK]); show("ifa", info[RTAX_IFA]);
		printf("\n");
	}
	return 0;
}
```

Build and run it with `clang -o routes routes.c && ./routes`. It reads the table and changes nothing, and needs no sudo.
