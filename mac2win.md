# Windows: four clicks to confirm after the opener left

A note for the Windows session. It's public and committed, so it names nobody: in anything written back here, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile.

## What changed

The page no longer reaches the operating system through Tauri's plugins. The opener and dialog plugins are gone from `Cargo.toml`, `lib.rs`, `package.json`, and `capabilities/default.json`, which now holds `core:default` and the five window grants and nothing else. In their place `process.rs` has `process_open`, one command that opens a file or an address with the program the system has for it, the way a double-click does, through the `open` crate that the opener plugin was a wrapper over. Its `shellexecute-on-windows` feature is on, so on Windows that is a direct `ShellExecuteEx` call rather than the PowerShell process the plugin used to start. The essay in `lib.rs` and the README's plugins section say why.

Every link the page used to open through the plugin now goes through that command: View Help in the Help menu, the two Windows Settings pages, and the home page link on the About page and the Settings page. None of it has run on Windows yet, and the Windows half of the `open` crate has never compiled here. One more Windows change rides along: the tray menu's Exit is now set apart from Show by a separator, as Steam's and Discord's are; it shows in any run.

## What to confirm

- **It builds.** `cargo check` from `src-tauri`, then `pnpm compile`. The crate's Windows path with the feature on is the part that has never compiled on this side.
- **View Help.** From the Help menu, and by F1 with the window focused, docs.ftorrent.com/help opens in the default browser and forwards to the documentation site, with no console flash. F1 was written on the Mac and has never been pressed on Windows.
- **The two Settings pages.** On the Settings page, the start-at-sign-in control's link opens Windows Settings at Startup Apps. The associations bar's link, when the bar shows, opens Default apps at ftorrent's own page, with the name that carries a query string arriving intact.
- **The home page.** The link at the foot of the About page opens ftorrent.com in the default browser.

If any of the four does nothing or opens the wrong thing, say exactly what happened. `ShellExecuteEx` prefers COM initialized on the calling thread and the crate doesn't do that, so a failure there is the first suspect; the fallback is the feature flag off in `Cargo.toml`, which takes the crate's PowerShell path, and that is a decision for the user, not a fix to make quietly.

## When it's done

A pass needs no letter back. A `win2mac.md` only if something failed or the Mac has something to do. This letter is spent either way.

---

# Windows: the Connection tab reads the machine

Added 2026-10-18 beneath the letter above, which stands on its own if it hasn't run yet. The same rules hold: say "the user," git is read-only, report results exactly as they came out, and never regenerate a lockfile.

## What changed

The Connection tab now reads the machine itself, which is Good3 in `docs/docs/good-service-planning.md`. Three general Rust commands are new:

- `net_interfaces`, in `net.rs`, lists every network interface with every address on it. On Windows it calls `GetAdaptersAddresses` with `GAA_FLAG_INCLUDE_GATEWAYS` and answers each adapter's friendly name, description, index, kind from `IfType`, operational status, and gateways, and for each unicast address its prefix length, prefix origin, suffix origin, DAD state, and valid and preferred lifetimes.
- `net_resolve`, in `net.rs`, answers every address the system resolver gives a name.
- `process_output`, in `process.rs`, runs a program and answers its exit code and what it printed, with `CREATE_NO_WINDOW`.

On a Mac and Linux the panel also runs the system's tools for address flags and gateways. On Windows it runs none, since `netsh` and `route` print in the system's language, and every fact comes from `net_interfaces`. `Cargo.toml` adds three features to the windows crate: `Win32_NetworkManagement_IpHelper`, `Win32_NetworkManagement_Ndis`, and `Win32_Networking_WinSock`.

The Windows half of `net_interfaces` was written on the Mac and has never compiled. A Windows build on the Mac stops at ring's C code and Tauri's resource compiler, before it reaches ftorrent's own code.

## What to do

- **Make it build.** Run `cargo check` from `src-tauri`, then `pnpm compile`. Fix any compile error in the Windows half of `net_interfaces` in place, keeping its shape: the same `Interface` and `Address` fields, and the same words for each enumeration's values.
- **Run Check.** Run `pnpm local`, open the Connection tab, and press Check. Set the table and the box beside `ipconfig /all` and each adapter's properties in Settings, under Network & internet:
  - every adapter `ipconfig` lists is in the box, under the name Settings shows, with its description;
  - every address `ipconfig` lists is under its adapter, with the right prefix length;
  - the IPv4 and IPv6 gateway rows match `ipconfig`'s Default Gateway;
  - the interface the box marks as the default route holds the address the Local rows show;
  - anything in the box that `ipconfig` leaves out, like the loopback, is listed in the letter back, with what the box says it is.
- **Find the rule for temporary addresses.** `ipconfig` labels each global IPv6 address "IPv6 Address" or "Temporary IPv6 Address." Windows gives both a random suffix by default, so the suffix origin alone may not tell them apart. Read each address's prefix origin, suffix origin, DAD state, and lifetimes, from `net_interfaces` or from `Get-NetIPAddress` in PowerShell, beside `ipconfig`'s label, and find which of them separates the two.
- **Read the resolver lines.** On a home network, DNS64 should read none, and the three resolve lines should each answer.

## When it's done

Write `win2mac.md` with what the panel showed against `ipconfig`, the rule for temporary addresses, and any fix made in `net.rs`. The Mac needs the rule to finish the Windows words in `desktop/src/connection.js`. That letter is public and committed, so before pasting anything into it:
- rewrite every public address into the documentation ranges: 192.0.2.0/24, 198.51.100.0/24, 203.0.113.0/24, and 2001:db8::/32;
- leave out the host name, the Physical Address lines, and the DHCP and DNS server lines;
- describe the adapters by kind, like "a Wi-Fi adapter," never by model.
