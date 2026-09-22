//./desktop/linux/build.js

import {execFileSync} from 'node:child_process'
import {cpSync, existsSync, mkdirSync, readdirSync, rmSync, statSync} from 'node:fs'
import {basename, join} from 'node:path'
import {fileURLToPath} from 'node:url'

/*
ftorrent's Linux packages, built on a Mac through Docker.

The client is developed on a Mac and tested on a Windows box, and its Linux users are somebody else's machines, so the Linux packages have to come from somewhere. This folder is that somewhere: three containers, no Linux computer, and nothing to check out on a third machine and remember to keep current.

## The pipe flushes clean

Two layers, and only one of them lasts. An *image* is the toolchain: Debian, Node, Rust, uv with the engine's Python, and the WebKit headers, or Flatpak and its runtime. It holds no source and no secrets. A *container* is one build: it is handed the source, makes a package, and is destroyed. Nothing carries between builds, so there is no warm target directory, no cargo registry, no node_modules, no Python environment from last time. Every run starts from the same known state, which is worth more here than speed: these packages are made a few times a year, not in a daily loop.

Three lockfiles are what make "clean" mean something. pnpm-lock.yaml and Cargo.lock are in the whitelist and enforced, so the JavaScript and Rust that land are the versions the Mac and the Windows box already build with. The engine's uv.lock rides inside the desktop folder and is enforced the same way: uv sync --frozen inside the container fetches the libtorrent wheel and PyInstaller by the hashes it records, on an interpreter the image already holds, and PyInstaller freezes them there. So the engine a Linux user gets was frozen on Linux, from the same pinned pieces the Mac's was, and never copied across from another platform.

## The whitelist, and why it is a whitelist

Four things go in: the root package.json, pnpm-workspace.yaml, pnpm-lock.yaml, and the desktop folder. Nothing else, and the list is stated positively rather than as a set of exclusions, because a whitelist fails closed: the private notes beside the workspaces, the deploy scripts, and .git never enter a container or an image layer.

Of the desktop folder, the things a build makes are left behind by name: node_modules, the Rust target directory, the Vite and PyInstaller outputs, the engine's virtual environment, and this folder itself, which would otherwise copy its own staging area into the staging area. The workspace file names other workspaces too, and nothing misses them: pnpm sees they are not there and carries on, installing the desktop's dependencies alone.

Source is mounted read only. A container cannot write into the working tree on the Mac; the one writable thing it is given is release/.

## The three containers, and the checks after them

	ftorrent-tauri:arm64     .deb for aarch64            native on Apple Silicon, so it runs first and fails fast
	ftorrent-tauri:amd64     .deb and .rpm for x86_64    emulated, several times slower, the ones most users want
	ftorrent-flatpak:amd64   .flatpak for x86_64         built from the amd64 .deb, engine and all

The two Tauri containers are the same image at two architectures. The Flatpak container consumes what the amd64 one produces, which is why build runs them in that order: the Flatpak wraps the deb rather than compiling anything. The rpm comes out of the same emulated run as the amd64 deb, one more word in the bundles argument, so asking for it on its own would mean paying for that slow build twice; distro is the word for those two together.

After them, containers that are not ours: a bare debian:12-slim unpacks each deb and a bare fedora:40 unpacks the rpm, with no toolchain and nothing of ours installed, and each runs the frozen engine from where the bundler put it. That answers the one question a build container cannot, whether the engine runs on a system that has only glibc and the base libraries, which is what the manylinux wheel and python-build-standalone both promise. The Flatpak gets a lighter check inside its own wrap, because its sandbox cannot start under Rosetta; none of these can show a window, so the app itself is still smoke tested only on a real Linux machine.

## What this does not do

It builds and it does not publish. What lands in release/ carries Tauri's own filenames, or the Flatpak script's; renaming a package to its published name, hashing it, and uploading it live in ../scripts.js, which the hash and upload commands in package.json reach, so that work exists once for every platform rather than once per workspace. An updater manifest is not written anywhere yet.
*/

//every path is built from this file's own location, never from the working directory, because pnpm runs this from the desktop workspace and a person may run it from anywhere
const here    = fileURLToPath(new URL('.', import.meta.url))
const root    = join(here, '..', '..')       //the monorepo, where the whitelist is gathered from
const stage   = join(here, '.stage')         //the whitelist copy, and the only thing a build container reads
const release = join(here, 'release')        //what comes out, and the only thing a container writes

