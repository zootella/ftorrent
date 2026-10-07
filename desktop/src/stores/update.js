import {ref, computed} from 'vue'
import {defineStore} from 'pinia'
import {getVersion} from '@tauri-apps/api/app'
import {useSettingsStore} from './settings.js'
import {platformName} from '../settings.js'
import {isInstalled} from '../paths.js'
import {brandStem, brandHomepage} from '../brand.js'
import {netGet} from '../net.js'
import {updateReplace, updateRestart} from '../update.js'
import {Time, sayMoment} from '../time.js'
import {log} from '../log.js'

//the factory timing: how often a running copy looks at the clock, and the range the next check is picked from, at random so every copy doesn't ask at once after a release
const updatePresets = {checkEvery: Time.hour, randomFrom: 24*Time.hour, randomTo: 48*Time.hour}
const sidecarLimit = 12*1024//bytes; a real sidecar is about 200, and this leaves room for it to grow
const sidecarSeconds = 10
const updateLimit = 50*1024*1024//bytes; a comfortable ceiling on the update's download, where a real one is about 22 MB, so a download can't go on without end; the hash, not the size, is what says the file is the right one
const updateSeconds = 10*60//a 22 MB update in ten minutes is about 300 kilobits a second, slower than any connection a torrent client is useful on

/*
Checking for a newer version, and installing it on the user's click. A check reads the sidecar pnpm hash publishes beside the file this platform updates from, ftorrent.app.zip.json on a Mac and ftorrent.exe.json on Windows, where the installer itself is the update, and the settings page shows its version and date. Only an installed copy checks, on a Mac or Windows, since that's the copy an update would write over.

The user's button checks at once. With update.automatic on, the clock checks whenever update.next has passed, looking at startup and every checkEvery after. A check that hears back writes update.last and a fresh random update.next to the file, so a restart doesn't pick again and a laptop waking after days checks on the next look; one that fails changes neither, and the clock tries again an hour later.

When the version found is newer than this one, the page offers it, and nothing more happens until the user clicks. Then install downloads the update into the data folder, under a comfortable ceiling on its size, checks its SHA-256 against the sidecar's, and hands it to update.rs, which swaps it in for this copy and starts it, while this one saves and quits. Nothing downloads before the click.

The sidecar is treated as coming from anywhere, though it comes from our own server: net.rs caps its size and time, and only version, date, bytes, and sha256 are read from it, each in exactly the shape pnpm hash writes, or the check fails. Nothing in it becomes an action or an address: the update's address is the sidecar's own without .json, its hash only decides whether the file that arrived is the one expected, and its bytes only go in the log.
*/

