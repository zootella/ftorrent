import {diskMkdir, diskStat, diskRmtree, diskSpace, diskAccess} from './disk.js'
import {netGet} from './net.js'
import {processRun, processStart} from './process.js'
import {lifecycleExit} from './lifecycle.js'
import {thisCopy} from './associate.js'
import {isInstalled} from './paths.js'
import {brandStem} from './brand.js'
import {platformName} from './settings.js'
import {log} from './log.js'

export const updateFile = `${brandStem}.${platformName == 'macOS' ? 'app.zip' : 'exe'}`//the published name of what this platform updates from, beside its sidecar on ftorrent.com
export const updateLimit = 50*1024*1024//bytes; a comfortable ceiling on the update's download, where a real one is about 22 MB, so a download can't go on without end; the hash, not the size, is what says the file is the right one
const updateSeconds = 10*60//a 22 MB update in ten minutes is about 300 kilobits a second, slower than any connection a torrent client is useful on
const updateSpace = 1024*1024*1024//bytes free on the data folder's volume, at the least, to start an update; the update needs about 140 MB at its peak, and the rest is room for torrents that arrive while it runs, since a Mac under a gigabyte free is in trouble already and an update shouldn't be what fills it
const replaceArgument = '--replace'//on the newer copy's command line, followed by the bundle to replace; install.rs knows it as ARGUMENT
const logArgument = '--log'//followed by the folder to log into, when logging is on; install.rs knows it as LOG_ARGUMENT

