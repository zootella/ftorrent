---
title: Desktop Architecture
description: How the ftorrent desktop client works under the hood — the processes it runs as, the files it is made of and writes, and the road its parts talk over.
---

# Desktop Architecture

The ftorrent desktop client is a [Tauri](https://tauri.app/) application: a Vue page in the operating system's own web view, a Rust core beneath it, and beside them a second process, the engine, that holds [libtorrent](https://www.libtorrent.org/). This document opens the hood. It shows the client three ways, as the processes it runs as, as the files it is made of and the files it writes, and as the road its parts talk over, and it explains each part plainly as it comes up. A last section steps back and says how the parts relate and why they are shaped the way they are.

Everything here is measured rather than reasoned from the design, on an Apple Silicon Mac running macOS 15 in September 2026, with the Linux layout taken from the built package and the Windows layout from an install on Windows 10 the following day. The client is early: the engine starts, reports itself, learns its download folders, and stops, and creates no libtorrent session yet. What this document describes is the structure that the torrent work will ride on.

## The processes

- A running client is five processes on macOS.
- One is the app. Three belong to the platform's web view. One is the engine.
- The engine is the app's child, and the only process the project itself adds.

<svg viewBox="0 0 680 340" width="100%" role="img" aria-label="The five processes of a running ftorrent desktop client" style="font: 12px var(--vp-font-family-mono); max-width: 680px; display: block; margin: 1.5rem auto;">
	<defs>
		<marker id="arrow-processes" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse">
			<path d="M 0 0 L 10 5 L 0 10 z" fill="currentColor"/>
		</marker>
	</defs>
	<rect x="20" y="20" width="310" height="92" rx="8" style="fill: var(--vp-c-bg-soft); stroke: currentColor; stroke-width: 1.5"/>
	<text x="36" y="44" style="font-size: 13px; font-weight: 700">ftorrent</text>
	<text x="36" y="66">Rust core, the window, Tauri's runtime</text>
	<text x="36" y="88" style="fill: var(--vp-c-text-2)">24 threads, 66 MB</text>
	<rect x="360" y="12" width="300" height="136" rx="8" style="fill: none; stroke: currentColor; stroke-width: 1; stroke-dasharray: 4 3"/>
	<text x="376" y="32" style="fill: var(--vp-c-text-2)">spawned by the platform for the window</text>
	<rect x="376" y="42" width="268" height="28" rx="6" style="fill: var(--vp-c-bg-soft); stroke: currentColor"/>
	<text x="388" y="61">WebContent</text>
	<text x="490" y="61" style="fill: var(--vp-c-text-2)">runs the page</text>
	<rect x="376" y="76" width="268" height="28" rx="6" style="fill: var(--vp-c-bg-soft); stroke: currentColor"/>
	<text x="388" y="95">GPU</text>
	<text x="490" y="95" style="fill: var(--vp-c-text-2)">draws it</text>
	<rect x="376" y="110" width="268" height="28" rx="6" style="fill: var(--vp-c-bg-soft); stroke: currentColor"/>
	<text x="388" y="129">Networking</text>
	<text x="490" y="129" style="fill: var(--vp-c-text-2)">fetches for it</text>
	<line x1="330" y1="66" x2="360" y2="66" style="stroke: currentColor; stroke-dasharray: 4 3"/>
	<rect x="20" y="210" width="310" height="110" rx="8" style="fill: var(--vp-c-bg-soft); stroke: currentColor; stroke-width: 1.5"/>
	<text x="36" y="234" style="font-size: 13px; font-weight: 700">ftorrent-engine</text>
	<text x="36" y="256">the launcher, the Python interpreter,</text>
	<text x="36" y="274">engine.py, and the libtorrent module</text>
	<text x="36" y="298" style="fill: var(--vp-c-text-2)">1 thread while idle, 20 MB</text>
	<line x1="100" y1="112" x2="100" y2="208" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-processes)"/>
	<text x="112" y="152">spawns it and holds</text>
	<text x="112" y="170">its three pipes</text>
	<text x="360" y="262" style="fill: var(--vp-c-text-2)">libtorrent adds its own threads</text>
	<text x="360" y="280" style="fill: var(--vp-c-text-2)">once a session exists</text>
</svg>

**The app** is the process named `ftorrent`. It is the compiled Rust program: Tauri's runtime, the code that creates the window, the commands the page can call, and the module that starts and stops the engine. It owns the window, and it is the parent of the engine. On the Mac it runs about two dozen threads, most of them Tauri's and the async runtime's.

