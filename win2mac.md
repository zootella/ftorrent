# Mac: window placement, the system font, the About page, and dark mode

A note for the Mac session. It's public and committed, so it names nobody: in anything written back, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile. Letters go by direction from now on: this one is `win2mac.md`, and a reply goes in `mac2win.md`.

## What changed

These changes landed on the Windows side, all in `desktop/`, and each has an essay where it lives. None of them adds a platform check; these are checks only a Mac can run.

**Window placement,** in `src/window.js`. A saved place goes back only onto the screen it was recorded on, found by an exact match of that screen's position, size, and scale, which `[screen]` in `ftorrent.toml` now records as `x`, `y`, `width`, `height`, and `scale` as a percent. Every position and size is in CSS pixels, which on the Mac are points. Before it's placed, the hidden window moves to the top left corner of its screen, so Tauri converts by that screen's scale. The old code found the screen under the window's middle, multiplying that point by the primary display's scale first; tao's `monitor_from_point` on macOS takes points, so on a Retina display the lookup usually missed and the window reopened in a fresh random place. A portable copy now records no place, and never opens zoomed.

**One stylesheet,** `src/style.css`, replacing the scaffold's CSS: Tailwind classes in templates, colors named by role, defaults for bare elements, and no style blocks in components. The root is `font: menu`, the CSS keyword that asks the web view for the platform's menu font, family and size together, so every Tailwind size and space measures from the system's own. It isn't `message-box`, the keyword for dialog text: WebKit answers that one with the same San Francisco at 13, but WebView2 answers it with Arial at 16 pixels.

**The About page,** `src/pages/AboutPage.vue`, lists the version of each part, the web view's through a new Rust command, `window_webview_version` in `src-tauri/src/window.rs`, and links to ftorrent.com, which the opener may now open, scoped to `https://ftorrent.com/*`. Its heading is in Jura, which the app carries in `public/fonts`.

**Dark mode,** a setting, `[appearance] mode` in `ftorrent.toml`: `"system"`, the factory value, or `"light"` or `"dark"`, chosen on the Settings page under Appearance. `themeWindow` in `src/window.js` calls Tauri's `setTheme`, with `null` for system, while the window is still hidden at startup and again whenever the setting changes. On Windows that one call turns the title bar, the menu bar, and the web view, which reports it to the page as `prefers-color-scheme`; each color in `src/style.css` holds a light and a dark value in `light-dark()`, with `color-scheme: light dark` on the root. Tauri documents the theme on macOS as app-wide, and WKWebView should report the app's appearance to the page the same way, but that side hasn't been run yet. The page's colors are matched to the Windows menu bar, white in light and `#2b2b2b` in dark, so on the Mac they sit beside macOS's own window colors rather than matching them.

## What's asked

Build and run on a Mac with a Retina display, work through the checks below, and write back in `mac2win.md` only if something differs from what's expected here.

1. **Replay on Retina.** The settings file is `~/Library/Application Support/com.ftorrent.ftorrent/ftorrent.toml`.
   - The first launch on this build gets a fresh place, since `[screen]` gains `scale`, and the 0 an older file gets for it matches no screen. Expected once.
   - Move and size the window away from the top left quarter of the screen, where the old lookup happened to work. Quit with ⌘Q and relaunch: same place, same size, and `[screen]` shows `scale = 200`.
   - Zoom it, with a double-click on the title bar or Option and the green button. Quit and relaunch: it opens zoomed, and zooming again returns it to the parked place.
   - Enter fullscreen, quit, and relaunch: it opens as the window it was before fullscreen, not in a Space of its own. That's on purpose.
   - If a second display at a different scale is at hand, park the window there, quit, and relaunch: same place. Then change a display's resolution, More Space or Larger Text in Displays settings, and relaunch: a fresh place, since the screen it was on no longer matches.
2. **The system font.** Expect San Francisco at 13 points, the same size as the menus. If WebKit gives something else, say what, and say whether the page's spacing looks right with everything measured from it.
3. **The About page.** The Web view row should show a WebKit version, libtorrent and Python should fill in once the engine is up, the heading should be in Jura, and ftorrent.com should open in the default browser.
4. **Hiding a fullscreen window.** The red button hides the window rather than closing it. With the window in fullscreen, click it: does macOS leave an empty black Space behind? Then click the Dock icon: does the window come back, and where? This one can't be checked from Windows at all, so report exactly what happens.
5. **Dark mode.** With ftorrent's Appearance on System, switch macOS between Light and Dark in System Settings, under Appearance: the title bar and the page should turn together, without a relaunch. Then set ftorrent's Appearance to Dark with macOS in Light, and to Light with macOS in Dark: the whole window should follow ftorrent, and should still after ⌘Q and a relaunch. If the page's white or `#2b2b2b` looks out of place beside macOS's own windows, say how.
