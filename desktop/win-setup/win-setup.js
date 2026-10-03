import {execFileSync, spawnSync} from 'node:child_process'
import {cpSync, existsSync, mkdirSync, readFileSync, rmSync, statSync, writeFileSync, readdirSync} from 'node:fs'
import {dirname, join, relative} from 'node:path'
import {fileURLToPath} from 'node:url'
import {parse as parseToml} from 'smol-toml'
import {uninstallInstructions} from './registry.js'

/*
The Windows installer, made from the pieces Windows already has.

This is the creator: it takes the program tauri built and the files tauri.conf.json says ride beside it, and makes one setup.exe that installs them with no interface at all. It is the descendant of the 2005 Zootella Setup Creator's Create.exe, a dialog box that packed a folder into a self-extracting program; the job is the same, and it moved from C to this file because the pipeline that calls it is already Node, on this machine and the Mac alike, and the only part of a Windows installer that must be native is the part a user runs. That part is setup.c beside this file, and this script compiles it fresh for every release, so everything the setup program knows, the names it uses and the icon and version block that Explorer and Add or Remove Programs show, comes from the configuration and never from a binary kept in the repository.

Four steps, each with a tool Windows or the toolchain already provides. Stage: copy the executable and every resource into one folder laid out exactly as it will be installed. Compress: makecab, part of Windows for a quarter century, packs that folder into one cabinet with LZX, the same format Windows Update uses for its own packages, and the setup program unpacks it with the decompressor in cabinet.dll, so neither end carries compression code of its own. Compile: this script writes build/stamp.h, a header of defines naming the product by its two names, brandName and brandStem as brand.js explains them, with its identifier, version, publisher, home page, icon, installed size, and the list of registry entries uninstall takes back from registry.js beside this file, and the Visual Studio C compiler builds setup.c and its resources with that header included. Append: the cabinet goes on the end of the compiled stub with a 24-byte trailer saying where it starts, how long it is, and the word ENCLOSED, which marks a stub that has a cabinet in it, and the result is the installer, named the way tauri named its own so hash and upload in scripts.js find it unchanged.

What the installer does is fixed, and that is the point of writing one: per user, into the user's local application data under the product's name, no page asking where, no choice of shortcuts, the program started at the end every time, and an upgrade that writes over the old files without uninstalling first, so nothing the program registered with Windows is ever taken away and given back. The names that vary come from tauri.conf.json and Cargo.toml, the two files that already name the product, so a fork that renames the app gets an installer with its own names, icon, and version by changing nothing here.
*/

//every path from this file's own location, for the reason scripts.js gives
const here      = fileURLToPath(new URL('.', import.meta.url))
const desktop   = join(here, '..')
const tauri     = join(desktop, 'src-tauri')
const target    = join(tauri, 'target/release')
const work      = join(target, 'win-setup')                   //staging and the cabinet, under tauri's own build output so a clean build clears them too
const bundled   = join(target, 'bundle/win-setup')            //where the installer lands, beside the folders tauri's bundlers use, where scripts.js looks

function say(line) { console.log(line) }

