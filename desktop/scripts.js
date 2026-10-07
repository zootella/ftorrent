import {execFile, execFileSync} from 'node:child_process'
import {createHash} from 'node:crypto'
import {copyFileSync, existsSync, mkdirSync, mkdtempSync, readdirSync, readFileSync, rmSync, writeFileSync} from 'node:fs'
import {homedir, tmpdir} from 'node:os'
import {dirname, join} from 'node:path'
import {fileURLToPath} from 'node:url'
import {parse as parseToml} from 'smol-toml'

/*
The publishing pipeline for the desktop client, in one file, reached by a verb: reveal, hash, upload, icons-collect, certificate. Everything the package.json scripts do beyond calling tauri or docker is here.

**This file publishes; it does not build.** The desktop workspace builds with tauri and, for the Mac's dmg, its own dmg.js; the nested linux workspace builds in containers; and both then call in here to stage, hash, and send, which is why hashing and uploading exist once rather than once per workspace. linux/build.js is the other half of that split and knows nothing about publishing.

Two machines publish ftorrent. Windows sends the exe. The Mac sends the dmg it built natively and the four Linux packages it built in Docker. So a command means the same thing everywhere while doing different work underneath: pnpm hash is one package in desktop on Windows and four in linux on the Mac, and nobody has to remember which computer they are sitting at. What differs is passed as --source by the workspace that asked.

It lives in the desktop folder rather than at the repository root because the root belongs to six workspaces and this file is about one product; the nested linux workspace reaches it as ../scripts.js. Every path is built from this file's own location, never from the working directory, because both workspaces call it from their own folders. It imports node builtins and smol-toml, to read the crate's name, and nothing else.
*/

/*
Every artifact ftorrent publishes, and the one place any of it is said.

source says where the built file is found: 'bundle' is this workspace's build output, under tauri's bundle folder, 'linux' is what the containers left in linux/release. publish is the name the file takes on the server, and its sidecar is that name plus .json.

The rule for a published name: **every Linux package states its architecture, and none carries a version.** macOS and Windows ship one architecture each by decision, so ftorrent.dmg and ftorrent.exe need no token. Linux ships two architectures and three formats, so every name there says which machine it is for, including the two formats with only one build today; giving the arm64 package the bare name would read as the ordinary choice while being the rarer one, and keeping every name explicit means none has to change when an aarch64 Flatpak or an ARM rpm turns up. The architecture token is each ecosystem's own word, amd64 for Debian and x86_64 for RPM and Flatpak, because a Debian user and a Fedora user each expect their own. A versioned filename would pin whatever version was current the day a link was shared, so a stable name is overwritten in place and every link ever shared keeps handing people the current build. Which version a download is belongs on the page, which reads it from the sidecar.
*/
//the two names, read before anything is named, from the two files brand.js reads them from for the page: brandName is tauri.conf.json's productName, the name people read, which tauri names what it builds with; brandStem is Cargo.toml's crate name, the name files carry, which every published name begins with
const brandName = JSON.parse(readFileSync(new URL('src-tauri/tauri.conf.json', import.meta.url), 'utf8')).productName
const brandStem = parseToml(readFileSync(new URL('src-tauri/Cargo.toml', import.meta.url), 'utf8')).package.name

const targets = {
	'dmg':       {source: 'bundle', folder: 'dmg',  suffix: '.dmg',       publish: `${brandStem}.dmg`},
	'app-zip':   {source: 'bundle', folder: 'app-zip', suffix: '.app.zip', publish: `${brandStem}.app.zip`},//the same app as the dmg, zipped for a running copy to update itself from
	'exe':       {source: 'bundle', folder: 'win-setup', suffix: '-setup.exe', publish: `${brandStem}.exe`},
	'deb-arm64': {source: 'linux',  match: /_(arm64)\.deb$/, publish: `${brandStem}.arm64.deb`},
	'deb-x64':   {source: 'linux',  match: /_(amd64)\.deb$/, publish: `${brandStem}.amd64.deb`},
	'rpm-x64':   {source: 'linux',  match: /\.(x86_64)\.rpm$/, publish: `${brandStem}.x86_64.rpm`},
	'flatpak-x64': {source: 'linux', match: /_(x86_64)\.flatpak$/, publish: `${brandStem}.x86_64.flatpak`},
}

