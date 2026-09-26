import {execFile, execFileSync} from 'node:child_process'
import {createHash} from 'node:crypto'
import {copyFileSync, existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync} from 'node:fs'
import {dirname, join} from 'node:path'
import {fileURLToPath} from 'node:url'

/*
The publishing pipeline for the desktop client, in one file, reached by a verb: reveal, hash, upload, icons-collect. Everything the package.json scripts do beyond calling tauri or docker is here.

**This file publishes; it does not build.** The desktop workspace builds with tauri and the nested linux workspace builds in containers, and both then call in here to stage, hash, and send, which is why hashing and uploading exist once rather than once per workspace. linux/build.js is the other half of that split and knows nothing about publishing.

Two machines publish ftorrent. Windows sends the exe. The Mac sends the dmg it built natively and the four Linux packages it built in Docker. So a command means the same thing everywhere while doing different work underneath: pnpm hash is one package in desktop on Windows and four in linux on the Mac, and nobody has to remember which computer they are sitting at. What differs is passed as --source by the workspace that asked.

It lives in the desktop folder rather than at the repository root because the root belongs to six workspaces and this file is about one product; the nested linux workspace reaches it as ../scripts.js. Every path is built from this file's own location, never from the working directory, because both workspaces call it from their own folders. It imports node builtins and nothing else.
*/

/*
Every artifact ftorrent publishes, and the one place any of it is said.

source says where the built file is found: 'bundle' is tauri's own output under this workspace, 'linux' is what the containers left in linux/release. publish is the name the file takes on the server, and its sidecar is that name plus .json.

The rule for a published name: **every Linux package states its architecture, and none carries a version.** macOS and Windows ship one architecture each by decision, so ftorrent.dmg and ftorrent.exe need no token. Linux ships two architectures and three formats, so every name there says which machine it is for, including the two formats with only one build today; giving the arm64 package the bare name would read as the ordinary choice while being the rarer one, and keeping every name explicit means none has to change when an aarch64 Flatpak or an ARM rpm turns up. The architecture token is each ecosystem's own word, amd64 for Debian and x86_64 for RPM and Flatpak, because a Debian user and a Fedora user each expect their own. A versioned filename would pin whatever version was current the day a link was shared, so a stable name is overwritten in place and every link ever shared keeps handing people the current build. Which version a download is belongs on the page, which reads it from the sidecar.
*/
const targets = {
	'dmg':       {source: 'bundle', folder: 'dmg',  suffix: '.dmg',       publish: 'ftorrent.dmg'},
	'exe':       {source: 'bundle', folder: 'nsis', suffix: '-setup.exe', publish: 'ftorrent.exe'},
	'deb-arm64': {source: 'linux',  match: /_(arm64)\.deb$/, publish: 'ftorrent.arm64.deb'},
	'deb-x64':   {source: 'linux',  match: /_(amd64)\.deb$/, publish: 'ftorrent.amd64.deb'},
	'rpm-x64':   {source: 'linux',  match: /\.(x86_64)\.rpm$/, publish: 'ftorrent.x86_64.rpm'},
	'flatpak-x64': {source: 'linux', match: /_(x86_64)\.flatpak$/, publish: 'ftorrent.x86_64.flatpak'},
}