function main() {
	if (process.platform != 'win32') return//the exe is windows' alone; the mac finishes with dmg.js
	let started = Date.now()

	//the facts, from the files that already hold them: the product name, version, identifier, publisher, and resources from tauri's configuration, and the crate's name from Cargo.toml, which is the executable's
	let configuration = JSON.parse(readFileSync(join(tauri, 'tauri.conf.json'), 'utf8'))
	let crate = parseToml(readFileSync(join(tauri, 'Cargo.toml'), 'utf8'))
	let brandName = configuration.productName//the name people read; brand.js says how it and brandStem are used across the app
	let brandStem = crate.package.name//the name files carry, the executable's among them, which Cargo names from the crate
	let version = configuration.version
	let bundle = configuration.bundle
	let icon = (bundle.icon || []).find(path => path.endsWith('.ico'))
	if (!icon) throw new Error('bundle.icon in tauri.conf.json names no .ico file for the installer to wear')
	let arch = {x64: 'x64', arm64: 'arm64'}[process.arch]
	if (!arch) throw new Error('no installer architecture for ' + process.arch)
	let executable = join(target, brandStem + '.exe')
	if (!existsSync(executable)) throw new Error(`${executable} is not built; tauri build makes it`)
	let registry = uninstallInstructions(brandName)//what uninstall takes out of the registry, from the list in registry.js, which reads brandStem from Cargo.toml itself

	//stage: the executable and every resource, laid out as installed. tauri.conf.json's resources are a map from a path relative to src-tauri to the path it takes beside the executable; a folder copies whole, which keeps the engine's _internal where the freeze expects it
	let stage = join(work, 'stage')
	rmSync(work, {recursive: true, force: true})
	mkdirSync(stage, {recursive: true})
	cpSync(executable, join(stage, brandStem + '.exe'))
	for (let [source, destination] of Object.entries(bundle.resources || {})) cpSync(join(tauri, source), join(stage, destination), {recursive: true})
	let files = list(stage)
	let bytes = files.reduce((sum, file) => sum + statSync(join(stage, file)).size, 0)
	say(`staged   ${files.length} files, ${megabytes(bytes)} MB`)

	//compress: a directive file tells makecab what goes in and where; LZX at its largest window is the best ratio it has, and the setup program unpacks it in well under a second
	let directives = [
		'.OPTION EXPLICIT', '.Set CabinetNameTemplate=payload.cab', `.Set DiskDirectoryTemplate="${work}"`,//quoted, as every path here is, so a checkout under a folder with a space in its name works
		'.Set Cabinet=on', '.Set Compress=on', '.Set CompressionType=LZX', '.Set CompressionMemory=21',
		'.Set MaxDiskSize=0', '.Set MaxCabinetSize=0', '.Set UniqueFiles=off', '.Set RptFileName=nul', '.Set InfFileName=nul',//one cabinet however large, and none of the report files makecab writes by default
	]
	for (let file of files) {
		directives.push('.Set DestinationDir=' + (file.includes('\\') ? `"${dirname(file)}"` : ''))//the folder inside the cabinet, which the setup program recreates under the install folder
		directives.push(`"${join(stage, file)}"`)
	}
	writeFileSync(join(work, 'payload.ddf'), directives.join('\r\n') + '\r\n')
	let compressing = Date.now()
	let makecab = join(process.env.SystemRoot || 'C:\\Windows', 'System32/makecab.exe')//by its full path, so an unusual PATH cannot put another program by that name first
	try { execFileSync(makecab, ['/F', join(work, 'payload.ddf')], {cwd: work, stdio: ['ignore', 'pipe', 'pipe']}) }
	catch (e) { throw new Error('makecab could not make the cabinet: ' + String(e.stdout || '').trim() + ' ' + String(e.stderr || '').trim()) }//makecab explains itself on standard output, so that is kept for the error rather than printed on every build
	let cabinet = readFileSync(join(work, 'payload.cab'))
	say(`cabinet  ${megabytes(cabinet.length)} MB in ${seconds(compressing)} s`)

	//compile: the stamp header is everything the stub knows, as C defines; setup.rc reads the names for the version block and setup.c reads all of them, then build.cmd runs the compiler
	let numbers = version.split(/[.+-]/).slice(0, 3).map(n => parseInt(n, 10) || 0)//a semantic version's three numbers, and a fourth of zero, which is what windows' four-part version wants
	mkdirSync(join(here, 'build'), {recursive: true})
	writeFileSync(join(here, 'build/stamp.h'), [
		'// written by win-setup.js for this build, in UTF-8; setup.rc and setup.c include it',
		`#define SETUP_ICON ${quote(join(tauri, icon))}`,
		`#define SETUP_BRAND_NAME ${quote(brandName)}`,
		`#define SETUP_BRAND_STEM ${quote(brandStem)}`,
		`#define SETUP_IDENTIFIER ${quote(configuration.identifier)}`,
		`#define SETUP_PUBLISHER ${quote(bundle.publisher || '')}`,
		`#define SETUP_COPYRIGHT ${quote(bundle.copyright || '')}`,
		`#define SETUP_HOMEPAGE ${quote(bundle.homepage || '')}`,
		`#define SETUP_VERSION ${[...numbers, 0].join(',')}`,
		`#define SETUP_VERSION_TEXT ${quote(version)}`,
		`#define SETUP_SIZE_KB ${Math.ceil(bytes / 1024)}`,//what the uninstall entry reports as the installed size
		`#define SETUP_UNINSTALL ${registry.map(instruction => 'L' + quote(instruction) + ', ').join('')}`,//the uninstall list as wide string literals, each followed by a comma, so setup.c can end the array with a null
		'',
	].join('\n'))
	let compiling = Date.now()
	let build = spawnSync(join(here, 'build.cmd'), [], {stdio: 'inherit', shell: true})//a batch file needs cmd, and shell: true hands it the path quoted whole, spaces and all
	if (build.status != 0) throw new Error('build.cmd could not build setup.exe')
	let stub = readFileSync(join(here, 'build/setup.exe'))
	say(`stub     ${kilobytes(stub.length)} KB in ${seconds(compiling)} s, ${registry.length} uninstall instructions`)

	//append: the cabinet, then the trailer, whose layout is the Trailer struct in setup.c: the cabinet's offset and size as two eight-byte numbers, and the eight letters ENCLOSED, our own marker for a stub with something in it
	let trailer = Buffer.alloc(24)
	trailer.writeBigUInt64LE(BigInt(stub.length), 0)
	trailer.writeBigUInt64LE(BigInt(cabinet.length), 8)
	trailer.write('ENCLOSED', 16, 'ascii')

	//the result, under tauri's name for an installer, product, version, and architecture, in a folder emptied first so hash in scripts.js never finds two
	let file = `${brandName}_${version}_${arch}-setup.exe`
	rmSync(bundled, {recursive: true, force: true})
	mkdirSync(bundled, {recursive: true})
	writeFileSync(join(bundled, file), Buffer.concat([stub, cabinet, trailer]))
	say(`finished ${join(bundled, file)}, ${megabytes(stub.length + cabinet.length + trailer.length)} MB in ${seconds(started)} s`)
}