/*
The app inside the dmg is signed with a certificate of ftorrent's own, asked for by one line: signingIdentity "ftorrent" under bundle.macOS in tauri.conf.json. That file cannot hold a comment, so the line is explained here, beside the pipeline that ships what it produces.

Without a signing identity, tauri skips signing, and the app leaves with only the stamp the linker puts on every arm64 executable: nothing seals the bundle, Info.plist is not bound, and codesign --verify reports "code has no resources but signature indicates they must be present". A browser quarantines whatever it downloads, and at the first launch of a quarantined app Gatekeeper reads a signature that fails to verify as corruption. The dialog says "ftorrent is damaged and can't be opened. You should move it to the Trash", with no button that proceeds. Development never shows this, because nothing there quarantines: a dmg built here, or fetched with curl, carries no quarantine attribute, and Gatekeeper never looks. The first 0.1.0 dmg went up in that state.

With an identity, tauri runs codesign over the executable and then the bundle, with hardened runtime. The seal verifies, so Gatekeeper can read what the app is, an unnotarized app from no known developer, and shows the dialog it has for that: "Apple could not verify ftorrent is free of malware", with Done and Move to Trash, and for about an hour afterwards an Open Anyway button under Privacy & Security in System Settings. Only a Developer ID certificate from Apple and notarization take that dialog away, and ftorrent ships without them; Gatekeeper trusts no other certificate, so to it ftorrent's own reads exactly as the identity "-" did, an ad-hoc seal with nobody's name on it. Tauri tries to notarize after signing, finds no credentials, and logs a warning, which is expected in every mac installer build.

What the certificate changes is who macOS thinks ftorrent is from one release to the next. A seal records a designated requirement, the rule a later copy must meet to count as the same program, and macOS keeps each permission the user grants, like access to the Downloads folder, against that rule. An ad-hoc seal's rule is the hash of that one build, so every release was a stranger, and asked for the Downloads folder again. A certificate's rule is the bundle identifier signed by that certificate, which every release meets, so a permission given once stays given. The login item never had this trouble, since macOS keeps it by bundle identifier and path.

So the certificate is made once, ever, and every release from every mac is signed with that one. It's self-signed, and holds the name ftorrent and nothing else: no email, no organization, no place, because each signed app carries it where anyone can read it with codesign -dvvv. It's state on the mac that publishes rather than in this repository, the key and certificate in the login keychain, where codesign finds them by name and signs without the certificate being trusted. From the desktop folder:

	pnpm certificate                                                 say whether this mac has the identity
	pnpm certificate make ~/Desktop/ftorrent-code-signing.p12        once, ever: make the identity and a password-protected backup
	pnpm certificate import ~/Desktop/ftorrent-code-signing.p12      on a new mac: bring in the original from that backup

Make writes the backup to keep somewhere safe off the mac, with its password, and import is how a new computer takes over publishing without users being asked again. Either way, the first pnpm installer afterward stops at a dialog asking for the mac's login password so codesign can use the key; Always Allow there is what lets every later build sign without asking. A lost backup means a new certificate, which costs each user one more round of permission prompts and nothing worse. A fork names its own identity here and in tauri.conf.json and makes its own certificate, or goes back to "-".

The engine rides inside the seal without being re-signed. PyInstaller already gave every Mach-O file in the ftorrent-engine folder an ad-hoc signature of its own when it froze it, so tauri's bundle signature takes them in as signed code under Resources, and the engine keeps its own signature, without hardened runtime, as the separate process it runs as. Hardened runtime on the main executable checks the libraries that executable loads, and it loads only the system's.

Windows is untouched by this and has the same story in its own words: an installer with no certificate meets SmartScreen's "Windows protected your PC", and Run anyway sits behind More info. Neither dialog is about the bytes; the sidecar's hash is.
*/

//which targets this computer stages and sends. Linux is deliberately absent: a Linux box can clone this repository and build the client for itself, and that is development and works, but a published package comes from the Mac, where all four are built together against one base image and one lockfile
const machines = {
	darwin: ['dmg', 'app-zip', 'deb-arm64', 'deb-x64', 'rpm-x64', 'flatpak-x64'],
	win32:  ['exe'],
}

