#!/bin/sh
# Check a built .deb on a bare Debian 12: no toolchain, nothing installed, the image a user's machine most resembles. Handed the package's filename.
#
# It does not install the package, because that would pull WebKitGTK and the rest of the desktop stack into a container with no display to show them on. It unpacks the package and answers three questions a build container cannot: what the package declares it depends on, where Tauri put the app and the engine, and whether the frozen engine runs on a system that has only glibc and the base libraries, which is the promise the manylinux wheel and python-build-standalone both make.
set -eu

deb="${1:?say which deb to check}"

echo "==> what the package declares"
dpkg-deb -f "/out/${deb}" Package Version Architecture Depends

echo "==> unpacking, without installing"
dpkg-deb -x "/out/${deb}" /x

echo "==> where the app and the engine landed"
find /x -type f \( -name ftorrent -o -name ftorrent-engine \) | sed 's|^/x||'

engine=$(find /x -type f -name ftorrent-engine | head -1)
[ -n "$engine" ] || { echo "the package carries no ftorrent-engine" >&2; exit 1; }
[ -x "$engine" ] || { echo "$engine is not executable; the bundler dropped the mode" >&2; exit 1; }

echo "==> the frozen engine, run on this bare system"
printf '{"command":"init"}\n{"command":"quit"}\n' | "$engine"
echo "==> engine exited cleanly"
