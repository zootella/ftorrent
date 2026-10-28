---
title: Names and Numbers
description: Every name and number built into the ftorrent desktop client, the servers it contacts, the trackers it writes into new torrents, the names it calls itself, and the file each one is set in.
---

# Names and Numbers

The desktop client is peer-to-peer, and it still carries a short list of fixed names: the servers it contacts, the trackers it writes into new torrents, the product's own names, and the way it introduces itself to trackers and peers. This page lists each one, the file it's set in, and what uses it.

- Renaming a fork? Start with the product's names: two files hold them, and the list under them names every file that spells one out by hand.
- Checking what the client contacts? Read the servers. Today the update check and the Connection tab are the two in use, and the tab's Start runs the engine's only libtorrent session so far.

## The product's names

| Name | Value | Set in | Used for |
|---|---|---|---|
| `brandName` | `ftorrent` | `productName` in `desktop/src-tauri/tauri.conf.json` | what people read: the window title, the tray, the menus, the installer, the Start menu shortcut, the Windows ProgIDs, the install folder, the default downloads folder |
| `brandStem` | `ftorrent` | `name` in `desktop/src-tauri/Cargo.toml` | what files carry: the executable, the engine `ftorrent-engine`, the settings file `ftorrent.toml`, the lock file, the log folder, the `.ftorrent` extension, the `ftorrent:` scheme, the published installers |
| identifier | `com.ftorrent.ftorrent` | `identifier` in `desktop/src-tauri/tauri.conf.json` | the data folder, the Mac bundle, the Windows AppUserModelID, the Flatpak |
| `urlHome` | `ftorrent.com` | `bundle.homepage` in `desktop/src-tauri/tauri.conf.json` | the About and Settings links, the update check's addresses |
| `urlHelp` | `docs.ftorrent.com/help` | `desktop/src/brand.js`, from `urlHome` | the Help menu |

The page reads the two source files when Vite builds it, in `desktop/src/brand.js`, and every other page file imports the names from there. The Rust core reads them from its package info, which Tauri fills from the same two files, and the engine receives `brandName` in the first message the app sends it. The build scripts read the two files themselves: `desktop/scripts.js` for the published names, `desktop/dmg.js` for the `.app` and the dmg, `desktop/win-setup/win-setup.js`, which compiles both names into the setup program, and `desktop/win-setup/registry.js` for the uninstall list.

- The two names are the same string here, and the code keeps them apart because a fork's may differ: a product called Candy Crush may be `candycrush` or `candy-crush` inside, and nothing derives one from the other.
- The identifier changes with the two names, and only before a fork's first release: it names the data folder and the Mac bundle, so changing it later leaves the old ones behind on every machine.
- `docs.ftorrent.com/help` is a fixed route that forwards to whichever page help lives on, set in `docs/docs/help/index.md`, so a copy that never updates still reaches current help.

These files spell a name out by hand, because nothing can read the sources there:

- `desktop/src-tauri/tauri.conf.json`: `publisher`, `copyright`, and the signing identity `ftorrent`. In `bundle.resources`, the engine's folder `ftorrent-engine`, on both sides of the entry, and the Start tile's `ftorrent.VisualElementsManifest.xml`, which Windows requires to match the executable's name, so rename the file in `desktop/src-tauri/icons/tile/` with it. The Linux desktop template `ftorrent.desktop.hbs` can keep its name, as long as the entries and the file agree.
- `desktop/src-tauri/Cargo.toml`: the library `ftorrent_lib` under `[lib]`. Leave Tauri's `mainBinaryName` unset, so the crate's name stays the executable's.
- `desktop/src-tauri/macos/Info.plist`: the type names `ftorrent File` and `ftorrent Link`, the type identifier `com.ftorrent.file`, the extension `ftorrent`, the scheme `ftorrent`, and the sentence macOS shows when the client first reaches the local network.
- `desktop/engine/ftorrent-engine.spec`: `ftorrent-engine`, in the two `name=` lines and the `output` path. Rename the file with it, and the `engine` script in `desktop/package.json` that names it, and `name` in `desktop/engine/pyproject.toml`. `engine.rs` finds the engine by the crate's name and `-engine`, so a mismatch shows only when the engine fails to start.
- `desktop/engine/engine.py`: the client code `FF`, in `fingerprint()`. Pick two letters that are in none of the tables at the end of this page.
- `desktop/icon-studio/Update-Icons.ps1`: `ftorrent.ico`, the studio's own copy of the application icon.
- `desktop/linux/build.js`, `inside-check.sh`, `inside-check-rpm.sh`, `inside-tauri.py`, and `inside-flatpak.py`: the image tags, the package name patterns, the engine's path, and the Flatpak identifier `com.ftorrent.ftorrent`.
- `desktop/release/` and `desktop/linux/release/`: the sidecars of ftorrent's published files, named for them. A fork's first `pnpm hash` writes its own beside them, and the old ones can go.

## Servers

### The update check

