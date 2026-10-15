#!/usr/bin/env python3
"""
What one build container does, start to finish: install the pinned dependencies, freeze the engine, build the app, bundle the packages it is asked for. It is handed a bundle list, deb or deb,rpm, and builds exactly that and nothing else.

/src is the whitelist copy, mounted read only so a container can never write into the working tree on the Mac. /out is linux/release, the only writable thing here. Everything in between happens in /work, which dies with the container.
"""
import re
import shutil
import subprocess
import sys
from pathlib import Path

bundles = sys.argv[1] if len(sys.argv) > 1 else sys.exit('say which bundles to make, like deb or deb,rpm')
work = Path('/work')
out = Path('/out')
desktop = work / 'desktop'
engine = desktop / 'engine/dist/ftorrent-engine/ftorrent-engine'  #where pnpm engine leaves the frozen folder, and where tauri-build looks for it

def say(text): print(text, flush=True)
def run(arguments, cwd=work, **options): subprocess.run([str(a) for a in arguments], cwd=cwd, check=True, **options)
def read(arguments): return subprocess.run([str(a) for a in arguments], check=True, capture_output=True, text=True).stdout

say('==> copying source out of the read-only mount')
shutil.rmtree(work, ignore_errors=True)
shutil.copytree('/src', work, symlinks=True)

#--frozen-lockfile is the whole reason pnpm-lock.yaml is in the whitelist. It refuses to resolve anything the lockfile does not already name, so this build installs the same versions the Mac and the Windows box installed rather than whatever the registry is serving today
say('==> pnpm install')
run(['corepack', 'enable'])
run(['pnpm', 'install', '--frozen-lockfile'])

#The engine first, because it is the cheaper half and the Rust build cannot start without it: tauri-build copies the frozen folder as a resource and refuses to build when it is missing. uv sync --frozen fetches the libtorrent wheel and PyInstaller by the hashes in uv.lock, on the interpreter the image already holds, and PyInstaller freezes them into engine/dist/ftorrent-engine
say('==> pnpm engine')
run(['pnpm', 'engine'], cwd=desktop)

#Prove the frozen engine runs on this architecture before spending minutes on Rust. It answers an init line with its ready line, or this build stops here with the reason
say('==> the frozen engine, run here')
run([engine], input=b'{"command":"init"}\n{"command":"quit"}\n')
say('==> what the engine folder holds, and which OpenSSL libtorrent carries on linux')
for name in sorted(path.name for path in (engine.parent / '_internal').iterdir()): say('    ' + name)
openssl = set()
for library in (engine.parent / '_internal/libtorrent').glob('*.so'):
	openssl.update(line for line in read(['strings', library]).splitlines() if re.match(r'^OpenSSL [0-9]', line))
for line in sorted(openssl): say('    ' + line)

#--bundles overrides tauri.conf.json's targets for this run only, which is what keeps the Linux specifics in this folder instead of editing what the Mac and Windows builds are told to make
say(f'==> tauri build --bundles {bundles}')
run(['pnpm', 'tauri', 'build', '--bundles', bundles], cwd=desktop)

say('==> collecting packages')
out.mkdir(parents=True, exist_ok=True)
found = sorted(path for path in (desktop / 'src-tauri/target/release/bundle').rglob('*') if path.is_file() and path.suffix in ('.deb', '.rpm'))
if not found: sys.exit('nothing was bundled; look above for what tauri said')
for package in found:
	shutil.copy2(package, out / package.name)
	say(f'    {package.name}')
say(f'==> {len(found)} package(s) in /out')
