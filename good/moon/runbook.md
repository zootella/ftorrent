# moon runbook

## The goal

- `https://moon.ftorrent.com` answers with the placeholder page, "hello from ftorrent moon", on both families.
- The three names point at moon: `moon.ftorrent.com` with A and AAAA, `ip4.moon.ftorrent.com` with A only, `ip6.moon.ftorrent.com` with AAAA only.
- moon updates and restarts itself, and runs on its own for weeks: uptime, upgrades applied, reboots taken, certificates renewed.
- A default-deny host firewall admits ssh, HTTP, and HTTPS to the box's own programs, and nothing else.
- A rebuild from the files here, with the same addresses, takes about ten minutes and changes nothing in DNS.

Coturn, the STUN counters, and the Go service come later, under Good6 in the good service planning article.

## The box

- A small virtual machine: 1 vCPU, 1 GB of memory, a 10 GB local disk, a few dollars a month at most.
- A public IPv4 on the interface, with no NAT.
- A /64 of IPv6, delivered on-link: the provider's router advertises the prefix and finds each address by neighbor discovery, so an address on a separate interface, as Good8's namespaces will have, needs proxy NDP.
- Both addresses are reserved resources at the provider, held separately from the instance, so a rebuild keeps them.

## The IPv6 address

moon answers on the first address of its /64, `::1`. A SLAAC address has two halves: the /64 the provider's router advertises, and an interface identifier the box chooses. The provider suggests an identifier formed from the instance's MAC and hands that address to cloud-init, and its router delivers to any address in the /64, so the box is free to add others. A new instance has a new MAC, so the suggested address changes with every rebuild even when the /64 is kept. `cloud-init.yaml` writes a netplan drop-in with an address token of `::1`, so networkd forms `::1` of whatever /64 the router advertises, with no address written in any file, and the provider's own address stays on the interface beside it. The names in DNS point at `::1`, and a rebuild that keeps the /64 keeps them true. The drop-in names `ens2`, the interface in the provider's own netplan file; check that file on a new box and change the name if it differs. Good8's namespaces take the next addresses in the /64, set by their script from the prefix in the host's route table and answered for by proxy NDP.

## The software

Debian 13, with Caddy, coturn, and Go from Debian's own packages, each running as a systemd unit. There is no container runtime. The main server runs its services in Docker containers because it hosts many things and the containers keep them apart; moon runs only these, the virtual machine is its isolation, and a runtime would put a layer between the reader and what moon exists to show: the addresses, the firewall rules, and the two kinds of NAT that Good8 builds from namespaces and rules by hand. Each service unit carries the hardening the main server's compose file gives its containers, written as systemd directives: an unprivileged user, a read-only system, no capabilities, memory and task limits. coturn's unit, under Good6, is the first.

Debian never moves a package's major version within a release, so the versions here hold until the next release: Caddy 2.6, coturn 4.6, Go 1.24, kernel 6.12. Debian's cloud image is small, a few hundred packages with most of the 1 GB free at idle, and ships none of the storage, modem, and desktop daemons a general-purpose image carries. unattended-upgrades takes security fixes, point releases, and the updates archive each morning.

On moon, Caddy faces the internet alone and requests its own certificates. Core Caddy has no rate limiting, so the Go service will do the per-source limits.

## The key

Make an ed25519 key pair for moon alone, `ssh-keygen -t ed25519 -f ~/.ssh/moon -C moon`, which writes `~/.ssh/moon` and `~/.ssh/moon.pub`. ssh writes moon's host key to `~/.ssh/moon_known_hosts` on the first connection.

## The ssh config

`good/moon/ssh.conf` holds a Host block named moon. `deploy.py` and `firewall.py` read it with `ssh -F`, and by hand the connection is `ssh -F good/moon/ssh.conf moon`. `HostName` is moon's public IPv4, which stays with the box and is what `moon.ftorrent.com` resolves to. Put your own box's address here; get it from the box's page in your provider's dashboard. `User` is root, which cloud-init leaves enabled with the key. `IdentityFile` is the key above, and `IdentitiesOnly` keeps ssh from offering every key in the agent before it. `UserKnownHostsFile` gives moon's host key a file of its own, and `StrictHostKeyChecking accept-new` writes the key there on the first connection and refuses a changed one after. A rebuilt box has a new host key at the same address, so empty that file before the first connection to it.

