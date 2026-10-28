# Unified executable

On the desktop, ftorrent is two programs. `ftorrent` is the Tauri app that runs the window. `ftorrent-engine` is Python on libtorrent, which the app starts as a child and talks to over stdin and stdout. Each platform's firewall asks its questions and keeps its rules per program, by path, so two programs mean two questions and two sets of rules. This note parks three ways to make them one program in the firewall's eyes, two that keep Python and one with no Python at all, and says why we might want to.

## Why

On Windows, Defender Firewall raises its security alert the first time a program binds a socket that could take packets unasked, and the rules it writes name that program's path alone. An installed copy has two paths, `%LOCALAPPDATA%\ftorrent\ftorrent.exe` and `%LOCALAPPDATA%\ftorrent\ftorrent-engine\ftorrent-engine.exe`. So it raises two alerts and ends with four rules, one for UDP and one for TCP per program. The alert writes two Block rules for the current profile the moment it appears, before anyone answers, and Allow access turns them into Allow rules under the same name.

Who sees which alert depends on what they do. Installing and launching raise nothing, since in ordinary use the app binds no such socket: it fetches over HTTPS, checks for an update, and keeps its lock and pipe on the machine. The engine's alert comes about a second after the first Start, which is the moment the person asked to share, so it's a question they expect. The app's own alert comes only from the Connection tab, the first time Check binds its test sockets. So a person who never presses Check meets one alert, and a person who does meets two.

The one alert everyone meets is the engine's, and it names a program the person has never heard of. It reads `ftorrent-engine.exe` beside PyInstaller's default Python icon, since `ftorrent-engine.spec` sets no icon and no version resource.

On a Mac the application firewall is off by default. When it's on, it asks per program too: the app on the reply to its UPnP search, and the engine on its first connection in. Its list keeps the two apart, so a development build's engine and an installed copy's are two more entries. The engine's first write into Downloads will raise the folder question about ftorrent-engine rather than ftorrent. On Linux nothing asks.

## Why the engine is its own program

Every idea below keeps two processes, and that's deliberate. A crash or a hang in libtorrent can't take down the window, and the app can start the engine again. The engine runs on the libtorrent maintainers' own Python wheels, built with WebTorrent. One `engine.py` runs on all three platforms. And the engine can be run by hand and fed JSON lines. Folding libtorrent into the window's own process would get rid of the second alert by giving up the first of those, so no idea here does that. The first two ideas load the same prebuilt wheel the engine loads today, and compile nothing in libtorrent. The third builds libtorrent from its C++ source, with Boost, OpenSSL, and WebTorrent, which is the work the wheels spare us today.

## What today costs

Downloading runs the same libtorrent, compiled from the same C++, over the same JSON pipe, in every arrangement here, today's and the three below. Python only drives it, so none of them changes how fast a torrent moves. What they change is the build, what can break on a person's machine, and one detail of the Check.

Today the engine takes two build steps. `pnpm engine` freezes it with PyInstaller into `desktop/engine/dist`, and `pnpm installer` packs whatever is there without building it again. So an installer can ship an engine older than its source, and one did: a Windows release shipped an engine frozen before the engine gained its `tables` command.

On a person's machine, PyInstaller's bootloader is mature. It handles cases a new bootloader would meet one at a time, like unusual characters in the user's folder name, and where Windows looks for the DLLs a program loads. Its known weakness is antivirus false positives, since malware is frozen with PyInstaller too, though none has turned up for ftorrent's engine.

## The engine holds the test sockets

Every socket the Connection tab binds in the app's own process that could take a packet unasked moves into the engine. That's net_stun's socket, the sockets udp.rs holds open for the filtering check, and the socket the UPnP search sends from. Which of the app's other sockets raise the alert is the first thing to measure. They include net_local's, which connects a UDP socket without sending, and the connected sockets NAT-PMP and PCP use toward the gateway. The HTTPS fetches, which only connect out, stay where they are.

The engine gains commands that mirror udp.rs: open a socket, ask STUN from it, send from it, read what has arrived on it, and close it. The engine is one loop over stdin with no threads, so no command can wait four seconds on a socket without stalling the poll that brings libtorrent's alerts. So its sockets are non-blocking, and a read answers whatever has arrived since the last one. The page calls it on its poll, as it calls for alerts, and keeps the clock itself. STUN is small in any language: a 20-byte binding request, and the XOR-MAPPED-ADDRESS read from the answer, which net.rs does in Rust today. UPnP is larger, since igd-next is a Rust crate, so the search either moves to a Python library or stays in Rust, if it turns out not to raise the alert.

What it gains. On Windows, Check and Start together raise one alert, the engine's, at whichever comes first. The filtering check then reads the firewall rule that strangers actually meet, the engine's, where today it reads the app's. So the panel's firewall note on filtering and its note on the inbound proof would be about one program. A narrower version would test the session's own UDP socket, the one peers reach, port and all, which the inbound proof already probes. A filtering check there needs the socket to send to moon first, and libtorrent's DHT direct request sends one DHT message from it to any address, with the answer arriving as an alert. That's worth a look before writing sockets of our own.