//every file under a folder, as paths relative to it with backslashes, which is how the cabinet and the setup program spell them
function list(folder) {
	let found = []
	for (let entry of readdirSync(folder, {withFileTypes: true, recursive: true})) {
		if (entry.isFile()) found.push(relative(folder, join(entry.parentPath, entry.name)))
	}
	return found.sort()
}

/*
A JavaScript string as the text of a C string literal, so it can be written into stamp.h and read back by the C compiler as the same string. The header is C source, and three characters in our values break C's rules for what may stand between double quotes. A backslash begins an escape, and the icon's path is full of them, so each becomes two, which C reads as one. A double quote ends the literal, and the uninstall list compares a registry value to "{program}" "%1", quotes included, since that is how a shell-open command is written in the registry, so each becomes a backslash and a quote. A tab separates an instruction's fields, and a literal tab inside a C string is legal but invisible, so each becomes the two characters backslash and t, which C reads back as a tab.

The three replacements run in that order on purpose: the first adds backslashes, and the other two add one each in front of a character, so if either ran before the first, the backslash it added would be doubled. With backslashes done first, nothing added later is touched again.

Nothing else is escaped because nothing else can appear: paths, names, and registry keys carry no newlines or control characters, and registry.js checks each entry in its list against the few characters an extension or a scheme may hold. The one non-ASCII character the stamp carries, the copyright sign, passes through as it is, which is right, since build.cmd tells the C compiler and the resource compiler both to read the header as UTF-8.
*/
function quote(text) { return '"' + text.replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/\t/g, '\\t') + '"' }
function megabytes(bytes) { return (bytes / 1048576).toFixed(1) }
function kilobytes(bytes) { return (bytes / 1024).toFixed(0) }
function seconds(since) { return ((Date.now() - since) / 1000).toFixed(1) }

try { main() } catch (e) { console.error('🚧 Error:', e.message || e); process.exitCode = 1 }
