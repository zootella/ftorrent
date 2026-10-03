import {execFileSync, spawnSync} from 'node:child_process'
import {createHash} from 'node:crypto'
import {cpSync, existsSync, mkdirSync, readFileSync, rmSync, statSync, writeFileSync, readdirSync} from 'node:fs'
import {dirname, join, relative} from 'node:path'
import {fileURLToPath} from 'node:url'
import {parse as parseToml} from 'smol-toml'

/*
The Windows installer, made from the pieces Windows already has.

This is the creator: it takes the program tauri built and the files tauri.conf.json says ride beside it, and makes one setup.exe that installs them with no interface at all. It is the descendant of the 2005 Zootella Setup Creator's Create.exe, a dialog box that packed a folder into a self-extracting program; the job is the same, and it moved from C to this file because the pipeline that calls it is already Node, on this machine and the Mac alike, and the only part of a Windows installer that must be native is the part a user runs. That part is setup.c beside this file, and this script compiles it fresh for every release, so the icon and version block that Explorer and Add or Remove Programs show come from the configuration and never from a binary kept in the repository.

Four steps, each with a tool Windows or the toolchain already provides. Stage: copy the executable and every resource into one folder laid out exactly as it will be installed. Compress: makecab, part of Windows for a quarter century, packs that folder into one cabinet with LZX, the same format Windows Update uses for its own packages, and the setup program unpacks it with the decompressor in cabinet.dll, so neither end carries compression code of its own. Compile: the Visual Studio C compiler builds setup.c with a header this script writes naming the product, version, publisher, and icon. Append: the cabinet, a table of strings the setup program reads, and a trailer holding their offsets and a SHA-256 of both go on the end of the compiled stub, and the result is the installer, named the way tauri named its own so hash and upload in scripts.js find it unchanged.

What the installer does is fixed, and that is the point of writing one: per user, into the user's local application data under the product's name, no page asking where, no choice of shortcuts, the program started at the end every time, and an upgrade that writes over the old files without uninstalling first, so nothing the program registered with Windows is ever taken away and given back. The names that vary come from tauri.conf.json and Cargo.toml, the two files that already name the product, so a fork that renames the app gets an installer with its own name, icon, and version by changing nothing here.
*/

//every path from this file's own location, for the reason scripts.js gives
const here      = fileURLToPath(new URL('.', import.meta.url))
const desktop   = join(here, '..')
const tauri     = join(desktop, 'src-tauri')
const target    = join(tauri, 'target/release')
const work      = join(target, 'win-setup')                   //staging, the cabinet, and the stamp header, under tauri's own build output so a clean build clears them too
const bundled   = join(target, 'bundle/win-setup')            //where the installer lands, beside the folders tauri's bundlers use, where scripts.js looks

function say(line) { console.log(line) }

