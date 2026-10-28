# Windows to Mac

A letter from the Windows session to the Mac session, written 2026-10-28 during Good7 on the good roadmap, `docs/docs/good-service-planning.md`. The Good7d and Good7f boxes are checked. Nothing in `udp.rs` or `connection.js` needed a change to run here. The lines below are as Copy gives them, with two addresses rewritten in the documentation ranges and one path shortened: the router's public IPv4 address reads 203.0.113.184, the machine's temporary IPv6 address reads 2001:db8:1:a000:aaaa:bbbb:cccc:dddd, and the installed engine's path starts at %LOCALAPPDATA%.

## udp.rs on Windows

`udp.rs` compiled here with no warnings, and its two tests pass. `receive_nothing` ends on the clock through the `TimedOut` arm, which is how Windows says a read timed out. Every receive in every Check below ended the same way, at about 4,010 milliseconds, with no error. None met `ConnectionReset`, though none was likely to: every server these sockets sent to answered, so nothing was refused, and the Windows firewall drops inbound ICMP refusals by default anyway, as Good5 found.

## How the Checks ran

The first Check ran in the development build against the six rules Good5 left, where the development build's ftorrent was allowed in on Public. Both filtering rows read address-and-port-dependent with no note, which was right.

Then every rule naming ftorrent or its engine was deleted, eight in all, so the installed copy would start where a new user does. Four Checks followed in an installed copy built by `pnpm installer`, which is also the release uploaded today. Each program's security alert was caught twice, once while it stood and once after Allow access.

The alert writes two Block rules for the current profile, one for UDP and one for TCP, the moment it appears and before anyone answers. So a Check while it stands reads "a rule blocks", and the "no rule lets in" branch of `_sayHolding` never ran here. Allow access turns the two Block rules into Allow rules under the same name.

### No session, with ftorrent's alert standing

```
IPv4 filtering: address-and-port-dependent; the Windows firewall may be what held them back, since a rule blocks ftorrent coming in on Public
IPv4 inbound: unknown, no session listening
IPv6 filtering: address-and-port-dependent; the Windows firewall may be what held them back, since a rule blocks ftorrent coming in on Public
IPv6 inbound: unknown, no session listening
Firewall: Public on, the current network's profile; Domain on; Private on
Firewall rules: ftorrent block in UDP on Public, block in TCP on Public, ftorrent-engine.exe no rules
```

```
Windows firewall rules for ftorrent.exe                             25 ms  2 rules
Windows firewall rules for ftorrent-engine.exe                      24 ms  0 rules
STUN stun.ftorrent.com:3478 from port 53643, kept open              73 ms  203.0.113.184:53643, 40 bytes
GET https://ip4.moon.ftorrent.com/poke?port=53643&token=b61cb27a   446 ms  3 of 3 packets to 203.0.113.184:53643 from port 55902, carrying b61cb27a
receive on port 53643                                             4012 ms  nothing in 4 seconds
STUN moon.ftorrent.com:3478 from port 64129, kept open             108 ms  203.0.113.184:64129, 40 bytes
GET https://ip4.moon.ftorrent.com/poke?port=64129&token=b1ad23e3   403 ms  3 of 3 packets to 203.0.113.184:64129 from port 60622, carrying b1ad23e3
receive on port 64129                                             4021 ms  nothing in 4 seconds
STUN stun.ftorrent.com:3478 from port 51332, kept open              70 ms  [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:51332, 52 bytes
GET https://ip6.moon.ftorrent.com/poke?port=51332&token=2acbf9c0   407 ms  3 of 3 packets to [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:51332 from port 50343, carrying 2acbf9c0
receive on port 51332                                             4012 ms  nothing in 4 seconds
STUN moon.ftorrent.com:3478 from port 59690, kept open             111 ms  [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:59690, 52 bytes
GET https://ip6.moon.ftorrent.com/poke?port=59690&token=b29d9200   399 ms  3 of 3 packets to [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:59690 from port 52046, carrying b29d9200
receive on port 59690                                             4017 ms  nothing in 4 seconds

Firewall Public on, the current network's profile; Domain on; Private on
  block in  UDP any ports   Public                     "ftorrent.exe"; this copy of ftorrent
  block in  TCP any ports   Public                     "ftorrent.exe"; this copy of ftorrent
  no rules  %LOCALAPPDATA%\ftorrent\ftorrent-engine\ftorrent-engine.exe; this copy's engine
```

Each socket's STUN answer still arrived while the Block rules stood, since it comes back from the address and port the socket sent to.

### No session, after Allow access

```
IPv4 filtering: address-and-port-dependent
IPv6 filtering: address-and-port-dependent
Firewall rules: ftorrent allow in UDP on Public, allow in TCP on Public, ftorrent-engine.exe no rules
```

This Check took about five seconds, from the run's first line to the stick that followed it.

### A session, with the engine's alert standing

```
IPv4 filtering: address-and-port-dependent
IPv4 inbound: not reachable: nothing came back to either ping, and the dial opened no connection; the engine counted 0 connections in from moon; the Windows firewall may be what held them back, since a rule blocks ftorrent-engine.exe coming in on Public
IPv6 filtering: address-and-port-dependent
IPv6 inbound: not reachable: nothing came back to either ping, and the dial opened no connection; the engine counted 0 connections in from moon; the Windows firewall may be what held them back, since a rule blocks ftorrent-engine.exe coming in on Public
Firewall rules: ftorrent allow in UDP on Public, allow in TCP on Public, ftorrent-engine.exe block in UDP on Public, block in TCP on Public
Session: running on port 64337
```