/*
The app inside the dmg carries an ad-hoc code signature, asked for by one line: signingIdentity "-" under bundle.macOS in tauri.conf.json. That file cannot hold a comment, so the line is explained here, beside the pipeline that ships what it produces.

Without it, tauri skips signing, and the app leaves with only the stamp the linker puts on every arm64 executable: nothing seals the bundle, Info.plist is not bound, and codesign --verify reports "code has no resources but signature indicates they must be present". A browser quarantines whatever it downloads, and at the first launch of a quarantined app Gatekeeper reads a signature that fails to verify as corruption. The dialog says "ftorrent is damaged and can't be opened. You should move it to the Trash", with no button that proceeds. Development never shows this, because nothing there quarantines: a dmg built here, or fetched with curl, carries no quarantine attribute, and Gatekeeper never looks. The first 0.1.0 dmg went up in that state.

With the identity "-", tauri runs codesign over the executable and then the bundle, with hardened runtime and no certificate — an ad-hoc signature is a seal with nobody's name on it. The seal verifies, so Gatekeeper can read what the app is, an unnotarized app from no known developer, and shows the dialog it has for that: "Apple could not verify ftorrent is free of malware", with Done and Move to Trash, and for about an hour afterwards an Open Anyway button under Privacy & Security in System Settings. It removes nothing: only a Developer ID certificate and notarization take the dialog away, and ftorrent ships without them. Tauri tries to notarize after signing, finds no credentials, and logs a warning, which is expected in every mac installer build.

The engine rides inside the seal without being re-signed. PyInstaller already gave every Mach-O file in the ftorrent-engine folder an ad-hoc signature of its own when it froze it, so tauri's bundle signature takes them in as signed code under Resources, and the engine keeps its own signature, without hardened runtime, as the separate process it runs as. Hardened runtime on the main executable checks the libraries that executable loads, and it loads only the system's.

Windows is untouched by this and has the same story in its own words: an installer with no certificate meets SmartScreen's "Windows protected your PC", and Run anyway sits behind More info. Neither dialog is about the bytes; the sidecar's hash is.
*/

//which targets this computer stages and sends. Linux is deliberately absent: a Linux box can clone this repository and build the client for itself, and that is development and works, but a published package comes from the Mac, where all four are built together against one base image and one lockfile
const machines = {
	darwin: ['dmg', 'deb-arm64', 'deb-x64', 'rpm-x64', 'flatpak-x64'],
	win32:  ['exe'],
}

function whatMachineMakes() {
	let found = machines[process.platform]
	if (!found) throw new Error(`ftorrent does not publish from ${process.platform}. The dmg and the Linux packages are staged on the Mac, the exe on Windows. Building here for your own use is a different thing and works: pnpm installer in desktop.`)
	return found
}

function readTarget(name) {
	let found = targets[name]
	if (!found) throw new Error(`no such target: ${name}; say one of ${Object.keys(targets).join(', ')}`)
	return found
}

//which targets a command acts on, decided in one place because hash and upload must agree. Named targets win and are an instruction; with none named it is everything this machine makes, narrowed by --source to the workspace that asked
function chosenTargets() {
	let args = process.argv.slice(3)
	let named = args.filter(a => !a.startsWith('-'))
	if (named.length) return {names: named, demanded: true}
	let source = (args.find(a => a.startsWith('--source=')) || '').split('=')[1]
	let names = whatMachineMakes()
	if (source) names = names.filter(name => readTarget(name).source == source)
	return {names, demanded: false}
}

const openers = {darwin: 'open', win32: 'explorer', linux: 'xdg-open'}//where a graphical file manager gets pointed, per platform

//every path from this file's own location
const here = fileURLToPath(new URL('.', import.meta.url))
const configurationFile = join(here, 'src-tauri/tauri.conf.json')      //the file that named the bundle, and the one place the version is written
const bundled = join(here, 'src-tauri/target/release/bundle')           //where tauri leaves what it built
const staged = {                                                        //where hash puts a package and its sidecar, and where upload looks for them
	bundle: join(here, 'release'),                                      //the dmg and the exe, copied out from under tauri's versioned name
	linux:  join(here, 'linux/release'),                                //the two the containers made, already sitting where they were written
}
const icons = join(here, 'src-tauri/icons')                             //committed artwork, generated rather than drawn

function say(line) { console.log(line) }

//open the file manager on this platform's finished installer, so it can be double-clicked the way a person who downloaded it would; that is a stronger test than starting a built binary in place, because an installer has a first-run experience of its own
function reveal() {
	let name = process.argv[3] || whatMachineMakes()[0]
	let target = readTarget(name)
	let folder = target.source == 'bundle' ? join(bundled, target.folder) : staged.linux//the folder, not the file: a filename carries the version and would need editing every release
	if (!existsSync(folder)) throw new Error('nothing built yet at ' + folder + '; run pnpm installer first')
	say('opening  ' + folder)
	execFile(openers[process.platform], [folder], error => {//an absolute path, because explorer resolves a relative one against its own working directory; and explorer answers 1 even when it opened the window, so its exit means nothing
		if (error && process.platform != 'win32') console.error('could not open the file manager: ' + error.message)
	})
}