## The files in `good/moon/`

- `cloud-init.yaml`: the user data that creates moon.
- `Caddyfile`: the three names, serving `/var/www/moon`.
- `site/index.html`: the placeholder page.
- `rules.v4` and `rules.v6`: the host firewall, which netfilter-persistent loads at boot.
- `firewall.py`: installs iptables-persistent and loads the rules, under a timer that reopens INPUT if ssh stops answering. Runs from anywhere.
- `deploy.py`: copies the Caddyfile and the site, then reloads Caddy. Runs from anywhere.
- `ssh.conf`: the Host block named moon: the box's address, root, the key, and a known hosts file of its own.
- `runbook.md`: this walkthrough.

## Creating the box in a dashboard

A provider's create flow asks these, in its own order. Ours went: a page of instance categories, then a configure page, then a review page. Fill it this way:

- **Before creating:** add moon's public key to the account's SSH keys, named moon. Check the dashboard's fingerprint against `ssh-keygen -lf ~/.ssh/moon.pub`, or `ssh-keygen -l -E md5 -f ~/.ssh/moon.pub` where it shows MD5.
- **Zone:** any for a first build. For a rebuild, the zone the reserved addresses are in, since an address belongs to its zone.
- **Category and size:** the development tier, and its smallest size, about 1 vCPU and 1 GB. The smallest plan is sometimes out of stock in a zone; the next size up works the same.
- **Name:** moon. Some dashboards fill in a random name; cloud-init sets the hostname either way.
- **Image:** Debian 13, the plain OS image, not an application image, even where a newer release is offered.
- **Storage:** one local disk of 10 GB, the smallest offered: we rebuild moon from its file and never snapshot, resize or detach its disk.
- **Addresses:** for a first build, a new public IPv4 and a new public IPv6, both on the box itself with no NAT. For a rebuild, choose the existing ones rather than new allocations.
- **SSH key:** moon.
- **User data:** paste `cloud-init.yaml` whole, starting with `#cloud-config`. Our dashboard put the box for it on the configure page; some keep it under advanced options or behind a switch.
- **Review:** check that the summary shows the zone, size, image, disk, and both addresses. It may not list the key or the user data.
- **After:** the box's page shows both addresses. Put the IPv4 in `ssh.conf` as `HostName`. The IPv6 it shows is the provider's suggested address, not the `::1` in DNS. cloud-init takes a few minutes and reboots once for the upgrade.

## Rebuilding the box

moon runs nothing that needs saving, so a rebuild is a delete and a create, and the addresses carry over.

- In the dashboard's list of reserved addresses, detach the IPv4 and the IPv6 from the instance, one at a time. The second detach warns that the instance becomes unreachable, which is the point. Don't delete either.
- Delete the instance, with its local disk. With both addresses detached, the delete dialog has none to offer deleting.
- Create the new box as above, in the same zone, choosing the existing addresses.
- Empty `~/.ssh/moon_known_hosts`, since the new box has a new host key at the same address.
- Once the box answers ssh, wait for `cloud-init status --wait` to say done, through the one reboot it takes, then run the checks below, `firewall.py`, and `deploy.py`. Caddy requests a new certificate as the Caddyfile lands, and the page answers within a minute.

Nothing in DNS changes, since the records name the reserved addresses and the `::1` of the kept /64.

## Checking the new box

Once cloud-init has finished, which takes a few minutes and one reboot for the upgrade, ssh in and check:

