
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
> <br>[Flatpak](https://flatpak.org/) runtime: GNOME 49
> <br>[Fedora](https://fedoraproject.org/): 40, for the rpm check

The desktop client's Linux packages, built on a Mac through Docker. The client is developed on a Mac and tested on a Windows box, and its Linux users are somebody else's machines, so the Linux packages have to come from somewhere. This folder is that somewhere: three containers, no Linux computer, and nothing to check out on a third machine and remember to keep current. Four packages come out: a `.deb` for each of two architectures, an `.rpm`, and a Flatpak, which between them reach the desktop Linux distributions people run today, from a Raspberry Pi to a Steam Deck. `build.js` carries the reasoning; this guide is how you use it.

## Before you start

Install [Docker Desktop](https://www.docker.com/products/docker-desktop/) and leave it running. The `docker` command is only a client, so if Docker Desktop is quit everything here fails with "cannot connect to the Docker daemon". `docker desktop status` says whether it is up, and `docker desktop start` starts it without opening the window.

In its settings, turn on **Use Rosetta for x86/amd64 emulation**. The x86-64 package builds in an emulated container, and Rosetta is several times faster than the alternative.

Then, from the repository root, `pnpm install` once; it installs every workspace. This one has no dependencies of its own, because everything it does is drive containers whose toolchains live inside them. It is a workspace nested inside the desktop one, so its commands are typed from here, and most days you are in `desktop` and come down to `linux` only to make a release.

## The commands

```
cd desktop/linux
pnpm build           # the four packages: images, staging, three builds, three checks
pnpm hash            # stage them under their published names and write the sidecars
pnpm upload          # send each package and then its sidecar to ftorrent.com
```

That is the whole of it, and it is the same shape as the desktop workspace's: one word builds, `hash` stages, `upload` sends. Three steps rather than one, because each leaves behind a different kind of thing: `build` writes packages, which are gitignored and disposable; `hash` writes sidecars, which are committed; `upload` puts files on a server, the one step you cannot take back, which is why it stays separate.

Five commands sit underneath so `build` is factored rather than one long function: `build-images` for the three toolchain images alone; `build-distro` for the two `.deb` files and the `.rpm`, out of the two Tauri runs; `build-flatpak` for the Flatpak, which needs the x86-64 `.deb` to exist first; `stage` for the whitelist copy a container is handed; and `check` for the bare-image tests on packages already built. You would rarely type one, and `build-distro` then `build-flatpak` is what `build` does between its images and its checks.

The first `pnpm build` on a new machine takes far longer than the rest, because it builds the three toolchain images before it builds anything else: Debian carrying Node, Rust, uv with the engine's Python, and the WebKit headers, once per architecture, and a Flatpak image holding the GNOME runtime and SDK, which is about five gigabytes on its own. Every build after that rebuilds the images too, and that is the answer to a question the design would otherwise have no answer to: how would you know the toolchain had moved? You could not. Somebody bumps a version in the Dockerfile, you pull it, and nothing tells you. Docker's layer cache makes checking free, a few seconds when nothing changed, so the build simply brings its own images up to date and uses them. It is not called `prepare`, which was the obvious name and is a trap: npm and pnpm treat `prepare` as a lifecycle script and run it on every install.

## Where everything goes

Four places, and the engine is frozen in the middle. The Flatpak takes a fifth, described after them.

**Source goes in.** Staging copies four things out of the repository into `linux/.stage/`: the root `package.json`, `pnpm-workspace.yaml`, `pnpm-lock.yaml`, and the desktop folder. Nothing else. Not `.git`, not the private notes beside the workspaces, not `node_modules`, not `src-tauri/target`, not this folder itself. The list is stated positively, a whitelist rather than a set of exclusions, because a whitelist fails closed. The copy is about 1.5 MB, and it is mounted **read-only** into the container, so a build can never write back into your working tree.

**The engine is frozen inside.** The container runs `pnpm install --frozen-lockfile`, then `pnpm engine`, which is `uv sync --frozen` and PyInstaller exactly as on the Mac: uv fetches the libtorrent wheel and PyInstaller by the hashes in `engine/uv.lock`, on the Python the image already holds, and PyInstaller freezes them into a folder. Before any Rust is compiled, the container runs that frozen engine once and stops if it does not answer. Then `pnpm tauri build --bundles deb`, or `deb,rpm` in the x86-64 container, builds the app and bundles it, the engine folder riding along as a resource; the `.rpm` costs nothing beyond the `.deb` because both come out of the one compile.

**Packages come out.** Each container writes to `linux/release/`, under the filename Tauri chose, or the Flatpak script's:

```
linux/release/ftorrent_0.1.0_arm64.deb        from build-distro    arm64 container, native
linux/release/ftorrent_0.1.0_amd64.deb        from build-distro    amd64 container, emulated
linux/release/ftorrent-0.1.0-1.x86_64.rpm     from build-distro    the same amd64 container, the same run
linux/release/ftorrent_0.1.0_x86_64.flatpak   from build-flatpak   wraps the amd64 .deb above
```

The `0.1.0` is read from `src-tauri/tauri.conf.json`, the one place the client's version is written.

**`pnpm hash` stages them under their published names** and writes a sidecar beside each, a small JSON file holding the filename, version, architecture, byte count, SHA-256, and build date:

```
linux/release/ftorrent.arm64.deb         from ftorrent_0.1.0_arm64.deb         with ftorrent.arm64.deb.json
linux/release/ftorrent.amd64.deb         from ftorrent_0.1.0_amd64.deb         with ftorrent.amd64.deb.json
linux/release/ftorrent.x86_64.rpm        from ftorrent-0.1.0-1.x86_64.rpm      with ftorrent.x86_64.rpm.json
linux/release/ftorrent.x86_64.flatpak    from ftorrent_0.1.0_x86_64.flatpak    with ftorrent.x86_64.flatpak.json
```

Every Linux name states its architecture and none carries a version, the rule `scripts.js` in the desktop workspace explains: a stable name is overwritten in place on every release, so a link anyone shares keeps handing people the current build, and the arm64 package is not given the bare name that would make it read as the ordinary choice. The architecture token is each ecosystem's own word, `amd64` for Debian and `x86_64` for RPM and Flatpak, because a Debian user and a Fedora user each expect their own, and the two formats with one build today still state it so no name changes when an aarch64 Flatpak or an ARM rpm turns up. The packages are gitignored and the sidecars are committed; so is nothing under `.stage/`.

**The Flatpak is wrapped, not compiled.** A third container takes the x86-64 `.deb`, unpacks it, and lays the same files out under `/app` instead of `/usr`: the app in `bin`, the engine folder whole under `lib/ftorrent`, and the desktop entry and icons renamed for the application id, `com.ftorrent.ftorrent`, which is what a Flatpak names everything by. The app finds the engine there by the same rule as in the `.deb`, `lib/ftorrent` beside its own `bin`. Then it grants the sandbox what a torrent client needs and no more, the display, the network, and the Downloads folder, and exports a single-file bundle. `inside-flatpak.sh` carries the whole account, including why it does not use flatpak-builder.

**The checks run last.** For each `.deb`, a container made from a bare `debian:12-slim`, and for the `.rpm` one made from a bare `fedora:40`, with no toolchain and nothing of ours installed, unpacks it rather than installing it into the system, reports what it declares it depends on and where the app and the engine landed, and runs the frozen engine from there. That proves the one thing a build container cannot: that the engine runs on a system with only glibc and the base libraries, which is the promise the manylinux wheel and python-build-standalone both make. Fedora 40 is the oldest Fedora the glibc floor reaches, so the rpm check reads that floor from the newer end as the Debian check reads it from the older. Neither installs the package into the system with its dependencies, because that would pull WebKitGTK and the desktop stack into a container with no display, and neither can show a window. The Flatpak gets a lighter check inside its own wrap, the engine run from the laid-out tree, because its sandbox cannot start under Rosetta. The app itself is smoke tested on a real Linux machine, the way it is on the other two.

## Why it is built this way

**The pipe flushes clean.** An image is the toolchain and holds no source and no secrets. A container is one build: it is handed the source, makes a package, and is destroyed. Nothing carries between builds. These packages are made a few times a year, not in a daily loop, so a known starting state is worth more than speed.

**Three lockfiles are what make "clean" mean something.** `pnpm-lock.yaml`, `Cargo.lock`, and the engine's `uv.lock` are all in the whitelist and all enforced, so the JavaScript, Rust, Python, and libtorrent that land are the versions the Mac and the Windows box already build with. Without them a fresh container would resolve every range against whatever the registries served that morning.

**The base image is `debian:12-slim`, and that is a decision rather than a default.** A binary is compatible with its build machine's glibc and every later one, never an earlier one, so the base sets a floor on who can run the result. Debian 12's glibc 2.36 reaches Debian 12 and 13, Ubuntu 24.04 LTS and 26.04, Mint 22, Fedora 40 and up, and both current generations of Raspberry Pi OS. Debian 13 would move that floor to 2.41 and shut out the current Ubuntu LTS. Reaching back costs nothing at the newer end.

**Rust is pinned** in `src-tauri/rust-toolchain.toml`, which governs these containers and the Mac and Windows builds alike, so the three cannot drift apart. The Dockerfile installs that same version into the image so the download sits in the durable layer; the toolchain file still governs, and if the two ever disagree the build slows down rather than compiling wrong. **Python is pinned** the same way: `engine/.python-version` governs, and the Dockerfile installs that version so it is already there.

**Two architectures from one file.** arm64 runs native on an Apple Silicon Mac and builds first, so anything wrong with the image, the whitelist, a lockfile, or the engine surfaces in a couple of minutes rather than twenty; amd64 runs emulated and makes the packages most Linux users want.

**Four packages, and who each is for.**

| Package | Reaches | Published as |
|---|---|---|
| `.deb` arm64 | Raspberry Pi OS and other ARM Debian machines | `ftorrent.arm64.deb` |
| `.deb` amd64 | Debian, Ubuntu, Mint, Pop!_OS, Zorin on x86-64, the largest single audience | `ftorrent.amd64.deb` |
| `.rpm` x86_64 | Fedora, RHEL, Rocky, AlmaLinux | `ftorrent.x86_64.rpm` |
| `.flatpak` x86_64 | any distribution, sandboxed; the only one that installs on SteamOS and Bazzite | `ftorrent.x86_64.flatpak` |

A Flatpak is less another package format than a different bargain: the application ships with its libraries and runs in a sandbox, so it does not care what the host has installed. That is what makes it the one package here that reaches SteamOS and Bazzite, whose root filesystems are read-only or atomic and where a `.deb` or an `.rpm` cannot be installed at all. Arch users can build from source.

## Things that will confuse you once

**Editing an `inside-*.sh` script takes effect immediately.** Those are mounted into the container at run time, not baked into the image, so there is no image to rebuild. Editing `Dockerfile.tauri` needs no action either: the next `pnpm build` rebuilds the image on its own.

**The amd64 build is the slow one.** Apple Silicon runs arm64 natively and emulates x86-64, so the same steps take several times longer the second time through. Nothing is wrong.

**The Flatpak is not built with `flatpak-builder`.** Bubblewrap installs a seccomp filter, and Rosetta rejects that call, so the usual manifest route cannot build an x86-64 Flatpak on this Mac at all. Since the work is unpacking a `.deb` and placing files rather than compiling, `inside-flatpak.sh` uses `flatpak build-init` and ordinary shell instead, which needs no sandbox. Its comments carry the whole account.

**The Flatpak container runs `--privileged`**, for one narrow reason: `flatpak build-export` validates the icon inside bubblewrap, and that needs a namespace an ordinary container cannot make.

**The Flatpak cannot be run here.** The same Rosetta limit that rules out flatpak-builder rules out `flatpak run`, so the bundle is built and its engine is checked from the laid-out tree, and the app inside the sandbox is checked on a real x86-64 Linux machine.

**The rpm check installs into an empty root.** The bare Fedora image carries `rpm` but not `cpio`, so rather than `rpm2cpio` the check installs the package into a root of its own with dependencies and scripts skipped, which is rpm's own way of laying files out and fetches nothing.

**The engine's freeze stops the build if it fails**, and it does so before the Rust compile, on purpose: the engine is the cheaper half, and a wheel that will not import or an interpreter that will not start is worth finding in the first minute. The line above the failure says what the engine answered, or did not.

**`pnpm upload` checks before it sends.** It compares each sidecar against the package beside it, and refuses the whole upload if any sidecar is stale, before it reads the destination; then it sends each package and then its sidecar, package first so a page never fetches a hash for a file still arriving. They land at the site's root under the same names every release, `ftorrent.com/ftorrent.amd64.deb` and so on. The destination and key come from `upload.hide.env` in the desktop workspace, which is gitignored; the comment above `upload` in `scripts.js` lists its five values, for a reader pointing this at a server of their own.

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

We ran the two-package pipeline on an Apple Silicon Mac on 2026-Sep-21, starting from no images at all. That first run took thirteen minutes end to end: two toolchain images built from scratch, then both `.deb` files, then both checks. The next run, with the images cached, took under eight minutes, most of it the emulated amd64 compile.

The four-package pipeline ran the next day, 2026-Sep-22, in ten minutes and seven seconds end to end: three images checked against the cache in seconds, the two Tauri runs, the Flatpak wrap, and the three checks. One note on that time: the Flatpak image's runtime layers were already in Docker's cache on this Mac, so this run did not pay for the five-gigabyte download a new machine would. The three images come to about 11.4 GB on disk, of which the Flatpak image is 5.5 GB.

| Package | Size | Declares |
|---|---|---|
| `ftorrent_0.1.0_arm64.deb` | 24.4 MB | `libwebkit2gtk-4.1-0`, `libgtk-3-0` |
| `ftorrent_0.1.0_amd64.deb` | 25.9 MB | `libwebkit2gtk-4.1-0`, `libgtk-3-0` |
| `ftorrent-0.1.0-1.x86_64.rpm` | 25.9 MB | `libwebkit2gtk-4.1.so.0`, `libgtk-3.so.0` |
| `ftorrent_0.1.0_x86_64.flatpak` | 18.5 MB | the GNOME 49 runtime, `org.gnome.Platform` |

Tauri puts the app at `/usr/bin/ftorrent` and the engine folder at `/usr/lib/ftorrent/ftorrent-engine/` in the `.deb` and the `.rpm` alike, with the executable bit intact through both bundlers, which is what `resource_dir()` resolves to on Linux. In a bare `debian:12-slim` on each architecture, and in a bare `fedora:40` for the rpm, the frozen engine answered its init line with libtorrent 2.1.1.0, WebTorrent on, Python 3.13.15, frozen, and exited cleanly. The Flatpak's engine, run from the laid-out `/app` tree before export, answered the same. The desktop entry in the `.deb` reads `Categories=Network;FileTransfer;P2P;` from the template, and the Flatpak exported that entry and three icons under `com.ftorrent.ftorrent`. The Linux libtorrent wheel carries its OpenSSL compiled into the module rather than as separate libraries, and at an older version than the macOS wheel's; the libtorrent provenance document on [docs.ftorrent.com](https://docs.ftorrent.com/) records which.

The first upload followed later that day: `pnpm hash` and `pnpm upload` sent the four packages and their sidecars to ftorrent.com, and each package, downloaded back from the site, hashed to the sidecar committed in the repository.

What remains unverified is the window itself, on every package. Installing a package and opening the app needs a Linux machine with a display, and the Flatpak in particular needs one, since its sandbox cannot start on this Mac; that is where the app gets smoke tested, the way it is on the other two platforms.
