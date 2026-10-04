_ftorrent/desktop/win-setup/README.md_

# The Windows Installer

> Prepared by [Claude Code](https://claude.ai/code) using Fable 5.1
> <br>Created: 2026-Oct
> <br>Last reviewed: 2026-Oct
> <br>Windows: 10 and 11, x64
> <br>Visual Studio: 2026, with MSVC 14.51 and the Windows 11 SDK
> <br>Node: 22

ftorrent's installer for Windows is a small program of its own, written in this folder, rather than a script for an installer toolkit. A user double-clicks `ftorrent.exe`, and about a second later ftorrent is running, with nothing shown in between. The same program is the uninstaller. This guide says what it is and why it exists, introduces every file and folder that takes part, and then tells the two stories that are the whole of it: what happens when the pipeline builds the installer, and what happens when a user runs it. The design that decided it is in the desktop client planning document on [docs.ftorrent.com](https://docs.ftorrent.com/), and each file here carries its own essay at the top, which this guide points to rather than repeats.

## Why ftorrent has its own

Tauri makes a Windows installer out of the box, with NSIS, the Nullsoft scriptable installer, and ftorrent shipped with it for a while. NSIS is good at what it's for: a wizard that asks a user where to install, which shortcuts to make, and whether to run the program at the end, driven by a script in its own language, with plugins for anything the language can't do. ftorrent asks none of those questions, so every page was one to skip, and the two things it most needed were the two the wizard made hard.

The first is that an upgrade must never uninstall first. ftorrent registers itself with Windows while it runs, offering to open `.torrent` files and `magnet:` links, and a user who chooses it in Default apps gets a sealed registry association, the user's pick held with a hash only Windows' own screens can write, which points at ftorrent's own registry entries. An installer that uninstalls the old version before writing the new one deletes those entries for a moment, and Windows, finding the sealed association pointing at nothing, resets it. ftorrent's installer writes the new files over the old ones and touches no registry entry, so the sealed association survives every release. The second is that install should be one fixed thing: per user, into the user's local application data, no elevation, one Start menu shortcut, the program started at the end every time. A user who wants ftorrent somewhere else has the portable copy.

What makes this possible without a toolkit is that Windows already has the pieces. `makecab.exe` packs files into a cabinet and `cabinet.dll` unpacks one, the same code that unpacks Windows Update, and both are in System32 on every Windows. The shortcut is written through the shell's own interfaces. Nothing is compiled in but the arrangement, which is a few hundred lines of C and a Node script, and nothing in the installer elevates.

## How it works

### The cast

Every file and folder that takes part, in the three places it lives. Paths in the repository are written from its root, `./`, and paths on a Windows PC from a user's profile, with `UserName` standing for whoever is signed in. A line marked transient exists only while its flow runs.

**In the repository**, what's checked out:

```
./desktop/win-setup/setup.c
./desktop/win-setup/setup.rc
./desktop/win-setup/setup.manifest
./desktop/win-setup/build.cmd
./desktop/win-setup/win-setup.js
./desktop/win-setup/registry.js
./desktop/win-setup/README.md
./desktop/src-tauri/tauri.conf.json
./desktop/src-tauri/Cargo.toml
./desktop/src-tauri/src/instance.rs
./desktop/package.json
./desktop/scripts.js
```

`setup.c` is the setup program's source, plain C, 64-bit, UTF-16 throughout; it is the installer and the uninstaller both. `setup.rc` and `setup.manifest` are what the resource compiler puts into it: the icon, the version block that Properties and Add or Remove Programs show, and the manifest, which says the program runs as whoever started it and never asks to elevate, draws sharp on high-resolution displays, and accepts long paths. `build.cmd` compiles those three, finding Visual Studio through `vswhere.exe` and loading its developer environment, which puts `rc.exe`, `cl.exe`, and `link.exe` on the path; run by hand with no stamp, it writes one with placeholder names first. `win-setup.js` is the creator, the script that makes an installer out of a built app. `registry.js` holds the one thing no other file says: the registry entries uninstall takes back, as one short list of the file extensions and link schemes the app opens, with the rules every app shares in the code beneath it. `README.md` is this guide.

The other five are the files the installer depends on without owning. `tauri.conf.json` names the product, version, identifier, publisher, copyright, home page, icon, and the resources that ride beside the executable, and its `bundle.targets` no longer names an installer for Windows. `Cargo.toml` names the executable. `instance.rs` is the application's single-instance code, which serves the pipe the installer uses to ask a running copy to exit. `package.json` has the `installer` script, and `scripts.js` the `hash` and `upload` steps that take the finished installer to ftorrent.com.

**Built**, by `pnpm installer` on Windows, all under `./desktop/src-tauri/target/release` and `./desktop/win-setup/build`, which git ignores:

```
./desktop/src-tauri/target/release/ftorrent.exe
./desktop/src-tauri/target/release/win-setup/stage/                             transient
./desktop/src-tauri/target/release/win-setup/payload.ddf                        transient
./desktop/src-tauri/target/release/win-setup/payload.cab                        transient
./desktop/win-setup/build/stamp.h
./desktop/win-setup/build/setup.res
./desktop/win-setup/build/setup.obj
./desktop/win-setup/build/setup.exe
./desktop/src-tauri/target/release/bundle/win-setup/ftorrent_0.1.0_x64-setup.exe
./desktop/release/ftorrent.exe
./desktop/release/ftorrent.exe.json
```

`target/release/ftorrent.exe` is the application, as Cargo leaves it. `stage` is a folder laid out exactly as the install will be, the application and its resources; `payload.ddf` is the directive file that tells `makecab.exe` what to pack, and `payload.cab` is the cabinet it packs, 13.8 MB from 44. `stamp.h` is everything the setup program knows, as C defines `win-setup.js` writes: the product's two names, `brandName` as people read it and `brandStem` as files carry it, which `src/brand.js` explains, with its identifier, version, publisher, copyright, home page, icon, installed size, and the uninstall list; `setup.rc` and `setup.c` both include it. `setup.res`, `setup.obj`, and `setup.exe` are the resource compiler's, the C compiler's, and the linker's outputs, and that `setup.exe` is the stub, about 160 KB, an installer with nothing in it yet. `ftorrent_0.1.0_x64-setup.exe` is the finished installer, 14 MB: the stub, then the cabinet, then a trailer, 24 bytes at the very end of the file that record where the cabinet begins and how long it is, so the stub can find it when it runs. The finished file below takes the three apart; under Tauri's own naming for an installer so that `pnpm hash` finds it where it looked before. `release/ftorrent.exe` is the same file under its published name, copied there by `pnpm hash`, with its sidecar of version, size, and SHA-256 beside it; the sidecar is committed, the installer is not.

**On the user's PC**, after a download and an install:

```
C:\Users\UserName\Downloads\ftorrent.exe
C:\Users\UserName\AppData\Local\ftorrent\ftorrent.exe
C:\Users\UserName\AppData\Local\ftorrent\ftorrent-engine\
C:\Users\UserName\AppData\Local\ftorrent\ftorrent.VisualElementsManifest.xml
C:\Users\UserName\AppData\Local\ftorrent\tile-medium.png
C:\Users\UserName\AppData\Local\ftorrent\tile-small.png
C:\Users\UserName\AppData\Local\ftorrent\torrent.ico
C:\Users\UserName\AppData\Local\ftorrent\uninstall.exe
C:\Users\UserName\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\ftorrent.lnk
C:\Users\UserName\AppData\Local\com.ftorrent.ftorrent\
C:\Users\UserName\AppData\Local\Temp\ftorrent-uninstall-1a2b3c.exe                transient
C:\Windows\System32\cabinet.dll
```

Two files are named `ftorrent.exe`, and the guide keeps them apart by where they are. `Downloads\ftorrent.exe` is the downloaded `ftorrent.exe`, the setup program, the same bytes as `ftorrent_0.1.0_x64-setup.exe` above. `AppData\Local\ftorrent\ftorrent.exe` is the installed `ftorrent.exe`, the application. Beside the application are its resources, the ones `stage` held: `ftorrent-engine`, the frozen Python holding libtorrent that the application starts as a second process; the Start tile's manifest and two images; and `torrent.ico`, the document icon. `uninstall.exe` is the setup program again, written by the install, without the cabinet. `ftorrent.lnk` is the one shortcut. `com.ftorrent.ftorrent` is the application's data folder, with `ftorrent.toml`, the lock file, the session state, and the web view's profile, which the installer never writes and the uninstaller never removes. The Temp copy is the uninstaller moved out of the folder it's about to remove, and the number in its name is different every time. `cabinet.dll` is the one part of Windows the setup program loads by name, to unpack the cabinet.

**In the registry**, all under `HKEY_CURRENT_USER`, the user's own hive, which the installer never leaves:

```
HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\ftorrent
HKCU\Software\Classes\ftorrent.torrent
HKCU\Software\Classes\.torrent\OpenWithProgids
HKCU\Software\Classes\magnet
\\.\pipe\ftorrent-0123456789abcdef
```

The `Uninstall\ftorrent` key is the entry Settings and Add or Remove Programs read, and the one key the installer writes. The next three are examples of what the application writes for itself while it runs, offering to open its types, and what the uninstall list in `registry.js` takes back: a ProgID of ftorrent's own, taken whole; ftorrent's line in the list of programs offered for an extension, one value among others; and the shared class for a link scheme, taken only while its command still runs ftorrent. The last line isn't a registry key but belongs with them: the named pipe the running application serves, whose name is the product and a hash of the lock file's path, and which the installer writes one line into to ask the application to exit.

### Building the installer

The developer runs `pnpm installer` in `./desktop` on Windows. It runs three programs in turn: `tauri build`, then `node dmg.js`, which returns at once since this isn't a Mac, then `node win-setup/win-setup.js`.

1. **`tauri build` builds the application,** `target/release/ftorrent.exe`. Vite bundles the page and Cargo compiles the Rust core. Tauri bundles nothing on Windows, since `bundle.targets` names only the Mac's `app` and Linux's `deb`, and Tauri skips a target that doesn't apply to the machine it's on.

2. **`win-setup.js` gathers the facts.**
   - From `tauri.conf.json`: the product name, version, identifier, publisher, copyright, home page, the `.ico` among the icons, and the `bundle.resources` map.
   - From `Cargo.toml`: the crate's name, which is `brandStem`, since Cargo names the executable from it.
   - From `registry.js`: the uninstall instructions, built from its list and the two names above, with `{program}` left for the setup program, which alone knows the installed path.

3. **`win-setup.js` stages the files** into `stage`: the application at the top, `ftorrent-engine` beside it with its `_internal` inside, the tile's manifest and images, and `torrent.ico`. Sixty-three files, 44 MB.

4. **`makecab.exe` packs them.** `win-setup.js` writes `payload.ddf`, naming each staged file and the folder it belongs in, and runs `makecab.exe` on it. The result is `payload.cab`, one cabinet, LZX compression, which takes about fifteen seconds and is most of the build.

5. **`build.cmd` builds the stub.**
   - `win-setup.js` writes `stamp.h`: the icon's path, `brandName` and `brandStem`, the identifier, publisher, copyright, and home page, the version as four numbers and as text, the staged size in kilobytes, and the uninstall instructions as an array of strings, each with its fields separated by tabs.
   - `win-setup.js` runs `build.cmd`. `rc.exe` reads `setup.rc`, which includes the stamp, and produces `setup.res`. `cl.exe` compiles `setup.c` to `setup.obj`. `link.exe` joins the two into `build/setup.exe`.
   - The stub now knows everything about ftorrent it will ever know. The same defines fill its version block, which Windows reads, and the constants its code uses.

6. **`win-setup.js` assembles the installer.** It reads the stub and the cabinet into memory, builds the trailer, and writes the three out end to end as one new file, `ftorrent_0.1.0_x64-setup.exe`. Nothing in the stub is edited; the cabinet and the trailer are appended after it, and the next section says what that file is made of. `pnpm hash` then copies it to `release/ftorrent.exe` and writes the sidecar, and `pnpm upload` sends both to ftorrent.com.

So every fact reaches the finished installer by one road, the stamp, and the only thing appended is the files.

### The finished file

`ftorrent_0.1.0_x64-setup.exe`, and `Downloads\ftorrent.exe` once a user has it, is three things laid end to end, and nothing joins them but their order:

```
offset 0          setup.exe      the compiled stub, about 166 KB
offset 166 KB     payload.cab    the cabinet, about 13.8 MB
offset 14.0 MB    trailer        24 bytes, the last in the file
```

**The stub** is the complete program as `link.exe` wrote it, unchanged. An executable records inside itself where its own code and data end, and the Windows loader maps only that much, so the loader runs the first 166 KB as the program and never reads past them. That is what lets the rest of the file be cargo the program carries rather than code Windows sees. The arrangement is the one every self-extracting installer has used for decades; the 2005 program this one descends from kept its payload inside the executable as a resource instead, which needs the resource-editing API to inject, where appending needs only concatenation.

**The cabinet** is `payload.cab` byte for byte, a file in Microsoft's cabinet format: a header beginning `MSCF`, a folder of LZX-compressed data in blocks of 32 KB, each block carrying a checksum that `cabinet.dll` verifies as it unpacks, and for every file its path relative to the cabinet's root, its size, its modified date, and where its bytes begin. Ours holds the sixty-three files of `stage`, which are the sixty-three files of an installed ftorrent:

```
ftorrent.exe                                    the application, 11 MB
ftorrent-engine\ftorrent-engine.exe             the engine, a frozen Python holding libtorrent
ftorrent-engine\_internal\python313.dll         its interpreter
ftorrent-engine\_internal\libtorrent\*.pyd      libtorrent itself, as a Python extension
ftorrent-engine\_internal\*.dll, *.pyd, *.zip   the interpreter's standard library and runtime, about fifty files
ftorrent.VisualElementsManifest.xml             the Start tile's manifest
tile-medium.png, tile-small.png                 the Start tile's images
torrent.ico                                     the document icon for .torrent files
```

The application and its engine are the two programs; everything under `_internal` is what the engine needs to run, 34 MB of the 44, and the last four files are what Windows reads to draw the tile and the document icon. The cabinet's root is the install folder, so each path above is where the file lands under `AppData\Local\ftorrent`.

**The trailer** is 24 bytes, written by `win-setup.js` and read by `setup.c` as the struct `Trailer`, which packs the three fields with no padding between them:

```
bytes  0 to  7   cabinetOffset   where the cabinet begins, as a 64-bit unsigned number, little-endian: the stub's length, 166 KB
bytes  8 to 15   cabinetSize     how long it is: the cabinet's length, 13.8 MB; zero marks uninstall.exe, which carries no cabinet
bytes 16 to 23   marker          the eight ASCII letters ENCLOSED
```

The trailer is at the end rather than the beginning for one reason: the stub is compiled before the cabinet exists, so nothing inside it can know the cabinet's size, and a record placed last is the one place a program can find without knowing anything in advance except to look at its own last 24 bytes. The two numbers are little-endian because that is how the x64 processor stores a number and how `win-setup.js` writes one with `writeBigUInt64LE`, so `setup.c` reads them into the struct with no conversion. The eight letters are our own marker, nothing Windows asks for: an executable's last section is padded to an alignment boundary with zeros, so a stub nothing was appended to could read its own tail as an offset of zero and a size of zero, which would pass every check and mean "uninstaller"; eight letters that only `win-setup.js` writes make the difference unmistakable.

`uninstall.exe` is the same stub, the first 166 KB of the downloaded file, followed by a trailer whose `cabinetSize` is zero and nothing else. The setup program writes it by copying its own first `cabinetOffset` bytes and appending that trailer.

### Installing

The user downloads `ftorrent.exe` from ftorrent.com and double-clicks it in `Downloads`. The installer isn't signed, so a downloaded copy meets SmartScreen's blue window once, the same as any unsigned program, and the installing guide on docs.ftorrent.com walks through it. After that, nothing appears.

1. **Windows reads the manifest** before the program's first instruction, and runs the downloaded `ftorrent.exe` as the user, high-DPI aware, with long paths allowed.

2. **The downloaded `ftorrent.exe` checks itself.**
   - Its first act restricts library loading to System32, so a file named like a system library in `Downloads` is never picked up. `cabinet.dll` is linked to load on first use, which is after this.
   - It opens its own file and reads the trailer, its last 24 bytes. It finds `ENCLOSED`, and checks that the cabinet's offset and size fit inside the file.
   - A stub with nothing appended has no marker and says so in a dialog. A cabinet that is damaged is caught later by `cabinet.dll`, which checks a sum in every block as it unpacks; and whether the file is the one ftorrent.com published is the sidecar's hash to answer, outside the file, as the installing guide describes.

3. **The downloaded `ftorrent.exe` works out its paths.** It knows the product's name, the executable to start, the identifier, and the rest from the stamp it was compiled with; from the product name it works out `AppData\Local\ftorrent`, local application data as the shell reports it joined with the name, and the installed `ftorrent.exe` and `uninstall.exe` inside it.

4. **The downloaded `ftorrent.exe` sees the cabinet has bytes,** so this is an install. A cabinet of zero bytes would mean it was running as `uninstall.exe`.

5. **A running installed `ftorrent.exe` exits.** The downloaded one tries to open the installed one for writing; a sharing violation means a copy is running from it.
   - The downloaded `ftorrent.exe` asks that copy to exit the way a second launch hands it a link: it derives the pipe's name from the product and a hash of the lock file's path, exactly as `instance.rs` computes it, and writes one line of JSON into the pipe, `{"args":["--exit"]}`. The installed `ftorrent.exe` quits the way File, Exit does, stopping `ftorrent-engine` and saving its state.
   - The downloaded `ftorrent.exe` waits up to fifteen seconds for the installed one's file to come free. A copy that never answers, or has no pipe, is ended, along with anything else running from the folder, and a copy that won't end even then stops setup with a sentence rather than a half-written folder.
   - On a first install, or with ftorrent closed, this step takes no time.

6. **`cabinet.dll` unpacks the cabinet.** The downloaded `ftorrent.exe` hands it a set of callbacks and asks it to copy the cabinet named `cabinet`.
   - When `cabinet.dll` opens that name, the downloaded `ftorrent.exe` gives it a window onto its own file, the cabinet's offset for the cabinet's size, so the cabinet is read without ever being a file.
   - For each of the sixty-three files, `cabinet.dll` announces it, the downloaded `ftorrent.exe` creates it under `AppData\Local\ftorrent`, making folders as needed, and hands back a handle; `cabinet.dll` writes it; the downloaded `ftorrent.exe` sets its modified date from the cabinet and closes it.
   - Each file is written over whatever was there, which is how an upgrade replaces the previous version.

7. **The downloaded `ftorrent.exe` writes `uninstall.exe`:** the first bytes of its own file, up to where the cabinet began, which are the stub, then a trailer with a cabinet size of zero.

8. **The downloaded `ftorrent.exe` writes the uninstall entry,** `Uninstall\ftorrent`, named for the product, so an install over an existing copy rewrites its entry rather than adding a second: the display name, icon, and version, the publisher, the install folder, the size in kilobytes, the date, the home page, and `uninstall.exe` as both the ordinary and the quiet uninstall command.

9. **The downloaded `ftorrent.exe` writes `ftorrent.lnk`,** through the shell's link and property store interfaces, carrying `com.ftorrent.ftorrent` as its AppUserModelID. The application tells the shell the same identifier for its process when it starts, in `src-tauri/src/lifecycle.rs`, so a pinned shortcut and the running window are one taskbar button; without that call Windows would know the window by the executable's path instead, and show two.

10. **The downloaded `ftorrent.exe` starts the installed `ftorrent.exe`** and exits. About one second has passed. Before starting it, the downloaded one reads the registry for Microsoft Edge WebView2, which every current Windows has and which the application draws its window with; a Windows without it gets a dialog and Microsoft's page for it opened instead of a program that can't start.

From here the installed `ftorrent.exe`, running, does the rest for itself: it writes its ProgIDs and offers itself for its file types and links, and asks the user whether to be their default, which is the application's own policy and none of the installer's.

### Uninstalling

The user removes ftorrent from Settings, or double-clicks `uninstall.exe`. Either way Windows runs that same program, and it asks nothing: a user who ran it by mistake installs again and finds everything as it was.

1. **`uninstall.exe` recognizes itself** by its trailer's cabinet size of zero, after the same self-check as an install.

2. **`uninstall.exe` moves out of the folder it's about to remove.** A program can't delete the file it's running from, so it copies itself to Temp as `ftorrent-uninstall-1a2b3c.exe`, starts that copy, and exits. The copy stays in Temp, a few hundred kilobytes Windows clears with the rest, because only an administrator can schedule a deletion for the next restart.

3. **A running installed `ftorrent.exe` exits,** asked through its pipe by the Temp copy, exactly as before an install.

4. **The Temp copy walks the uninstall list,** the instructions compiled in from `registry.js`, in order. Each names a key under HKCU and one of five actions: delete the key whole; delete one value; delete the value only while it still equals the text given; delete the key only if nothing is left in it; delete the key whole only while a value under it still equals the text given. That last one, with `{program}` filled in as the installed `ftorrent.exe`'s path, is how `Classes\magnet` goes only while its command still runs ftorrent, and stays if another client has taken it since. Anything already gone is simply gone.

5. **The Temp copy removes the uninstall entry and the shortcut.** It deletes `Uninstall\ftorrent`, unpins `ftorrent.lnk` from Start and the taskbar through the shell's pinned list, deletes the file, and clears the jump list Windows kept for `com.ftorrent.ftorrent`. Then it tells Explorer the file associations changed, so the icons and menus Explorer cached for ftorrent's types are dropped.

6. **The Temp copy removes `AppData\Local\ftorrent`,** every file and subfolder, after waiting for the `uninstall.exe` that started it to finish exiting.

What stays: `com.ftorrent.ftorrent`, the data folder, with `ftorrent.toml` and the session state in it; every download, wherever it is; and the sealed registry associations the user made in Default apps, which the system owns and only its own screens change. A user who installs again finds their settings and torrents as they left them.

## Building and trying it by hand

`pnpm installer` is the build, and `pnpm reveal` opens the folder the installer landed in; double-clicking it there is the test, the same experience a person who downloaded it has, apart from SmartScreen, which inspects only a file carrying the mark of the web. To see what a downloader sees, give a local copy the `Zone.Identifier` stream by hand, as the signing essay in `scripts.js` describes.

To work on the stub alone, `build.cmd` from this folder compiles it in a second or two. The resulting `build/setup.exe` can be given a payload by hand with a few lines of Node that do what `win-setup.js`'s last step does: read the stub and a cabinet made by `makecab.exe`, write the 24-byte trailer, and concatenate the three. Running that on a scratch folder exercises everything but the real files.

Visual Studio is the one requirement beyond what building ftorrent already needs, and it's already met: Rust's Windows toolchain links with Microsoft's `link.exe`, so a machine that builds ftorrent has the C compiler too. `build.cmd` asks for the newest Visual Studio with the C++ tools, Community edition included.

## Using it for another Tauri app

Nothing in the installer is specific to ftorrent except what it reads from the configuration, so another Tauri app can take the folder whole.

1. Copy `win-setup` into the app's workspace, beside `src-tauri`, and keep the layout: `win-setup.js` finds `tauri.conf.json` and `Cargo.toml` relative to itself.
2. Run `node win-setup/win-setup.js` after `tauri build` in the app's installer script, and drop `nsis` from `bundle.targets` in `tauri.conf.json`, so Tauri stops making the installer it used to.
3. Put the app's own file extensions and link schemes in the list in `registry.js`; an app that opens no types of its own leaves it empty. The rest of the uninstall list follows from the two names and the way `src/associate.js` registers an app, so the rules beneath the list change only for an app that registers itself differently.
4. Make sure `bundle.icon` names an `.ico`, since that's the icon the installer wears, and that `bundle.resources` names everything that must sit beside the executable, since `stage` is built from that list and nothing else.

What the app gets is what ftorrent gets, and what it doesn't get, by design, is a way to install anywhere else or for every user on the machine.

## Where it came from

`setup.c` descends from a 2005 program, the Zootella Setup Creator, which made self-extracting installers that never showed an interface, from a zlib stream in a resource with ANSI strings. It's preserved in the repository's history, in the commit that brought it in, and the commits after it show the steps from that program to this one. The creator was C then because there was no scripting language in the pipeline; it's Node now because the pipeline is.