- `cloud-init status --long`: done, with no errors.
- `cat /etc/netplan/50-cloud-init.yaml`: the provider's network file names `ens2`, the interface our drop-in names too.
- `ip -6 addr show dev ens2 scope global`: two addresses, the /64's `::1` with lifetimes from the router, and the provider's.
- `caddy version`: 2.6.
- `systemctl is-enabled caddy` and `systemctl --failed`: enabled, nothing failed.
- `sshd -T | grep -E '^(passwordauthentication|permitrootlogin)'`: passwords off, and root by key only, which sshd prints as without-password.
- `unattended-upgrade --dry-run -v`: the allowed origins include the security archive, the release itself, and the updates archive from `52moon`.
- `curl -4 https://ip4.ftorrent.com` and `curl -6 https://ip6.ftorrent.com`, against `ip addr`: the reflectors see the addresses on the interface itself, so nothing translates the box's traffic, and the IPv6 they see is `::1`.

## The firewall

Once the box answers ssh, run `good/moon/firewall.py`. It installs iptables-persistent, which loads `/etc/iptables/rules.v4` and `rules.v6` at every boot, copies the two files there, and loads them now. The package asks at install whether to save the live tables into those files, and the script answers no, so the files in `good/moon/` stay the source. Before loading, the script starts a three-minute timer on the box that reopens INPUT; a fresh ssh connection after the load proves ssh still passes, and stops the timer. If that connection fails, wait three minutes and ssh again.

The rules admit, for the box's own programs: loopback, replies to connections the box opened, ICMP and ICMPv6, DHCP replies for the IPv4 the provider assigns, ssh on 22, and Caddy on 80, 443, and 443 over UDP for HTTP/3. INPUT drops everything else. Every service on moon listens on the box itself, so INPUT is the whole firewall until Good8's namespaces add FORWARD rules of their own, and the STUN counting rules, Good6e, sit in INPUT and OUTPUT. To change a rule, edit the file and run `firewall.py` again. Never run `netfilter-persistent save` on moon: it would overwrite the files with the live tables.

## The names in DNS

At the domain's DNS host, once the box's page shows its addresses, make these records:

- `moon.ftorrent.com`: an A record with the IPv4 and an AAAA record with `::1` of the /64, not the IPv6 the dashboard shows, which is the provider's suggested address.
- `ip4.moon.ftorrent.com`: the A record only.
- `ip6.moon.ftorrent.com`: the AAAA record only.

If the names already exist, parked at documentation addresses, change the addresses. Caddy requests certificates for the three names as soon as a Caddyfile naming them is in place, so point the names before the first deploy; until then Caddy serves its default page and asks for nothing.

## The first deploy

Once the three names resolve to moon from outside, run `good/moon/deploy.py`. Caddy requests certificates for the three names from Let's Encrypt, and `https://moon.ftorrent.com` answers with the placeholder page on both families. From here moon runs on its own: upgrades each morning, a restart at 04:00 UTC when one needs it, and certificates renewed by Caddy.

## Reading moon's health

Run these on the box, through `ssh -F good/moon/ssh.conf moon`:

- `uptime`, and `journalctl --list-boots` for every boot with its time: automatic restarts show here.
- `/var/log/unattended-upgrades/unattended-upgrades.log`: each morning's run, with what it installed.
- `zgrep -h " install \| upgrade " /var/log/dpkg.log*`: every package installed or upgraded, by date.
- `systemctl status caddy` and `journalctl -u caddy --since today`: whether Caddy is up, and its certificate work.
- `iptables -S INPUT` and `ip6tables -S INPUT`: the firewall as loaded, matching the two files.
- `ip -6 addr show dev ens2 scope global`: `::1` still present, with lifetimes the router's advertisements keep refreshing. It is a SLAAC address, so a day without advertisements would expire it while the provider's static address stayed.
- `echo | openssl s_client -connect moon.ftorrent.com:443 -servername moon.ftorrent.com 2>/dev/null | openssl x509 -noout -dates`, from anywhere: the certificate's dates. Caddy renews when a third of the certificate's lifetime remains.
- `df -h /` and `free -m`: disk and memory.