function readVersion() {//the version comes from the same file that named the bundle, so the two cannot disagree
	let version = JSON.parse(readFileSync(configurationFile, 'utf8')).version
	if (!version) throw new Error('tauri.conf.json has no version')
	return version
}

//find the one file a target describes, and say which architecture it turned out to be; read from the filename rather than from the machine running this, so every number describes the file that exists
function findBuilt(target, version) {
	if (target.source == 'bundle') {
		let folder = join(bundled, target.folder)
		if (!existsSync(folder)) return false
		let prefix = `ftorrent_${version}_`
		let names = readdirSync(folder).filter(n => n.startsWith(prefix) && n.endsWith(target.suffix))
		if (names.length > 1) throw new Error(`expected one ${prefix}*${target.suffix} in ${folder}, found ${names.length}: ${names.join(', ')}`)
		if (!names.length) return false
		return {folder, file: names[0], arch: names[0].slice(prefix.length, names[0].length - target.suffix.length)}
	}
	//a container wrote this one, under the name tauri or the flatpak script chose. The regex does two jobs: it finds the file, and its group is where the architecture comes from, so a sidecar describes the file that exists. hash stages into the same folder, so after one run ftorrent.arm64.deb sits beside ftorrent_0.1.0_arm64.deb and ftorrent.x86_64.rpm matches the same pattern ftorrent-0.1.0-1.x86_64.rpm does; skipping every published name is what makes hash repeatable
	let folder = staged.linux
	if (!existsSync(folder)) return false
	let published = new Set(Object.values(targets).map(t => t.publish))
	let names = readdirSync(folder).filter(n => target.match.test(n) && !published.has(n))
	if (names.length > 1) throw new Error(`expected one ${target.match} in ${folder}, found ${names.length}: ${names.join(', ')}`)
	if (!names.length) return false
	return {folder, file: names[0], arch: names[0].match(target.match)[1]}
}

/*
Stage what this machine built and write a sidecar beside each, building nothing.

Two things a sidecar must not lie about, and each is read rather than assumed: the version comes from tauri.conf.json, the file that named the bundle; the architecture comes out of the built file's own name, so it describes the file that exists rather than the computer that ran this. A target that has not been built yet is reported and skipped, because staging a dmg should not fail merely because nobody has run the Linux containers today; a target asked for by name is an instruction, so that one throws.
*/
function hash() {
	let {names, demanded} = chosenTargets()
	let version = readVersion()
	for (let name of names) hashOne(name, version, demanded)
}

function hashOne(name, version, demanded) {
	let target = readTarget(name)
	let built = findBuilt(target, version)
	if (!built) {
		if (demanded) throw new Error(`${name}: nothing built to stage; run the build that makes it first`)
		say(`skipped ${name}: nothing built yet`)
		return false
	}
	let stage = staged[target.source]
	mkdirSync(stage, {recursive: true})
	let destination = join(stage, target.publish)
	if (join(built.folder, built.file) != destination) copyFileSync(join(built.folder, built.file), destination)//copy first, then measure what landed, so every number describes the file the site will ship
	let bytes = readFileSync(destination)
	let sidecar = {
		file: target.publish,
		version,
		arch: built.arch,
		bytes: bytes.length,
		sha256: createHash('sha256').update(bytes).digest('hex'),
		date: new Date().toISOString().slice(0, 10),
	}
	writeFileSync(join(stage, target.publish + '.json'), JSON.stringify(sidecar, null, '\t') + '\n')
	say(`${sidecar.sha256}  ${sidecar.bytes} bytes  ${target.publish}`)//the hash whole, because a cropped hash cannot check a download
	return true
}