/*
Downloading an update and putting it in place of this copy, once stores/update.js has found a newer version and the user has clicked. Reliability comes first in this design, simplicity second, and the moment the user sees third: the window goes away and comes back updated a second or two later, and nothing in between can leave a broken app or a strange dialog behind.

On a Mac the update is ftorrent.app zipped, and these are all the places it touches:

	https://ftorrent.com/ftorrent.app.zip.json                                          the sidecar, with the update's version, date, size, and hash
	https://ftorrent.com/ftorrent.app.zip                                               the update
	/Applications/ftorrent.app/                                                         this copy, running, and where the newer version goes
	~/Library/Application Support/com.ftorrent.ftorrent/                                the data folder, with ftorrent.toml and ftorrent.lock
	~/Library/Application Support/com.ftorrent.ftorrent/update/                         the temporary folder, made for an update and removed at the next startup
	~/Library/Application Support/com.ftorrent.ftorrent/update/ftorrent.app.zip.part    the download as it arrives
	~/Library/Application Support/com.ftorrent.ftorrent/update/ftorrent.app.zip         the download, whole, whose SHA-256 has to match the sidecar's
	~/Library/Application Support/com.ftorrent.ftorrent/update/ftorrent.app/            the newer version, unzipped, and after the swap, the older one
	~/ftorrent-logs/                                                                    the log folder, when logging is on, where the newer copy writes its lines too

First, at startup, whether this copy can replace itself at all: updateInstallable answers, and the button offers an update only when the answer is yes. It has to be the installed copy, in /Applications, since that's the one an update writes over. This user has to be able to write both the bundle and the folder holding it, which a standard account without an administrator's rights can't, the same test Sparkle makes before it asks for a password. And the data folder has to be on the same volume as the bundle, since the swap below is a rename, and a home folder moved to another drive would put the two apart. Each is asked with a general command, disk_access over POSIX access and the device number disk_stat answers, and a copy that fails one says why in the log and never offers the button; when a check finds a newer version there, the status line says to get it at ftorrent.com, with the address a link that opens the system's browser.

At the click, the store reads the sidecar again, for the hash beside the update it's about to fetch. Then this file removes whatever a past update left in the temporary folder, checks that the data folder's volume has updateSpace free, makes the folder, and streams the update into ftorrent.app.zip.part, renamed to ftorrent.app.zip once the last byte is in, whose SHA-256 has to match the sidecar's. ditto unzips it beside itself into the newer version, since dmg.js zipped the app with --keepParent, and a stat confirms the bundle is there. Nothing has touched the installed app yet, and a failure anywhere so far removes the temporary folder and leaves everything as it was.

Then the handoff. This copy starts the executable inside the newer bundle, directly rather than through open, with --replace and the path of this copy's bundle, and --log and the log folder when logging is on, and quits through the same Exit every quit reaches, which writes ftorrent.toml with the window's place, stops the engine, and lets go of ftorrent.lock as the process ends. The newer copy is the installer. install.rs sees --replace in setup, before it has a lock, an engine, or a window, and never builds any of them: it waits for the lock, which the kernel releases only when the old process has ended, however long its quit took; takes it; exchanges the two bundles in one step, renamex_np with RENAME_SWAP, so that /Applications/ftorrent.app is the newer version and the temporary folder holds the older one; touches the newer bundle; opens /Applications/ftorrent.app by its path with open -n and --update; and exits. That copy, started with --update, waits for the lock rather than handing over, takes it as the installer leaves, reads ftorrent.toml, opens its window where the old one was, and at its startup removes the temporary folder, the older version inside it.

Why this order and not a shorter one. The first version of this update had the running copy move its own bundle aside, move the newer one into /Applications, and then quit, and three things broke that a user would meet. Once a running app's bundle moves, macOS's privacy daemon can no longer find its code, and every read and write in Downloads, Documents, and Desktop fails with "Operation not permitted" for the rest of that process's life, which includes the resume data libtorrent saves as the engine stops. Finder, which had seen the bundle renamed away under a hidden name, a new one renamed in, and the old one deleted beneath it, refused for an hour afterward to replace the app from a disk image, with "an item with the name "" already exists." And a new copy waiting a fixed time for the lock would give up on a slow quit and leave nothing running. The order here is Sparkle's, the updater most Mac apps outside the App Store use, each step there for a reason it learned. The old copy quits before its bundle is touched, so every grant works through its last write. The swap is one atomic exchange under the same name, which happens whole or not at all, so a failure leaves the installed app exactly where it was, and Finder sees a single replacement rather than a departure, an arrival, and a deletion. The touch sets the modification time, which is what makes Launch Services and Finder read the bundle again, where Sparkle found LSRegisterURL unreliable. And the relaunch goes through Launch Services by path: macOS charges privacy prompts and grants to the process responsible for an app, a spawned child to its parent and an app opened by path to itself, so a copy this one spawned directly would answer for its privacy to a process that no longer exists; and by path rather than by identifier, so the system opens this bundle and not another copy that shares its name, like a build in the repository's target folder. Whatever stops the installer, something comes back: a swap that fails changes nothing, and the installer opens the bundle at the target path either way, the newer one or the old one, and the log says which. Sparkle ships its installer as a small program of its own, submitted to launchd so it outlives the app; here the newer copy is the installer, which bundles nothing and keeps one program, and install.rs says how that fits Rust's rule of staying general.

The temporary folder stays after a successful update on purpose, since it holds the older bundle, and removing it is the newer copy's first act at startup rather than the installer's last, so the installer does as little as it can with the lock held. A copy that was never installed, a portable one or a build in the repository's target folder, never has a temporary folder and skips all of this.

On Windows the update is the setup program, downloaded into the same temporary folder in the data folder, %LOCALAPPDATA%\com.ftorrent.ftorrent\update\, and started from there: it closes this copy through the instance pipe, writes over the install folder, and starts the new copy. The temporary folder is still there afterward, since setup runs from it, and the next startup removes it.
*/

export async function updateInstallable(paths) {//whether this copy can be replaced in place, decided once at startup: an installed copy, in a bundle and a folder this user can write, with the data folder on the same volume; the answer says why not
	if (!isInstalled(paths)) return {success: false, outcome: 'not the installed copy'}
	if (platformName != 'macOS') return {success: false, outcome: `replacing in place isn't tested on ${platformName}`}
	let place = _place(paths)
	let folder = place.bundle.slice(0, place.bundle.lastIndexOf(place.separator))//the folder holding the bundle, /Applications
	for (let path of [place.bundle, folder]) {
		let v = await _step(diskAccess(path))
		if (!v.success) return v
		if (!v.value.write) return {success: false, outcome: `this user can't write ${path}`}
	}
	let bundle = await _step(diskStat(place.bundle))
	let data   = await _step(diskStat(place.data))
	if (!bundle.success) return bundle
	if (!data.success)   return data
	if (bundle.value.device != data.value.device) return {success: false, outcome: `${place.data} and ${place.bundle} are on different volumes`}
	return {success: true}
}

