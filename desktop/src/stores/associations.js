import {ref, computed} from 'vue'
import {defineStore} from 'pinia'
import {getCurrentWindow} from '@tauri-apps/api/window'
import {openUrl} from '@tauri-apps/plugin-opener'
import {useSettingsStore} from './settings.js'
import {installedCopy, register, whoOpens, typeNames, applicationName} from '../associate.js'

/*
Whether ftorrent opens its four types, .torrent and .ftorrent files and magnet: and ftorrent: links, where three facts meet: what the user answered, in the associations.default setting; what Windows would open each with right now, asked of the shell; and whether the banner that asks is up. associate.js is the policy, what gets written and what gets given back, and its essay says why; this store is the flow around it, which the banner in App.vue and the choice on the Settings page both read.

The pass is one thing, run the same way every time: register by the current answer, then ask Windows what it would open each type with. It runs at startup, after every answer, and whenever the window comes back into focus, which is how ftorrent notices a user returning from Windows' Settings. Passes queue behind each other rather than overlapping, since a focus can arrive while an answer's pass is still writing. One rule rides along: an answer of ask with Windows already opening all four with this ftorrent means the user chose ftorrent in Windows' own Settings, which is an answer, so the setting becomes yes and the pass claims the fallbacks that go with it. It never turns a no into anything.

The store decides the banner once, at startup: up when the answer is ask, or when it's yes and Windows opens any of the four with another program, which leaves the user a way back to Windows' Settings. After that it only ever comes down: when the answer becomes no, when a yes is done because Windows agrees, or when the user closes it, which changes nothing else and brings it back next launch. Choosing ask on the Settings page likewise brings the banner back at the next launch rather than at once.

Yes goes to Windows' Settings when the fallbacks alone don't make Windows agree, because Windows has saved a choice of another program there, and only the user can change it, only in Windows' own screens. The link names ftorrent's registration, which Windows 11 opens as ftorrent's own page under Default apps, listing its types; Windows 10 doesn't know the parameter and opens Default apps, where Set defaults by app leads to the same list. One link for both, and no version check.
*/

export const useAssociationsStore = defineStore('associations', () => {
	let settings = useSettingsStore()
	let installed = ref(false)//an installed copy on windows, the only kind that registers, asks, or offers the choice
	let opens = ref({})//what windows would run for each of the four, by name, as whoOpens answers; empty before the first pass
	let bannerUp = ref(false)//decided once at startup, and afterwards only ever taken down
	let changed = ref(0)//values the last pass wrote or took back, for the main page
	let trouble = ref('')//what went wrong in the last pass, blank when nothing did
	let paths = null//where everything is, from startup

	let answer = computed(() => settings.settings.associations.default)//yes, no, or ask, as the file says
	let ours = name => !!opens.value[name]?.executable && opens.value[name].executable.toLowerCase() == paths.executable.toLowerCase()//windows would run this very copy of ftorrent for this type
	let allOurs = computed(() => typeNames.every(ours))
	let anyOurs = computed(() => typeNames.some(ours))
	let disagrees = computed(() => installed.value && ((answer.value == 'yes' && !allOurs.value) || (answer.value == 'no' && anyOurs.value)))//the setting and windows tell different stories, which only the user can settle, in windows' own Settings

	let running = Promise.resolve()//the last pass queued, so the next one waits for it
	function refresh() { running = running.then(pass); return running }//a pass, after any still going

	async function pass() {//register by the answer, ask windows where things stand, and follow a choice the user already made there
		try {
			changed.value = await register(paths, answer.value)
			opens.value = await whoOpens()
			if (answer.value == 'ask' && allOurs.value) {//chose ftorrent in windows before answering the banner, which is an answer
				settings.settings.associations.default = 'yes'
				await settings.save()
				changed.value += await register(paths, 'yes')//and claim the fallbacks that go with it
			}
			trouble.value = ''
		} catch (error) {
			trouble.value = String(error)
		}
		if (answer.value == 'no' || (answer.value == 'yes' && allOurs.value)) bannerUp.value = false//answered no, or answered yes and done
	}

	async function start(startPaths) {//call once at startup, after the settings are read; on anything but an installed windows copy this returns at once and nothing is ever shown
		paths = startPaths
		installed.value = installedCopy(paths)
		if (!installed.value) return
		await refresh()
		bannerUp.value = answer.value == 'ask' || (answer.value == 'yes' && !allOurs.value)
		await getCurrentWindow().onFocusChanged(({payload: focused}) => { if (focused) refresh() })//back from windows' Settings, or from anywhere a default might have changed
	}

	async function choose(value) {//the user's answer, yes, no, or ask, from the banner or the Settings page
		if (!installed.value) return
		settings.settings.associations.default = value
		await settings.save()
		await refresh()
		if (value == 'yes' && !allOurs.value) await openWindowsSettings()//windows has saved a choice of another program, which only the user can change, there
	}

	function dismiss() {//the banner's ×: gone for now, and asked again next launch
		bannerUp.value = false
	}

	async function openWindowsSettings() {//windows' own Default apps, at ftorrent's page on windows 11
		try {
			await openUrl(`ms-settings:defaultapps?registeredAppUser=${encodeURIComponent(applicationName)}`)//the name ftorrent registered under, escaped as Microsoft asks, since a fork's name may hold a space
		} catch (error) {
			trouble.value = `could not open Windows Settings, ${error}`
		}
	}

	let report = computed(() => {//lines for the main page's report, none for a copy that doesn't register
		if (!installed.value) return []
		let lines = [`associations: ${answer.value}, ${changed.value} values changed on the last pass${trouble.value ? ', ' + trouble.value : ''}`]//the last pass only, which after a focus has usually changed nothing
		let found = typeNames.map(name => { let opener = opens.value[name]; return opener ? `${name} with ${opener.program}, ${opener.executable}` : `${name} with nothing` })
		if (Object.keys(opens.value).length) lines.push(`opens: ${found.join('; ')}`)
		return lines
	})

	return {installed, opens, bannerUp, answer, disagrees, report, start, choose, dismiss, openWindowsSettings}
})