**The web view's helpers** are three processes the operating system starts when the app creates its window, and they belong to the platform's web engine rather than to the project. macOS renders every web view outside the app that owns it, for isolation, so the page itself runs in a content process, a second process draws it, and a third would fetch for it if the page fetched anything. Activity Monitor lists them with the app's name in front, and they end when the window does. The other platforms do the same with different names: WebView2 on Windows starts several helper processes of its own, and WebKitGTK on Linux starts a web process and a network process.

**The engine** is the process named `ftorrent-engine`, and it is one process holding four things. A small native launcher is the executable itself. It loads the Python interpreter, the whole of CPython, from a library file beside it. The interpreter runs our program, a short Python file that reads lines from its input, calls libtorrent, and writes lines to its output. And the interpreter loads libtorrent as a module: one compiled file that is the C++ library, with the WebTorrent stack linked in, wrapped in the binding code that lets Python call it. libtorrent is a library, not a program, so there is no libtorrent process; there is this one, and libtorrent runs inside it. While idle it has a single thread. When it creates a libtorrent session it will gain libtorrent's own threads, for the network, for disk reads and writes, and for hashing, while the count of processes stays the same.

The engine is the only process the project adds to what a plain Tauri app already is, and it is entirely ours: our program, running the build of libtorrent we chose, started by our Rust, and reachable by nothing else on the machine.

## The files

- The engine is a folder, not a single executable: a launcher beside a directory named `_internal`.
- The launcher finds `_internal` beside itself, never by an absolute path and never by the working directory.
- The app finds the engine folder by asking Tauri for its resource directory, which is a different place on each platform.

<svg viewBox="0 0 680 360" width="100%" role="img" aria-label="The files the engine is made of, and how the app and the launcher find them" style="font: 12px var(--vp-font-family-mono); max-width: 680px; display: block; margin: 1.5rem auto;">
	<defs>
		<marker id="arrow-files" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse">
			<path d="M 0 0 L 10 5 L 0 10 z" fill="currentColor"/>
		</marker>
	</defs>
	<rect x="20" y="20" width="250" height="40" rx="8" style="fill: var(--vp-c-bg-soft); stroke: currentColor; stroke-width: 1.5"/>
	<text x="36" y="45" style="font-size: 13px; font-weight: 700">the app's executable</text>
	<text x="380" y="30" style="fill: var(--vp-c-text-2)">the resource directory is</text>
	<text x="380" y="50" style="fill: var(--vp-c-text-2)">macOS    Contents/Resources</text>
	<text x="380" y="68" style="fill: var(--vp-c-text-2)">Linux    /usr/lib/ftorrent</text>
	<text x="380" y="86" style="fill: var(--vp-c-text-2)">Windows  beside the .exe</text>
	<line x1="145" y1="60" x2="145" y2="108" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-files)"/>
	<text x="157" y="80">asks Tauri for its resource</text>
	<text x="157" y="98">directory, joins ftorrent-engine/</text>
	<rect x="20" y="110" width="640" height="232" rx="8" style="fill: none; stroke: currentColor; stroke-width: 1.5"/>
	<text x="36" y="134" style="font-size: 13px; font-weight: 700">ftorrent-engine/</text>
	<text x="200" y="134" style="fill: var(--vp-c-text-2)">one folder, moved as a unit</text>
	<rect x="40" y="150" width="220" height="56" rx="6" style="fill: var(--vp-c-bg-soft); stroke: currentColor"/>
	<text x="56" y="172" style="font-weight: 700">ftorrent-engine</text>
	<text x="56" y="192" style="fill: var(--vp-c-text-2)">the launcher</text>
	<text x="40" y="228">looks beside itself</text>
	<text x="40" y="246">for _internal</text>
	<line x1="260" y1="178" x2="298" y2="178" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-files)"/>
	<rect x="300" y="150" width="340" height="176" rx="6" style="fill: var(--vp-c-bg-soft); stroke: currentColor"/>
	<text x="316" y="172" style="font-weight: 700">_internal/</text>
	<text x="316" y="198">libpython3.13</text>
	<text x="456" y="198" style="fill: var(--vp-c-text-2)">the interpreter</text>
	<text x="316" y="218">base_library.zip</text>
	<text x="456" y="218" style="fill: var(--vp-c-text-2)">standard library</text>
	<text x="316" y="238">libtorrent/</text>
	<text x="456" y="238" style="fill: var(--vp-c-text-2)">the C++ module</text>
	<text x="316" y="258">libssl, libcrypto</text>
	<text x="456" y="258" style="fill: var(--vp-c-text-2)">OpenSSL, macOS only</text>
	<text x="316" y="300" style="fill: var(--vp-c-text-2)">what our program's bytecode runs on,</text>
	<text x="316" y="318" style="fill: var(--vp-c-text-2)">carried inside the launcher itself</text>
