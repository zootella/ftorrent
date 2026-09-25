# Portable Sprint

The desktop client now builds and installs the ordinary way on both active platforms. The NSIS installer puts it in the user's own folder on Windows, and the disk image and a drag to the Trash install and remove it on macOS. The engine, with libtorrent inside it, rides along in both. Before the client downloads its first torrent, this sprint settles three things that everything after it will lean on:

- **Paths.** Where an installed copy keeps its settings, its libtorrent state, its crash log, and the web view's own data, and how a download folder carries its session data with it.
- **The portable edition.** A zip for Windows and macOS that runs from a folder or a USB stick and keeps its state beside itself.
- **Instances.** ftorrent runs once per user and once per copy. A second launch of the same copy hands its request to the one already running. A portable copy runs beside an installed one without either noticing the other, and two people signed in to their own accounts on one machine each run their own.

The desktop client planning document on the docs site describes the paths, the portable edition, and the instance lock as first designed. This sprint builds on that design, and the next section records where checking it against the platforms changed it. The planning document gets one correction pass when the sprint ends.

## What changed from the first design

### The shipped bundles hold no symlinks

The libtorrent wheel for macOS keeps its OpenSSL libraries in a folder of its own, `libtorrent.dylibs`. The libtorrent module loads `@rpath/libssl.3.dylib`, and its search path is the engine's `_internal` folder. PyInstaller bridges the two with a pair of symlinks in `_internal`, and the engine fails to start without them.