- `https://ftorrent.com/ftorrent.app.zip.json` on a Mac, `https://ftorrent.com/ftorrent.exe.json` on Windows: the sidecar, with the newest version's number, date, size, and hash.
- The same addresses without `.json`: the update itself, downloaded only when the user clicks to install it.
- Built in `desktop/src/update.js` and `desktop/src/stores/update.js` from `urlHome`.
- Asked when the user presses the Settings page's button. With `automatic = true` under `[update]` in `ftorrent.toml`, the default, also asked at the first startup and then at a random moment one to two days after each answer, which the client looks for at startup and every hour.
- Asked only by an installed copy on a Mac or Windows. A portable copy never asks, and neither do the Linux packages, which the package manager updates.
- Sent over HTTPS, checked against Mozilla's roots compiled into the client, following no redirect, with these headers and no others:

```
User-Agent: ureq/3.4.2
Accept: */*
Accept-Encoding: gzip
```

### The Connection tab

- `https://ip4.ftorrent.com/` and `https://ip6.ftorrent.com/`: each answers the address the request arrived from. The first name has only an IPv4 record and the second only an IPv6 record, so each request travels over one family. Sent over HTTPS, checked against Mozilla's roots compiled into the client, following no redirect, with the same three headers as the update check.
- `stun.ftorrent.com:3478`, `moon.ftorrent.com:3478`, and `stun.cloudflare.com:3478`: a STUN binding request over UDP, over each family, to all three at once from one local port, picked at random from 49152 to 65535 for each run. The request is STUN's twenty-byte header and nothing else, sent again half a second, a second and a half, and three and a half seconds after the first until an answer comes.
- `https://ip4.moon.ftorrent.com/poke` and `https://ip6.moon.ftorrent.com/poke`, for the filtering check, sent over HTTPS the way the reflector requests are. From two more local ports, picked the same way and each kept open for its own test, the client asks `stun.ftorrent.com:3478` from one and `moon.ftorrent.com:3478` from the other, then asks moon to poke the public port each answer showed, with `?port=` and a `&token=` of eight random hexadecimal digits; moon sends three small UDP packets carrying the token to the address the request came from, and the client listens on that port for four seconds. That's two pokes for each family at each Check, both at once.
- While a session runs, Check also has moon probe the session's port in each family, over HTTPS at the same two names: `/ping?port=` with `&kind=dht`, a DHT ping, and with `&kind=utp`, a uTP connection request, and `/dial?port=`, a TCP connection carrying a BitTorrent handshake, with `&infohash=` and the chosen torrent's v1 info-hash when one is open. moon sends each only to the address the request came from. With the two pokes, that's five of moon's ten requests a minute for each family at each Check.
- `ipv4only.arpa`, `ip4.ftorrent.com`, and `ip6.ftorrent.com`: resolved through the system's resolver. The first has only IPv4 addresses by definition, RFC 7050, so an IPv6 answer reveals DNS64; the other two show the single-family records the reflector checks rest on.
- The router, at the gateway address the default route names, on UDP port 5351, two requests at once. NAT-PMP's public address request, two bytes, sent again a quarter second after the first and then at doubling intervals until the router answers, whose answer is the router's external IPv4 address and the seconds since its mapping table started. And PCP's ANNOUNCE, a 24-byte header carrying this machine's own local address and asking for nothing, sent again at STUN's intervals, whose answer is a result code and the same epoch, or, from a router that speaks only NAT-PMP, that protocol's answer that the version is unsupported. A router that refuses the port speaks neither.
- `239.255.255.250:1900`, the SSDP multicast address, over UDP: UPnP's search for an Internet Gateway Device, a few lines of text. A router answers from its own address with the location of its device description, which the client fetches over plain HTTP on the local network, then asks that description's WAN connection service by SOAP for GetExternalIPAddress and for GetGenericPortMappingEntry by index until the router says the list has ended, and, where the description offers IPv6 firewall control, GetFirewallStatus.
- Built in `desktop/src/connection.js`, with ftorrent's names from `urlHome`.
- Asked only when the user presses Check on the Connection tab, each given four seconds to answer.
- Start, beside Check, opens a libtorrent session in the engine that runs until Stop, at a port picked at random from 49152 to 65535 for each Start. It joins the mainline DHT through the bootstrap nodes in the engine's table, below, and from them exchanges messages with DHT nodes across the internet over both families. With a torrent file chosen, it also announces to the trackers that torrent lists and takes connections from the peers they and the DHT name, in upload mode, which downloads nothing; for a WebSocket tracker, WebTorrent also asks the STUN server in the engine's table.

### The Windows installer

- `https://developer.microsoft.com/microsoft-edge/webview2/`, set in `desktop/win-setup/setup.c`: opened in the user's browser, with a dialog saying so, only on a Windows machine without the WebView2 runtime. The setup program fetches nothing itself.

### The engine's table

The engine holds three lists in one table, `centralized_servers` in `desktop/engine/engine.py`, and hands them to libtorrent's settings when it creates its session. Each list puts ftorrent's own server first. The Connection tab's session contacts the bootstrap nodes at every Start, and the STUN server for a torrent with a WebSocket tracker; the trackers wait for the client to create torrents.