function main() {
	if (process.platform != 'win32') return//the exe is windows' alone; the mac finishes with dmg.js
	let started = Date.now()

	//the facts, from the files that already hold them: the product name, version, publisher, and resources from tauri's configuration, and the executable's name from the crate, which is what tauri builds
	let configuration = JSON.parse(readFileSync(join(tauri, 'tauri.conf.json'), 'utf8'))
	let crate = parseToml(readFileSync(join(tauri, 'Cargo.toml'), 'utf8'))
	let product = configuration.productName
	let binary = configuration.mainBinaryName || crate.package.name
	let version = configuration.version
	let bundle = configuration.bundle
	let icon = (bundle.icon || []).find(path => path.endsWith('.ico'))
	if (!icon) throw new Error('bundle.icon in tauri.conf.json names no .ico file for the installer to wear')
	let arch = {x64: 'x64', arm64: 'arm64'}[process.arch]
	if (!arch) throw new Error('no installer architecture for ' + process.arch)
	let executable = join(target, binary + '.exe')
	if (!existsSync(executable)) throw new Error(`${executable} is not built; tauri build makes it`)

	//stage: the executable and every resource, laid out as installed. tauri.conf.json's resources are a map from a path relative to src-tauri to the path it takes beside the executable; a folder copies whole, which keeps the engine's _internal where the freeze expects it
	let stage = join(work, 'stage')
	rmSync(work, {recursive: true, force: true})
	mkdirSync(stage, {recursive: true})
	cpSync(executable, join(stage, binary + '.exe'))
	for (let [source, destination] of Object.entries(bundle.resources || {})) cpSync(join(tauri, source), join(stage, destination), {recursive: true})
	let files = list(stage)
	say(`staged   ${files.length} files, ${megabytes(files.reduce((sum, file) => sum + statSync(join(stage, file)).size, 0))} MB`)

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

	//compile: the stamp header names what the resource script compiles into the stub's icon and version block, then build.cmd runs the compiler
	let numbers = version.split(/[.+-]/).slice(0, 3).map(n => parseInt(n, 10) || 0)//a semantic version's three numbers, and a fourth of zero, which is what windows' four-part version wants
	mkdirSync(join(here, 'build'), {recursive: true})
	writeFileSync(join(here, 'build/stamp.h'), [
		'// written by win-setup.js for this build, in UTF-8; setup.rc includes it',
		`#define SETUP_ICON ${quote(join(tauri, icon))}`,
		`#define SETUP_PRODUCT ${quote(product)}`,
		`#define SETUP_PUBLISHER ${quote(bundle.publisher || '')}`,
		`#define SETUP_COPYRIGHT ${quote(bundle.copyright || '')}`,
		`#define SETUP_VERSION ${[...numbers, 0].join(',')}`,
		`#define SETUP_VERSION_TEXT ${quote(version)}`,
		'',
	].join('\n'))
	let compiling = Date.now()
	let build = spawnSync(join(here, 'build.cmd'), [], {stdio: 'inherit', shell: true})//a batch file needs cmd, and shell: true hands it the path quoted whole, spaces and all
	if (build.status != 0) throw new Error('build.cmd could not build setup.exe')
	let stub = readFileSync(join(here, 'build/setup.exe'))
	say(`stub     ${kilobytes(stub.length)} KB in ${seconds(compiling)} s`)

	//append: the table of strings the setup program reads, in the order setup.c documents, then the trailer, whose layout is the Trailer struct there: the hash, four offsets and sizes, and the magic that marks a stub with something in it
	let table = Buffer.from([product, binary + '.exe'].map(string => string + '\0').join(''), 'utf16le')
	let trailer = Buffer.alloc(72)
	createHash('sha256').update(cabinet).update(table).digest().copy(trailer, 0)
	trailer.writeBigUInt64LE(BigInt(stub.length), 32)
	trailer.writeBigUInt64LE(BigInt(cabinet.length), 40)
	trailer.writeBigUInt64LE(BigInt(stub.length + cabinet.length), 48)
	trailer.writeBigUInt64LE(BigInt(table.length), 56)
	trailer.write('winsetup', 64, 'ascii')

	//the result, under tauri's name for an installer, product, version, and architecture, in a folder emptied first so hash in scripts.js never finds two
	let file = `${product}_${version}_${arch}-setup.exe`
	rmSync(bundled, {recursive: true, force: true})
	mkdirSync(bundled, {recursive: true})
	writeFileSync(join(bundled, file), Buffer.concat([stub, cabinet, table, trailer]))
	say(`finished ${join(bundled, file)}, ${megabytes(stub.length + cabinet.length + table.length + trailer.length)} MB in ${seconds(started)} s`)
}

//every file under a folder, as paths relative to it with backslashes, which is how the cabinet and the setup program spell them
function list(folder) {
	let found = []
	for (let entry of readdirSync(folder, {withFileTypes: true, recursive: true})) {
		if (entry.isFile()) found.push(relative(folder, join(entry.parentPath, entry.name)))
	}
	return found.sort()
}

function quote(text) { return '"' + text.replace(/\\/g, '\\\\').replace(/"/g, '\\"') + '"' }//a C string literal, with the backslashes a windows path has doubled
function megabytes(bytes) { return (bytes / 1048576).toFixed(1) }
function kilobytes(bytes) { return (bytes / 1024).toFixed(0) }
function seconds(since) { return ((Date.now() - since) / 1000).toFixed(1) }

try { main() } catch (e) { console.error('🚧 Error:', e.message || e); process.exitCode = 1 }
