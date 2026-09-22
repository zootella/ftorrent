
_ftorrent/desktop/linux/README.md_

# Building the Desktop Client for Linux

> Prepared by [Claude Code](https://claude.ai/code) using Fable 5.1
> <br>Created: 2026-Sep
> <br>Last reviewed: 2026-Sep
> <br>[Docker Engine](https://docs.docker.com/engine/): 29
> <br>[Debian](https://www.debian.org/): 12
> <br>[Tauri](https://tauri.app/): 2.11
> <br>[Rust](https://www.rust-lang.org/): 1.98
> <br>Node: 22
> <br>[uv](https://docs.astral.sh/uv/): 0.12
> <br>[Python](https://www.python.org/): 3.13

The desktop client's Linux package, built on a Mac through Docker. The client is developed on a Mac and tested on a Windows box, and its Linux users are somebody else's machines, so the Linux package has to come from somewhere. This folder is that somewhere: two containers, no Linux computer, and nothing to check out on a third machine and remember to keep current. `build.js` carries the reasoning; this guide is how you use it.

## Before you start

Install [Docker Desktop](https://www.docker.com/products/docker-desktop/) and leave it running. The `docker` command is only a client, so if Docker Desktop is quit everything here fails with "cannot connect to the Docker daemon". `docker desktop status` says whether it is up, and `docker desktop start` starts it without opening the window.

In its settings, turn on **Use Rosetta for x86/amd64 emulation**. The x86-64 package builds in an emulated container, and Rosetta is several times faster than the alternative.

Then, from the repository root, `pnpm install` once; it installs every workspace. This one has no dependencies of its own, because everything it does is drive containers whose toolchains live inside them. It is a workspace nested inside the desktop one, so its commands are typed from here, and most days you are in `desktop` and come down to `linux` only to make a release.

## The commands

```
cd desktop/linux
pnpm build           # the two packages: images, staging, both builds, both checks
pnpm hash            # stage them under their published names and write the sidecars
pnpm upload          # send them to ftorrent.com; a stub until the server has a downloads directory
```

That is the whole of it, and it is the same three words the desktop workspace uses for its own installer. Three steps rather than one, because each leaves behind a different kind of thing: `build` writes packages, which are gitignored and disposable; `hash` writes sidecars, which are committed; `upload` will put files on a server, the one step you cannot take back, which is why it stays separate.

Three commands sit underneath so `build` is factored rather than one long function: `build-images` for the two toolchain images alone, `stage` for the whitelist copy a container is handed, and `check` for the bare-Debian test on packages already built. You would rarely type one.

The first `pnpm build` on a new machine takes far longer than the rest, because it builds the two toolchain images before it builds anything else: Debian carrying Node, Rust, uv with the engine's Python, and the WebKit headers, once per architecture. Every build after that rebuilds the images too, and that is the answer to a question the design would otherwise have no answer to: how would you know the toolchain had moved? You could not. Somebody bumps a version in the Dockerfile, you pull it, and nothing tells you. Docker's layer cache makes checking free, a few seconds when nothing changed, so the build simply brings its own images up to date and uses them. It is not called `prepare`, which was the obvious name and is a trap: npm and pnpm treat `prepare` as a lifecycle script and run it on every install.

## Where everything goes

Four places, and the engine is frozen in the middle.

**Source goes in.** Staging copies four things out of the repository into `linux/.stage/`: the root `package.json`, `pnpm-workspace.yaml`, `pnpm-lock.yaml`, and the desktop folder. Nothing else. Not `.git`, not the private notes beside the workspaces, not `node_modules`, not `src-tauri/target`, not this folder itself. The list is stated positively, a whitelist rather than a set of exclusions, because a whitelist fails closed. The copy is about 1.5 MB, and it is mounted **read-only** into the container, so a build can never write back into your working tree.

**The engine is frozen inside.** The container runs `pnpm install --frozen-lockfile`, then `pnpm engine`, which is `uv sync --frozen` and PyInstaller exactly as on the Mac: uv fetches the libtorrent wheel and PyInstaller by the hashes in `engine/uv.lock`, on the Python the image already holds, and PyInstaller freezes them into a folder. Before any Rust is compiled, the container runs that frozen engine once and stops if it does not answer. Then `pnpm tauri build --bundles deb` builds the app and bundles it, the engine folder riding along as a resource.

**Packages come out.** Each container writes its `.deb` to `linux/release/`, under the filename Tauri chose:

```
linux/release/ftorrent_0.1.0_arm64.deb     built native
linux/release/ftorrent_0.1.0_amd64.deb     built emulated
```

The `0.1.0` is read from `src-tauri/tauri.conf.json`, the one place the client's version is written.

**`pnpm hash` stages them under their published names** and writes a sidecar beside each, a small JSON file holding the filename, version, architecture, byte count, SHA-256, and build date:

```
linux/release/ftorrent.arm64.deb    from ftorrent_0.1.0_arm64.deb    with ftorrent.arm64.deb.json
linux/release/ftorrent.amd64.deb    from ftorrent_0.1.0_amd64.deb    with ftorrent.amd64.deb.json
```

Every Linux name states its architecture and none carries a version, the rule `scripts.js` in the desktop workspace explains: a stable name is overwritten in place on every release, so a link anyone shares keeps handing people the current build, and the arm64 package is not given the bare name that would make it read as the ordinary choice. The packages are gitignored and the sidecars are committed; so is nothing under `.stage/`.

**The check runs last.** For each package, a container made from a bare `debian:12-slim`, with no toolchain and nothing of ours installed, unpacks it without installing it, reports what it declares it depends on and where the app and the engine landed, and runs the frozen engine from there. That proves the one thing a build container cannot: that the engine runs on a system with only glibc and the base libraries, which is the promise the manylinux wheel and python-build-standalone both make. It does not install the package, because that would pull WebKitGTK and the desktop stack into a container with no display, and it cannot show a window. The app itself is smoke tested on a real Linux machine, the way it is on the other two.

## Why it is built this way

**The pipe flushes clean.** An image is the toolchain and holds no source and no secrets. A container is one build: it is handed the source, makes a package, and is destroyed. Nothing carries between builds. These packages are made a few times a year, not in a daily loop, so a known starting state is worth more than speed.

**Three lockfiles are what make "clean" mean something.** `pnpm-lock.yaml`, `Cargo.lock`, and the engine's `uv.lock` are all in the whitelist and all enforced, so the JavaScript, Rust, Python, and libtorrent that land are the versions the Mac and the Windows box already build with. Without them a fresh container would resolve every range against whatever the registries served that morning.

**The base image is `debian:12-slim`, and that is a decision rather than a default.** A binary is compatible with its build machine's glibc and every later one, never an earlier one, so the base sets a floor on who can run the result. Debian 12's glibc 2.36 reaches Debian 12 and 13, Ubuntu 24.04 LTS and 26.04, Mint 22, Fedora 40 and up, and both current generations of Raspberry Pi OS. Debian 13 would move that floor to 2.41 and shut out the current Ubuntu LTS. Reaching back costs nothing at the newer end.

**Rust is pinned** in `src-tauri/rust-toolchain.toml`, which governs these containers and the Mac and Windows builds alike, so the three cannot drift apart. The Dockerfile installs that same version into the image so the download sits in the durable layer; the toolchain file still governs, and if the two ever disagree the build slows down rather than compiling wrong. **Python is pinned** the same way: `engine/.python-version` governs, and the Dockerfile installs that version so it is already there.

**Two architectures from one file.** arm64 runs native on an Apple Silicon Mac and builds first, so anything wrong with the image, the whitelist, a lockfile, or the engine surfaces in a couple of minutes rather than twenty; amd64 runs emulated and is the package most Linux users want.

## Things that will confuse you once

**Editing an `inside-*.sh` script takes effect immediately.** Those are mounted into the container at run time, not baked into the image, so there is no image to rebuild. Editing `Dockerfile.tauri` needs no action either: the next `pnpm build` rebuilds the image on its own.

**The amd64 build is the slow one.** Apple Silicon runs arm64 natively and emulates x86-64, so the same steps take several times longer the second time through. Nothing is wrong.

**The engine's freeze stops the build if it fails**, and it does so before the Rust compile, on purpose: the engine is the cheaper half, and a wheel that will not import or an interpreter that will not start is worth finding in the first minute. The line above the failure says what the engine answered, or did not.

**`pnpm upload` sends nothing yet.** It runs every check the real one will, compares each sidecar against the package beside it, and then says what it would send, because ftorrent.com has no downloads directory for the client and no account that writes one. Standing those up is server-side work; when it lands, the command sends each package and then its sidecar, package first so a page never fetches a hash for a file still arriving.

## Building on Linux instead

You can clone this repository on Ubuntu or Raspberry Pi OS and build the client for the machine you are sitting at. That is the desktop workspace, not this folder: install Tauri's Linux prerequisites and [uv](https://docs.astral.sh/uv/), then

```bash
pnpm install
cd desktop
pnpm engine
pnpm installer
```

`pnpm engine` has to come first, for the reason the desktop guide gives: the Rust build carries the frozen engine as a resource and refuses to start without it.

## Verified

We ran the whole pipeline on an Apple Silicon Mac in September 2026, starting from no images at all. The first run took thirteen minutes end to end: two toolchain images built from scratch, then both packages, then both checks. The next run, with the images cached, took under eight minutes, most of it the emulated amd64 compile.

| Package | Size | Unpacked | Declares |
|---|---|---|---|
| `ftorrent_0.1.0_arm64.deb` | 24.4 MB | about 75 MB | `libwebkit2gtk-4.1-0`, `libgtk-3-0` |
| `ftorrent_0.1.0_amd64.deb` | 25.9 MB | about 75 MB | `libwebkit2gtk-4.1-0`, `libgtk-3-0` |

Tauri puts the app at `/usr/bin/ftorrent` and the engine folder at `/usr/lib/ftorrent/ftorrent-engine/`, with the executable bit intact through the bundler, which is what `resource_dir()` resolves to on Linux. In a bare `debian:12-slim` on each architecture, the frozen engine answered its init line with libtorrent 2.1.1.0, WebTorrent on, Python 3.13.15, frozen, and exited cleanly. The Linux libtorrent wheel carries its OpenSSL compiled into the module rather than as separate libraries, and at an older version than the macOS wheel's; the libtorrent provenance document on [docs.ftorrent.com](https://docs.ftorrent.com/) records which.

What remains unverified is the window itself. Installing the package and opening the app needs a Linux machine with a display, and that is where the app gets smoke tested, the way it is on the other two platforms.