//the three toolchain images. One Tauri file serves both architectures because the only difference is the platform docker is told to build for; the Flatpak file is its own, holding the GNOME runtime instead of a compiler
const images = {
	'ftorrent-tauri:arm64':   {file: 'Dockerfile.tauri',   platform: 'linux/arm64'},
	'ftorrent-tauri:amd64':   {file: 'Dockerfile.tauri',   platform: 'linux/amd64'},
	'ftorrent-flatpak:amd64': {file: 'Dockerfile.flatpak', platform: 'linux/amd64'},
}

//what goes into a container, said positively. desktop is copied whole except for the names below, which are all things a build makes rather than things a build needs
const whitelist = ['package.json', 'pnpm-workspace.yaml', 'pnpm-lock.yaml', 'desktop']
const leaveBehind = ['node_modules', 'target', 'dist', 'build', '.venv', 'release', '.stage', 'linux', '.DS_Store']

function run(args, options = {}) {//one place that shells out, so every docker invocation looks the same
	execFileSync('docker', args, {stdio: 'inherit', ...options})
}

//what a container runs is mounted rather than baked, so a change to a build step takes effect on the next run instead of on the next image build. It is also what keeps an image honestly the toolchain and nothing else
function inside(script) { return ['--entrypoint', 'sh', '-v', `${join(here, script)}:/build.sh:ro`] }

function say(line) { console.log(line) }

//gather the whitelist into .stage. Wiped first every time rather than synced, because a file deleted upstairs has to disappear down here too, and a copy is cheap next to what follows it
function stageSource() {
	say('==> staging source')
	rmSync(stage, {recursive: true, force: true})
	mkdirSync(stage, {recursive: true})
	for (let name of whitelist) {
		let from = join(root, name)
		if (!existsSync(from)) throw new Error('the whitelist names something that is not there: ' + from)
		let to = join(stage, name)
		if (!statSync(from).isDirectory()) { cpSync(from, to); continue }
		//a directory is copied one child at a time rather than whole, because this folder lives inside the desktop folder: node refuses to copy a directory into a descendant of itself, and skipping the child that holds the staging area is what makes the rest of the copy safe
		mkdirSync(to, {recursive: true})
		for (let child of readdirSync(from)) {
			if (leaveBehind.includes(child)) continue
			cpSync(join(from, child), join(to, child), {
				recursive: true,
				filter: source => !leaveBehind.includes(basename(source)),//node_modules and target are the two that matter most: both are large, and both hold binaries compiled for macOS that would be worse than useless to a Linux container
			})
		}
	}
	say('    ' + whitelist.join(', '))
}

function buildImage(tag) {
	let image = images[tag]
	say(`==> image ${tag}  (${image.platform})`)
	run(['build', '--platform', image.platform, '-f', join(here, image.file), '-t', tag, here])
}

function buildImages() { for (let tag of Object.keys(images)) buildImage(tag) }

//one tauri run. The bundle list is an argument rather than a change to tauri.conf.json, so what the Mac and the Windows box are told to build stays exactly as it was
function tauri(tag, bundles) {
	let image = images[tag]
	say(`==> ${tag}  --bundles ${bundles}`)
	mkdirSync(release, {recursive: true})
	run(['run', '--rm', '--platform', image.platform,
		...inside('inside-tauri.sh'),
		'-v', `${stage}:/src:ro`,
		'-v', `${release}:/out`,
		tag, '/build.sh', bundles])
}

//wrap the amd64 deb into a flatpak. --privileged is for one narrow thing: build-export validates the icon inside bubblewrap, which needs a namespace an ordinary container cannot make; inside-flatpak.sh carries the whole account, including why flatpak-builder is not what runs here
function flatpak() {
	let deb = newestIn(release, /^ftorrent_.*_amd64\.deb$/)
	if (!deb) throw new Error('the flatpak wraps the x86_64 deb and there is not one in release/ yet; run pnpm build-distro first')
	say(`==> ftorrent-flatpak:amd64  wrapping ${deb}`)
	run(['run', '--rm', '--platform', 'linux/amd64', '--privileged',
		...inside('inside-flatpak.sh'),
		'-v', `${release}:/out`,
		'ftorrent-flatpak:amd64', '/build.sh', deb])
}

