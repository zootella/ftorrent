import {diskMkdir, diskStat, diskRmtree, diskSpace} from './disk.js'
import {netGet} from './net.js'
import {processRun, processStart} from './process.js'
import {lifecycleExit} from './lifecycle.js'
import {thisCopy} from './associate.js'
import {brandStem} from './brand.js'
import {platformName} from './settings.js'
import {log} from './log.js'

export const updateFile = `${brandStem}.${platformName == 'macOS' ? 'app.zip' : 'exe'}`//the published name of what this platform updates from, beside its sidecar on ftorrent.com
export const updateLimit = 50*1024*1024//bytes; a comfortable ceiling on the update's download, where a real one is about 22 MB, so a download can't go on without end; the hash, not the size, is what says the file is the right one
const updateSeconds = 10*60//a 22 MB update in ten minutes is about 300 kilobits a second, slower than any connection a torrent client is useful on
const updateSpace = 1024*1024*1024//bytes free on the data folder's volume, at the least, to start an update; the update needs about 140 MB at its peak, and the rest is room for torrents that arrive while it runs, since a Mac under a gigabyte free is in trouble already and an update shouldn't be what fills it
export const updateArgument = '--update'//on the new copy's command line, where instance.rs knows it as UPDATE_ARGUMENT

/*
Downloading an update and putting it in place of this copy, once stores/update.js has found a newer version and the user has clicked. On a Mac the update is ftorrent.app zipped, and these are all the places it touches:

	https://ftorrent.com/ftorrent.app.zip.json                                      the sidecar, with the update's hash
	https://ftorrent.com/ftorrent.app.zip                                           the update
	/Applications/ftorrent.app/                                                     this copy, running, where the newer version goes
	~/Library/Application Support/com.ftorrent.ftorrent/                            the data folder, with ftorrent.toml and ftorrent.lock
	~/Library/Application Support/com.ftorrent.ftorrent/update/                     a temporary folder we use during an update
	~/Library/Application Support/com.ftorrent.ftorrent/update/ftorrent.app.zip.part
	~/Library/Application Support/com.ftorrent.ftorrent/update/ftorrent.app.zip     which unzips to
	~/Library/Application Support/com.ftorrent.ftorrent/update/ftorrent.app/        newer version we switch to
	~/Library/Application Support/com.ftorrent.ftorrent/update/ftorrent.app.old/    running older version we move here, then close

At the click, the store reads the sidecar again, for the hash beside the update it's about to fetch. This file checks that the data folder's volume has updateSpace free, and starts nothing if it doesn't, then makes the temporary folder, and the update streams into ftorrent.app.zip.part as it arrives, renamed to ftorrent.app.zip once the last byte is in, whose SHA-256 has to match the sidecar's. ditto unzips it beside itself, to the newer version, since dmg.js zipped the app with --keepParent. Then the switch: mv moves the running copy into the temporary folder as the older version, and the newer version into /Applications in its place. On most Macs the home folder shares a volume with /Applications, and each move is a rename, instant whatever the size; where the user has moved the home folder to another drive, mv copies and then removes, a second or two longer, and the update works the same. macOS lets a running bundle be moved, its code already in memory. The temporary folder goes, the older version and the zip with it, and open -n starts the copy now in /Applications by its path, never its identifier, since Launch Services may otherwise pick another copy with the same one, like a build in the repository's target folder. This copy quits through the same Exit every quit reaches, which writes ftorrent.toml, the window's place among it, stops the engine, and lets go of ftorrent.lock. The new copy, started with --update, waits for that lock rather than handing over and leaving, takes it, reads ftorrent.toml, and opens its window where the old one was.

Whatever stops the update partway, the temporary folder goes with it, and nothing of the update is left behind. The one exception guards the user's own copy: if the newer version won't move into /Applications, whatever part of it arrived there is removed and the older version moves back. The temporary folder stays only when it holds the user's copy: when the older version won't move back, or when a move across volumes copied this copy out but couldn't finish removing it from /Applications.

On Windows the update is the setup program, downloaded into the same temporary folder in the data folder, %LOCALAPPDATA%\com.ftorrent.ftorrent\update\, and started from there: it closes this copy through the instance pipe, writes over the install folder, and starts the new copy. The temporary folder is still there afterward, since setup runs from it, and the next update's first step removes it.
*/

