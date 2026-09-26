# PyInstaller's recipe for freezing the engine. pnpm engine in the desktop workspace runs it, and dist/ftorrent-engine/ is what comes out: the executable, and beside it an _internal folder holding the interpreter, the standard library, and libtorrent. tauri.conf.json carries that folder into the app as a resource.
#
# One folder rather than one file, on purpose. A single-file build unpacks itself into a temporary directory on every launch, and that self-extracting shape is what antivirus heuristics on Windows most often flag; a folder unpacks nothing and looks like the ordinary program it is. The folder also starts faster, since nothing is extracted, and it is the same shape on every platform, so the Rust that starts the engine has one path to build.

a = Analysis(
	['engine.py'],
	pathex=[],
	binaries=[],
	datas=[],
	hiddenimports=[],
	hookspath=[],
	runtime_hooks=[],
	excludes=[],
	noarchive=False,
)
pyz = PYZ(a.pure)
exe = EXE(
	pyz,
	a.scripts,
	[],
	exclude_binaries=True,#the binaries go in the folder, collected below, rather than inside the executable
	name='ftorrent-engine',#the process name a user sees in Activity Monitor or Task Manager, so it says whose it is
	debug=False,
	bootloader_ignore_signals=False,
	strip=False,
	upx=False,#never pack the executable: a packed program is another classic antivirus trigger, and the folder is not large
	console=True,#a plain console program, since the app talks to it over pipes; on windows the app starts it without a console window of its own
	disable_windowed_traceback=False,
	argv_emulation=False,
	target_arch=None,#the machine's own architecture, apple silicon on the mac
	codesign_identity=None,#ad hoc on the mac, which is what apple silicon needs to run it at all; ftorrent ships unsigned
	entitlements_file=None,
)
coll = COLLECT(
	exe,
	a.binaries,
	a.datas,
	strip=False,
	upx=False,
	name='ftorrent-engine',
)

# No symlinks in the output. On the mac, the libtorrent wheel keeps its OpenSSL libraries in _internal/libtorrent.dylibs, while libtorrent loads them from _internal itself, and PyInstaller bridges the two with a pair of symlinks. A symlink can't survive the places this folder has to go: an exFAT drive, or a zip unpacked on Windows, for the portable edition. So each link is replaced by the file it points to, moved rather than copied so there's one copy of each library, and a folder the moves leave empty is removed. On Windows and Linux there's nothing for this to find
import os
output = os.path.join(DISTPATH, 'ftorrent-engine')#DISTPATH is where PyInstaller wrote the folder, dist unless told otherwise
links = [os.path.join(folder, name) for folder, names, files in os.walk(output) for name in names + files if os.path.islink(os.path.join(folder, name))]#found first, then changed, so the walk never sees a folder mid-edit
for link in links:
	target = os.path.realpath(link)#the real file at the end of the link, following any chain of them
	if not target.startswith(os.path.realpath(output) + os.sep) or not os.path.exists(target):#a link out of the folder, or a second link to a file already moved, isn't a shape this was written for
		raise SystemExit(f'ftorrent-engine.spec: cannot replace the symlink {link} with {target}')
	os.remove(link)
	os.rename(target, link)#the file takes the link's place, and its old place is left empty
	if not os.listdir(os.path.dirname(target)):#only a folder these moves emptied, never one PyInstaller wrote empty on purpose
		os.rmdir(os.path.dirname(target))
