#!/bin/sh
# What one build container does, start to finish: install the pinned dependencies, freeze the engine, build the app, bundle the packages it is asked for. It is handed a bundle list, deb or deb,rpm, and builds exactly that and nothing else.
#
# /src is the whitelist copy, mounted read only so a container can never write into the working tree on the Mac. /out is linux/release, the only writable thing here. Everything in between happens in /work, which dies with the container.
set -eu

bundles="${1:?say which bundles to make, like deb or deb,rpm}"

echo "==> copying source out of the read-only mount"
rm -rf /work
mkdir -p /work
cp -a /src/. /work/
cd /work

# --frozen-lockfile is the whole reason pnpm-lock.yaml is in the whitelist. It refuses to resolve anything the lockfile does not already name, so this build installs the same versions the Mac and the Windows box installed rather than whatever the registry is serving today.
echo "==> pnpm install"
corepack enable
pnpm install --frozen-lockfile

# The engine first, because it is the cheaper half and the Rust build cannot start without it: tauri-build copies the frozen folder as a resource and refuses to build when it is missing. uv sync --frozen fetches the libtorrent wheel and PyInstaller by the hashes in uv.lock, on the interpreter the image already holds, and PyInstaller freezes them into engine/dist/ftorrent-engine.
echo "==> pnpm engine"
cd desktop
pnpm engine

# Prove the frozen engine runs on this architecture before spending minutes on Rust. It answers an init line with its ready line, or this build stops here with the reason.
echo "==> the frozen engine, run here"
printf '{"command":"init"}\n{"command":"quit"}\n' | engine/dist/ftorrent-engine/ftorrent-engine
echo "==> what the engine folder holds, and which OpenSSL libtorrent carries on linux"
ls engine/dist/ftorrent-engine/_internal
strings engine/dist/ftorrent-engine/_internal/libtorrent/*.so | grep -E '^OpenSSL [0-9]' | sort -u || true

# --bundles overrides tauri.conf.json's targets for this run only, which is what keeps the Linux specifics in this folder instead of editing what the Mac and Windows builds are told to make.
echo "==> tauri build --bundles ${bundles}"
pnpm tauri build --bundles "${bundles}"

echo "==> collecting packages"
mkdir -p /out
found=0
for f in $(find src-tauri/target/release/bundle -type f \( -name '*.deb' -o -name '*.rpm' \) | sort); do
	cp -v "$f" /out/
	found=$((found + 1))
done
[ "$found" -gt 0 ] || { echo "nothing was bundled; look above for what tauri said" >&2; exit 1; }
echo "==> $found package(s) in /out"
