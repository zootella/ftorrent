---
title: Desktop Architecture
description: How the ftorrent desktop client works under the hood — the processes it runs as, the files it is made of, and the channel its parts talk over.
---

# Desktop Architecture

The ftorrent desktop client is a [Tauri](https://tauri.app/) application: a Vue page in the operating system's own web view, a Rust core beneath it, and beside them a second process, the engine, that holds [libtorrent](https://www.libtorrent.org/). This document opens the hood. It shows the client three ways, as the processes it runs as, as the files it is made of, and as the channel its parts talk over, and it explains each part plainly as it comes up. A last section steps back and says how the parts relate and why they are shaped the way they are.

Everything here is measured rather than reasoned from the design, on an Apple Silicon Mac running macOS 15 in September 2026, with the Linux layout taken from the built package. The client is early: the engine starts, reports itself, and stops, and creates no libtorrent session yet. What this document describes is the structure that the torrent work will ride on.

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
	<text x="316" y="258">libtorrent.dylibs/</text>
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
/Applications/ftorrent.app/Contents/Resources/ftorrent-engine/_internal/libtorrent.dylibs/libssl.3.dylib
/Applications/ftorrent.app/Contents/Resources/ftorrent-engine/_internal/libtorrent.dylibs/libcrypto.3.dylib
```

On Linux, installed from the `.deb`, the app goes where programs go and the engine where a program's private files go:

```
/usr/bin/ftorrent                                                      the app
/usr/lib/ftorrent/ftorrent-engine/ftorrent-engine                      the launcher
/usr/lib/ftorrent/ftorrent-engine/_internal/libpython3.13.so.1.0
/usr/lib/ftorrent/ftorrent-engine/_internal/libstdc++.so.6
/usr/lib/ftorrent/ftorrent-engine/_internal/libgcc_s.so.1
/usr/lib/ftorrent/ftorrent-engine/_internal/base_library.zip
/usr/lib/ftorrent/ftorrent-engine/_internal/libtorrent/__init__.cpython-313-x86_64-linux-gnu.so
```

There are no OpenSSL files on Linux because the Linux build of libtorrent compiles OpenSSL into the module. On Windows, installed, the layout is expected to sit under the user's own profile, with the engine folder beside the app's executable; that layout is pinned in the lockfile and not yet built, and this page will say what was found.

```
C:\Users\username\AppData\Local\ftorrent\ftorrent.exe                                the app
C:\Users\username\AppData\Local\ftorrent\ftorrent-engine\ftorrent-engine.exe         the launcher
C:\Users\username\AppData\Local\ftorrent\ftorrent-engine\_internal\python313.dll
C:\Users\username\AppData\Local\ftorrent\ftorrent-engine\_internal\libtorrent\__init__.cp313-win_amd64.pyd
```

None of these are files the engine writes. It writes nothing yet. The settings file, libtorrent's saved state, and the per-download session folders arrive with later work, in the per-user locations the planning document lays out.

## The communication

- Two hops: the page talks to the Rust core, and the Rust core talks to the engine.
- Down is a request: a click becomes a command the Rust core runs, which can become a line the engine reads.
- Up is a stream: the engine writes a line whenever it has something to say, a Rust thread reads it as it arrives, and Rust can hand it to the page as an event.
- Every message on the pipe is one line of JSON.

<svg viewBox="0 0 680 400" width="100%" role="img" aria-label="The path down from the page to the engine, and the path up from the engine to the page" style="font: 12px var(--vp-font-family-mono); max-width: 680px; display: block; margin: 1.5rem auto;">
	<defs>
		<marker id="arrow-channel" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse">
			<path d="M 0 0 L 10 5 L 0 10 z" fill="currentColor"/>
		</marker>
	</defs>
	<text x="200" y="22" text-anchor="middle" style="font-size: 13px; font-weight: 700">the path down</text>
	<text x="480" y="22" text-anchor="middle" style="font-size: 13px; font-weight: 700">the path up</text>
	<rect x="140" y="40" width="400" height="44" rx="8" style="fill: var(--vp-c-bg-soft); stroke: currentColor; stroke-width: 1.5"/>
	<text x="156" y="67" style="font-size: 13px; font-weight: 700">the page</text>
	<text x="250" y="67" style="fill: var(--vp-c-text-2)">Vue, in the web view's process</text>
	<line x1="200" y1="86" x2="200" y2="188" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-channel)"/>
	<text x="188" y="120" text-anchor="end">invoke: a request,</text>
	<text x="188" y="138" text-anchor="end">JSON arguments down,</text>
	<text x="188" y="156" text-anchor="end">JSON result back</text>
	<line x1="480" y1="188" x2="480" y2="86" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-channel)"/>
	<text x="492" y="120">an event, emitted</text>
	<text x="492" y="138">whenever Rust decides,</text>
	<text x="492" y="156">JSON payload</text>
	<rect x="140" y="190" width="400" height="44" rx="8" style="fill: var(--vp-c-bg-soft); stroke: currentColor; stroke-width: 1.5"/>
	<text x="156" y="217" style="font-size: 13px; font-weight: 700">the Rust core</text>
	<text x="290" y="217" style="fill: var(--vp-c-text-2)">in the app's process</text>
	<line x1="200" y1="236" x2="200" y2="338" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-channel)"/>
	<text x="188" y="270" text-anchor="end">stdin: one JSON line</text>
	<text x="188" y="288" text-anchor="end">per command</text>
	<line x1="480" y1="338" x2="480" y2="236" style="stroke: currentColor; stroke-width: 1.5" marker-end="url(#arrow-channel)"/>
	<text x="492" y="262">stdout: one JSON line</text>
	<text x="492" y="280">per event, as it happens</text>
	<text x="492" y="306" style="fill: var(--vp-c-text-2)">stderr: text, kept</text>
	<rect x="140" y="340" width="400" height="44" rx="8" style="fill: var(--vp-c-bg-soft); stroke: currentColor; stroke-width: 1.5"/>
	<text x="156" y="367" style="font-size: 13px; font-weight: 700">the engine</text>
	<text x="260" y="367" style="fill: var(--vp-c-text-2)">Python and libtorrent, its own process</text>
</svg>

### The path down

A click, or anything else the page decides to do, starts in Vue. The page calls `invoke` with the name of a command and a JSON object of arguments. Tauri carries that across its IPC channel from the web view's process into the app's process, finds the one Rust function registered under that name, deserializes the arguments, and runs it. When the function returns, Tauri serializes the result as JSON and sends it back, and the page's promise resolves with it. So the first hop is a request with an answer, and the page is the one that begins it.

When a command needs the engine, the Rust function writes one line to the engine's standard input: a JSON object with a `command` field, ending in a newline. The engine's program is reading its input line by line, so it takes the line, parses it, and makes the matching libtorrent call. That is the whole second hop downward. Today the only lines that travel this way are `init`, which the Rust core writes once as soon as it has started the engine, and `quit`.

### The path up

The path up begins in the engine. When it has something to report, it writes one line to its standard output: a JSON object with an `event` field, ending in a newline, flushed at once so nothing waits in a buffer. Today it reports `ready` in answer to `init`, and `error` for a line it could not read or a command it does not know. Later it will report what libtorrent reports: progress, peers, tracker replies, alerts.

In the app's process, a thread does nothing but read the engine's output. It sits blocked on the pipe and wakes the moment a line arrives, parses the JSON, and records what it says. That thread is also how the app learns the engine has died: the pipe ends, the read returns nothing more, and the thread records the exit. A second thread does the same for the engine's standard error, keeping the last hundred lines in memory so a failure has something to show.

From the Rust core, the last hop up is Tauri's event system. Rust calls `emit` with an event name and a JSON payload, and any listener the page has registered for that name runs with it. The page does not have to have asked. Today the page does not use this yet: the main page asks for the engine's status once a second through an ordinary command, because that was the smallest thing that showed the engine was there. The planned shape is the event: a store in the page listens once, for the life of the app, and every component reads the store.

### What travels on the wire

The pipe carries newline-delimited JSON in both directions: one object per line, UTF-8, terminated by a newline, written whole and flushed. Going down, the object has a `command` field. Coming up, it has an `event` field. A line the engine cannot parse gets an `error` event back rather than a crash, and a line the Rust core cannot parse is dropped. The Tauri hop above it also speaks JSON, serialized by Tauri on both sides, with the page's command names fixed at compile time in the Rust core's handler list.

## How the parts relate

With each part described on its own, the relationships are short to state.

**Down is a request and up is a stream.** The page asks and gets an answer; the engine speaks and is heard. A request has a natural end, which is why `invoke` returns a promise. A stream has none, which is why the Rust core reads the engine on a thread that never returns while the engine lives. Neither direction polls at the mechanism level. The one place the client polls today, the status line on the main page, is a placeholder for an event listener, not a limit of the channel.

**Every anchor is a location, not a setting.** The launcher finds `_internal` beside itself; the app finds the engine folder under a directory Tauri names for the platform. Nothing is configured, so nothing can be misconfigured, and the same code runs on all three platforms and in development.

**The channel is private because of what it is made of.** The pipes are unnamed pipes, the kind `pipe(2)` makes on Unix and Windows calls anonymous pipes: the Rust core creates them when it starts the engine, and the engine inherits them as that one child. They have no filename and no port, so no other process on the machine can open, read, or write them. A local network server, the usual alternative, would bind a port that any local process or any browser tab could reach. The engine trusts its input completely, because only the Rust core can write it. The Rust core treats the engine's output as data: it parses JSON and stores values, and the page renders those values through Vue's text interpolation, so a torrent name that someday carries markup arrives on screen as text.

**Only the Rust core can start a process.** No shell plugin is registered, so nothing the page can call spawns anything, and the engine is started from a path the Rust core computes. The list of commands the page can call is the handler list in the Rust core, fixed when the app is compiled. The content security policy keeps foreign script out of the page that could otherwise call them. Those three facts together are the client's attack surface, and each is small enough to read.

**The engine is a process, not a library, for reasons that reinforce each other.** libtorrent's only maintained bindings are Python, so the bridge had to be a Python program, and a Python program has to be its own process. That shape also gives the client crash isolation, since a fault inside libtorrent cannot take the window down; a clean restart, since the Rust core can start a new engine without restarting itself; and the private channel above, since a parent and its child are the two ends of a pipe. Had a perfect Rust binding existed, running libtorrent inside the app's own process would have looked simpler and been worse.

**The engine is a folder, not a single file, for a reason that belongs to one platform and costs nothing on the others.** PyInstaller can also produce one self-extracting executable that unpacks itself into a temporary directory at every launch. That shape is what antivirus heuristics on Windows most often flag, and the folder is not: it unpacks nothing and looks like the ordinary program it is. The folder also starts faster, and it is the same shape everywhere, so the Rust core has one path to build.

**The web view's helpers are not part of this design.** They come with the platform's web engine and would exist for any Tauri app. The one process the project adds is the engine, and everything in this document that is about ftorrent rather than about Tauri is about that process and its two pipes.

## What is not built yet

- The engine creates no libtorrent session and opens no sockets. It starts, answers `init`, and stops.
- The page polls for the engine's status. The event path up is available and not yet used.
- The engine's captured stderr reaches the Rust core and not yet the page.
- Windows is pinned and not yet built; its file layout above is expected, not measured.