/*
Send what this machine staged: every package it makes, and the sidecar beside each.

The installers go as an account with no shell at all: chrooted to the downloads directory and able to speak only the SFTP file protocol, so a compromise of a machine that builds an installer could overwrite the installers and their sidecars and nothing else. rsync is unavailable to such an account by construction, since rsync works by running a program on the far side; scp is what remains, and since OpenSSH 9.0 scp transfers over SFTP anyway. The site ships separately, as an account that administers the server, into a directory beside this one. The server answers one hostname from both, so an installer sits at the apex, like ftorrent.com/ftorrent.dmg, and a site deploy has no way to reach it.

## Running this against your own server

The destination comes from upload.hide.env in this workspace, which is gitignored, so a fresh clone will not have one and readServer will say which values are missing and stop. Write it yourself, five values and no logic:

	DEPLOY_HOST=files.example.com
	DEPLOY_PORT=22
	DEPLOY_FILES_USER=upload
	DEPLOY_FILES_PATH=/downloads/
	DEPLOY_FILES_KEY=/home/you/.ssh/upload_ed25519

DEPLOY_FILES_PATH is written as the chrooted account sees it: its own directory is its filesystem root. Write the absolute path the server shows everyone else instead, and scp fails with no such file or directory, since inside the chroot that path does not exist. Keep the trailing slash, which turns a missing directory into an error rather than a file written by that name. DEPLOY_FILES_KEY is a full path rather than one starting with ~, because scp is started from an argument array with no shell to expand the tilde; naming the key, with IdentitiesOnly beside it, means ssh offers that key and no other, not even one loaded in ssh-agent.

The name is chosen so Vite never reads it. Vite loads .env, .env.local, and .env.[mode] from a workspace it builds, and copies any VITE_-prefixed value into the client bundle; this workspace is one Vite builds, and upload.hide.env is none of those names. Node does not read it on its own either: the package.json scripts pass --env-file-if-exists, which is why this runs as pnpm upload rather than node scripts.js. The tolerant spelling is deliberate, because plain --env-file makes node refuse to start when the file is absent, before readServer can say anything useful.
*/
function upload() {
	let {names, demanded} = chosenTargets()
	let version = readVersion()

	//gather and check everything before reading the destination, so a missing env file is never what hides a stale sidecar, and a half-finished release is never half uploaded
	let sending = []
	for (let name of names) {
		let target = readTarget(name)
		let stage = staged[target.source]
		let sidecarName = target.publish + '.json'
		if (!existsSync(join(stage, sidecarName))) {
			if (demanded) throw new Error(`${name}: no sidecar staged; run pnpm hash on this machine first`)
			say(`skipped  ${name.padEnd(12)} not staged`)
			continue
		}
		let sidecar = JSON.parse(readFileSync(join(stage, sidecarName), 'utf8'))
		checkSidecar(name, stage, sidecar, sidecarName, version)
		sending.push({stage, file: sidecar.file, sidecarName})
	}
	if (!sending.length) throw new Error('nothing staged to upload; run pnpm hash first')

	let server = readServer()
	reportTransport()
	for (let one of sending) { send(server, one.stage, one.file); send(server, one.stage, one.sidecarName) }//the package before its sidecar, every time, so a page never fetches a hash for a file still arriving
	say(`sent     ${sending.length} package${sending.length == 1 ? '' : 's'} and ${sending.length} sidecar${sending.length == 1 ? '' : 's'}`)
}

function readServer() {//gather the destination from the environment, naming whatever the script did not fill
	let required = ['DEPLOY_HOST', 'DEPLOY_PORT', 'DEPLOY_FILES_USER', 'DEPLOY_FILES_PATH', 'DEPLOY_FILES_KEY']
	let missing = required.filter(name => !process.env[name])
	if (missing.length) throw new Error(`upload.hide.env in the desktop workspace is missing ${missing.join(', ')}; see the comment above upload in scripts.js, and run this as pnpm upload rather than node, so the env file is passed`)
	return {
		host: process.env.DEPLOY_HOST,
		port: process.env.DEPLOY_PORT,
		user: process.env.DEPLOY_FILES_USER,//no shell, chrooted, SFTP only
		path: process.env.DEPLOY_FILES_PATH,
		key:  process.env.DEPLOY_FILES_KEY, //that account's own key, so an upload never offers an administrative one
	}
}