export async function updateInstall(paths, url, sha256) {//download the update at url, check it against sha256, put it in place of this copy, and start it; this copy quits on the way, and the answer only comes back when something stopped it
	let place = _place(paths)
	let v = await _download(place, url, sha256)
	if (v.success && platformName == 'macOS')        v = await _swapMac(place)
	else if (v.success && platformName == 'Windows') v = await _step(processStart(place.download, []))//setup closes this copy itself
	else if (v.success)                              v = {success: false, outcome: `no update in place on ${platformName}`}
	if (!v.success && !v.kept) await _step(diskRmtree(place.work))//whatever went wrong, nothing of the update stays behind
	return v
}

function _place(paths) {//every path the update touches, worked out from where this copy runs
	let s = paths.data.includes('\\') ? '\\' : '/'//the separator rust gave the paths in
	let bundle = thisCopy(paths)//like /Applications/ftorrent.app on a mac
	let name = bundle.slice(bundle.lastIndexOf(s) + 1)//ftorrent.app
	let work = paths.data + s + 'update'//the temporary folder, in the data folder
	return {
		data: paths.data, bundle, name, work,
		download: work + s + updateFile,        //ftorrent.app.zip, or ftorrent.exe on windows
		fresh:    work + s + name,              //the newer version, which the zip unzips to
		old:      work + s + name + '.old',     //the running older version, moved here
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

async function _swapMac(place) {
	let v = await _step(processRun('/usr/bin/ditto', ['-x', '-k', place.download, place.work]))
	if (v.success && v.value != 0) return {success: false, outcome: `ditto exited ${v.value}`}
	if (v.success) v = await _step(diskStat(place.fresh))
	if (v.success && !v.value.is_dir) return {success: false, outcome: `the update didn't unpack to ${place.name}`}
	if (!v.success) return v

	v = await _move(place.bundle, place.old)
	if (!v.success) return (await _step(diskStat(place.old))).success ? {...v, kept: true} : v//across volumes mv copies and then removes, and a removal that stopped partway leaves the whole copy here, the user's own, so the temporary folder stays
	v = await _move(place.fresh, place.bundle)
	if (!v.success) {
		await _step(diskRmtree(place.bundle))//a copy across volumes that stopped partway can leave part of the newer version here, and the older one has to go back to an empty place
		let back = await _move(place.old, place.bundle)//put this copy back, so a failed update leaves the one that was there
		return back.success ? v : {...v, kept: true}//and if it won't go back, keep the temporary folder, which holds it
	}
	await _step(diskRmtree(place.work))
	log(`update: replaced ${place.bundle}`)

	v = await _step(processStart('/usr/bin/open', ['-n', place.bundle, '--args', updateArgument]))//-n starts a second process even though launch services sees this one running under the same identifier
	if (!v.success) return v//the new version is in place and starts next time, while this copy runs on
	log('update: started the new copy, and quitting')
	await lifecycleExit()
	return {success: true}
}

async function _move(from, to) {//what mv does: a rename when both places are on one volume, and otherwise a copy, links, permissions, and attributes kept, then a removal; a copy that fails leaves the source where it was
	let v = await _step(processRun('/bin/mv', [from, to]))
	if (v.success && v.value != 0) return {success: false, outcome: `mv exited ${v.value}`}
	return v
}

async function _step(call) {//the bottom gate for one command: its answer as a value, or why it refused
	try {
		return {success: true, value: await call}
	} catch (error) {
		return {success: false, outcome: String(error)}
	}
}
