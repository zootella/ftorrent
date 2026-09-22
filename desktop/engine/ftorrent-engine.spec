#./desktop/engine/ftorrent-engine.spec
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