A cross-platform stick is formatted exFAT, because exFAT is the one format both Windows and macOS write natively (FAT32 stops at 4 GB per file, and macOS reads NTFS but doesn't write it). exFAT can't hold a symlink, and neither can a zip unpacked by Windows. So we checked whether anything we ship depends on one:

- The PyInstaller build on Windows makes no symlinks.
- When Tauri copies the engine folder into the Mac `.app` as a resource, it copies the file each symlink points to, so the bundle already holds two real files where the links were, plus the originals in `libtorrent.dylibs`.
- Moving the two real files up into `_internal` and deleting `libtorrent.dylibs` still gives an engine that starts and answers `ready`, because `libssl` finds `libcrypto` through the same search path.

The engine recipe does that move at the end of each build: every symlink in the output is replaced by the file it points to, and a folder left empty is removed. Nothing on any platform, installed or portable, relies on a symlink after that. The Mac bundle loses about 5.8 MB of duplicate libraries along the way.

### A quarantined Mac app runs from somewhere else

A zip downloaded in a browser carries the quarantine mark, and Archive Utility passes the mark on to everything it unpacks. When macOS launches a quarantined app from a place Finder didn't move it to, it runs a copy from a randomized, read-only mount under `/private/var/folders/…/AppTranslocation/`. This is App Translocation. Its purpose is to stop an app from finding files planted beside it, and portable mode depends on exactly that, a folder named `portable` beside the app.

A translocated portable copy would look beside itself, find no `portable/ftorrent.json`, and start as an installed copy. Translocation happens when three things are all true:

- the app carries the quarantine mark;
- Launch Services opens it, from Finder or `open`, not from a shell;
- the app hasn't been moved on its own in Finder out of the folder it arrived in.

Approving the app with **Open Anyway** doesn't end it. The approval is recorded in the quarantine attribute, and the app is translocated again on every launch. What ends it is removing the mark.

Where the zip is unpacked decides whether the mark is there at all:

- **Downloaded on a Mac and unpacked with Archive Utility:** every file inside carries the mark, wherever it's unpacked to. This is the one route that leads to translocation.
- **Unpacked on Windows onto an exFAT stick:** the Mac finds no mark. Windows never writes the Mac's quarantine attribute. Its own Mark of the Web is an NTFS alternate data stream, which exFAT can't hold, so on the stick the Windows files carry no mark either.
- **Fetched with a command-line tool:** no mark is set.

The design handles the one bad route in two layers:

- The portable instructions for the Mac clear the mark after unpacking, with the same `xattr -dr com.apple.quarantine` command the installing page already gives, or fetch the zip with `curl`.
- At startup, a copy whose own path runs through `/AppTranslocation/` knows it can't find its folder. It says so in its window, with the command that fixes it, and doesn't fall back to acting as an installed copy.

The Security framework has a call that recovers the original path, `SecTranslocateCreateOriginalPathForURL`, but its header left the public SDK years ago. Apple's developer support says there's no supported way to detect translocation or find the original path. The path check is a heuristic we can see and test. We name the problem and show the fix rather than depending on a private call.

### A portable copy writes nothing, and the operating system still keeps records

The first design promised that portable mode never touches the host. ftorrent's own code can keep that promise. The platforms and the web view keep their usual records of any program that runs, whether or not the program asks them to:

- **Windows.** WebView2 keeps a profile folder. By default Tauri puts it at `%LOCALAPPDATA%\com.ftorrent.ftorrent\EBWebView`, and it can be pointed elsewhere, so a portable copy puts it under `portable/`. Beyond that, SmartScreen remembers a program it has approved. SmartScreen checks only programs the shell launches, so the engine, which ftorrent starts directly, meets no check of its own. The Windows Defender Firewall asks the first time a program listens. An administrator who allows it creates allow rules. For a user who isn't an administrator, Windows creates block rules whichever option is chosen, and inbound connections stay blocked until an administrator changes them. Either way the prompt doesn't return while the rules exist. The shell keeps its recent and jump-list entries.
- **macOS.** WKWebView keeps its data under the bundle identifier, and no public API moves it: `~/Library/WebKit/`, `~/Library/Caches/`, and `~/Library/HTTPStorages/` each get a folder named for it. AppKit may keep window state in `~/Library/Saved Application State/`. Launch Services registers every app that runs, along with the URL schemes and document types its `Info.plist` declares; a test app launched from an exFAT image was registered with its URL scheme the moment it opened. Gatekeeper keeps its approval, and the privacy system keeps any grant for the removable volume. On the stick itself, macOS 15 writes a `._` file beside every file it touches, to hold extended attributes exFAT can't store. Windows shows those files as ordinary files.

The portable promise becomes: ftorrent itself writes nothing to the host, and the operating system keeps its usual records of a program that ran. The portable page lists those records plainly, so a reader who cares knows exactly where to look.

### The portable Mac bundle has its own identifier

The portable `.app` is built as `com.ftorrent.portable`, and the installed one stays `com.ftorrent.ftorrent`. With separate identifiers:

- Launch Services sees two different apps. Which of two copies with the *same* identifier receives a clicked magnet isn't documented, and reports disagree. With separate identifiers the question never comes up: a clicked magnet goes to the installed copy and never to a portable copy that happens to be running.
- The portable `Info.plist` declares no document types and no URL schemes, so running it from a stick registers nothing as a handler.
- WebKit keeps the two copies' data apart.

An installed copy offers to handle `.torrent` files and `magnet:` links. A portable copy never does. A user running portable brings torrents in explicitly: by dragging a `.torrent` file onto the window, by pasting a magnet link, or with an Open command that shows a file dialog. All three arrive with the torrent interface, after this sprint, and work the same in an installed copy.

The portable `.app` belongs inside its folder. Dragged out on its own, away from `portable/ftorrent.json`, it isn't a supported way to run ftorrent, and the build does nothing special to catch it.

### Portable is Windows and macOS

The first design put Linux in the zip too. A bare Tauri binary on Linux depends on the system's WebKitGTK, which not every distribution installs. Linux ships on two architectures. And Linux sits outside the active test matrix. The zip carries Windows and macOS, and Linux keeps its four packages. This also removes a collision the first design flagged: the Windows and Linux executables would have shared one folder and both wanted `ftorrent-engine/` beside them. With Linux out, the Windows engine folder sits beside `ftorrent.exe` exactly as it does in an installed copy, and the Mac engine stays inside its bundle.

### One lock mechanism on every platform

The first design used a named mutex on Windows and `flock()` on macOS and Linux. A plain named mutex lives in the per-session namespace, so two people signed in to one Windows machine and running the *same* portable copy from one stick would each get the lock, and both would write the same `portable/state`.

Rust's standard library has had file locking since 1.89: `File::try_lock` takes an exclusive lock with `flock()` on macOS and Linux and `LockFileEx` on Windows, and the operating system releases it however the process ends. ftorrent pins Rust 1.98. So every platform uses one mechanism: an exclusive lock on `ftorrent.lock`, beside `ftorrent.json`, held for the life of the process. A file lock applies to everyone who opens the file, whatever account they're signed in as. It needs no hashing and no extra crate, and it leaves no stale lock after a crash. The lock file stays empty and separate from `ftorrent.json` for two reasons. `ftorrent.json` is replaced whole on every save, which would drop a lock held on the old file. And a lock on Windows is mandatory rather than advisory: while it's held, no other process can read the locked file at all.

Tauri's own single-instance plugin shows why ftorrent carries its own lock. It keys everything on the bundle identifier. On Windows that's a named mutex in the per-session namespace, and on macOS it's a socket in the shared `/tmp`. So it would stop a portable copy from running beside an installed one, and it can't tell one account's copy from another's.

### Download folders are locked too

The settings lock keeps two launches of one copy apart. It doesn't cover the resource two different copies can actually share. If an installed copy and a portable copy both list `D:/torrents`, each would load the same resume data from `D:/torrents/.ftorrent/`, seed the same files, and overwrite each other's saves.

So each instance also takes an exclusive lock on `.ftorrent/ftorrent.lock` inside every download folder it loads. When another instance already holds a folder, the second one skips that folder's torrents and shows the folder as in use by another copy of ftorrent. The same lock works across accounts, and across machines when the folder is on a network share.

## The design

### Installed paths

Everything an installed copy keeps for itself sits in one folder per user, named by the bundle identifier:

```
C:\Users\username\AppData\Local\com.ftorrent.ftorrent\      # Windows, with WebView2's EBWebView folder inside it
~/Library/Application Support/com.ftorrent.ftorrent/        # macOS
```

```
ftorrent.json    settings, only the values the user has changed
ftorrent.lock    empty, exists to be locked
state            libtorrent's session state, the DHT routing table among it
crash.log        the engine's last words, written only when it fails
```

Downloads default to `~/Downloads/ftorrent`, and each download folder carries its own `.ftorrent` folder with one subfolder per torrent, as the planning document describes.

### Portable paths

The zip unpacks to one folder:

```
ftorrent/
ftorrent/ftorrent.exe                   # Windows
ftorrent/ftorrent-engine/               # the Windows engine, where an installed copy also keeps it
ftorrent/ftorrent.app/                  # macOS, identifier com.ftorrent.portable, engine inside
ftorrent/portable/ftorrent.json         # its presence is what makes a copy portable
```

After a first run, the portable folder holds the rest, and downloads land beside it:

```
ftorrent/portable/ftorrent.lock
ftorrent/portable/state
ftorrent/portable/crash.log             # only after a failure
ftorrent/portable/EBWebView/            # WebView2's profile, on Windows
ftorrent/downloads/
ftorrent/downloads/.ftorrent/
```

`portable/ftorrent.json` ships containing `{"download_folders": ["./downloads"]}`. A `./` path resolves against the folder that holds the program: the folder containing `ftorrent.exe` on Windows, and the folder containing `ftorrent.app` on macOS. Paths are stored with forward slashes. `~` means the current user's home folder. An absolute path stays exactly as written, and it only resolves on the machine it names.

### Startup

1. Find the program's own location: the folder of the executable on Windows, and the folder containing the `.app` bundle on macOS. On macOS, a location under `/AppTranslocation/` stops here with the explanation described above.
2. Look for `portable/ftorrent.json` at that location. If it's there, this copy is portable, and the `portable` folder is its data folder. If it isn't, the data folder is the installed one, created if missing.
3. Try the exclusive lock on `ftorrent.lock` in the data folder. If another process holds it, hand this launch's request to that process and exit.
4. Read `ftorrent.json`. A missing key takes its built-in default, and a missing file is written with the one default download folder.
5. Start the engine and send `init` with the resolved paths: the data folder, the state file, and the download folders.
6. For each download folder that exists, try the lock on its `.ftorrent/ftorrent.lock`. Load the folder's torrents when the lock is free, and report the folder as in use when it isn't. A folder that doesn't exist on this machine is skipped silently and kept in the list.

The main page shows the resolved values: portable or installed, the data folder, and each download folder with its state. That way the whole resolution can be checked before any torrent exists.

### Handing a second launch to the running one

A second launch happens when the user opens a copy that's already running, or when the operating system opens a `.torrent` file or a `magnet:` link in an installed copy that's already running.

- **macOS.** Launch Services brings the running app to the front rather than starting a second process for the same bundle, and it delivers a file or link to the running app as an Apple Event. The file lock stays as the backstop for a launch that bypasses Launch Services, such as running the binary directly or `open -n`. A second process in that case exits after bringing the first one forward.
- **Windows.** Each launch starts a new process, so ftorrent carries the request itself. The running instance serves a named pipe, `\\.\pipe\ftorrent-` followed by a hash of the lock file's full path. A second launch that finds the lock held connects to that pipe, writes its command-line arguments as one line of JSON, and exits. The running instance brings its window to the front and passes any `.torrent` path or magnet link on to the engine. Tauri's runtime already includes tokio, whose `named_pipe` module provides both ends.

The pipe name comes from the lock path, so each copy only ever hears from launches of itself. A portable copy's pipe is reached only by opening that portable copy again. The server end is set up with three properties:

- **Inbound only.** The pipe carries requests to the running instance and nothing back, so a process that can connect has nothing to read.
- **Local clients only.** tokio refuses clients from other machines by default.
- **First instance.** Pipe names are global across the whole machine, so the running instance asks to create the first instance of its name. If something else already holds the name, creating it fails rather than joining another process's pipe. ftorrent then runs without the handoff, and a second launch of the same copy only exits.

By default a named pipe gives full access to the account that created it, to administrators, and to the system, and read access to everyone else. So when another account already runs the same portable copy, the second person's launch can't open the first person's pipe for writing. It shows that this copy is already running under another account, and exits.

### Building the zip

- **The Mac portable bundle.** `tauri build --config tauri.portable.conf.json --bundles app` applies a second file over `tauri.conf.json` as a JSON merge patch, and the result is also the config compiled into the app. The patch sets `identifier` to `com.ftorrent.portable`. A `null` deletes a key, so `"bundle": {"fileAssociations": null}` removes the file associations and `"plugins": {"deep-link": null}` removes the URL schemes. `--bundles app` builds the `.app` and no disk image.
- **The Windows side.** It's the same `ftorrent.exe` an installed copy runs, taken from the release build with its engine folder, before NSIS wraps them. The same binary serves both modes, and only the folder beside it differs. A portable copy needs the WebView2 runtime, the part of Edge that draws the window. Windows 11 includes it, and nearly every Windows 10 machine already has it. An installed copy's NSIS installer fetches it when it's missing. A portable copy can't install it, so on a machine without it, it shows Tauri's message naming the download and doesn't open. The portable page says so.
- **The portable WebView2 folder.** Tauri points WebView2 at `AppData\Local\com.ftorrent.ftorrent` unless told otherwise. The window moves from `tauri.conf.json` into the startup code, where `WebviewWindowBuilder::data_directory` can name the data folder that startup resolved, whether installed or portable. WebView2 keeps its files in a subfolder of the folder it's given. Microsoft documents the subfolder without naming it, and `EBWebView` is the name observed in practice.
- **Which machine builds what.** Each half has to be built on its own platform. Tauri's Mac bundler only compiles on a Mac. Tauri can cross-compile a Windows executable from a Mac, but it calls that experimental and a last resort, and PyInstaller can't cross-build at all, so the Windows engine is frozen on Windows. Windows builds its half first, and the Mac builds its half and assembles the zip. The Mac is the right place to zip because zip tools on Windows don't record Unix permissions: a `.app` zipped on Windows and unpacked on a Mac would lose the executable bit on its binaries.
- **Getting the Windows half to the Mac.** The repository carries source and records, not build output, so the Windows half travels through the download server instead. On Windows, the upload step packs `ftorrent.exe` and its engine folder into `portable_win.zip`, the Windows portion of the portable build, with `tar -a`, which ships with Windows 10 and later and writes forward slashes. It uploads that zip to `https://ftorrent.com/portable_win.zip`, beside `ftorrent.exe` and `ftorrent.exe.json`, and nothing links to it. The sidecar `pnpm hash` writes for it, `portable_win.zip.json`, is committed and pushed like the others. On the Mac, the assembly step downloads the half and computes its hash. It goes on only if the hash matches the sidecar in the repository. The server is only the courier, and the committed hash is what the Mac trusts.
- **Assembly.** The Mac unpacks the Windows half, adds the portable `.app` and `portable/ftorrent.json`, and zips the folder as `ftorrent.zip`, without macOS metadata (`__MACOSX`, `._` files) and without symlinks.
- **Publishing.** `ftorrent.zip` gets a sidecar from `pnpm hash` and a box on the installing page like the other packages. The page gains a portable section that clears the mark before the first launch.

## Sprint steps

1. **Commit this plan**, so it's versioned and reaches the Windows build machine.
2. **Remove the symlinks from the engine build.** Change `ftorrent-engine.spec` to replace symlinks with the files they point to. Confirm the engine answers `ready` on macOS, that the `.app` holds one copy of each OpenSSL library, and that the Linux containers still build.
3. **Resolve paths.** Build the startup steps in the Rust core, from finding the program's own location through reading `ftorrent.json`, and show the results on the main page. An installed copy on Windows keeps everything in `AppData\Local`, and never writes to Roaming.
4. **Lock the settings folder** with `File::try_lock`.
5. **Hand off a second launch** through the named pipe on Windows, and confirm that Launch Services already covers it on macOS.
6. **Lock the download folders**, and show a folder that's in use.
7. **Set up WebView2 for portable copies**, pointing its profile into `portable/` on Windows.
8. **Build the portable Mac bundle** from `tauri.portable.conf.json`.
9. **Assemble and publish the zip**: the sidecar, and the installing page's portable section.
10. **Run the tests below** on both machines.
11. **Correct the planning document** once, to match what we built.

## Tests

- Launch an installed copy twice, and confirm the second launch brings the first forward and no second window appears.
- Run an installed copy and a portable copy side by side, confirm each keeps its own folder and window, and confirm neither sees the other's pipe or lock.
- Sign in as two users and run each user's installed copy at the same time, on Windows and on macOS.
- On Windows, have two signed-in users launch the same portable copy from one stick, and confirm the second is told it's running under another account.
- Give an installed copy and a portable copy the same download folder, and confirm the second shows it as in use and loads nothing from it.
- Unzip on Windows onto an exFAT stick, carry the stick to the Mac, clear the mark, and run the `.app`. Then do it the other way round.
- Launch a portable Mac copy without clearing the mark, and confirm it explains translocation instead of starting as an installed copy.
- Kill the app outright, relaunch, and confirm no stale lock blocks it.
- After a portable session on each platform, list what the host gained, and check it against the list on the portable page.

## Additional discussion and research

### On Windows, Local or Roaming?

Windows gives each user two application-data folders. `AppData\Roaming` is meant for settings that follow a user between machines when an organization sets up roaming profiles. `AppData\Local` is meant for data tied to one machine, and for anything large or regenerable. ftorrent uses exactly one of them, never a mixture. We chose Local: everything an installed copy keeps for itself goes in `AppData\Local\com.ftorrent.ftorrent`, and ftorrent never writes to Roaming.

We said we'd stay in Roaming if it turned out to be the common default today. It isn't:

- **Microsoft has stopped roaming.** Its documentation for the Windows application-data API says that roaming data and settings are no longer supported as of Windows 11. The folder still exists, but nothing copies it. The enterprise settings sync that replaced it carries Windows settings, not an app's own folder. OneDrive's folder backup covers Desktop, Documents, Pictures, Screenshots, and Camera Roll, never AppData.
- **The torrent clients split.** Transmission keeps its settings and resume data in Local. qBittorrent keeps settings in Roaming and its resume data, the `BT_backup` folder, in Local. Deluge keeps everything in Roaming, and so does µTorrent.
- **Everything-in-Roaming mostly comes from Electron's default.** Electron puts an app's whole data folder in Roaming, and its web cache with it, unless the app says otherwise. VS Code, Discord, and Slack all keep that default.
- **Chrome keeps its whole profile in Local.**
- **Tauri already chose Local for the web view.** Its source forces WebView2's folder to `app_local_data_dir`, and Microsoft's WebView2 guidance says to keep that folder with the rest of an app's data. The NSIS installer already puts the program itself in `AppData\Local\ftorrent`, a different folder from the data folder, so uninstalling the program never touches the data.
- **The data itself belongs on one machine.** The DHT routing table in `state` is this machine's view of the network.

Keeping to one folder takes a little discipline, because a stock Tauri app already writes to both. `app_data_dir` and `app_config_dir` are Roaming on Windows. The store and window-state plugins default to Roaming, and the log plugin writes to Local. So ftorrent resolves its data folder once, from `app_local_data_dir`, and passes that one path to everything that writes. A plugin that picks its own folder is pointed at ours before we adopt it. Tauri has an unreleased option, `appDirectoriesOverride`, that sends every one of those paths to one root, and we'll take it when it ships.

The installed copies built so far write nothing of their own yet, so moving now costs nothing. It's done at the start of the sprint, while paths are resolved for the first time.

### Locks on exFAT, FAT, and network shares

On macOS 15, `flock()` holds on exFAT and FAT32 volumes: a second process trying the lock on a test disk image got `EAGAIN`. On those volumes every file shows as executable and owned by the current user, and an ad-hoc signed `.app` copied onto one verified and launched. On Windows, the FAT driver handles byte-range locks with the kernel's standard lock code. Microsoft publishes that driver as a sample, and it's likely that exFAT's closed driver does the same. What's left for the test matrix is a real USB stick on both platforms, and a download folder on an SMB share. The fallback for a volume that can't lock is to run without the lock and say so, rather than refuse to start.

### Handing a second launch to the running one without a pipe

The named pipe is the common, well-documented choice on Windows. Tauri's single-instance plugin uses a hidden window instead: the second process finds it by class name with `FindWindowW` and sends its arguments with `WM_COPYDATA`. The pipe keeps the handoff out of window management and gives us plain bytes to parse. If it runs into trouble, the window message is the known alternative.
