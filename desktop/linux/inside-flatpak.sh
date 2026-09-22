#!/bin/sh
# Wrap the x86-64 .deb into a .flatpak bundle. Handed the deb's filename, which carries the version and the architecture, so nothing here has to be told either.
#
# ## Why this does not use flatpak-builder
#
# The usual way to make a Flatpak is a YAML manifest handed to flatpak-builder, which runs every build step inside a bubblewrap sandbox. That is right when a manifest compiles something. Ours does not: the Tauri container already built the app and froze the engine, and every step here is unpacking an archive and putting files where a Flatpak expects them. A sandbox around a copy earns nothing.
#
# It also could not run. bubblewrap installs a seccomp filter, and under Rosetta, which is how an amd64 container executes on an Apple Silicon Mac, that call fails and the build stops. So the commands below are the other way: build-init lays out a tree, ordinary shell puts files in it, and build-export and build-bundle do the OSTree work. None of them needs a sandbox, so the same script runs native and emulated alike.
#
# The container still runs --privileged, for one narrow reason: build-export validates the icon it is exporting, and that check runs in bubblewrap, which needs a namespace an ordinary container cannot make. That call installs no seccomp filter, which is why it survives Rosetta where flatpak-builder's does not.
#
# Submitting to Flathub would want a manifest, because that is what their build farm reads. It does not exist yet and is not needed to make the bundle; this script is the specification for it when the day comes, and the permissions below are exactly what a manifest's finish-args would say.
set -eu

deb="${1:?say which deb to wrap}"

# The application identifier, and the second place it is written: tauri.conf.json holds the first, and this container never sees that file. Nothing checks that the two agree, so change them together; a Flatpak built under one id and a desktop entry naming another is a bundle that installs and does not appear. The form is what Flatpak requires: three components, our domain reversed, and the application's own name last.
id=com.ftorrent.ftorrent

# Ask the image which runtime it holds rather than naming one here. The Dockerfile's ARG is the single say, and this reads what it did, so bumping the version there cannot leave this script building against a runtime that is not installed.
runtime_version=$(flatpak list --columns=application,branch | awk '$1 == "org.gnome.Platform" {print $2; exit}')
runtime_version="${runtime_version:?no org.gnome.Platform installed in this image; rebuild it with pnpm build-images}"
echo "==> building against the GNOME ${runtime_version} runtime"

# Tauri writes ftorrent_<version>_<arch>.deb, and Flatpak names architectures differently from Debian
version=$(echo "$deb" | sed -n 's/^ftorrent_\([0-9][0-9.]*\)_.*/\1/p')
debian_arch=$(echo "$deb" | sed -n 's/^ftorrent_[0-9][0-9.]*_\(.*\)\.deb$/\1/p')
case "$debian_arch" in
	amd64) flatpak_arch=x86_64 ;;
	arm64) flatpak_arch=aarch64 ;;
	*) echo "no flatpak architecture known for '$debian_arch'" >&2; exit 1 ;;
esac

rm -rf /work
mkdir -p /work
cp "/out/${deb}" /work/ftorrent.deb
cd /work

echo "==> unpacking the deb"
ar x ftorrent.deb
tar -xf data.tar.gz

echo "==> build-init"
flatpak build-init build "$id" org.gnome.Sdk org.gnome.Platform "$runtime_version"

# The layout mirrors the deb's under /app instead of /usr, and that is not a convenience: Tauri finds its resources at ../lib/<name> beside the executable, so with the app at /app/bin/ftorrent it looks in /app/lib/ftorrent, the same rule that finds /usr/lib/ftorrent from /usr/bin. The engine folder is copied whole with cp -a so every file keeps its mode; the launcher has to stay executable and _internal has to stay a folder.
echo "==> laying out /app"
install -Dm755 usr/bin/ftorrent build/files/bin/ftorrent
mkdir -p build/files/lib
cp -a usr/lib/ftorrent build/files/lib/ftorrent

# Everything a Flatpak ships is named for the application id rather than for the binary, so the renames below are not tidying: a .desktop file or an icon under any other name is simply not found.
install -Dm644 usr/share/applications/ftorrent.desktop "build/files/share/applications/${id}.desktop"
sed -i "s/^Icon=.*/Icon=${id}/" "build/files/share/applications/${id}.desktop"
# Tauri writes a 256x256@2 folder, which is not a size freedesktop recognises, so that one is installed as the 512x512 it actually is
install -Dm644 usr/share/icons/hicolor/32x32/apps/ftorrent.png   "build/files/share/icons/hicolor/32x32/apps/${id}.png"
install -Dm644 usr/share/icons/hicolor/128x128/apps/ftorrent.png "build/files/share/icons/hicolor/128x128/apps/${id}.png"
install -Dm644 "usr/share/icons/hicolor/256x256@2/apps/ftorrent.png" "build/files/share/icons/hicolor/512x512/apps/${id}.png"

# The engine, run from the tree the bundle will carry. This is outside the sandbox, so it proves the folder arrived intact and executable, not that the sandbox lets it run; that second thing is only checked on a real Linux machine, because the sandbox cannot start under Rosetta.
echo "==> the engine, run from the laid-out tree"
printf '{"command":"init"}\n{"command":"quit"}\n' | build/files/lib/ftorrent/ftorrent-engine/ftorrent-engine

# A sandboxed application starts with no way to draw, no network, and no way to reach a file. The first four are what any GUI needs: Wayland with an X11 fallback covers both kinds of session, dri is what makes the web engine's compositing hardware accelerated rather than software, and ipc is what X11 wants for shared memory.
#
# The last two are ftorrent's own. A torrent client is nothing without the network, so the sandbox shares it. And downloads have to land somewhere the user can find them: the client's default download location is a folder under Downloads, so the sandbox is given that folder and no more. When the client lets the user choose any folder, this widens, and the choice of how is made then.
echo "==> build-finish"
flatpak build-finish build \
	--socket=wayland \
	--socket=fallback-x11 \
	--device=dri \
	--share=ipc \
	--share=network \
	--filesystem=xdg-download \
	--command=ftorrent

echo "==> build-export"
# this repository is also, exactly, what a Flatpak remote of our own would serve, so a single-file bundle and a self-hosted repo are the same build with a different last step
flatpak build-export repo build

echo "==> build-bundle"
out="/out/ftorrent_${version}_${flatpak_arch}.flatpak"
flatpak build-bundle repo "$out" "$id"

ls -la "$out"
echo "==> bundled $out"