What it costs. The lib.rs essay and the good plan keep general network operations in Rust, and make the engine the place where JSON meets libtorrent, one entry per libtorrent call. This puts plain sockets into the engine, and rewrites code already built and tested in Rust. And the one alert still names `ftorrent-engine.exe`.

The build doesn't change, and the code and its tests move from Rust to Python. The engine gets more fragile, since a bug in its socket code can crash or stall the process that holds the session, where a bug in udp.rs today fails one Check. And the Check's timing gets coarser. The engine runs only when a line arrives on stdin, and Python on Windows can't wait on a pipe and a socket at once, since its select takes sockets alone. So a packet sits unread until the page's next poll, about once a second, and the box's arrival times, each datagram's milliseconds from the start of the receive, become times of the poll that read it. A thread in the engine would keep them exact, at the price of the engine's one loop. Nobody would feel the difference, but it's a loss for a measurement tool.

## One executable, two roles, with Python embedded

`ftorrent.exe` started with an argument, `--engine`, runs the engine in place of the window. engine.rs starts the app's own executable with that argument, where it starts `ftorrent-engine.exe` today. Chrome, Edge, VS Code, and Electron apps run this way on Windows: one executable, many processes, each told its role on its command line. The firewall sees one path, so there's one alert, one pair of rules, and both processes are covered by them.

In its engine role, the executable embeds CPython, which is what PyInstaller's bootloader does today. That bootloader is a small C program that loads the Python library, points the interpreter at a bundled standard library, and runs the frozen `engine.py`. Here `ftorrent.exe` becomes its own bootloader, through PyO3, the Rust crate for embedding Python. Beside it ship the Python library, the standard library, libtorrent's extension module and the libraries it loads, and `engine.py`, which stays as it is. So does the road of JSON lines over stdin and stdout.

Five details decide whether it works:

- The window's role shouldn't load Python at all, so the app can't link the Python library the usual way, when the process starts. On Windows it's delay-loaded, and on macOS and Linux it's opened at run time, in the engine's role alone.
- The embedded interpreter has to be the exact minor version libtorrent's wheel was built for, the one uv's lock chooses today.
- The interpreter runs isolated, with no PYTHONPATH, no user site-packages, and the bundled paths alone, as PyInstaller sets it up.
- On a Mac, the engine role runs from the one executable inside the bundle, so it's signed with the app, rather than as a second program inside it.
- The engine role branches at the very top of `main`, before the single-instance lock. Otherwise the engine looks like a second launch, hands its arguments to the running copy, and exits, the way a development build does beside a running installed copy.

What it gains. On Windows the one alert names ftorrent, with ftorrent's icon, and leaves one pair of rules. On a Mac the firewall's list has one entry, and the Downloads question names ftorrent. The engine's PyInstaller icon goes away with PyInstaller, and so does the antivirus risk that comes with a PyInstaller executable. One build makes both roles, so an installer can no longer ship an engine older than its source. There's one binary to sign. And the process boundary stays where it is.

What it costs. The freezing PyInstaller maintains becomes ours: collecting Python's library, its standard library, libtorrent's extension module, and the libraries that module loads, on Windows, the Mac, and both Linux architectures, Flatpak included. `pnpm engine` and `ftorrent-engine.spec` give way to Python's layout inside the app's own build. PyO3 needs the matching Python at build time, delay-loading is set up per platform, and every move to a new Python version touches the Rust build as well. On a person's machine, our bootloader starts out less tested than PyInstaller's, with the cases above to meet. Task Manager lists ftorrent twice, which Chrome has made ordinary. Running the engine by hand becomes `ftorrent.exe --engine`. Speed is the same: the engine starts about as fast as PyInstaller's does, and the two processes even share the executable's code pages in memory, which saves a little and matters not at all. It's a much larger change than the engine holding the test sockets, and nearly all of it is build work.

## One executable, two roles, with libtorrent built in

This is the single executable above, with its engine role written in Rust and libtorrent compiled from its C++ source into `ftorrent.exe`. There's no interpreter, no wheel, and no PyInstaller. The engine still runs as a second process of the same executable, started with `--engine`, so the process boundary stays. So does the road of JSON lines, and the page doesn't change. Linking libtorrent into the window's own process instead would also work, and would give up the boundary, which is why the role is kept. The single executable's details about the single-instance lock and the Mac's signing apply here unchanged.

What we'd build. libtorrent with WebTorrent on, which brings in libdatachannel and the libraries beneath it for WebRTC's ICE, SCTP, and DTLS. Boost, which libtorrent uses mostly as headers. And OpenSSL, for TLS and for DTLS. Each is built on Windows with MSVC, on the Mac for Apple silicon, on Linux for both architectures, and for the Flatpak. Cargo can drive the C++ through CMake, so one `cargo build` makes the whole executable, and the compiled libraries cache in the target folder as the Rust does. A first build, or a change of compiler, builds libtorrent and its libraries again from nothing, which takes minutes. OpenSSL is the usual hard part on Windows, where configuring it wants Perl.

