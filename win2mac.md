# Mac: two names for the product, the Windows installer is ours, and four shared files changed

A note for the Mac session. It's public and committed, so it names nobody: in anything written for the public, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile. This letter is complete in itself and asks for no reply.

## Two names, and the words for them

The product has two names, and the code now keeps them apart with two words, used in every language and file: **`brandName`**, the name people read, `productName` in `tauri.conf.json`, `ftorrent` here and `Fuji` in the sibling project; and **`brandStem`**, the stem of the executable's name, the crate's `name` in `Cargo.toml`, since Cargo names the executable from it, `ftorrent` here and `fuji` there; the Fuji session proposed the pair, both beginning with `brand` as `brand.js`'s other exports do, and stem being the languages' own word for a file name without its extension. Nothing derives one from the other. The essay atop `desktop/src/brand.js` says which goes where, the desktop README has a section, The two names, on how each flows through the page, the Rust core, and the pipelines, the planning document states the rule under Design principles, and `style.md` names the two words.

What moved in the shared code, all mechanical:

- **`src/brand.js`** exports `brandStem` beside `brandName`, read from `Cargo.toml` through a raw import and smol-toml. The page's file names moved to it: the settings file, the lock, the session folder, the log folder, the `.ftorrent` extension and `ftorrent:` scheme in `associate.js` and `settings.js`. Everything a person reads stays `brandName`.
- **The Rust core** names the settings file, the lock, the pipe, and the engine's folder from `package_info().crate_name` rather than `name`, in `paths.rs`, `instance.rs`, and `engine.rs`. The window title, the tray, the menus, and the client name the engine receives keep `name`.
- **`scripts.js`** builds the six published names from `brandStem` and the prefix it looks for from `brandName`, both read at the top of the file, and imports smol-toml for the crate's name. **`dmg.js`** calls the product name `brandName`. Neither changes what either does for ftorrent.

## The Windows installer

`desktop/win-setup/` holds a setup program of ftorrent's own in place of NSIS: `setup.c`, a 64-bit C program with no interface that unpacks a cabinet from the end of its own file into local application data, asks a running copy to exit first, writes the Start menu shortcut and the uninstall entry, starts the program, and is its own uninstaller; `win-setup.js`, the creator, which stages what `tauri.conf.json` names, packs it with Windows' own `makecab`, compiles the stub fresh with the release's names and version, and appends the cabinet; and `registry.js`, which builds the uninstall list that `hooks.nsh` used to hold from one short list, the file extensions and link schemes the app opens, with the rules beneath it. The README there tells the whole story as two flows. An upgrade writes over the files in place and never uninstalls first, which is what keeps a user's choice of ftorrent for `.torrent` and `magnet:` through a release.

The shared files it touched: `tauri.conf.json` lost the `nsis` target and the `bundle.windows` block, so `bundle.targets` is `["app", "deb"]`, which changes nothing for a Mac build; `package.json`'s `installer` runs `node win-setup/win-setup.js` after `dmg.js`, and it returns at once on a Mac exactly as `dmg.js` does on Windows; and `instance.rs` quits on a handoff whose only argument is `--exit`, inside the Windows-only pipe code, which is how the installer closes a running copy.

## Where to look

- `desktop/src/brand.js`, the essay, and the section The two names in `desktop/README.md`.
- `desktop/win-setup/README.md`, for the installer whole, and the essays atop `setup.c` and `win-setup.js`.