function send(server, folder, name) {//copy one file into the downloads directory as the restricted account
	//run from the staging directory and name the file bare: a Windows absolute path contains the colon scp uses to split host from path, and a bare filename makes that question stop existing
	execFileSync('scp', [
		'-P', server.port,//scp spells the port capital -P, unlike ssh and rsync
		'-o', 'IdentitiesOnly=yes',//offer only the key named below; without this, keys loaded in ssh-agent are offered too, and can go first
		'-i', server.key,
		name,
		`${server.user}@${server.host}:${server.path}`,
	], {stdio: 'inherit', cwd: folder})
}

//say which scp is about to run. Windows carries two OpenSSH installs, and which one a script gets depends on the shell it was launched from; they judge a private key's permissions differently, so this is the first thing anyone will want to know when authentication fails. Diagnostic only, so it never throws; a missing scp is reported by the transfer
function reportTransport() {
	try {
		let found = execFileSync(process.platform == 'win32' ? 'where.exe' : 'which', ['scp'], {encoding: 'utf8'}).trim().split('\n')[0]
		say('using    ' + found.trim())
	} catch (e) {
		say('using    could not locate scp')
	}
}

//catch a sidecar left from a previous release, which would otherwise publish a hash describing a file nobody can download
function checkSidecar(name, stage, sidecar, sidecarName, version) {
	if (!existsSync(join(stage, sidecar.file))) throw new Error(`${name}: ${sidecarName} names ${sidecar.file}, which is not beside it`)
	let bytes = readFileSync(join(stage, sidecar.file))
	let sha256 = createHash('sha256').update(bytes).digest('hex')
	if (sidecar.version != version)    throw new Error(`${name}: sidecar says version ${sidecar.version}, tauri.conf.json says ${version}; re-run pnpm hash`)
	if (sidecar.bytes != bytes.length) throw new Error(`${name}: sidecar says ${sidecar.bytes} bytes, the file is ${bytes.length}; re-run pnpm hash`)
	if (sidecar.sha256 != sha256)      throw new Error(`${name}: sidecar hash does not describe this file; re-run pnpm hash`)
	say(`checked  ${sidecar.sha256}  ${bytes.length} bytes  ${sidecar.file}  ${sidecar.version} ${sidecar.arch}`)
}

//the three tauri icon runs stay in package.json, where pnpm is what puts the tauri cli on the path; this is the part after them. Each run writes a folder of its own, and these are the files that have to come out from under a generated name and sit where tauri.conf.json and the Windows manifest expect them. The copying is JavaScript rather than cp because this repository is built on Windows too
function iconsCollect() {
	let copies = [
		['.mac/icon.icns',              'mac/icon.icns'],        //the dock icon, inset to apple's grid, kept where the shared run cannot overwrite it
		['.tile/Square284x284Logo.png', 'tile/tile-medium.png'], //the start menu tile, renamed because the manifest attribute names the size and the file is just an asset
		['.tile/Square142x142Logo.png', 'tile/tile-small.png'],
	]
	for (let [from, to] of copies) {
		mkdirSync(dirname(join(icons, to)), {recursive: true})
		copyFileSync(join(icons, from), join(icons, to))
		say('copied   ' + to)
	}
}

//the verb package.json passes; spelled out rather than shared because the script names a person types differ between the two workspaces on purpose
const commands = {
	'reveal':        reveal,
	'hash':          hash,
	'upload':        upload,
	'icons-collect': iconsCollect,
}

function main() {//one gate in, one gate out
	let command = commands[process.argv[2]]
	if (!command) throw new Error('say which: ' + Object.keys(commands).join(', '))
	command()
}
try { main() } catch (e) { console.error('🚧 Error:', e.message || e); process.exitCode = 1 }