What we'd write. No maintained Rust binding covers what the engine uses, so we'd write a narrow one through the cxx crate, which joins Rust and C++ with types checked on both sides. It covers the session and its settings, reading a torrent file, adding a torrent, popping alerts and reading each kind's fields, the session's counters, and the DHT's routing tables. Then `engine.py`'s dispatch table, and its translation of each alert into JSON, move into Rust beside the binding.

How it compares with embedding Python. Both make a native build of the engine ours, on every platform we ship, so they're close in kind. Embedding Python packages things already compiled: the interpreter, its library, and the wheel's module. This compiles them, and adds the binding and the port, so it's the larger job up front. What it leaves is simpler to live with. There's no interpreter to ship, to isolate, or to keep in step with a wheel's Python version, and no bootloader of our own. On Windows the engine is 34 MB today, mostly libtorrent's module at 13 MB, OpenSSL at 8 MB, and Python's library at 6 MB, with Python's standard library and the C runtime's files around them. Built in, libtorrent and OpenSSL stay, and Python's part goes. With no interpreter to bring up, the engine starts faster and holds less memory, though torrent speed is libtorrent's either way.

What it gains, beyond what the single executable gains. There's no Python at all, so there's no Python version to pin and nothing to freeze. And it brings independence. We'd choose libtorrent's revision and its build options ourselves, WebTorrent among them, where today we take the options the maintainers chose for their wheels, on their release schedule. That's the control over a project's updates the Software Selections essay weighs as heavily as its code.

What it costs, beyond the build. Changing the engine means compiling Rust, where today it means editing `engine.py`. The engine can still be run by hand, as `ftorrent.exe --engine`, and fed JSON lines. A new libtorrent release, or a fix to OpenSSL, reaches people when we rebuild, as it does with a wheel. But nobody else compiles it first, so a compiler or library update that breaks the build is ours to fix.

## Smaller steps either way

Two changes are worth making while the engine is frozen with PyInstaller, whichever road comes after.

`pnpm installer` can run `pnpm engine` first, so every installer freezes the engine from the source it ships with. That closes today's gap between the two build steps, at the cost of the freeze's dozen seconds on every build.

And PyInstaller can stamp the engine with ftorrent's icon and a version resource that describes it as ftorrent. That's a change to `ftorrent-engine.spec` alone. Whether the alert then reads ftorrent rather than the file name takes a test. The development build's rules took the program's description as their name, and the installed copy's took the file name. It leaves two alerts, but both would look like ftorrent.

## What we think

The engine holding the test sockets is the least appealing of the three. It bends the boundary between Rust and the engine, and makes the process that holds the session more fragile. It coarsens the very measurement it exists to serve. And it leaves the one alert ordinary use raises still naming `ftorrent-engine.exe`.

Embedding Python is less work up front, and leaves something odd for good. Most of it is packaging, and what's hard is platform plumbing. On Windows that's where the system looks for DLLs and delay-loading Python's library, and on a Mac it's library paths and signing each library in the bundle. Everywhere, PyO3 has to stay pinned to the wheel's exact Python. None of it is deep, but it rebuilds PyInstaller's job with less testing behind it than PyInstaller has. PyOxidizer was the general tool for exactly this, a Rust executable with Python embedded, and its author stepped back from it in 2024, in a post on shifting open source priorities. That suggests this kind of tool is harder to keep working than to get working. And at the end there's still an interpreter inside a Rust app.

Building libtorrent is harder up front, and hairy in known places. OpenSSL under MSVC on Windows is the classic difficulty. WebTorrent is libtorrent's newest part, and it brings libdatachannel, with libjuice and usrsctp beneath it, a less travelled path than libtorrent alone. Arm64 Linux and the Flatpak each have their own small surprises. What tames it is that the maintainers' wheels already prove the whole tree builds on every platform we ship, WebTorrent included, and their CI recipe is public, so the build follows a recipe rather than inventing one. The binding is tedious rather than hard: libtorrent's alerts are a class hierarchy, so the shim needs a small function for each kind of alert the engine reads.

So if we unify, we'd build libtorrent. The total work is comparable, since both roads make a native build of the engine ours. The native build spends more of it early, on hard problems with known answers. Embedding Python spends less early, and keeps spending on the plumbing, on every Python version, and on the interpreter carried for good. And the native road ends with one executable, nothing frozen, and libtorrent's revision and options our own.

For now, the two programs stay. The two smaller steps cover most of what a person actually sees, and neither touches the architecture.

## Where it stands

Parked, with no idea planned. The two programs stay, and ordinary use raises one alert. If Check's second alert, or the engine's name on the first, becomes the thing to fix, the smaller steps come first, and building libtorrent is the road after them.