</svg>

The engine is frozen. That is PyInstaller's word for turning a Python program into something that runs without Python installed: a copy of the interpreter, the parts of the standard library the program imports, the packages it uses, and a small native launcher, gathered into one folder that never changes after it is made. Our program's own bytecode travels inside the launcher. The folder for the Mac comes to 45 MB, and its largest pieces are the libtorrent module at 20 MB and the interpreter at 17 MB.

The launcher starts by asking the operating system for the path of its own executable. It takes that executable's folder and looks there for `_internal`. From that one anchor it loads the interpreter library, points the interpreter at the bundled standard library and at `_internal` for modules, and runs our program. Nothing is resolved from the working directory and nothing is configured, so the folder works from any location as long as it moves intact. A copy that scatters the contents of `_internal` beside the launcher does not work, because the launcher finds nothing where it looks.

The app finds the folder the same way, from an anchor rather than a setting. It asks Tauri for its resource directory and joins `ftorrent-engine/` to it. The resource directory is inside the application bundle on macOS, beside the executable on Windows, under the application's own directory in `/usr/lib` on Linux, and next to the debug binary during development, where the build tool copies the folder so the same code path finds it there. The folder name is the same everywhere, so the app builds the path once and never branches on platform.

On macOS, installed, everything is inside the `.app`, which is a folder the Finder shows as one item:

```
/Applications/ftorrent.app/Contents/MacOS/ftorrent                                the app
/Applications/ftorrent.app/Contents/Resources/ftorrent-engine/ftorrent-engine     the launcher
/Applications/ftorrent.app/Contents/Resources/ftorrent-engine/_internal/libpython3.13.dylib
/Applications/ftorrent.app/Contents/Resources/ftorrent-engine/_internal/base_library.zip
/Applications/ftorrent.app/Contents/Resources/ftorrent-engine/_internal/libtorrent/__init__.cpython-313-darwin.so
/Applications/ftorrent.app/Contents/Resources/ftorrent-engine/_internal/libssl.3.dylib
/Applications/ftorrent.app/Contents/Resources/ftorrent-engine/_internal/libcrypto.3.dylib
```

On Linux, installed from the `.deb` or the `.rpm`, the app goes where programs go and the engine where a program's private files go:

```
/usr/bin/ftorrent                                                      the app
/usr/lib/ftorrent/ftorrent-engine/ftorrent-engine                      the launcher
/usr/lib/ftorrent/ftorrent-engine/_internal/libpython3.13.so.1.0
/usr/lib/ftorrent/ftorrent-engine/_internal/libstdc++.so.6
/usr/lib/ftorrent/ftorrent-engine/_internal/libgcc_s.so.1
/usr/lib/ftorrent/ftorrent-engine/_internal/base_library.zip
/usr/lib/ftorrent/ftorrent-engine/_internal/libtorrent/__init__.cpython-313-x86_64-linux-gnu.so
```

The Flatpak carries the same layout under `/app` instead of `/usr`, and the same rule finds it: the app looks for `lib/ftorrent` beside its own `bin`, wherever that is. There are no OpenSSL files on Linux because the Linux build of libtorrent compiles OpenSSL into the module. On Windows, installed, everything sits under the user's own profile, the engine folder beside the app's executable, measured from an install on Windows 10 in September 2026:

```
C:\Users\username\AppData\Local\ftorrent\ftorrent.exe                                the app
C:\Users\username\AppData\Local\ftorrent\uninstall.exe                               the uninstaller
C:\Users\username\AppData\Local\ftorrent\ftorrent-engine\ftorrent-engine.exe         the launcher
C:\Users\username\AppData\Local\ftorrent\ftorrent-engine\_internal\python313.dll
C:\Users\username\AppData\Local\ftorrent\ftorrent-engine\_internal\libcrypto-3-x64.dll
C:\Users\username\AppData\Local\ftorrent\ftorrent-engine\_internal\base_library.zip
C:\Users\username\AppData\Local\ftorrent\ftorrent-engine\_internal\libtorrent\__init__.cp313-win_amd64.pyd
```