export async function updateClean(paths) {//remove the temporary folder a past update left, at startup, which after a successful update holds the older version; nothing to do when it isn't there
	let place = _place(paths)
	let v = await _step(diskStat(place.work))
	if (!v.success) return//not there, which is the usual case
	v = await _step(diskRmtree(place.work))
	log(v.success ? `update: removed ${place.work}` : `update: could not remove ${place.work}, ${v.outcome}`)
}

export async function updateInstall(paths, url, sha256, logFolder) {//download the update at url, check it against sha256, and hand over to the newer copy, which puts itself in place of this one; this copy quits on the way, and the answer only comes back when something stopped it first
	let place = _place(paths)
	let v = await _download(place, url, sha256)
	if (v.success && platformName == 'macOS')        v = await _handOverMac(place, paths, logFolder)
	else if (v.success && platformName == 'Windows') v = await _step(processStart(place.download, []))//setup closes this copy itself
	else if (v.success)                              v = {success: false, outcome: `no update in place on ${platformName}`}
	if (!v.success) await _step(diskRmtree(place.work))//whatever went wrong, nothing of the update stays behind
	return v
}

function _place(paths) {//every path the update touches, worked out from where this copy runs
	let separator = paths.data.includes('\\') ? '\\' : '/'//the separator rust gave the paths in
	let bundle = thisCopy(paths)//like /Applications/ftorrent.app on a mac, and the executable itself on windows
	let name = bundle.slice(bundle.lastIndexOf(separator) + 1)//ftorrent.app
	let work = paths.data + separator + 'update'//the temporary folder, in the data folder
	return {
		data: paths.data, separator, bundle, name, work,
		download:   work + separator + updateFile,                      //ftorrent.app.zip, or ftorrent.exe on windows
		fresh:      work + separator + name,                            //the newer version, which the zip unzips to
		executable: work + separator + name + paths.executable.slice(bundle.length),//the program inside the newer version, at the same place inside its bundle as this copy's is inside this one
	}
}

async function _download(place, url, sha256) {
	await _step(diskRmtree(place.work))//one left from an update that stopped partway, or from setup on windows
	let v = await _step(diskSpace(place.data))
	if (v.success && v.value < updateSpace) return {success: false, outcome: `${v.value} bytes free, under updateSpace`}
	if (!v.success) return v
	v = await _step(diskMkdir(place.work))
	if (!v.success) return v
	log(`update: downloading ${url} to ${place.download}`)
	v = await _step(netGet(url, updateLimit, updateSeconds, place.download))
	if (v.success && v.value != sha256) return {success: false, outcome: 'the download is not the file the sidecar describes'}
	return v
}

async function _handOverMac(place, paths, logFolder) {//unzip the newer version, start it as the installer, and quit; install.rs takes it from there
	let v = await _step(processRun('/usr/bin/ditto', ['-x', '-k', place.download, place.work]))
	if (v.success && v.value != 0) return {success: false, outcome: `ditto exited ${v.value}`}
	if (v.success) v = await _step(diskStat(place.fresh))
	if (v.success && !v.value.is_dir) return {success: false, outcome: `the update didn't unpack to ${place.name}`}
	if (!v.success) return v

	let args = [replaceArgument, place.bundle]
	if (logFolder) args.push(logArgument, logFolder)
	log(`update: starting ${place.executable} ${args.join(' ')}, and quitting`)
	v = await _step(processStart(place.executable, args))//directly, not through open, so the installer is a plain process that outlives this one; it's the copy it opens afterward that goes through launch services
	if (!v.success) return v
	v = await _step(lifecycleExit())//the same exit every quit reaches: the settings are written, the engine stops, and the lock goes with the process, which is what the installer waits for
	if (!v.success) log(`update: could not quit, ${v.outcome}; the installer is waiting, and the update lands when this copy quits`)//the installer started, so the temporary folder has to stay, and the next quit, however it comes, finishes the update
	return {success: true}
}

async function _step(call) {//the bottom gate for one command: its answer as a value, or why it refused
	try {
		return {success: true, value: await call}
	} catch (error) {
		return {success: false, outcome: String(error)}
	}
}