```
GET https://ip4.moon.ftorrent.com/ping?port=64337&kind=dht        3334 ms  203.0.113.184:64337 from moon's port 33823: no answer to 3 sends in 3000 ms
GET https://ip4.moon.ftorrent.com/ping?port=64337&kind=utp        3320 ms  203.0.113.184:64337 from moon's port 47774: no answer to 3 sends in 3000 ms
GET https://ip4.moon.ftorrent.com/dial?port=64337                 3321 ms  203.0.113.184:64337: no connection in 3000 ms, dial tcp 203.0.113.184:64337: i/o timeout
GET https://ip6.moon.ftorrent.com/ping?port=64337&kind=dht        3326 ms  [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:64337 from moon's port 36039: no answer to 3 sends in 3000 ms
GET https://ip6.moon.ftorrent.com/ping?port=64337&kind=utp        3327 ms  [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:64337 from moon's port 54745: no answer to 3 sends in 3000 ms
GET https://ip6.moon.ftorrent.com/dial?port=64337                 3323 ms  [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:64337: no connection in 3000 ms, dial tcp [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:64337: i/o timeout

Firewall Public on, the current network's profile; Domain on; Private on
  allow in  UDP any ports   Public                     "ftorrent.exe"; this copy of ftorrent
  allow in  TCP any ports   Public                     "ftorrent.exe"; this copy of ftorrent
  block in  UDP any ports   Public                     "ftorrent-engine.exe"; this copy's engine
  block in  TCP any ports   Public                     "ftorrent-engine.exe"; this copy's engine
```

The filtering lines in both session Checks read like the first installed Check's: each STUN answered, moon sent three of three packets, and every receive heard nothing in four seconds.

### A session, after Allow access

```
IPv4 filtering: address-and-port-dependent
IPv4 inbound: not reachable: nothing came back to either ping, and the dial opened no connection; the engine counted 0 connections in from moon
IPv6 filtering: address-and-port-dependent
IPv6 inbound: not reachable: nothing came back to either ping, and the dial opened no connection; the engine counted 0 connections in from moon
Firewall rules: ftorrent allow in UDP on Public, allow in TCP on Public, ftorrent-engine.exe allow in UDP on Public, allow in TCP on Public
Session: running on port 64337
```

```
GET https://ip4.moon.ftorrent.com/ping?port=64337&kind=dht        3293 ms  203.0.113.184:64337 from moon's port 33860: no answer to 3 sends in 3000 ms
GET https://ip4.moon.ftorrent.com/ping?port=64337&kind=utp        3303 ms  203.0.113.184:64337 from moon's port 34675: no answer to 3 sends in 3000 ms
GET https://ip4.moon.ftorrent.com/dial?port=64337                 3292 ms  203.0.113.184:64337: no connection in 3000 ms, dial tcp 203.0.113.184:64337: i/o timeout
GET https://ip6.moon.ftorrent.com/ping?port=64337&kind=dht        3302 ms  [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:64337 from moon's port 38103: no answer to 3 sends in 3001 ms
GET https://ip6.moon.ftorrent.com/ping?port=64337&kind=utp        3297 ms  [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:64337 from moon's port 58921: no answer to 3 sends in 3000 ms
GET https://ip6.moon.ftorrent.com/dial?port=64337                 3302 ms  [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:64337: no connection in 3000 ms, dial tcp [2001:db8:1:a000:aaaa:bbbb:cccc:dddd]:64337: i/o timeout

Firewall Public on, the current network's profile; Domain on; Private on
  allow in  UDP any ports   Public                     "ftorrent.exe"; this copy of ftorrent
  allow in  TCP any ports   Public                     "ftorrent.exe"; this copy of ftorrent
  allow in  UDP any ports   Public                     "ftorrent-engine.exe"; this copy's engine
  allow in  TCP any ports   Public                     "ftorrent-engine.exe"; this copy's engine
```

After the last Check, `netsh advfirewall firewall show rule` listed the same four rules the panel did, each inbound, enabled, Allow, Public only, and any port.

## What the rows say

The firewall note was right every time. It appeared while a Block rule stood, and it was gone once Allow access replaced the rule. With both programs allowed, the router is what holds a stranger back here, over both families and both transports. On this router the Windows firewall's filtering and the router's read alike, both address-and-port-dependent, so telling the two layers apart needs a network whose router lets more in.

## What the release found

The installer takes the engine from `desktop/engine/dist`, and `pnpm installer` doesn't rebuild it. This box's engine was built on 2026-10-22, before the engine gained its `tables` command, so the Windows release of 2026-10-24 shipped an engine without it. `pnpm engine` rebuilt it before today's release, and the session's table now shows a DHT row for each of six routing tables, one for each address it listens on.

The engine's security alert shows PyInstaller's default Python icon beside `ftorrent-engine.exe`, since `ftorrent-engine.spec` sets no icon. It's the first time a new user meets the engine, and it waits for a task of its own.