//unpack a built deb on a bare debian and run the engine from where the bundler put it; inside-check.sh says what that proves
function checkDeb(platform, arch) {
	let deb = newestIn(release, new RegExp(`^ftorrent_.*_${arch}\\.deb$`))
	if (!deb) throw new Error(`no ${arch} .deb in release/ to check; run pnpm build first`)
	say(`==> checking ${deb} on a bare debian:12-slim  (${platform})`)
	run(['run', '--rm', '--platform', platform,
		...inside('inside-check.sh'),
		'-v', `${release}:/out:ro`,
		'debian:12-slim', '/build.sh', deb])
}

//the same for the rpm on a bare fedora, the oldest one the glibc floor reaches, so the check reads the floor from the other side; inside-check-rpm.sh says how it unpacks without cpio
function checkRpm() {
	let rpm = newestIn(release, /^ftorrent-.*\.x86_64\.rpm$/)
	if (!rpm) throw new Error('no x86_64 .rpm in release/ to check; run pnpm build first')
	say(`==> checking ${rpm} on a bare fedora:40  (linux/amd64)`)
	run(['run', '--rm', '--platform', 'linux/amd64',
		...inside('inside-check-rpm.sh'),
		'-v', `${release}:/out:ro`,
		'fedora:40', '/build.sh', rpm])
}

//find a build output in release/ by pattern, newest first. The patterns are anchored to the names tauri and the flatpak script write, ftorrent_ with an underscore and ftorrent- with a hyphen, which is what tells a build output from the published copy pnpm hash stages beside it under a dotted name
function newestIn(folder, pattern) {
	if (!existsSync(folder)) return ''
	let names = readdirSync(folder)
		.filter(name => pattern.test(name))
		.map(name => ({name, when: statSync(join(folder, name)).mtimeMs}))
		.sort((a, b) => b.when - a.when)
	return names.length ? names[0].name : ''
}

/*
The steps build runs, in the order it runs them, each also reachable on its own from package.json.

The images are rebuilt every time rather than being something a person has to remember. Docker's layer cache is what makes that free: with nothing changed it is a few seconds, and when a version inside a Dockerfile has moved, the affected image rebuilds itself and this build uses the new toolchain. Nobody has to know that happened, which is the point, because there is no way they could have.

arm64 first on purpose: it runs native on Apple Silicon where amd64 runs emulated, so anything wrong with the image, the whitelist, a lockfile, or the engine surfaces in a couple of minutes rather than twenty. The rpm rides in the amd64 run because it comes from the same compile. The flatpak goes last among the builds because it consumes the amd64 deb.
*/
const steps = {
	images:  () => buildImages(),
	distro:  () => { stageSource(); tauri('ftorrent-tauri:arm64', 'deb'); tauri('ftorrent-tauri:amd64', 'deb,rpm') },
	flatpak: () => flatpak(),
	check:   () => { checkDeb('linux/arm64', 'arm64'); checkDeb('linux/amd64', 'amd64'); checkRpm() },
}

function build() {
	steps.images()
	steps.distro()
	steps.flatpak()
	steps.check()
	report()
}

//what build says when it is done
function report() {
	say('')
	say('desktop/linux/release now holds:')
	if (!existsSync(release)) return say('    nothing')
	let names = readdirSync(release).sort()
	if (!names.length) return say('    nothing')
	for (let name of names) say('    ' + name.padEnd(36) + statSync(join(release, name)).size + ' bytes')
}

/*
The verbs package.json passes. build runs everything; the rest are here for factoring and for coming back to one step: build-distro after a source change, build-flatpak after a change to the wrap, check to re-run the bare-image tests on packages already built, stage as a window into what a container is handed when something about the whitelist needs looking at.

Not named prepare, which would be the obvious name for the image step and is a trap: npm and pnpm treat prepare as a lifecycle script and run it on every install, so cloning the repository and running pnpm install would try to build gigabytes of Docker images, and fail the install outright on a machine where Docker is not running.
*/
const commands = {
	'build':         () => build(),
	'build-images':  () => steps.images(),
	'build-distro':  () => steps.distro(),
	'build-flatpak': () => steps.flatpak(),
	'stage':         () => stageSource(),
	'check':         () => steps.check(),
}

function main() {//one gate in, one gate out
	let command = commands[process.argv[2]]
	if (!command) throw new Error('say which: ' + Object.keys(commands).join(', '))
	command()
}
try { main() } catch (e) { console.error('🚧 Error:', e.message || e); process.exitCode = 1 }
