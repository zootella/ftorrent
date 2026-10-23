#!/usr/bin/env python3
"""
Wrap the x86-64 .deb into a .flatpak bundle. Handed the deb's filename, which carries the version and the architecture, so nothing here has to be told either.

## Why this does not use flatpak-builder

The usual way to make a Flatpak is a YAML manifest handed to flatpak-builder, which runs every build step inside a bubblewrap sandbox. That is right when a manifest compiles something. Ours does not: the Tauri container already built the app and froze the engine, and every step here is unpacking an archive and putting files where a Flatpak expects them. A sandbox around a copy earns nothing.

It also could not run. bubblewrap installs a seccomp filter, and under Rosetta, which is how an amd64 container executes on an Apple Silicon Mac, that call fails and the build stops. So the commands below are the other way: build-init lays out a tree, this script puts files in it, and build-export and build-bundle do the OSTree work. None of them needs a sandbox, so the same script runs native and emulated alike.

The container still runs --privileged, for one narrow reason: build-export validates the icon it is exporting, and that check runs in bubblewrap, which needs a namespace an ordinary container cannot make. That call installs no seccomp filter, which is why it survives Rosetta where flatpak-builder's does not.

Submitting to Flathub would want a manifest, because that is what their build farm reads. It does not exist yet and is not needed to make the bundle; this script is the specification for it when the day comes, and the permissions below are exactly what a manifest's finish-args would say.
"""
import re
import shutil
import subprocess
import sys
from pathlib import Path

deb = sys.argv[1] if len(sys.argv) > 1 else sys.exit('say which deb to wrap')
work = Path('/work')
out = Path('/out')

#The application identifier, and the second place it is written: tauri.conf.json holds the first, and this container never sees that file. Nothing checks that the two agree, so change them together; a Flatpak built under one id and a desktop entry naming another is a bundle that installs and does not appear. The form is what Flatpak requires: three components, our domain reversed, and the application's own name last
identifier = 'com.ftorrent.ftorrent'

def say(text): print(text, flush=True)
def run(arguments, **options): subprocess.run([str(a) for a in arguments], cwd=work, check=True, **options)
def read(arguments): return subprocess.run([str(a) for a in arguments], check=True, capture_output=True, text=True).stdout
def install(source, target, mode):  #one file into the tree, its parents made and its mode set, as install -D does
	target.parent.mkdir(parents=True, exist_ok=True)
	shutil.copyfile(source, target)
	target.chmod(mode)

#Ask the image which runtime it holds rather than naming one here. The Dockerfile's ARG is the single say, and this reads what it did, so bumping the version there cannot leave this script building against a runtime that is not installed
installed = [line.split() for line in read(['flatpak', 'list', '--columns=application,branch']).splitlines()]
runtime_version = next((branch for application, branch in installed if application == 'org.gnome.Platform'), '')
if not runtime_version: sys.exit('no org.gnome.Platform installed in this image; rebuild it with pnpm build-images')
say(f'==> building against the GNOME {runtime_version} runtime')

#Tauri writes ftorrent_<version>_<arch>.deb, and Flatpak names architectures differently from Debian
named = re.fullmatch(r'ftorrent_([0-9][0-9.]*)_(.*)\.deb', deb)
if not named: sys.exit(f'{deb} is not named the way tauri names a deb')
version, debian_arch = named.groups()
flatpak_arch = {'amd64': 'x86_64', 'arm64': 'aarch64'}.get(debian_arch)
if not flatpak_arch: sys.exit(f"no flatpak architecture known for '{debian_arch}'")

shutil.rmtree(work, ignore_errors=True)
work.mkdir()
shutil.copy2(out / deb, work / 'ftorrent.deb')

say('==> unpacking the deb')
run(['ar', 'x', 'ftorrent.deb'])
run(['tar', '-xf', 'data.tar.gz'])

say('==> build-init')
run(['flatpak', 'build-init', 'build', identifier, 'org.gnome.Sdk', 'org.gnome.Platform', runtime_version])

#The layout mirrors the deb's under /app instead of /usr, and that is not a convenience: Tauri finds its resources at ../lib/<name> beside the executable, so with the app at /app/bin/ftorrent it looks in /app/lib/ftorrent, the same rule that finds /usr/lib/ftorrent from /usr/bin. The engine folder is copied whole with its modes and links kept; the launcher has to stay executable and _internal has to stay a folder
say('==> laying out /app')
files = work / 'build/files'
install(work / 'usr/bin/ftorrent', files / 'bin/ftorrent', 0o755)
shutil.copytree(work / 'usr/lib/ftorrent', files / 'lib/ftorrent', symlinks=True)

#Everything a Flatpak ships is named for the application id rather than for the binary, so the renames below are not tidying: a .desktop file or an icon under any other name is simply not found
entry = files / f'share/applications/{identifier}.desktop'
install(work / 'usr/share/applications/ftorrent.desktop', entry, 0o644)
entry.write_text(re.sub(r'^Icon=.*$', f'Icon={identifier}', entry.read_text(), flags=re.M))
for made, size in (('32x32', '32x32'), ('128x128', '128x128'), ('256x256@2', '512x512')):  #Tauri writes a 256x256@2 folder, which is not a size freedesktop recognises, so that one is installed as the 512x512 it actually is
	install(work / f'usr/share/icons/hicolor/{made}/apps/ftorrent.png', files / f'share/icons/hicolor/{size}/apps/{identifier}.png', 0o644)

#The engine, run from the tree the bundle will carry. This is outside the sandbox, so it proves the folder arrived intact and executable, not that the sandbox lets it run; that second thing is only checked on a real Linux machine, because the sandbox cannot start under Rosetta
say('==> the engine, run from the laid-out tree')
run([files / 'lib/ftorrent/ftorrent-engine/ftorrent-engine'], input=b'{"command":"init"}\n{"command":"quit"}\n')

#A sandboxed application starts with no way to draw, no network, and no way to reach a file. The first four are what any GUI needs: Wayland with an X11 fallback covers both kinds of session, dri is what makes the web engine's compositing hardware accelerated rather than software, and ipc is what X11 wants for shared memory.
#
#The last two are ftorrent's own. A torrent client is nothing without the network, so the sandbox shares it. And a torrent client is handed folders: one to download into, a folder under Downloads by default and anywhere the user points it after, a microSD card or a USB drive under /run/media among them, and one to make a torrent from, which it reads whole. The system's open and save boxes run inside the sandbox and see what it sees. So we give the sandbox the host filesystem, which shares /run/media with it, as qBittorrent, Transmission, and KTorrent do on Flathub, and the engine and the boxes reach the same folders the user does. A Flatpak asking for the host filesystem is flagged as weakening its own sandbox, which is true: the user's files are open to it, and the host's system folders stay out of reach
say('==> build-finish')
run(['flatpak', 'build-finish', 'build', '--socket=wayland', '--socket=fallback-x11', '--device=dri', '--share=ipc', '--share=network', '--filesystem=host', '--command=ftorrent'])

say('==> build-export')
run(['flatpak', 'build-export', 'repo', 'build'])  #this repository is also, exactly, what a Flatpak remote of our own would serve, so a single-file bundle and a self-hosted repo are the same build with a different last step

say('==> build-bundle')
bundle = out / f'ftorrent_{version}_{flatpak_arch}.flatpak'
run(['flatpak', 'build-bundle', 'repo', bundle, identifier])
say(f'==> bundled {bundle}, {bundle.stat().st_size} bytes')
