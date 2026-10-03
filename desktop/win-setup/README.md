_ftorrent/desktop/win-setup/README.md_

# The Windows Installer

> Prepared by [Claude Code](https://claude.ai/code) using Fable 5.1
> <br>Created: 2026-Oct
> <br>Last reviewed: 2026-Oct
> <br>Windows: 10 and 11, x64
> <br>Visual Studio: 2026, with MSVC 14.51 and the Windows 11 SDK
> <br>Node: 22

ftorrent's installer for Windows is a small program of its own, written here, rather than a script for an installer toolkit. It installs with no interface, per user, into one fixed place, starts the program when it's done, and comes back as the uninstaller. This guide is how it's built and how it works, and how to use it for another Tauri app. The design that decided it is in the desktop client planning document on [docs.ftorrent.com](https://docs.ftorrent.com/), and each file here carries its own essay at the top; this guide points to those rather than repeating them.

## What it does

A user double-clicks `ftorrent.exe`, and about a second later ftorrent is running. In between, with nothing on screen, the setup program checks itself, asks a copy of ftorrent that is already running to exit, unpacks the files into `%LOCALAPPDATA%\ftorrent`, writes `uninstall.exe` beside them and the entry that Settings and Add or Remove Programs read, puts one shortcut in the Start menu, and starts ftorrent. There is no page asking where to install, no choice of shortcuts, no license to accept, and no finish page with a checkbox. The one thing it can show is a dialog box when something goes wrong, so a user who sees nothing can trust that nothing did.

An upgrade is the same program run again. It writes over the previous version's files in place and never uninstalls first, so nothing ftorrent registered with Windows is taken away and given back, and a user who chose ftorrent for `.torrent` files and `magnet:` links keeps that choice through every release. The uninstaller removes the folder, the entry, the shortcut, and exactly the registry entries listed in `win-setup.toml`, each only while it is still ftorrent's, and leaves the data folder, `%LOCALAPPDATA%\com.ftorrent.ftorrent`, with the settings and session state in it, and every download wherever it is. A user who ran it by mistake installs again and finds everything as it was.

Nothing in the installer elevates, and nothing in it is anyone's but Windows'. The files travel in a cabinet that Windows' own `makecab` packed, and Windows' own `cabinet.dll` unpacks them, the same code that unpacks Windows Update. The hash is Windows' cryptography API. The shortcut is written through the shell's own interfaces. No compression library, no scripting engine, and no plugin is compiled in.

## The pieces

- **`setup.c`** is the program a user runs, in plain C, 64-bit, UTF-16 throughout, compiled fresh for every release. It is both the installer and the uninstaller. Its essay says how it reads the payload from the end of its own file, checks it, unpacks it, and does the rest.
- **`setup.rc`** and **`setup.manifest`** are what the resource compiler puts into it: the icon, the version block that Properties and Add or Remove Programs show, and the manifest that says it runs as whoever started it and never asks to elevate, draws sharp on high-resolution displays, and accepts long paths.
- **`build.cmd`** compiles those three into `build\setup.exe`, finding Visual Studio through `vswhere` and loading its developer environment. Run by hand it builds a stub with placeholder names; run by the creator it reads `build\stamp.h`, which the creator wrote with the real ones.
- **`win-setup.js`** is the creator. `pnpm installer` runs it after `tauri build` on Windows. It stages the executable and the resources `tauri.conf.json` names, packs them with `makecab`, writes the stamp header and runs `build.cmd`, and appends the cabinet, a table of strings, and a trailer to the compiled stub. The result lands under Tauri's own name for an installer, `src-tauri/target/release/bundle/win-setup/ftorrent_0.1.0_x64-setup.exe`, where `pnpm hash` and `pnpm upload` in `desktop/scripts.js` already look.
- **`win-setup.toml`** is the one configuration file, and most of it is prose. Everything the installer needs to know about the product, its name, version, identifier, publisher, home page, icon, and the files that ride beside the executable, comes from `tauri.conf.json` and `Cargo.toml`, which already say it; the TOML holds what no other file does, the list of registry entries uninstall takes back, in five instructions its own comments define.

## How the payload is laid out

The installer is the compiled stub followed by three things the creator appended: the cabinet, the table, and a 72-byte trailer. The trailer is the last bytes of the file, and the stub finds everything from it: a SHA-256 over the cabinet and the table, the offset and size of each, and the eight letters `winsetup`. A stub with nothing appended says so in a dialog rather than reading garbage, and a file whose hash doesn't match, a download cut short or a file altered after it was built, stops before a byte is written.

The table is UTF-16 strings, each ending in a null, in a fixed order: the product name, the program to start, the identifier, the version, the publisher, the home page, and then the uninstall instructions, one string each with its fields separated by tabs. The stub reads the first six by position and walks the rest to the end, so a later addition goes on the end and an older stub still finds what it knows. `uninstall.exe` is the same stub with the same table and a trailer whose cabinet size is zero, which is how the program knows which job it has when it runs.

## Building and trying it

`pnpm installer` from `desktop` is the whole build on Windows: Tauri compiles the app, `dmg.js` returns at once since it isn't a Mac, and `win-setup.js` makes the installer in about twenty seconds, most of it `makecab` compressing 44 MB of files to 14. `pnpm reveal` opens the folder it landed in, and double-clicking the file there is the test, the same experience a person who downloaded it has, apart from SmartScreen, which inspects only a file carrying the mark of the web.

To work on the stub alone, `build.cmd` from this folder compiles it in a second or two, and the resulting `build\setup.exe` can be given a payload by hand with a few lines of Node that do what the creator's last step does: read the stub, a cabinet made by `makecab`, and a table, write the trailer, and concatenate the four. Running that on a scratch folder exercises everything but the real files. An installer built on the machine running it never meets SmartScreen; to see what a downloader sees, give a local copy the `Zone.Identifier` stream by hand, as the signing essay in `desktop/scripts.js` describes.

Visual Studio is the one requirement beyond what building ftorrent already needs, and it's already met: Rust's Windows toolchain links with Microsoft's linker, so a machine that builds ftorrent has the C compiler too. `build.cmd` asks for the newest Visual Studio with the C++ tools, Community edition included.

## Using it for another Tauri app

The installer is ftorrent's, but nothing in it is specific to ftorrent except what it reads from the configuration, so another Tauri app can take the folder whole.

1. Copy `win-setup` into the app's workspace, beside `src-tauri`, and keep the layout: the creator finds `tauri.conf.json` and `Cargo.toml` relative to itself.
2. Run `node win-setup/win-setup.js` after `tauri build` in the app's installer script, and drop `nsis` from `bundle.targets` in `tauri.conf.json`, so Tauri stops making the installer it used to.
3. Write `win-setup.toml` for the app: the prose at the top, and the uninstall list, which is whatever the app itself writes to the registry under `HKCU`, if anything. An app that registers nothing can leave the list empty. The three placeholders, `{product}`, `{binary}`, and `{program}`, mean the product name, the executable's name, and the installed executable's path, so a list written once is right for any name.
4. Make sure `bundle.icon` names an `.ico`, since that's the icon the installer wears, and that `bundle.resources` names everything that must sit beside the executable, since the staged folder is built from that list and nothing else.

What the app gets is what ftorrent gets: a silent per-user install into `%LOCALAPPDATA%` under its product name, an upgrade that writes in place, a Start menu shortcut carrying its identifier as the AppUserModelID, which Tauri also sets on the window so the two are one taskbar button, and an uninstaller that takes back only what the list names. What it doesn't get, by design, is a way to install anywhere else or for every user on the machine.

## What a Windows user meets

The installer isn't signed, so a downloaded copy meets SmartScreen's blue window once, the same as any unsigned program, and the installing guide on docs.ftorrent.com walks through it. Nothing else appears. Afterwards ftorrent is in the Start menu, in Settings under Apps with its version and size, and in Settings under Default apps once it has offered itself for its file types, which the running program does on its own. Removing it from Settings runs `uninstall.exe`, which leaves a copy of itself in the user's temporary folder, a few hundred kilobytes Windows clears with the rest, because a program can't delete the file it's running from and only an administrator can schedule a deletion for the next restart.

## Where it came from

`setup.c` descends from a 2005 program, the Zootella Setup Creator, which made self-extracting installers that never showed an interface, from a zlib stream in a resource with ANSI strings; it's preserved in the repository's history, in the commit that brought it in, and the commits after it show the steps from that program to this one. The creator was C then because there was no scripting language in the pipeline; it's Node now because the pipeline is.