The Windows build of libtorrent compiles OpenSSL in as well, so the `libcrypto` library there is not libtorrent's. It belongs to the interpreter, which needs it for the standard library's hashlib module, and PyInstaller collects it for that reason. The rest of `_internal` on Windows is the interpreter's extension modules, the C++ runtime, and some forty small Windows API forwarding libraries the interpreter ships, which together bring the folder to 33 MB, the smallest of the three platforms because the module is 13 MB there.

## The files it writes

- The app keeps its own files in one data folder: the signed-in user's local application data for an installed copy, and a folder named `portable` beside the program for a portable one.
- A copy is portable when `portable/ftorrent.toml` exists beside the program, and installed in every other case.
- Each download folder holds a hidden `.ftorrent` folder for the session data of the torrents in it.
- The engine writes nothing yet.

The data folder of an installed copy is named by the app's identifier rather than its product name, and on Windows it's under Local rather than Roaming, so nothing the client keeps follows a user between machines on a domain:

```
~/Library/Application Support/com.ftorrent.ftorrent/ftorrent.toml                     the settings
~/Library/Application Support/com.ftorrent.ftorrent/ftorrent.lock                     held while this copy runs

C:\Users\username\AppData\Local\com.ftorrent.ftorrent\ftorrent.toml
C:\Users\username\AppData\Local\com.ftorrent.ftorrent\ftorrent.lock
C:\Users\username\AppData\Local\com.ftorrent.ftorrent\EBWebView\                    WebView2's profile
```

A portable copy keeps the same files in `portable/` beside the program, and on Windows the app points WebView2's profile there too. `ftorrent.toml` is the settings, a TOML file the page reads at startup and writes whole when a setting changes, every setting under a comment explaining it. `ftorrent.lock` is how a copy knows it is the one running: the app takes an exclusive lock on it at startup, and the operating system releases the lock however the process ends. The state file, where libtorrent will save its session and the DHT's routing table, is named there too, `state`, and appears once the engine has a session to save.

A download folder, `~/Downloads/ftorrent` unless the settings say otherwise, gets a `.ftorrent` folder inside it the first time the client prepares it, hidden on Windows with the file attribute and on the other platforms by its leading dot. While a copy uses the folder it holds an exclusive lock on `.ftorrent/ftorrent.lock`, so two copies, such as an installed one and a portable one, never load the same torrents. The session data of each torrent will live in `.ftorrent` beside the files it describes, so a download folder moved to another machine or opened by a portable copy carries its torrents with it.

## The communication

- Two hops: the page talks to the Rust core, and the Rust core talks to the engine.
- Down, the page builds each command as a line of JSON, and the Rust core passes it to the engine without reading it.
- Up, the engine writes a line whenever it has something to say, the Rust core puts it on a queue without reading it, and the page takes everything waiting four times a second.
- Every message on the pipe is one line of JSON, and the Rust core reads none of them.

<svg viewBox="0 0 680 400" width="100%" role="img" aria-label="The road down from the page to the engine, and the road up from the engine to the page" style="font: 12px var(--vp-font-family-mono); max-width: 680px; display: block; margin: 1.5rem auto;">
	<defs>
		<marker id="arrow-channel" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse">
			<path d="M 0 0 L 10 5 L 0 10 z" fill="currentColor"/>
		</marker>
	</defs>
	<text x="200" y="22" text-anchor="middle" style="font-size: 13px; font-weight: 700">the road down</text>
	<text x="480" y="22" text-anchor="middle" style="font-size: 13px; font-weight: 700">the road up</text>
	<rect x="140" y="40" width="400" height="44" rx="8" style="fill: var(--vp-c-bg-soft); stroke: currentColor; stroke-width: 1.5"/>
	<text x="156" y="67" style="font-size: 13px; font-weight: 700">the page</text>
	<text x="250" y="67" style="fill: var(--vp-c-text-2)">Vue, in the web view's process</text>
	<line x1="200" y1="86" x2="200" y2="188" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-channel)"/>
	<text x="188" y="120" text-anchor="end">engine_send:</text>
	<text x="188" y="138" text-anchor="end">one line of JSON</text>
	<text x="188" y="156" text-anchor="end">the page built</text>
	<line x1="480" y1="188" x2="480" y2="86" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-channel)"/>
	<text x="492" y="120">engine_take: every</text>
	<text x="492" y="138">line waiting, taken</text>
	<text x="492" y="156">four times a second</text>
	<rect x="140" y="190" width="400" height="44" rx="8" style="fill: var(--vp-c-bg-soft); stroke: currentColor; stroke-width: 1.5"/>
	<text x="156" y="217" style="font-size: 13px; font-weight: 700">the Rust core</text>
	<text x="290" y="217" style="fill: var(--vp-c-text-2)">in the app's process, reads neither</text>
	<line x1="200" y1="236" x2="200" y2="338" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-channel)"/>
	<text x="188" y="270" text-anchor="end">stdin: the same</text>
	<text x="188" y="288" text-anchor="end">line, passed on</text>
	<line x1="480" y1="338" x2="480" y2="236" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-channel)"/>
	<text x="492" y="262">stdout: one JSON line</text>
	<text x="492" y="280">per event, onto a queue</text>
	<text x="492" y="306" style="fill: var(--vp-c-text-2)">stderr: text, kept</text>
	<rect x="140" y="340" width="400" height="44" rx="8" style="fill: var(--vp-c-bg-soft); stroke: currentColor; stroke-width: 1.5"/>
	<text x="156" y="367" style="font-size: 13px; font-weight: 700">the engine</text>
	<text x="260" y="367" style="fill: var(--vp-c-text-2)">Python and libtorrent, its own process</text>
