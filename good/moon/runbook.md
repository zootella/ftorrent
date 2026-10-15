# moon runbook

## The goal

- `https://moon.ftorrent.com` answers with the placeholder page, "hello from ftorrent moon", on both families.
- The three names point at moon: `moon.ftorrent.com` with A and AAAA, `ip4.moon.ftorrent.com` with A only, `ip6.moon.ftorrent.com` with AAAA only.
- moon updates and restarts itself, and runs on its own for weeks: uptime, upgrades applied, reboots taken, certificates renewed.
- A default-deny host firewall admits ssh, HTTP, and HTTPS to the box's own programs, and nothing else.

Coturn, the STUN counters, and the Go service come later, under Good6 in the good service planning article.

## The box

- A small virtual machine: 1 vCPU, 1 GB of memory, a 10 GB local disk, a few dollars a month.
- A public IPv4 on the interface, with no NAT.
- A /64 of IPv6, delivered on-link: the provider's router advertises the prefix and finds each address by neighbor discovery, so an address on a separate interface, as Good8's namespaces will have, needs proxy NDP.

## The IPv6 address

moon answers on the first address of its /64, `::1`. A SLAAC address has two halves: the /64 the provider's router advertises, and an interface identifier the box chooses. The provider suggests an identifier formed from the instance's MAC and hands that address to cloud-init, and its router delivers to any address in the /64, so the box is free to add others. A new instance has a new MAC, so the suggested address changes with every rebuild even when the /64 is kept. `cloud-init.yaml` writes a netplan drop-in with an address token of `::1`, so networkd forms `::1` of whatever /64 the router advertises, with no address written in any file, and the provider's own address stays on the interface beside it. The names in DNS point at `::1`, and a rebuild that keeps the /64 keeps them true. Good8's namespaces take the next addresses in the /64, set by their script from the prefix in the host's route table and answered for by proxy NDP.

## The software, matched to the main server

Ubuntu 24.04, with Caddy and Docker from Ubuntu's own packages, the same builds as the main server: Caddy 2.6, Docker 29, Compose 2.40, containerd 2.2 and runc 1.3. Ubuntu publishes Compose, and the newest runc, to noble-updates and not to noble-security, so unattended-upgrades takes both pockets.

We planned Debian 13 first. Its own Docker is 26.1, and Docker 28 rebuilt the iptables and ip6tables rules behind port publishing, with later 28 releases changing `DOCKER-USER`, the chain the STUN counters will use. Docker 29 on Debian would have meant Docker's own repository with pins, so we chose Ubuntu.

The rule: the two boxes differ only where we can say why. They match in anything a measurement or a guide passes through. They differ in their place on the internet and in what handles web traffic. Patch levels and kernel builds may differ on a given day. Majors move together: when the main server moves, we rebuild moon from its file.

On moon, Caddy faces the internet alone and requests its own certificates. Core Caddy has no rate limiting, so the Go service will do the per-source limits.

## The key

Make an ed25519 key pair for moon alone, `ssh-keygen -t ed25519 -f ~/.ssh/moon -C moon`, which writes `~/.ssh/moon` and `~/.ssh/moon.pub`. ssh writes moon's host key to `~/.ssh/moon_known_hosts` on the first connection.

## The ssh config

`good/moon/ssh.conf` holds a Host block named moon. `deploy.py` and `firewall.py` read it with `ssh -F`, and by hand the connection is `ssh -F good/moon/ssh.conf moon`. `HostName` is moon's public IPv4, which stays with the box and is what `moon.ftorrent.com` resolves to. Put your own box's address here; get it from the box's page in your provider's dashboard. `User` is root, which cloud-init leaves enabled with the key. `IdentityFile` is the key above, and `IdentitiesOnly` keeps ssh from offering every key in the agent before it. `UserKnownHostsFile` gives moon's host key a file of its own, and `StrictHostKeyChecking accept-new` writes the key there on the first connection and refuses a changed one after.

## The files in `good/moon/`

- `cloud-init.yaml`: the user data that creates moon.
- `Caddyfile`: the three names, serving `/var/www/moon`.
- `site/index.html`: the placeholder page.
- `rules.v4` and `rules.v6`: the host firewall, which netfilter-persistent loads at boot.
- `firewall.py`: installs iptables-persistent, loads the rules, and restarts Docker, under a timer that reopens INPUT if ssh stops answering. Runs from anywhere.
- `deploy.py`: copies the Caddyfile and the site, then reloads Caddy. Runs from anywhere.
- `ssh.conf`: the Host block named moon: the box's address, root, the key, and a known hosts file of its own.
- `runbook.md`: this walkthrough.

