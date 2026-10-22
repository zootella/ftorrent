# Mac: the system's boxes

A note from the Windows session. It's public and committed, so it names nobody: in anything written back, say "the user." Git is read-only on both boxes, and the user makes every commit.

## The three boxes, and the Mac half to compile

`dialog.rs` now offers the boxes every desktop platform has, in general terms, so the page asks for what it wants and Rust knows nothing of torrents:

- `dialog_open`, for files or for a folder, one or several, answering a list of paths, empty for a cancel.
- `dialog_save`, suggesting a file name, answering one path, blank for a cancel.
- Both take `start`, the folder the box opens in, and a filter, `kind` and `extensions`. `dialog.js` is the page's half.

A box choosing a file or a folder in one is `either`, which only the Mac's panel can, so off the Mac it's a box for files and the asker writes the same call on every platform; ftorrent's page never asks for it. A blank `start` leaves the folder to the system, which on Windows put the box in the installed program's own folder, so every box the page shows names its folder: the Connection tab's Open… starts in Downloads with the torrent filter. The folders come from `downloadsFolder` and `documentsFolder` in `paths.js`, which ask Tauri's `downloadDir` and `documentDir`, so they're where the system keeps them, not a guess under the home folder. A filter is for files: a box choosing a folder takes none, which matters most on the Mac, where filtering would show every folder as one that can't be chosen.

The Windows and Linux half is the dialog plugin's, built and clicked through on Windows. The Mac half was written on Windows and has never compiled: `mod mac` in `dialog.rs`, with open and save panels as sheets, `directoryURL` from `start`, `allowedContentTypes` from each extension's `UTType`, `nameFieldStringValue` from `name`, and New Folder on; `NSArray` added to `objc2-foundation`'s features in `Cargo.toml`, which leaves `Cargo.lock` as it is. A box goes over the window asking only while that window is on the screen; hidden or minimized, the panel goes up on its own through `beginWithCompletionHandler`, and `dialog_alone` is the way in for Rust with no window at all, which ftorrent doesn't call. On the Mac, `either` sets both of the open panel's switches and takes no filter. The file is meant to be identical in a sibling project, so its examples are neutral. And File, Open… with ⌘O, above Close Window in `lifecycle.rs`'s Mac menu, bringing the window forward first, as About and Settings do.

- **Compile it,** and fix what the compiler finds in the Mac half.
- **Click through the spike** with the user, the way it went on Windows. It's on Home, `DialogSpike.vue` with `spike.js`: six buttons, each putting up a box a different way, and a line under them saying what came back. Open a file or folder asks for `either`, which on Windows is a file box and on the Mac should take a file or a folder. File, Open… and ⌘O report on the same line. Each sheet should open in the folder its button names, the torrent boxes should offer only torrent files, Save should add `.torrent` and ask before replacing, and a cancel should read cancelled. The spike stays on Home after.
- **Run the same in a Linux build,** where the plugin runs the same code Windows does.

## The Linux box in the Flatpak

The plugin's default feature, `gtk3`, puts up GTK's own file chooser, which sees only what its sandbox can. The Flatpak grants `--filesystem=xdg-download`, so its boxes see Downloads and little else, and choosing a folder elsewhere, to make a torrent from or to download into, can't work there. The desktop portal is the correct box in a sandbox, KDE's own on KDE, and rfd falls back to zenity where there's no portal. It's the plugin's `xdg-portal` feature, in place of the default, and it adds crates, so it changes `Cargo.lock`. The recommendation is to switch, on the Mac, where Linux builds, if the user agrees.