**STUN.** WebTorrent uses STUN to learn the public address its WebRTC connections appear from. libtorrent takes one server, in `webtorrent_stun_server`, and the engine gives it the first. The engine doesn't fall back to the other two yet.

| Server | Run by |
|---|---|
| `stun.ftorrent.com:3478` | ftorrent |
| `stun.cloudflare.com:3478` | Cloudflare |
| `stun.l.google.com:19302` | Google; libtorrent's own default |

**DHT bootstrap.** A DHT routing table that starts empty learns its first nodes from these, and every session starts empty today, since the engine doesn't save its routing table yet. libtorrent takes all three, in `dht_bootstrap_nodes`, in place of its own default, which is `dht.libtorrent.org:25401` alone. On 2026-10-27 all three answered a DHT query over IPv4, and ftorrent's and libtorrent's over IPv6 as well; the Transmission project's IPv6 address didn't answer. The list serves the DHT bootstrap node at `dht.ftorrent.com` too, which starts from the same nodes less its own entry, set in `open/dht/dht.py`, so a change to one is a change to both.

| Node | Run by |
|---|---|
| `dht.ftorrent.com:51420` | ftorrent |
| `dht.libtorrent.org:25401` | libtorrent's author; libtorrent's own default |
| `dht.transmissionbt.com:6881` | the Transmission project |

**Trackers.** These are written into a torrent ftorrent creates. The client creates no torrents yet.

| Tracker | Run by |
|---|---|
| `udp://open.ftorrent.com:443/announce` | ftorrent |
| `https://open.ftorrent.com/announce` | ftorrent |
| `wss://open.ftorrent.com` | ftorrent |
| `wss://tracker.webtorrent.dev` | the WebTorrent project |
| `wss://tracker.openwebtorrent.com` | OpenWebTorrent |
| `udp://tracker.opentrackr.org:1337/announce` | not publicly named |

### The system's own

- Names resolve through the system's DNS. The client sets no resolver.
- The web view is the platform's: WebKit on macOS and Linux, WebView2 on Windows.
- The Connection tab runs the system's own tools through `process_output` in `desktop/src-tauri/src/process.rs`, for what the system's list of interfaces leaves out: on macOS `/sbin/ifconfig -L`, `/sbin/route -n get default` and its `-inet6` form, and `/usr/sbin/networksetup -listallhardwareports`, and on Linux `ip -o address` and `ip route show default` in each family. Each reads the machine and contacts no one. On Windows, `GetAdaptersAddresses` answers all of it, and no tool runs.
- The web view reaches no network itself. The content security policy in `desktop/src-tauri/tauri.conf.json` admits only the app's own files and its calls into Rust, so every request goes through `net_get` in `desktop/src-tauri/src/net.rs`. The fonts ship inside the app, in `desktop/public/fonts`.

## Addresses

- `192.0.2.1` and `2001:db8::1`, in `desktop/src/connection.js`: addresses set aside for documentation, by RFC 5737 and RFC 3849, which name no one. The Connection tab hands each to `net_local` in `desktop/src-tauri/src/net.rs`, which connects a UDP socket to it at port 9. Connecting sends nothing; the system picks a route, and the client reads the source address the system would send from.

No other address is written in the client, apart from the unspecified `0.0.0.0` and `::` that `net_local` and `net_stun` bind their sockets to, and `localhost:7100` in `desktop/src-tauri/tauri.conf.json`, where a development build finds Vite.

## What the client calls itself

- **The peer id** begins `-FF0100-`, from `fingerprint()` in `desktop/engine/engine.py`. `FF` is ftorrent's two-letter client code, the digits are the version, one per part, and random bytes for the session follow. Clients that read only the peer id look the code up in a table, and the tables are listed at the end of this page.
- **The client name** is `ftorrent/0.1.0 libtorrent/2.1.1.0`, from `client_name()` in `desktop/engine/engine.py`, built from `brandName`, the app's version, and libtorrent's. Trackers and web seeds see it as the `User-Agent`, in libtorrent's `user_agent` setting, and peers see it in the extension handshake, in `handshake_client_version`.
- **The DHT version tag**, `v`, is `LT` and libtorrent's version, which libtorrent writes into every DHT message itself, from no setting.
- **The update check** sends `ureq/3.4.2`, the HTTP library's own user agent. Nothing in the request names the copy, its version, or the user.

The engine reports its table, its client name, and its peer id prefix in its `ready` line, so the app and anyone running the engine by hand see the same values this page lists.

The tables of client codes, where `FF` isn't listed yet:

- [BEP 20](https://www.bittorrent.org/beps/bep_0020.html), the convention, with the original list
- [the community wiki's peer_id section](https://wiki.theory.org/BitTorrentSpecification#peer_id)
- [libtorrent's table](https://github.com/arvidn/libtorrent/blob/RC_2_1/src/identify_client.cpp), which qBittorrent and Deluge inherit
- [Transmission's table](https://github.com/transmission/transmission/blob/main/libtransmission/clients.cc)
- [bittorrent-peerid](https://github.com/webtorrent/bittorrent-peerid), the WebTorrent clients' table
