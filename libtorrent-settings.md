# libtorrent settings

The engine hands libtorrent its settings in two places in `desktop/engine/engine.py`: `session_settings`, which holds ftorrent's own choices for good, the names it gives itself, the DHT bootstrap nodes, and the STUN server, and `command_session`, which adds what the Connection tab's session needs. The only session so far is that one, and it measures how the network treats a client that listens, so a few of libtorrent's defaults are turned off to keep the measurement clean. This note keeps the list of them, so each goes back on when the session that downloads arrives.

## Off for now

- **Local service discovery**, `enable_lsd`, on by default in libtorrent.
- **UPnP**, `enable_upnp`, on by default.
- **NAT-PMP**, `enable_natpmp`, on by default.
- **Leaving upload mode**, `optimistic_disk_retry`, ten minutes by default.

Local service discovery finds peers on the same network without the internet: for each torrent, the session multicasts its info-hash and port every few minutes, and clients on the same network who have the torrent connect directly. The Connection tab measures whether strangers on the internet can reach the session, which a peer on the same network says nothing about. And in the first run on a Mac, with a socket on each of twenty addresses, the session heard its own announcement 939 times in a single poll, all at its own port from two of the Mac's own IPv6 addresses, dialed itself, and counted two of its own connections among the strangers who connected in. The session that downloads turns it back on, since a client should find a peer on the same network as readily as every other client does.

UPnP and NAT-PMP ask the router to forward the session's port, which libtorrent does on its own as soon as it listens. The Connection tab wants to see what the network does before anything asks it to change, and a mapping made on the quiet would hide the answer. Good5 reads the router without asking for anything, and Good9 asks it for mappings with the user's say-so, so the session that downloads turns these back on with what those two found in hand, by default or behind a setting.

The Connection tab adds its torrent in upload mode, which announces and takes connections but requests nothing. libtorrent treats upload mode as what a torrent falls into after a disk error, so it takes a torrent back out after `optimistic_disk_retry`, in case the disk has recovered, as long as the torrent is auto-managed, which every torrent is by default. In an early run that's what happened: ten minutes in, the session downloaded the whole torrent into the temporary folder, and every session after that found it there, checked it, and seeded it. Adding the torrent unmanaged held it in upload mode, but it also lost the start that auto-management gives it, and with it the first DHT lookup, and a thirteen-minute run did none, since the next waits for `dht_announce_interval`, fifteen minutes. So the torrent is added the usual way, and the session raises `optimistic_disk_retry` to sixty-eight years. The session that downloads puts it back to ten minutes, where it's meant to recover a torrent from a full disk.

## Decided later

- **Which addresses to listen on**, `listen_interfaces`, which the Connection tab sets to every address in both families at one port.

libtorrent reads every address as each address the machine has. In the first run on a Mac that was twenty addresses and forty sockets: loopback, link-local addresses on the tunnel and wireless interfaces, and nine global IPv6 addresses, most of them deprecated temporaries. From each of the ten that reach beyond the machine, it ran a DHT node with its own routing table, announced to the trackers, and learned the address peers see. That's what the Connection tab wants to show. The session that downloads might want fewer, the current addresses alone or the interface carrying the default route, and that choice waits for it.