</svg>

### The road down

A click, or anything else the page decides to do, starts in Vue. When it's something for the engine, the page builds the command as a JavaScript object, turns it into one line of JSON, and calls `invoke('engine_send', {line})`. Tauri carries that across its IPC channel from the web view's process into the app's process and runs the Rust function registered under that name. The function checks one thing, that the text holds no line break, since a line break would make it two messages, puts a newline on the end, and queues it for a writer thread of its own, which feeds the engine's standard input in order. It never parses the line, so the Rust core has no idea whether it's a command the engine knows.

The writer thread is there because of a cycle. The engine handles one message at a time and blocks writing its answer until someone reads it, and the thread that reads its answers takes the same lock a command takes to reach the engine. If a command wrote straight into a full pipe while holding that lock, it would wait on the engine, the engine would wait on the reader, and the reader would wait on the lock, for good. And a Tauri command that isn't async runs on the app's main thread, so the window would freeze with it. With the writing on its own thread, `engine_send` holds the lock only long enough to hand the line over, and returns at once, whatever the pipe is doing.

The engine reads its input line by line. It parses each line and looks the `command` field up in its dispatch table, one entry per command, each a short Python function that translates the message into libtorrent's own terms and makes the call. The table has two entries today, `init` and `folders`, and the loop itself handles `quit`. Two lines take a different road, because they're about the process rather than the conversation: the Rust core writes `init` itself, the moment it has started the engine and before the page exists, carrying the product name, the version, and the paths of the data folder and state file; and it writes `quit` when the app is closing.

### The road up

The road up begins in the engine. When it has something to report, it writes one line to its standard output: a JSON object with an `event` field, ending in a newline, flushed at once so nothing waits in a buffer. Today it reports `ready` in answer to `init`, `folders` in answer to `folders`, repeating what it kept, and `error` for a line it could not read or a command it does not know. Later it will report what libtorrent reports: progress, peers, tracker replies, alerts.

In the app's process, a thread does nothing but read the engine's output. It sits blocked on the pipe, wakes the moment a line arrives, and pushes the line as text onto a queue, without parsing it. That thread is also how the app learns the engine has died: the pipe ends, the read returns nothing more, and the thread records the exit. A second thread does the same for the engine's standard error, keeping the last hundred lines in memory so a failure has something to show.

The queue is drained. The page calls `engine_take` and receives every line waiting, oldest first, and the queue is left empty, so the page sees each line exactly once. A store in the page does this four times a second, for the life of the app rather than for any one screen, parses each line, and keeps what the page cares about; every component reads the store. The queue holds at most a thousand lines, so a page that stops taking can't make memory grow without end: a line past the cap pushes the oldest one out, the next take reports how many were dropped, and the page shows the count rather than losing lines silently.

The same shape carries one more thing up. When a second launch of a running copy hands over a file or link it was opened with, or this copy starts with one on its command line, the request goes on a second drained queue, and the same store takes it on the same timer.

### What travels on the wire

The pipe carries newline-delimited JSON in both directions: one object per line, UTF-8, terminated by a newline, written whole and flushed. Going down, the object has a `command` field. Coming up, it has an `event` field. A line the engine cannot parse gets an `error` event back rather than a crash, and a line coming up that isn't JSON is passed over by the page. The Tauri hop above it also speaks JSON, serialized by Tauri on both sides, with the page's command names fixed at compile time in the Rust core's handler list.