No compose file yet; Docker is installed and idle until coturn.

## Creating the box in a dashboard

A provider's create page asks these, in its own order. Fill it this way:

- **Before creating:** add moon's public key to the account's SSH keys, named moon.
- **Region:** any.
- **Size:** the smallest, about 1 vCPU and 1 GB.
- **Name:** moon. Some dashboards fill in a random name; cloud-init sets the hostname either way.
- **Image:** Ubuntu 24.04, the main server's release, even where a newer one is offered, and the plain image, not an app.
- **Storage:** one disk of 10 GB, the cheapest kind offered: we rebuild moon from its file and never snapshot, resize or detach its disk.
- **Addresses:** a new public IPv4 and a new public IPv6, both on the box itself with no NAT. For a rebuild, the existing ones: both are reserved addresses, separate from the instance, so keep them when deleting the old box and attach them to the new one, and every record in DNS stays true.
- **SSH key:** moon's. Check the dashboard's fingerprint against `ssh-keygen -lf ~/.ssh/moon.pub`, or `ssh-keygen -l -E md5 -f ~/.ssh/moon.pub` where it shows MD5.
- **User data:** paste `cloud-init.yaml` whole, starting with `#cloud-config`. Some dashboards keep the field under advanced options or behind a switch.
- **Review:** check that the summary shows the size, image, disk and both addresses.
- **After:** the box's page shows both addresses. Put the IPv4 in `ssh.conf` as `HostName`. cloud-init takes a few minutes and may reboot once.

## Checking the new box

Once cloud-init has finished, which takes a few minutes and one reboot for the upgrade, ssh in and check:

- `cloud-init status --long`: done, with no errors.
- `caddy version`, `docker version`, `docker compose version`, `containerd --version`, and `runc --version`: the main server's majors.
- `docker info --format '{{.LoggingDriver}}'`: local.
- `apt-config dump Unattended-Upgrade::Allowed-Origins` and `apt-config dump DPkg::Options`: the updates pocket among the origins, and both dpkg options from `52moon`.
- `ip -6 addr show dev ens2 scope global`: two addresses, the /64's `::1` with lifetimes from the router, and the provider's.
- `systemctl is-enabled caddy docker` and `systemctl --failed`: both enabled, nothing failed.
- `sshd -T | grep -E '^(passwordauthentication|permitrootlogin)'`: passwords off, and root by key only, which sshd prints as without-password.
- `snap list`: none.
- `curl -4 https://ip4.ftorrent.com` and `curl -6 https://ip6.ftorrent.com`, against `ip addr`: the reflectors see the addresses on the interface itself, so nothing translates the box's traffic.

## The firewall

Once the box answers ssh, run `good/moon/firewall.py`. It installs iptables-persistent, which loads `/etc/iptables/rules.v4` and `rules.v6` at every boot, copies the two files there, loads them now, and restarts Docker, since loading a rules file flushes the chains Docker keeps for its published ports. The package asks at install whether to save the live tables into those files, and the script answers no, so the files in `good/moon/` stay the source. Installing it removes ufw, which Ubuntu's images ship inactive. Before loading, the script starts a three-minute timer on the box that reopens INPUT; a fresh ssh connection after the load proves ssh still passes, and stops the timer. If that connection fails, wait three minutes and ssh again.

The rules admit, for the box's own programs: loopback, replies to connections the box opened, ICMP and ICMPv6, DHCP replies for the IPv4 the provider assigns, ssh on 22, and Caddy on 80, 443, and 443 over UDP for HTTP/3. INPUT drops everything else. A port Docker publishes for a container never reaches INPUT: the kernel rewrites the packet to the container first and it crosses FORWARD, which Docker manages, and the STUN counting rules, Good6e, will sit in DOCKER-USER there. To change a rule, edit the file and run `firewall.py` again. Never run `netfilter-persistent save` on moon: it would overwrite the files with the live tables, Docker's chains included.

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
- `docker version` and `docker ps`: the engine's version, and nothing running until coturn.
- `iptables -S INPUT` and `ip6tables -S INPUT`: the firewall as loaded, matching the two files.
- `ip -6 addr show dev ens2 scope global`: `::1` still present, with lifetimes the router's advertisements keep refreshing. It is a SLAAC address, so a day without advertisements would expire it while the provider's static address stayed.
- `echo | openssl s_client -connect moon.ftorrent.com:443 -servername moon.ftorrent.com 2>/dev/null | openssl x509 -noout -dates`, from anywhere: the certificate's dates. Caddy renews when a third of the certificate's lifetime remains.
- `df -h /` and `free -m`: disk and memory.
