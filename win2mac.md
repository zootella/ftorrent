# Mac: the one-click update is built on Windows, and three checks are yours

A note for the Mac session. It's public and committed, so it names nobody: say "the user." Git is read-only on both boxes, and the user makes every commit.

## What's built

The Windows half of the one-click update is built, tested, and recorded. The answer to the question `mac2win.md` left open is that the setup program is the whole installer on Windows: ftorrent downloads the same `ftorrent.exe` a person downloads, checks its hash, and starts it with no arguments, and from there it is the install a double-click gets. Nothing was added to `win-setup/setup.c` for it. The only code change is in `updateInstallable` in `desktop/src/update.js`, which on Windows answers yes for the installed copy; the essay at the top of that file now has a Windows section above the Mac's, in the same shape, with every path. The research that decided it is folded into that essay and into the planning doc's section, at a sentence each, and `update.md` is spent and deleted; what it still owed is the three checks at the end of this letter.

The test ran on 2026-10-08, 0.1.0 installed to 0.1.1 published, with the log on: 1.75 seconds from setup starting to the new page, no prompt, no console, nothing in Windows Security's history, the shortcut rewritten, the login entry intact. Afterward everything went back to 0.1.0, the repo, the installed copy, and the upload, so the Windows sidecar is dated 2026-10-08 with a new hash.

## What changed in files you share

- `desktop/src-tauri/src/process.rs`: `process_start` on Windows sets only `DETACHED_PROCESS`, since Windows ignores `CREATE_NO_WINDOW` beside it; the comment says so. It gains `process_id`, a plain command answering this process's id, registered in `lib.rs`.
- `desktop/src/pages/AboutPage.vue`: the ftorrent row and the libtorrent row now show a process id after the version, this copy's and the engine's, from `process_id` and `engine_status`.
- `docs/docs/desktop-client-planning.md`: the section is One-Click Update, and the feature is called that, or just the update, everywhere; never automatic update, since the user clicks once and nothing downloads before that. Its Windows paragraph and smoke test say what Windows does and doesn't do: every copy that checks can install there, so the ftorrent.com link never shows on Windows, and a write setup can't finish ends in its dialog rather than the old copy coming back.
- `docs/docs/names-and-numbers.md`: the Windows installer line now describes setup.c, which fetches nothing, and the update section says the click downloads with the same headers.
- `docs/docs/installing-ftorrent.md`: one sentence per platform saying an update brings no prompt back, and why.

## What's yours

Three checks from the update work can only run on a Mac. Run or drop each as you judge:

- **A standard account.** The immutable-flag run stood in for the same refusal; this would exercise `disk_access` answering that the user can't write `/Applications`, so the button stays away and the link shows.
- **The copy run from the disk image before dragging.** The startup check turns it away by path.
- **Sleep and wake.** A laptop waking past its `next` should check on the next hourly look.

That's the whole list, and this letter is the only place it's written down.

## The Mac's About

A separate thing for the Mac to clean up. The ftorrent menu's About item opens a small window of its own, with the name, version, and copyright, which is AppKit's standard About panel: Tauri builds a default menu on macOS alone, only when the app sets none, and that menu's About is a predefined item that opens the panel. Windows sets its own menu in `lifecycle.rs`, with Help, About routing into the page, and Tauri adds no default there or on Linux, so this is the Mac's alone. The user wants the path a Mac user expects, About ftorrent at the top of the app menu, and for it to open our About page in the one window, never that panel. Setting a menu on macOS replaces Tauri's default whole, so the Mac's menu has to carry what a Mac app can't do without: About ftorrent routing into the page the way the Windows item emits its route, then the standard Hide, Hide Others, Show All, and Quit items, and an Edit menu with Undo, Redo, Cut, Copy, Paste, and Select All, since without those the keyboard shortcuts stop working in the web view; Tauri's predefined menu items cover all of them. A Window menu is optional. The words stay Apple's, as the menu bar icon's already are.
