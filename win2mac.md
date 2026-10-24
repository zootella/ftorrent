# Windows to Mac

A letter from the Windows session to the Mac session, written 2026-10-24 at the end of Good5's Windows side on the good roadmap, `docs/docs/good-service-planning.md`. The Good5b box is checked and the Windows paragraph of the Good5 section is rewritten as what was built; the diff carries the rest.

## Good5b as built

The draft of `desktop/src-tauri/src/firewall.rs` compiled and read the real firewall correctly as written, the VARIANT read included. Three things changed in it. The COM apartment: a pool thread that already has an apartment in the other mode answers RPC_E_CHANGED_MODE, and the read now goes ahead in that one and leaves it alone, closing only an apartment it opened. The local ports: the firewall answers `*` for any port, not blank, so Rust passes `*` through as the firewall's own word and the panel says any ports. And the VARIANT's reference: a copy, as the draft had it, is right, because the crate's VARIANT clears itself when dropped and releases the reference the enumerator put in it; moving that reference out instead crashed on the second rule, and the essay now says so.

The module was verified before the app ran it, through a scratch crate outside the repository that compiles the repository's `firewall.rs` by path with a stand-in for Tauri's command attribute, and runs the two commands against this machine; the walk covers 651 rules in about fifteen milliseconds. Then it ran in the app, where the user pressed Check three times: twice in the development build, and once in an installed copy built by `pnpm installer` with everything in this letter in it.

## The panel beside the system

The panel's Firewall rows and section, as Copy gives them, from the installed copy's Check:

```
Firewall: Public on, the current network's profile; Domain on; Private on
Firewall rules: ftorrent allow in UDP on Public, allow in TCP on Public, ftorrent-engine.exe allow in UDP on Public, allow in TCP on Public
```

```
Windows firewall profiles                                37 ms  Domain on; Private on; Public on, current
Windows firewall rules for ftorrent.exe                  49 ms  2 rules
Windows firewall rules for ftorrent-engine.exe           49 ms  2 rules
```

```
Firewall Public on, the current network's profile; Domain on; Private on
  allow in  UDP any ports   Public                     "ftorrent.exe"; this copy of ftorrent
  allow in  TCP any ports   Public                     "ftorrent.exe"; this copy of ftorrent
  allow in  UDP any ports   Public                     "ftorrent-engine.exe"; this copy's engine
  allow in  TCP any ports   Public                     "ftorrent-engine.exe"; this copy's engine
```

The development build's Checks showed that copy's two rules, named `ftorrent` after the program's description, and no rules for its engine, with the engine's path on the no-rules line, which is where the `\\?\` prefix below first showed.

The system's side came from `netsh advfirewall`, which reads the same policy store Windows Security shows, since PowerShell's `Get-NetFirewallRule` answered access denied from the unelevated shell the session runs in. `show currentprofile` named Public, with the firewall on and the policy BlockInbound, AllowOutbound, and `show allprofiles state` had all three on. `show rule name=all verbose` held six rules naming ftorrent, every one inbound, Allow, enabled, Public only, local port Any, and edge traversal deferred to the user:

| Rule name | Program | Protocol |
|---|---|---|
| ftorrent | the development build, `desktop\src-tauri\target\debug\ftorrent.exe` | UDP |
| ftorrent | the same | TCP |
| ftorrent.exe | the installed copy, `%LOCALAPPDATA%\ftorrent\ftorrent.exe` | UDP |
| ftorrent.exe | the same | TCP |
| ftorrent-engine.exe | the installed copy's engine, `%LOCALAPPDATA%\ftorrent\ftorrent-engine\ftorrent-engine.exe` | UDP |
| ftorrent-engine.exe | the same | TCP |

The user then read the same off the console itself, Windows Defender Firewall with Advanced Security, which Windows Security's Firewall & network protection page opens through its Advanced settings link: the Overview's three profiles, each on with inbound blocked by default and Public marked active, and under Inbound Rules, sorted by name, the six rows, each Public, enabled, Allow, with the program paths in lowercase in the Program column. Both agree with the panel's rows. netsh prints each program path in lowercase too, which is how the firewall stores the path the security alert gave it, so the case-insensitive match in `rules` is what makes the walk find them. The installed copy's four are the ones in the panel's paste above, and the development build's engine has no rules, as its panel said.

## One read the user asked for here

The UPnP row now names the router. The search's description, which igd-next reads for the control paths and keeps nothing else of, also carries the root device's friendly name, maker, model name and number, and settings page, so `net_upnp_description` in `net.rs` fetches it again from the gateway the page hands back, over plain http on the local network as UPnP serves it, with a limit and a time like net_get's, and answers the text; `net_get` itself still refuses plain http. The page reads the root device's direct children with DOMParser in `_readDevice`, says the maker and model in `_sayDevice` with a word the description repeats said once, and puts them in parentheses after the router's address in the UPnP row, the way an interface's name follows an address; the box's line for the GET has the rest of the fields. It's a fourth UPnP read after the search, platform-general, and registered in `lib.rs`. The router here describes itself as `Cisco DOCSIS` in every field, so the row reads `WANIPConnection:1 at 192.168.1.1:49154 (Cisco DOCSIS)`. The Good5 section's router paragraph and the world map say so; the router's uptime still waits. This read has run only on Windows so far. Nothing in it is platform-specific, so a Check on the Mac against this same router should put `(Cisco DOCSIS)` in its UPnP row too, and that Check is the confirmation to make there.

## What Windows did that the plan didn't expect

The engine's path arrived with the `\\?\` prefix. Tauri's current_exe canonicalizes the executable's path to resolve links, which on Windows puts the verbatim prefix on, and engine.rs built the engine's path from Tauri's resource directory, so engine_status answered `\\?\C:\...`, which matches no rule, since the firewall stores plain paths. paths.rs reads `std::env::current_exe` itself, so ftorrent's own path was plain and its rules matched, which is why the first Check looked right apart from the engine's line in the box. The fix is in `paths.rs`: `display` is now `pub(crate)` and writes every path through `dunce::simplified`, and engine.rs writes the engine's path through it. dunce is a new direct dependency in `Cargo.toml`, already in the tree through Tauri at the same version, so `Cargo.lock` gained one line. The installed copy's Check above, with its engine's two rules found, is the fix at work; before it, that line would have read no rules.

The Windows firewall drops the router's refusal of port 5351. The router answers NAT-PMP and PCP with an ICMP port-unreachable, which the Mac's socket hears in milliseconds, but the one inbound ICMPv4 rule the firewall has on by default admits only type 3 code 4, fragmentation needed, so the refusal never reaches the socket, both reads say no answer after four seconds, and Check takes four seconds here where the Mac's ends in milliseconds. A Python socket sent the same two bytes to the gateway, connected and unconnected, and timed out both ways. On Windows, no answer on port 5351 is also what a refused port looks like; the Good5 section and the essay in `connection.js` say so.

A page that throws at startup leaves ftorrent invisible. The first development run's page failed on its first import, since this box's `desktop/node_modules` still held ergo 0.1.3 against the lockfile's 0.2.0, and the window never appeared: window.rs leaves showing the window to the page, and bring_forward in lifecycle.rs waits for the page to have placed it, so the tray's click and its Show did nothing while Exit still worked. `pnpm install --frozen-lockfile` brought ergo to 0.2.0 and left the lockfile unchanged, and the next run was whole. The tray's gate is working as designed; that a broken page leaves no way to see the app is noted for a later task, not changed here.

The window pass on Windows was clean: close hides the window, a left click on the tray icon brings it back, and Show from the tray's menu does the same. `pnpm compile` passes end to end with everything above in.
