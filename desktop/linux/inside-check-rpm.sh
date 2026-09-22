#!/bin/sh
# Check the built .rpm on a bare Fedora: no toolchain, nothing installed, the image a Fedora user's machine most resembles. Handed the package's filename.
#
# It is the same test inside-check.sh runs for the .deb on Debian, on the other side of the packaging divide: what the package declares, where the app and the engine landed, and whether the frozen engine runs on a system that has only glibc and the base libraries. Fedora 40 is the oldest Fedora the glibc floor reaches, so a pass here also checks that floor from the newer end.
#
# The bare Fedora image carries rpm but not cpio, so rather than rpm2cpio the package is installed into an empty root of its own with its scripts and dependencies skipped, which is rpm's own way of laying files out and needs nothing fetched.
set -eu

package="${1:?say which rpm to check}"

echo "==> what the package declares"
rpm -qp --queryformat 'Name: %{NAME}\nVersion: %{VERSION}-%{RELEASE}\nArchitecture: %{ARCH}\nLicense: %{LICENSE}\n' "/out/${package}"
rpm -qp --requires "/out/${package}" | grep -v '^rpmlib' || true

echo "==> unpacking into a root of its own, without running its scripts"
mkdir -p /x
rpm --root /x --initdb
rpm --root /x --install --nodeps --noscripts --nodigest --nosignature "/out/${package}"

echo "==> where the app and the engine landed"
find /x -type f \( -name ftorrent -o -name ftorrent-engine \) | sed 's|^/x||'

engine=$(find /x -type f -name ftorrent-engine | head -1)
[ -n "$engine" ] || { echo "the package carries no ftorrent-engine" >&2; exit 1; }
[ -x "$engine" ] || { echo "$engine is not executable; the bundler dropped the mode" >&2; exit 1; }

echo "==> the frozen engine, run on this bare system"
printf '{"command":"init"}\n{"command":"quit"}\n' | "$engine"
echo "==> engine exited cleanly"