export const useUpdateStore = defineStore('update', () => {
	let settings = useSettingsStore()
	let shown = ref(false)//this copy checks, and the settings page shows the section
	let checking = ref(false)//so the button waits, and the clock doesn't start a second check
	let found = ref(null)//{version, date, bytes, sha256} from this session's latest check
	let running = ref('')//this copy's version, to compare with what a check finds
	let installing = ref(false)//from the click until this copy quits, or the install fails
	let installable = ref(false)//whether this platform's half of installing is written; the mac's is
	let newer = computed(() => !!found.value && !!running.value && isNewer(found.value.version, running.value))
	const updateFile = `${brandStem}.${platformName == 'macOS' ? 'app.zip' : 'exe'}`//the published name of what this platform updates from
	const updateUrl = new URL(updateFile, brandHomepage).href
	const sidecarUrl = updateUrl + '.json'
	let paths = null

	async function start(startPaths) {//call once at startup, after the settings are read
		if (!isInstalled(startPaths)) return
		paths = startPaths
		running.value = await getVersion()
		installable.value = platformName == 'macOS'
		shown.value = true
		tick()
		setInterval(tick, updatePresets.checkEvery)
	}

	function tick() {//check if checking automatically and it's time; the clock calls this, and so does turning the checkbox on
		if (!settings.settings.update.automatic) return
		if (Date.now() < Date.parse(settings.settings.update.next)) return//a blank next parses to NaN, which nothing is less than, so blank means now
		check()
	}

	async function check() {
		if (checking.value || installing.value) return
		checking.value = true
		let v = await _sidecarRead()
		found.value   = v.success ? {version: v.version, date: v.date, bytes: v.bytes, sha256: v.sha256} : null
		if (v.success) {
			let now = Date.now()
			settings.settings.update.last = sayMoment(now)
			settings.settings.update.next = sayMoment(now + updatePresets.randomFrom + Math.random()*(updatePresets.randomTo - updatePresets.randomFrom))
			await settings.save()
			log(`update: ${v.version} from ${v.date} is the newest, next check ${settings.settings.update.next}`)
		} else {
			log(`update: could not check, ${v.outcome}`)
		}
		checking.value = false
	}

	async function install() {//the user's click: read the sidecar again, download, check, swap, and restart; on success this copy quits partway through the last step
		if (!newer.value || !installable.value || installing.value || checking.value) return
		installing.value = true
		let fresh = await _sidecarRead()//again, since the page may have sat open through a newer release, and the hash has to be the one beside the file downloaded now
		if (fresh.success) found.value = {version: fresh.version, date: fresh.date, bytes: fresh.bytes, sha256: fresh.sha256}
		if (!fresh.success || !newer.value) {
			installing.value = false
			log(`update: did not install, ${fresh.success ? `${fresh.version} is no newer than ${running.value}` : fresh.outcome}`)
			return
		}
		let want = found.value
		let save = paths.data + (paths.data.includes('\\') ? '\\' : '/') + updateFile//in the data folder, ftorrent's own, rather than a temporary folder
		log(`update: downloading ${want.version}, ${want.bytes} bytes, to ${save}`)
		let v = await _install(want, save)
		if (!v.success) {
			installing.value = false
			log(`update: could not install ${want.version}, ${v.outcome}`)
		}
	}

	async function _install(want, save) {//the steps after the click, as one answer; a success never returns, since this copy quits at the end
		try {
			let sha256 = await netGet(updateUrl, updateLimit, updateSeconds, save)
			if (sha256 != want.sha256) return {success: false, outcome: 'the download is not the file the sidecar describes'}
			await updateReplace(save)
			await updateRestart()
			return {success: true}
		} catch (error) {//the bottom gate: rust's refusal at any step becomes an ordinary answer
			return {success: false, outcome: String(error)}
		}
	}

	async function _sidecarRead() {//the sidecar's version, date, size, and hash, or the reason there aren't any
		let sidecar
		try {
			sidecar = JSON.parse(await netGet(sidecarUrl, sidecarLimit, sidecarSeconds))
		} catch (error) {//the bottom gate: rust's refusal, or a body that isn't json, becomes an ordinary answer
			return {success: false, outcome: String(error)}
		}
		let {version, date, bytes, sha256} = sidecar ?? {}//?? because null is valid json
		if (typeof version != 'string' || !/^\d+\.\d+\.\d+$/.test(version))   return {success: false, outcome: 'the sidecar has no version'}
		if (typeof date    != 'string' || !/^\d{4}-\d\d-\d\d$/.test(date))    return {success: false, outcome: 'the sidecar has no date'}
		if (!Number.isInteger(bytes) || bytes < 1 || bytes > updateLimit)     return {success: false, outcome: 'the sidecar has no size, or one past updateLimit'}
		if (typeof sha256  != 'string' || !/^[0-9a-f]{64}$/.test(sha256))     return {success: false, outcome: 'the sidecar has no hash'}
		return {success: true, version, date, bytes, sha256}
	}

	return {shown, checking, found, running, installing, installable, newer, start, tick, check, install}
})

//whether version a, like 0.2.0, is later than b, comparing the three numbers in order
function isNewer(a, b) {
	let x = a.split('.').map(Number), y = b.split('.').map(Number)
	for (let i = 0; i < 3; i++) { if (x[i] != y[i]) return x[i] > y[i] }
	return false
}
