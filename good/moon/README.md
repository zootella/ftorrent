_ftorrent/good/moon/README.md [moon.ftorrent.com](https://moon.ftorrent.com)_

# moon, the second vantage

> Prepared by [Claude Code](https://claude.ai/code) using Fable 5.1
> <br>Created: 2026-Oct
> <br>Last reviewed: 2026-Oct
> <br>Debian: 13
> <br>Caddy: 2.6
> <br>coturn: 4.6
> <br>Go: 1.24
> <br>cloud-init: 25

## What this is

good.ftorrent.com asks whether your connection is good for peer-to-peer, and some of its questions can only be answered from a second place on the internet. Whether your router gives you one public port for every destination shows only when two servers at different addresses compare what they saw. Whether a peer can reach you at all is proven only by a server that tries. moon is that second place: a small virtual server of our own at another location, with a public address in each family, running three things.

- **A STUN server**, coturn, on UDP 3478. It answers a binding request with the address and port the request arrived from, as `stun.ftorrent.com` does, so a client can ask both and compare.
- **A reflector** over HTTPS at `ip4.moon.ftorrent.com` and `ip6.moon.ftorrent.com`. It answers with the address the request came from, the same contract as `ip.ftorrent.com`, one name per family so a client can force each.
- **A poke and a dial.** On request, moon sends three small UDP packets back to the requester at a port they name, or opens a TCP connection to it with a BitTorrent handshake, and reports what happened. That proves a peer can be reached rather than inferring it.

Nothing else runs there, no account is needed to use any of it, and nothing is logged. The whole box is made from the files in this folder: a cloud-init file for the provider's dashboard, two Python scripts, and the configurations they carry. Making it from nothing takes about ten minutes, and this guide is the walkthrough. moon pairs with the rest of good.ftorrent.com, but on its own it is a working STUN server, reflector, and reachability prober, which is worth running by itself. Where it fits in the larger plan, and what comes next on it, is in the planning article on docs.ftorrent.com.

## What you need

- An account at a provider of virtual machines with a web dashboard. Its create page must take a cloud-init file as user data and an SSH key, offer a public address in each family, and hold addresses apart from instances, so a rebuild keeps them. A console, the machine's own screen and keyboard, is good to have for the night a firewall rule locks you out.
- A domain whose DNS you control, for three names: one with an A and a AAAA record, one with the A alone, one with the AAAA alone.
- A machine with ssh, rsync, and Python 3 to deploy from. Nothing else; the Go service builds on the box.
- Your names in place of ours. The files name `moon.ftorrent.com`, `ip4.moon.ftorrent.com`, and `ip6.moon.ftorrent.com`. Put your three in `Caddyfile`, your first in the realm line of `turnserver.conf`, and your box's address in `ssh.conf`, and leave everything else as it is.

## The files

- `cloud-init.yaml`: the user data that creates the box.
- `ssh.conf`: a Host block named moon: the box's address, root, the key, and a known hosts file of its own.
- `firewall.py`: installs the host firewall from `rules.v4` and `rules.v6`, and the hourly read of its STUN counters, `stuncount.py` under `stuncount.service` and `stuncount.timer`. Runs from anywhere.
- `deploy.py`: installs coturn with `turnserver.conf` and the drop-in in `coturn.service.d/`, builds and installs the Go service from `go-moon/` with `ftorrent-moon.service`, copies the page in `site/`, and puts `Caddyfile` in place. Runs from anywhere, and again after any edit.
- `site/index.html`: the page at the root of `moon.ftorrent.com`, a link to this guide.
- `README.md`: this guide.

## Standing it up

### The key

Make an ed25519 key pair for moon alone, `ssh-keygen -t ed25519 -f ~/.ssh/moon -C moon`, which writes `~/.ssh/moon` and `~/.ssh/moon.pub`. The key that opens moon then opens nothing else. `ssh.conf` names it, and every ssh here reads that file: the scripts with `ssh -F`, and you with `ssh -F good/moon/ssh.conf moon`. `HostName` there is the box's public IPv4, which you fill in once the box exists; `User` is root, which cloud-init leaves enabled with the key; `IdentitiesOnly` keeps ssh from offering every key in the agent first; and `UserKnownHostsFile` with `StrictHostKeyChecking accept-new` writes the box's host key to a file of its own on the first connection and refuses a changed one after.

### The box, in a provider's dashboard

A provider's create flow asks these, in its own order. Ours went: a page of instance categories, then a configure page, then a review page.

- **Before creating:** add the public key to the account's SSH keys, named moon. Check the dashboard's fingerprint against `ssh-keygen -lf ~/.ssh/moon.pub`, or `ssh-keygen -l -E md5 -f ~/.ssh/moon.pub` where it shows MD5.
- **Zone:** any for a first build. For a rebuild, the zone the reserved addresses are in, since an address belongs to its zone.
- **Category and size:** the development tier, and its smallest size, about 1 vCPU and 1 GB. The smallest plan is sometimes out of stock in a zone; the next size up works the same.
- **Name:** moon. Some dashboards fill in a random name; cloud-init sets the hostname either way.
- **Image:** Debian 13, the plain OS image, not an application image, even where a newer release is offered.
- **Storage:** one local disk of 10 GB, the smallest offered. The box is rebuilt from its file and never snapshotted, resized, or detached.
- **Addresses:** for a first build, a new public IPv4 and a new public IPv6, both on the box itself with no NAT. For a rebuild, choose the existing ones rather than new allocations.
- **SSH key:** moon.
- **User data:** paste `cloud-init.yaml` whole, starting with `#cloud-config`. Our dashboard put the box for it on the configure page; some keep it under advanced options or behind a switch.
- **Review:** check that the summary shows the zone, size, image, disk, and both addresses. It may not list the key or the user data.
- **After:** the box's page shows both addresses. Put the IPv4 in `ssh.conf` as `HostName`. The IPv6 it shows is the provider's suggested address, not the one DNS will name, which the next section explains. cloud-init takes a few minutes and reboots once for the upgrade.

### The names in DNS

At your DNS host, once the box's page shows its addresses, make three records:

- The first name: an A record with the IPv4, and an AAAA record with `::1` of the /64, which is the first address of the block the dashboard shows, not the longer address it suggests.
- The second name: the A record only.
- The third name: the AAAA record only.

Point the names before the first deploy. Caddy requests certificates for the three names as soon as a Caddyfile naming them is in place, and until then it serves its default page and asks for nothing.

### The first look at the box

Once cloud-init has finished, which takes a few minutes and one reboot, ssh in and check:

- `cloud-init status --long`: done, with no errors.
- `cat /etc/netplan/50-cloud-init.yaml`: the provider's network file names `ens2`, the interface the drop-in in `cloud-init.yaml` names too. If it names another, change the drop-in to match.
- `ip -6 addr show dev ens2 scope global`: two addresses, the /64's `::1` with lifetimes from the router, and the provider's.
- `caddy version`: 2.6.
- `systemctl is-enabled caddy` and `systemctl --failed`: enabled, nothing failed.
- `sshd -T | grep -E '^(passwordauthentication|permitrootlogin)'`: passwords off, and root by key only, which sshd prints as without-password.
- `unattended-upgrade --dry-run -v`: the allowed origins include the security archive, the release itself, and the updates archive from `52moon`.
- `curl -4 https://ip4.ftorrent.com` and `curl -6 https://ip6.ftorrent.com`, against `ip addr`: the reflectors see the addresses on the interface itself, so nothing translates the box's traffic, and the IPv6 they see is `::1`.

### The firewall

Run `good/moon/firewall.py`. It installs iptables-persistent, which loads `/etc/iptables/rules.v4` and `rules.v6` at every boot, copies the two files there, and loads them now. The package asks at install whether to save the live tables into those files, and the script answers no, so the files here stay the source. Before loading, the script starts a three-minute timer on the box that reopens INPUT; a fresh ssh connection after the load proves ssh still passes, and stops the timer. If that connection fails, wait three minutes and ssh again. The same run installs the hourly read of the STUN counters. To change a rule later, edit the file and run the script again. Never run `netfilter-persistent save` on the box: it would overwrite the files with the live tables.

### The deploy

Run `good/moon/deploy.py`. It installs coturn from Debian's package and restarts it under the configuration and drop-in here; installs Debian's Go, copies the Go service's source to the box, builds it there, and starts it under its unit; copies the page in `site/` to `/var/www/moon`; and last puts the Caddyfile in place and reloads Caddy, so Caddy proxies to a service already answering. Caddy then requests a certificate for each of the three names, one after another over some seconds. Every step is the same the second time, so running the script again after an edit is how a change goes up.

### Checking it from outside

From a machine elsewhere:

- `curl https://moon.ftorrent.com/`: the page, a link to this guide.
- `curl https://ip4.moon.ftorrent.com/` and `curl https://ip6.moon.ftorrent.com/`: that machine's address in each family. With `-H 'X-Forwarded-For: 192.0.2.1'` added, still that address, since Caddy ignores the forged header.
- A STUN binding request to the first name over each family: an answer of 40 bytes over IPv4 and 52 over IPv6, carrying XOR-MAPPED-ADDRESS and FINGERPRINT. coturn's own `turnutils_stunclient` sends one, and the ftorrent desktop client's Connection tab shows the whole answer in hex.
- `curl 'https://ip4.moon.ftorrent.com/poke?port=9'`: a JSON report of three packets sent to that machine's address at port 9, which it has no way to receive; the report proves the service, and a listener on a port of your choosing proves the packets.

## Rebuilding

The box holds nothing that needs saving, so a rebuild is a delete and a create, and the addresses carry over.

- In the dashboard's list of reserved addresses, detach the IPv4 and the IPv6 from the instance, one at a time. The second detach warns that the instance becomes unreachable, which is the point. Don't delete either.
- Delete the instance, with its local disk. With both addresses detached, the delete dialog has none to offer deleting.
- Create the new box as above, in the same zone, choosing the existing addresses.
- Empty `~/.ssh/moon_known_hosts`, since the new box has a new host key at the same address.
- Once the box answers ssh, wait for `cloud-init status --wait` to say done, through the one reboot it takes, then take the first look, run `firewall.py`, and run `deploy.py`.

Nothing in DNS changes, since the records name the reserved addresses and the `::1` of the kept /64. End to end it takes about ten minutes.

## How it works

### The box and its addresses

A small virtual machine: 1 vCPU, 1 GB of memory, a 10 GB local disk, a few dollars a month at most. It has a public IPv4 on the interface with no NAT, and a /64 of IPv6 delivered on-link: the provider's router advertises the prefix and finds each address by neighbor discovery. Both are reserved resources at the provider, held apart from the instance.

The box answers on the first address of its /64, `::1`. A SLAAC address has two halves: the /64 the provider's router advertises, and an interface identifier the box chooses. The provider suggests an identifier formed from the instance's MAC and hands that address to cloud-init, and its router delivers to any address in the /64, so the box is free to add others. A new instance has a new MAC, so the suggested address changes with every rebuild even when the /64 is kept. `cloud-init.yaml` writes a netplan drop-in with an address token of `::1`, so networkd forms `::1` of whatever /64 the router advertises, with no address written in any file, and the provider's own address stays on the interface beside it. The names in DNS point at `::1`, and a rebuild that keeps the /64 keeps them true.

### The software, and why no containers

Debian 13, with Caddy, coturn, and Go from Debian's own packages, each running as a systemd unit. There is no container runtime. Our main server runs its services in Docker containers because it hosts many things and the containers keep them apart; this box runs only these, the virtual machine is its isolation, and a runtime would put a layer between you and what the box exists to show: the addresses, the firewall rules, and the NATs that later work builds here by hand. Each service unit carries the hardening the main server's compose file gives its containers, written as systemd directives: an unprivileged user, a read-only system, no capabilities, memory and task limits. The sections on coturn and the Go service below say what each closes.

Debian never moves a package's major version within a release, so the versions here hold until the next release: Caddy 2.6, coturn 4.6, Go 1.24, kernel 6.12. Debian's cloud image is small, a few hundred packages with most of the 1 GB free at idle, and ships none of the storage, modem, and desktop daemons a general-purpose image carries. unattended-upgrades takes security fixes, point releases, and the updates archive each morning, with a restart at 04:00 UTC when one is needed, and Caddy renews its certificates by itself, so the box runs for weeks with no one touching it. Caddy faces the internet alone and has no rate limiting in its core, so the Go service does its own per-source limits.

### The firewall and the STUN counters

The firewall is two files, one per family. INPUT drops everything but loopback, replies to connections the box opened, ICMP and ICMPv6, DHCP replies for the IPv4 the provider assigns, ssh on 22, coturn on 3478 over UDP, and Caddy on 80, 443, and 443 over UDP for HTTP/3. Every service listens on the box itself, so INPUT is the whole firewall, and FORWARD stays closed until something is built that needs it.

A STUN server answers anyone who asks, with no account and no log of who asked, and two things follow. A count is the only way to know how much it's used, and a count keeps nothing about anyone. And anyone can forge the source of a request so that the answer lands on someone else, which makes an open STUN server a usable reflector, though a poor amplifier: the answer is about one and a half times the request on the wire. The counters measure both, and they go in before the port is even open, since anything answering on UDP 3478 is found within days of coming up.

Three rules per family sit first in the firewall, so every packet passes them before any rule decides, and each has no target, so it only counts. In INPUT, `stun4-in` counts binding requests arriving for coturn, and `stun4-excess` counts the requests above 20 a second from one source address, after a burst of 40 allowed first. In OUTPUT, `stun4-out` counts coturn's answers leaving. The IPv6 file holds the same three as `stun6-in`, `stun6-excess`, and `stun6-out`. The excess rule is the guard's own rule without its drop: a dry run that stays at zero under ordinary use, since a real client sends a request or two, and climbs only when something is using the server as a reflector. The guard itself, the same rule with a drop, waits on a week of these numbers and takes its threshold from them.

`stuncount.py` reads the six by the comment on each rule and appends a row to `/var/log/stuncount.csv` on the hour: the time, then packets and bytes for each rule in the order above. `stuncount.timer` runs it, since Debian's cloud image ships no cron and a timer is a unit like everything else here. A reboot reloads the rules with their counters at zero, so a row lower than the one before marks a reboot, and a rule missing from its chain reads -1.

### coturn

The STUN server is Debian's coturn package under the configuration in `turnserver.conf`: STUN only, UDP only, no authentication, no RFC 5780, no command-line interface, a FINGERPRINT on every answer, and the log on stdout, which the journal keeps. `deploy.py` puts the file at `/etc/turnserver.conf`, which the package's unit reads, and sets it world-readable, since rsync writes a root-owned file that the turnserver user the unit runs as couldn't otherwise open. coturn reads its configuration only at start, so an edit goes up by running `deploy.py` again.

Three lines turn off attributes this version of coturn adds to a binding answer by default: `no-stun-backward-compatibility` drops MAPPED-ADDRESS, the 2003 form of the same address; `response-origin-only-with-rfc5780` drops RESPONSE-ORIGIN, which only the NAT behavior discovery we have off would read; and `no-software-attribute` drops SOFTWARE, twenty bytes naming the server and its version. With them the answer is XOR-MAPPED-ADDRESS and FINGERPRINT alone, 40 bytes over IPv4 and 52 over IPv6, and on the wire about 1.4 times the request. Without them it was 92 bytes, and a request answered at twice its size is twice as useful to anyone reflecting traffic off the server.

The drop-in in `coturn.service.d/` gives the package's unit, which already runs coturn as its own user, the rest of the hardening: the filesystem read-only apart from coturn's state folder, where it opens its user database at start even in stun-only mode; no view of home folders, devices, or other processes; no capabilities, since 3478 is above the privileged ports; sockets of four families only, the two internet families, netlink, which getifaddrs uses to find the addresses to listen on, and Unix, which carries the readiness notice to systemd; the system calls an ordinary service uses; and limits of 128 MB, half a core, and 64 tasks. What stays open is what coturn needs to answer at all, and none of this touches the risk that matters most for an open STUN server, being used as a reflector; no sandbox closes that, and the counters above and the guard that follows them are what address it.

### The Go service

The Go service is one small program, `ftorrent-moon`, standard library only, that answers over HTTPS behind Caddy at the three names, at every path but the root of `moon.ftorrent.com`, which Caddy answers itself with the page:

```
GET https://ip4.moon.ftorrent.com/                → your IPv4, plain text; ?format=json for {"ip":"..."}
GET https://ip6.moon.ftorrent.com/                → your IPv6, the same way
GET https://ip4.moon.ftorrent.com/poke?port=N     → three UDP packets to your IPv4 at port N, and a JSON report of what left
GET https://ip4.moon.ftorrent.com/dial?port=N     → a TCP connection to your IPv4 at port N with a BitTorrent handshake, and a JSON report of what came back
```

The family follows the name asked: the same two calls at `ip6.moon.ftorrent.com` poke and dial your IPv6. `deploy.py` builds the program on the box with Debian's Go, since a program built where it runs needs no toolchain on the machine that deploys it, then installs the binary and the unit and restarts it. The unit runs it as a user systemd makes for the run and discards after it, under the same restrictions coturn's drop-in sets, without the state folder coturn needs or the netlink socket it opens. The program listens on the loopback address alone, so nothing reaches it but Caddy on the same box. Caddy terminates TLS, sets X-Forwarded-For to the address it accepted each connection from, ignoring any such header the client sent, and proxies the request.

The reflector reads the last address in that header and writes it back, as text at `/` and as JSON with `?format=json`, with the CORS header that lets a page on any origin read its own address. `/health` answers ok, and every other path is a 404.

The poke and the dial are the two endpoints that send, and one rule governs both: the box sends only to the address the request came from, never to an address the request names, so nobody can point it at anyone else. The requester names a port and nothing more, and the HTTPS request that asks is a TCP connection, whose source can't be forged the way a UDP packet's can.

The poke opens a UDP socket on an ephemeral port in the requester's family, bound to no address, so the kernel sends from the address it would use toward the requester, the box's own, and sends three packets fifty milliseconds apart to the requester's address at port N, each reading `poke <token> 1/3` and so on, a few dozen bytes, with a token the requester supplied as `&token=` or the service made up. The answer names the port the packets left from, which is what a filtering test needs to know, and the requester, listening on N, reports whether they arrived.

The dial opens a TCP connection to the requester's address at port N with a three-second limit, sends a BitTorrent handshake, the protocol's name, eight reserved bytes, an info-hash the requester supplied as `&infohash=` in forty hex digits or a random one, and a peer id beginning `-FF0000-`, ftorrent's client code and a version no client has, and reads for up to three seconds. The answer says whether the connection opened and how long that took, the port it left from, and what came back: a handshake, with the peer's info-hash and peer id, when a client that holds that torrent answered; a close with nothing, which a client without the torrent does and which still proves the connection opened; or the error, a timeout or a refusal, when nothing listened or something in between dropped it.

Both are limited per source address to ten a minute in a bucket of ten, spent by every request that reaches them, so a flood of malformed requests is as limited as a flood of good ones; the eleventh answers 429. The reflector, which sends nothing anywhere, has no limit. Nothing is logged: not the address, not the port, not the result.

From a machine behind an ordinary home router with no port mapping, the poke arrives nowhere and the dial times out over both families, since the router's NAT drops unsolicited IPv4 and its firewall drops unsolicited IPv6. That is not a fault in the service; it is the measurement, and the ftorrent desktop client's Connection tab is where it is taken and read.

### Security: what a break-in could reach

The box's security is a set of plain choices, each of which bounds what a break-in could do, and the whole is best understood by walking a break-in through them.

The box runs these services and nothing else. It holds no user data, no accounts, and no secret worth taking: the private half of its own host key, the certificate keys Caddy holds for its three names, and a sqlite file coturn creates and never fills. The ssh key that opens it lives on one machine elsewhere and opens nothing else, passwords are off, and the firewall admits ssh, HTTP, HTTPS, and STUN to the box's own programs and drops everything else before any program sees it. Debian patches the box each morning, and the box as a whole is rebuilt from the files here in ten minutes, which is also the recovery from anything that goes wrong on it.

Each service runs as its own user under a systemd unit, and the unit says what the process may touch: a read-only filesystem apart from one folder of its own, no view of other users' files, devices, or other processes, no capabilities, only the socket families it needs, only the system calls an ordinary service uses, and a cap on memory and threads. These are the kernel's own restrictions, set once in a drop-in and enforced from the process's first instruction.

The likely break-in is a bug in one of the programs that read bytes from strangers. coturn parses UDP packets from anyone on the internet, which in stun-only mode is a short code path, the binding request, and nothing of TURN. Caddy parses TLS and HTTP. The Go service parses a URL's query and sends a few packets. Someone who finds a bug in one of them and runs code inside that process gets exactly what the unit grants it and no more: the process's own user, its one writable folder, and its sockets. From inside coturn that means sending and receiving UDP as the box, a reflector or a scanner at the box's bandwidth, until someone notices; it does not reach Caddy's keys, which belong to another user, or the host key, which belongs to root, or anything that could open another machine, since nothing on the box opens one. Getting further means a second bug, in the kernel, and the system-call filter narrows what a process can even ask the kernel to do.

What the sandbox can't address is the service doing exactly what it was built to do for the wrong person. A STUN server answers whoever asks, and a request with a forged source address makes it answer someone else; that is misuse, not a break-in, and no unit directive touches it. The counters in the firewall measure it, and the guard that follows them, a per-source rate with a drop, is what limits it. The same holds for the Go service's poke and dial, which only ever send to the address a request came from over a TCP connection that can't be forged, and are rate-limited per source besides.

## Reading moon's health

Run these on the box, through `ssh -F good/moon/ssh.conf moon`:

- `uptime`, and `journalctl --list-boots` for every boot with its time: automatic restarts show here.
- `/var/log/unattended-upgrades/unattended-upgrades.log`: each morning's run, with what it installed.
- `zgrep -h " install \| upgrade " /var/log/dpkg.log*`: every package installed or upgraded, by date.
- `systemctl status caddy` and `journalctl -u caddy --since today`: whether Caddy is up, and its certificate work.
- `systemctl status coturn` and `journalctl -u coturn --since today`: whether coturn is up, and what it logged at its last start; `ss -lunp | grep turnserver` lists its sockets, UDP on 3478 at every address and no TCP.
- `systemctl status ftorrent-moon` and `journalctl -u ftorrent-moon --since today`: whether the Go service is up; `ss -ltnp | grep ftorrent-moon` shows its one socket, on the loopback address.
- `iptables -S INPUT` and `ip6tables -S INPUT`: the firewall as loaded, matching the two files.
- `iptables -nvxL INPUT` and `ip6tables -nvxL INPUT`: the counters, with the STUN rules on the first two lines of each.
- `tail /var/log/stuncount.csv` and `systemctl list-timers stuncount.timer`: the hourly rows, and when the next one comes.
- `ip -6 addr show dev ens2 scope global`: `::1` still present, with lifetimes the router's advertisements keep refreshing. It is a SLAAC address, so a day without advertisements would expire it while the provider's static address stayed.
- `echo | openssl s_client -connect moon.ftorrent.com:443 -servername moon.ftorrent.com 2>/dev/null | openssl x509 -noout -dates`, from anywhere: the certificate's dates. Caddy renews when a third of the certificate's lifetime remains.
- `df -h /` and `free -m`: disk and memory.