## How the parts relate

With each part described on its own, the relationships are short to state.

**The page decides and the Rust core carries out.** The Rust core's commands are general: send a line, take the lines waiting, read a file, lock a file, write a registry value. None of them names a torrent, a setting, or a feature, and each could belong to any desktop application. What a line means, which folder to lock, whether a value should be written, all of that is the page's. So one idea lives in one place, and the Rust core grows only for what the page can't do: work only the operating system can do, and work that has to happen before the page exists, like starting the engine and taking the instance lock.

**One road, two edits.** Because the Rust core reads neither direction, a libtorrent call the client hasn't used before is two edits: the page that sends the command, and the entry in the engine's dispatch table that makes the call. Pausing a torrent will be `{"command":"pause",…}` from the page and one entry calling `handle.pause()`; libtorrent's answer comes up as an alert the engine already turns into a line. A Rust command per engine message would have been six edits across four layers for the same thing, and would rebuild libtorrent's surface in every layer, one feature at a time.

**Up is a stream the page drains.** The engine speaks whenever it has something to say, and a Rust thread hears it at once, but the page is the one that asks. The Rust core could push each line to the page as a Tauri event instead. It doesn't, because an event emitted before the page is listening is lost, and the engine's first line comes before the page exists; because a queue lets the page take a burst of lines in one call; and because a take the page makes on its own schedule means the Rust core never needs to know whether a page is there.

**Every anchor is a location, not a setting.** The launcher finds `_internal` beside itself; the app finds the engine folder under a directory Tauri names for the platform. Nothing is configured, so nothing can be misconfigured, and the same code runs on all three platforms and in development.

**The road is private because of what it is made of.** The pipes are unnamed pipes, the kind `pipe(2)` makes on Unix and Windows calls anonymous pipes: the Rust core creates them when it starts the engine, and the engine inherits them as that one child. They have no filename and no port, so no other process on the machine can open, read, or write them. A local network server, the usual alternative, would bind a port that any local process or any browser tab could reach. The engine trusts its input, because only the app can write it, and the app writes only what its own page built or what the Rust core itself sends. The page treats the engine's output as data: it parses JSON and stores values, and renders those values through Vue's text interpolation, so a torrent name that someday carries markup arrives on screen as text.

**Only the Rust core can start a process.** No shell plugin is registered, so nothing the page can call spawns anything, and the engine is started from a path the Rust core computes. The list of commands the page can call is the handler list in the Rust core, fixed when the app is compiled. The content security policy keeps foreign script out of the page that could otherwise call them, and the page runs only its own code, since untrusted text, like file names, torrent metadata, and what peers send, reaches it only through Vue's escaping interpolation. That's why the commands carry no guards of their own: the power they hold stays in the page's hands, and the page is where the knowledge of what's right to do lives. Those facts together are the client's attack surface, and each is small enough to read.

**The engine is a process, not a library, for reasons that reinforce each other.** libtorrent's only maintained bindings are Python, so the bridge had to be a Python program, and a Python program has to be its own process. That shape also gives the client crash isolation, since a fault inside libtorrent cannot take the window down; a clean restart, since the Rust core can start a new engine without restarting itself; and the private road above, since a parent and its child are the two ends of a pipe. Had a perfect Rust binding existed, running libtorrent inside the app's own process would have looked simpler and been worse.

**The engine is a folder, not a single file, for a reason that belongs to one platform and costs nothing on the others.** PyInstaller can also produce one self-extracting executable that unpacks itself into a temporary directory at every launch. That shape is what antivirus heuristics on Windows most often flag, and the folder is not: it unpacks nothing and looks like the ordinary program it is. The folder also starts faster, and it is the same shape everywhere, so the Rust core has one path to build.

**The web view's helpers are not part of this design.** They come with the platform's web engine and would exist for any Tauri app. The one process the project adds is the engine, and everything in this document about how ftorrent's parts talk is about that process, its pipes, and the page that drives it.

## What is not built yet

- The engine creates no libtorrent session and opens no sockets. It starts, answers `init` and `folders`, and stops.
- The engine's captured stderr reaches the page in the engine's status, and the main page doesn't show it yet.
- The page lists the requests that reach a running copy and does nothing more with them; adding a torrent will.
- The portable build isn't made yet, so a portable copy keeping everything in `portable/` is the design rather than a measurement.
