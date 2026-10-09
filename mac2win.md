# Windows: four clicks to confirm after the opener left

A note for the Windows session. It's public and committed, so it names nobody: in anything written back here, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile.

## What changed

The page no longer reaches the operating system through Tauri's plugins. The opener and dialog plugins are gone from `Cargo.toml`, `lib.rs`, `package.json`, and `capabilities/default.json`, which now holds `core:default` and the five window grants and nothing else. In their place `process.rs` has `process_open`, one command that opens a file or an address with the program the system has for it, the way a double-click does, through the `open` crate that the opener plugin was a wrapper over. Its `shellexecute-on-windows` feature is on, so on Windows that is a direct `ShellExecuteEx` call rather than the PowerShell process the plugin used to start. The essay in `lib.rs` and the README's plugins section say why.

Every link the page used to open through the plugin now goes through that command: View Help in the Help menu, the two Windows Settings pages, and the home page link on the About page and the Settings page. None of it has run on Windows yet, and the Windows half of the `open` crate has never compiled here.

## What to confirm

- **It builds.** `cargo check` from `src-tauri`, then `pnpm compile`. The crate's Windows path with the feature on is the part that has never compiled on this side.
- **View Help.** From the Help menu, and by F1 with the window focused, docs.ftorrent.com/help opens in the default browser and forwards to the documentation site, with no console flash. F1 was written on the Mac and has never been pressed on Windows.
- **The two Settings pages.** On the Settings page, the start-at-sign-in control's link opens Windows Settings at Startup Apps. The associations bar's link, when the bar shows, opens Default apps at ftorrent's own page, with the name that carries a query string arriving intact.
- **The home page.** The link at the foot of the About page opens ftorrent.com in the default browser.

If any of the four does nothing or opens the wrong thing, say exactly what happened. `ShellExecuteEx` prefers COM initialized on the calling thread and the crate doesn't do that, so a failure there is the first suspect; the fallback is the feature flag off in `Cargo.toml`, which takes the crate's PowerShell path, and that is a decision for the user, not a fix to make quietly.

## When it's done

A pass needs no letter back. A `win2mac.md` only if something failed or the Mac has something to do. This letter is spent either way.