function whatMachineMakes() {
	let found = machines[process.platform]
	if (!found) throw new Error(`${brandName} does not publish from ${process.platform}. The dmg and the Linux packages are staged on the Mac, the exe on Windows. Building here for your own use is a different thing and works: pnpm installer in desktop.`)
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
const loginKeychain = join(homedir(), 'Library/Keychains/login.keychain-db')//where the signing identity lives on the mac that publishes

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
		let prefix = `${brandName}_${version}_`//tauri and win-setup.js name a build brandName, version, architecture
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

//the signing identity on the mac that publishes, which the signing essay above explains: with nothing after it, say whether the login keychain holds it; make writes a new certificate and its backup, once ever; import brings that backup onto another mac. Either way the identity goes in through the backup, so the first run proves the backup works
function certificate() {
	if (process.platform != 'darwin') throw new Error('the signing certificate belongs to the mac that publishes; nothing on this platform signs with one')
	let name = JSON.parse(readFileSync(configurationFile, 'utf8')).bundle.macOS.signingIdentity//the identity tauri signs with, and the certificate's whole subject
	if (!name || name == '-') throw new Error('tauri.conf.json signs ad hoc, with signingIdentity "-", so there is no certificate to look for')
	let [action, backup] = process.argv.slice(3)
	let found = _certificateFind(name)
	if (found) {
		say(`found    ${found} "${name}" in the login keychain`)
		if (action) throw new Error(`the login keychain already holds "${name}", so ${action} would only make a second identity by that name`)
		return
	}
	if (!action) { say(`missing  no identity named "${name}" in the login keychain; pnpm certificate import <backup.p12> brings the original onto this mac, and pnpm certificate make <backup.p12> makes a new one, once ever`); return }
	if (!backup) throw new Error(`say where the backup is: pnpm certificate ${action} <backup.p12>`)
	if (action == 'make') _certificateMake(name, backup)
	else if (action != 'import') throw new Error('say which: make or import')
	execFileSync('security', ['import', backup, '-k', loginKeychain, '-T', '/usr/bin/codesign'], {stdio: 'inherit'})//no -P, so macOS asks for the backup's password in its own dialog; -T names codesign in the key's access list, though macOS still asks once, at the first signing, as the essay says
	let made = _certificateFind(name)
	if (!made) throw new Error(`imported ${backup}, but the login keychain still has no identity named "${name}"; the backup holds a different one`)
	say(`ready    ${made} "${name}" in the login keychain`)
}

function _certificateMake(name, backup) {//a new key and certificate, written only into the password-protected backup; the loose key lives a moment in a private folder and is gone either way
	if (existsSync(backup)) throw new Error(`${backup} is already there, and may be the only copy of the original; if make wrote it and the import didn't finish, pnpm certificate import ${backup} picks up from there, and otherwise choose another path`)
	let folder = mkdtempSync(join(tmpdir(), 'certificate-'))//the user's own temporary folder, readable only by them
	try {
		let key = join(folder, 'key.pem'), certificate = join(folder, 'certificate.pem')
		execFileSync('/usr/bin/openssl', ['req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', key, '-out', certificate,//macos's own libressl rather than whatever openssl is first on the path: its backup is one security imports, where openssl 3's fails as a wrong password
			'-days', '7300',//twenty years, since codesign won't sign with an expired certificate, and a new one is a new identity
			'-subj', `/CN=${name}`,//the name and nothing else, since every signed app carries this certificate where anyone can read it
			'-addext', 'keyUsage=critical,digitalSignature', '-addext', 'extendedKeyUsage=critical,codeSigning', '-addext', 'basicConstraints=critical,CA:false',//good for signing code and nothing else
		], {stdio: ['ignore', 'ignore', 'inherit']})
		say(`made     "${name}"; now a password for the backup, twice`)
		execFileSync('/usr/bin/openssl', ['pkcs12', '-export', '-inkey', key, '-in', certificate, '-name', name, '-out', backup], {stdio: 'inherit'})//openssl asks for the password itself, so it never passes through here
		say(`wrote    ${backup}; keep it, and its password, somewhere safe off this mac`)
	} finally {
		rmSync(folder, {recursive: true, force: true})
	}
}

function _certificateFind(name) {//the sha-1 of the code signing identity named exactly name in the search list, or false; find-identity lists untrusted ones too, which a self-signed certificate always is, and codesign signs with them all the same
	let listed = execFileSync('security', ['find-identity', '-p', 'codesigning'], {encoding: 'utf8'})
	for (let [, hash, named] of listed.matchAll(/\d+\) ([0-9A-F]{40}) "(.*)"/g)) if (named == name) return hash
	return false
}

//the verb package.json passes; spelled out rather than shared because the script names a person types differ between the two workspaces on purpose
const commands = {
	'reveal':        reveal,
	'hash':          hash,
	'upload':        upload,
	'icons-collect': iconsCollect,
	'certificate':   certificate,
}

function main() {//one gate in, one gate out
	let command = commands[process.argv[2]]
	if (!command) throw new Error('say which: ' + Object.keys(commands).join(', '))
	command()
}
try { main() } catch (e) { console.error('🚧 Error:', e.message || e); process.exitCode = 1 }
