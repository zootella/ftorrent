import {ref} from 'vue'
import {defineStore} from 'pinia'
import {useSettingsStore} from './settings.js'
import {platformName} from '../settings.js'
import {isInstalled} from '../paths.js'
import {brandStem, brandHomepage} from '../brand.js'
import {netGet} from '../net.js'
import {Time, sayMoment} from '../time.js'
import {log} from '../log.js'

//the factory timing: how often a running copy looks at the clock, and the range the next check is picked from, at random so every copy doesn't ask at once after a release
const updatePresets = {checkEvery: Time.hour, randomFrom: 24*Time.hour, randomTo: 48*Time.hour}
const sidecarLimit = 4*1024//bytes; a real sidecar is about 200
const sidecarSeconds = 10

/*
Checking for a newer version. A check reads the sidecar pnpm hash publishes beside this platform's installer, ftorrent.dmg.json or ftorrent.exe.json, and the settings page shows its version and date; acting on a newer version comes later. Only an installed copy checks, on a Mac or Windows, since that's the copy an update would write over.

The user's button checks at once. With update.automatic on, the clock checks whenever update.next has passed, looking at startup and every checkEvery after. A check that hears back writes update.last and a fresh random update.next to the file, so a restart doesn't pick again and a laptop waking after days checks on the next look; one that fails changes neither, and the clock tries again an hour later.

The sidecar is treated as coming from anywhere, though it comes from our own server: net.rs caps its size and time, and only version and date are read from it, each in exactly the shape pnpm hash writes, or the check fails. Nothing in it becomes an action or an address.
*/

export const useUpdateStore = defineStore('update', () => {
	let settings = useSettingsStore()
	let shown = ref(false)//this copy checks, and the settings page shows the section
	let checking = ref(false)//so the button waits, and the clock doesn't start a second check
	let found = ref(null)//{version, date} from this session's latest check
	let trouble = ref('')//why that check failed, blank when it didn't
	const sidecarUrl = new URL(`${brandStem}.${platformName == 'macOS' ? 'dmg' : 'exe'}.json`, brandHomepage).href

	function start(paths) {//call once at startup, after the settings are read
		if (!isInstalled(paths)) return
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
		if (checking.value) return
		checking.value = true
		let v = await _sidecarRead()
		found.value   = v.success ? {version: v.version, date: v.date} : null
		trouble.value = v.success ? '' : v.outcome
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

	async function _sidecarRead() {//the sidecar's version and date, or the reason there aren't any
		let sidecar
		try {
			sidecar = JSON.parse(await netGet(sidecarUrl, sidecarLimit, sidecarSeconds))
		} catch (error) {//the bottom gate: rust's refusal, or a body that isn't json, becomes an ordinary answer
			return {success: false, outcome: String(error)}
		}
		let {version, date} = sidecar ?? {}//?? because null is valid json
		if (typeof version != 'string' || !/^\d+\.\d+\.\d+$/.test(version)) return {success: false, outcome: 'the sidecar has no version'}
		if (typeof date    != 'string' || !/^\d{4}-\d\d-\d\d$/.test(date))  return {success: false, outcome: 'the sidecar has no date'}
		return {success: true, version, date}
	}

	return {shown, checking, found, trouble, start, tick, check}
})
