---
title: libtorrent Provenance
description: Where the BitTorrent engine inside the ftorrent desktop client comes from, who built it, how we fetch and verify it, and the hashes we pin.
---

# libtorrent Provenance

The ftorrent desktop client does its BitTorrent and WebTorrent work through [libtorrent](https://www.libtorrent.org/), and this document is the engineering record of that dependency: which libtorrent, built by whom and how, the path it takes onto a build machine, what we checked when it arrived, and the hashes we hold it to. A dependency we bundle into a client is a supply chain we vouch for on behalf of everyone who installs it, so we keep this record in public and update it as the engine changes. The log at the end says when.

## The engine in one paragraph

libtorrent is C++, and its only maintained first-party bindings are Python. So the client carries a small Python program, the engine, frozen together with its interpreter and libtorrent into a folder the app ships beside itself. The app's Rust core starts that process, talks to it over its own pipes in newline-delimited JSON, and stops it when the app quits. The pieces:

- libtorrent 2.1.1, through its Python bindings, with WebTorrent compiled in
- Python 3.13.15, from python-build-standalone, installed and pinned by uv
- PyInstaller 6.22.3, which freezes the three into one folder
- a lockfile, `uv.lock`, that names every platform's artifact by SHA-256

Nothing prebuilt is committed. Each build machine fetches its own pieces by hash and freezes its own engine, and the lockfile is the single record that makes every machine fetch the same things.

## Why we do not build it from source

- libtorrent 2.1.0 shipped on 2026-Jul-09 with WebTorrent support enabled by default.
- libtorrent 2.1.1 followed on 2026-Aug-10, and the maintainers published wheels for it to PyPI the same day.
- Before 2.1, WebTorrent lived only on the development branch behind a build flag, and our plan for the client assumed a from-source build on all three platforms to reach it.

That assumption is now out of date, and letting it go is the whole reason for this document. The methodology we use for every dependency prefers standard packaging over bespoke builds, and a wheel published by the library's own maintainers from the library's own repository is as standard as packaging gets. It also spares us three build pipelines that each had a hard part: Boost from source on Windows, and long emulated compiles in the Linux containers.

The road not taken stays open. libtorrent's repository carries the exact recipe its wheels are built from, and if we ever need a patch, a different OpenSSL, or a build flag the wheels do not set, that recipe is where a source build starts. Until then, we take the maintainers' build and verify it.

## The chain of custody

Three sources, each with its own trust story.

### libtorrent, from its own repository

The wheels are built inside [arvidn/libtorrent](https://github.com/arvidn/libtorrent) by a workflow in that repository, `cibuildwheel.yml`, which runs when a version tag is pushed. It checks the repository out at the tag with submodules recursively, which is how libdatachannel, the WebRTC implementation behind WebTorrent, and its own dependencies arrive. It builds on GitHub-hosted runners, macOS 14 and 15 for Apple Silicon, Windows Server 2022, and Ubuntu 24.04 for the Linux wheels, using cibuildwheel 2.23.4, for Python 3.9 through 3.13. The finished wheels are uploaded to PyPI with an API token held by the two maintainers of the [PyPI project](https://pypi.org/project/libtorrent/), one of whom is libtorrent's author.

What that chain lets us verify, and what it does not. PyPI publishes a SHA-256 digest for every file, our lockfile records the digest of each wheel we use, and uv refuses to install a file whose digest differs. So the bytes on every build machine are exactly the bytes the maintainers uploaded. What we cannot verify is the step between the runner and PyPI: the upload uses a token rather than PyPI's trusted-publishing path, so PyPI holds no attestation binding the wheel to the workflow run that made it. We trust the maintainers and the public workflow for that link, which is the ordinary trust anyone extends to a package they did not build.

### Python, from python-build-standalone

The interpreter comes from [astral-sh/python-build-standalone](https://github.com/astral-sh/python-build-standalone), release 20260807, as the `install_only_stripped` build for each platform. uv downloads it and checks it against a list of hashes compiled into uv itself, so a substituted download fails to install. Two lines pin it: `.python-version` in the engine folder names 3.13.15 exactly, and the manifest sets uv's python preference to managed builds only, so no machine's own Python, whatever version it carries, is ever what the engine is frozen against.

The wheels stop at Python 3.13, which is why the engine does not run on a newer interpreter, and it is also why the choice of interpreter is pinned rather than left to each machine.

### PyInstaller, from PyPI

[PyInstaller](https://pyinstaller.org/) 6.22.3, released 2026-09-12, freezes the engine. Its wheel matters more than a build tool's usually does, because it carries the bootloader, a small precompiled program that becomes the engine's executable and runs on every user's machine. The lockfile pins its wheel per platform along with the handful of pure-Python helpers it depends on, which are build-time only and ship nothing.

### uv, the tool that reads the lock

[uv](https://docs.astral.sh/uv/) performs the fetches and the checks. On the Mac it is 0.12.3 from Homebrew; on Windows and Linux it comes from Astral's installers. It is a build tool and ships nothing to users, so it is not pinned by hash here, but it is the component that enforces every hash above.

## The hashes

These are the artifacts a build machine fetches, in the form `sha256sum` reads. They are also in `uv.lock`, which is the copy that is enforced; this copy is the one a person can read.

The libtorrent wheels, one per platform we ship, all uploaded 2026-08-10:

```
bae8a30fa1b4988881124c76867f8e61f9887a42e55638693d7a1b0038bdebe8  libtorrent-2.1.1-cp313-cp313-macosx_15_0_arm64.whl
fa11aa801d5b9184f067f559c465d3074a4e878d9c671b00dd4930d6dfce2b46  libtorrent-2.1.1-cp313-cp313-win_amd64.whl
c81d768e04915c1627ac108a6abfa64c740bdd1a1634d5fffcdc56ed6e6d915f  libtorrent-2.1.1-cp313-cp313-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
d456d5c772e64267211eca995f5a61cfa9e476de9494b300b42fd6fdf71675ca  libtorrent-2.1.1-cp313-cp313-manylinux_2_17_aarch64.manylinux2014_aarch64.whl
```

The interpreters, from python-build-standalone release 20260807:

```
dbadb0ffe46f8bace50daaf8a0c5fc6903c003690776da9eb5269e33c856bb53  cpython-3.13.15+20260807-aarch64-apple-darwin-install_only_stripped.tar.gz
44bf9ae71f4b45e3ba3104ae331c6eff3f7002593c26fd12453eb9310c4f259a  cpython-3.13.15+20260807-x86_64-pc-windows-msvc-install_only_stripped.tar.gz
faae10a9faa9bec06da009ac69326cc1d9691dc138fec6a1b69159dff1781f35  cpython-3.13.15+20260807-x86_64-unknown-linux-gnu-install_only_stripped.tar.gz
1dfc9565c26f8892a33202b5966bdf9ff45c56a57b06e8fa65fecf05030afe5b  cpython-3.13.15+20260807-aarch64-unknown-linux-gnu-install_only_stripped.tar.gz
```

PyInstaller, uploaded 2026-09-12; the macOS wheel is one universal file:

```
052f4a1cd4f81092ccb7a18fe8ebc9ecf832a5917317e39fb3775af4e959b253  pyinstaller-6.22.3-py3-none-macosx_10_13_universal2.whl
500bd58c7bf7e584a8435adccbd763a0b918d5c12b08d74ff50fd79b2915458b  pyinstaller-6.22.3-py3-none-win_amd64.whl
451a4ae14b719365bf1a2f0a99dae7b3463060061c3a394c70d5264cfb439528  pyinstaller-6.22.3-py3-none-manylinux2014_x86_64.whl
312da84b4b31ab9750066a823734c11b7c68757fdd67c9f038bf19c330a954e4  pyinstaller-6.22.3-py3-none-manylinux2014_aarch64.whl
```

The macOS wheel is tagged for macOS 15, which is the floor the client already sets; a wheel for macOS 14 exists on PyPI and is not the one a build on macOS 15 selects.

## What is inside the wheel

We opened the macOS wheel rather than taking its contents on faith.

- The Python extension module is a single 20 MB file. libtorrent, Boost, and the WebTorrent stack are linked into it statically.
- The WebTorrent stack is there: the module's socket type includes a WebRTC stream, and the symbols of libdatachannel, libjuice, and usrsctp are present.
- The four WebTorrent settings exist: the STUN server, the connection timeout, the minimum WebSocket announce interval, and the offer limit.
- OpenSSL 3.6.3, dated June 2026 in its own version string, rides beside the module as two shared libraries the wheel carries. On Linux and Windows the maintainers' build compiles it in instead; we will confirm that when we build there.

One consequence to hold onto: OpenSSL fixes reach the engine only through new libtorrent wheels. We watch libtorrent's releases for that reason as much as for libtorrent's own fixes.

The release notes for 2.1 say plainly that WebTorrent "significantly widens the attack surface" of libtorrent, and they are right: it adds a WebRTC implementation, a WebSocket client, and SDP parsing to a library that already parses raw packets from the internet. We want it, because a single engine that holds desktop and browser peers in one swarm is the point of this client, and we take it with eyes open: it is a release feature now, fuzz targets for its parsers are part of libtorrent's own testing, and the module carrying it runs in a process of its own, apart from the app's window.

## What we verified

Checked 2026-Sep-21 on an Apple Silicon Mac running macOS 15.7, against the wheel and interpreter named above.

- A session created with the wheel accepted our own STUN server, stun.ftorrent.com, as its WebTorrent STUN setting.
- An announce for a Creative Commons torrent over `wss://open.ftorrent.com`, our WebSocket tracker, got a tracker reply in under a second. So did one to `wss://tracker.webtorrent.dev`, the WebTorrent project's own tracker. A UDP announce to our tracker returned peers, and the torrent's metadata arrived within five seconds. libtorrent's WebTorrent and our Aquatic WebSocket tracker speak to each other.
- The frozen engine folder is 45 MB, freezes in under five seconds, and its executable is ad-hoc signed, which is what Apple Silicon requires to run it at all. Run by hand, it answers an init line with its ready line: libtorrent 2.1.1.0, WebTorrent on, Python 3.13.15, frozen.
- Started by the app, it reports the same over the pipe, and quitting the app takes it down with no process left behind.

Windows and Linux are pinned in the lockfile and not yet built. Their entries in the log below will say what we found.

## Checking it yourself

The lockfile is the record and uv is the check, so a fresh clone verifies everything above by installing it:

```
cd desktop/engine
uv sync --frozen
uv run --frozen python -c "import libtorrent as lt; s = lt.default_settings(); print(lt.version, 'webtorrent' if 'webtorrent_stun_server' in s else 'no webtorrent')"
```

To compare a wheel against this page by hand, download it without installing and hash it:

```
uv run --frozen pip download libtorrent==2.1.1 --no-deps --only-binary :all: -d /tmp/wheel
shasum -a 256 /tmp/wheel/libtorrent-*.whl
```

## Log

- **2026-Sep-21.** First record. libtorrent 2.1.1 from the maintainers' wheels, Python 3.13.15 from python-build-standalone 20260807, PyInstaller 6.22.3. Verified and frozen on macOS; Windows and Linux pinned, not yet built.
