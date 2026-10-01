import {ref, computed} from 'vue'
import {defineStore} from 'pinia'
import {getCurrentWindow} from '@tauri-apps/api/window'
import {openUrl} from '@tauri-apps/plugin-opener'
import {useSettingsStore} from './settings.js'
import {installedCopy, thisCopy, register, whoOpens, typeNames, applicationName} from '../associate.js'
import {platformName} from '../settings.js'
import {log} from '../log.js'

/*
Whether ftorrent opens its four types, .torrent and .ftorrent files and magnet: and ftorrent: links, where three facts meet: what the user answered, in the associations.default setting; what the system would open each with right now, asked of the Windows shell or of Launch Services on the Mac; and whether the banner that asks is up. associate.js is the policy, what gets written and what gets given back, and its essay says why; this store is the flow around it, which the banner in App.vue and the choice on the Settings page both read.

ftorrent writes only when the user answers. A yes claims all four, and a no gives back what ftorrent can, each once, at that moment, and never again on its own; ask writes nothing. Every other time, at startup and whenever the window comes back into focus, a pass only reads: it renews the offer, which takes nothing from anyone, and asks the system what it would open each type with. A pass never claims on its own, because a type another app holds now may be there by the user's own choice; associate.js has the rest of why. The one pass that writes is the first after a yes given again on Windows sent the user to Settings, which carries that yes back with it, as below. Passes queue behind each other rather than overlapping, since a focus can arrive while an answer is still writing.

The bar follows one rule, worked out again on every pass from what's true now, with no history to keep: it's up when the answer isn't no, the system opens any of the four with something other than this copy, and the user hasn't closed it in this run. So it asks on a first launch, and it asks again whenever another app has taken a type since a yes, however that came about; it never asks after a no, and it has nothing to ask while the system already opens all four with this copy, whether because the user said yes or because nothing else on the machine wants them. Its × keeps it down until the next launch, and changes nothing else.

Yes goes to Windows' Settings in two cases, both of which only the user can settle, and only in Windows' own screens. One is when the fallbacks alone don't make Windows agree, because Windows has saved a choice of another program. The other is a yes given again: the answer is already yes and ftorrent has lost a type since, which with no saved choice in the way means another program has rewritten the fallbacks, as an older client written when they were the whole battlefield does at every turn. Writing them back would win again only until its next turn, the bar would return, and the duel would go on for as long as the user kept clicking. So a yes given again goes straight to Settings before it writes anything, where the types the other program took still show it, and the user's choice of ftorrent there is saved above the fallbacks, where no program's rewrites reach, ending the duel. The fallbacks wait for the user to come back: the first pass on returning writes all four, under the same yes, which covers whatever the user didn't set in Settings, ftorrent's own two among them, and every type after a reinstall, where the uninstall took the fallbacks and left the answer. A fallback written under a type the user did set changes nothing, since the saved choice outranks it. The yes rides along in memory only, for that one trip, and a user who quits from Settings instead loses nothing but the write, which the next yes makes again. A first yes still writes the fallbacks at once, so it covers ftorrent's own two types, which the user won't look for in Settings, and finishes in one click wherever nothing else is in the way. The link names ftorrent's registration, which Windows 11 opens as ftorrent's own page under Default apps, listing its types; Windows 10 doesn't know the parameter and opens Default apps, where Set defaults by app leads to the same list. One link for both, and no version check. On the Mac a yes is finished in ftorrent itself, since macOS changes a default without asking, so there's no other screen to send the user to.
*/

export const useAssociationsStore = defineStore('associations', () => {
	let settings = useSettingsStore()
	let installed = ref(false)//an installed copy, on windows or the mac, the only kind that registers, asks, or offers the choice
	let opens = ref({})//what the system would open each of the four with, by name, as whoOpens answers; empty before the first pass
	let bannerUp = ref(false)//worked out on every pass, by the rule in the essay
	let closed = false//the user closed the bar in this run, which keeps it down until the next launch
	let changed = ref(0)//values the last pass wrote or took back, for the main page
	let trouble = ref('')//what went wrong in the last pass, blank when nothing did
	let paths = null//where everything is, from startup
	let returning = false//a yes given again sent the user to windows' Settings, and the first pass on coming back writes the fallbacks under it; in memory only, for that one trip

	let answer = computed(() => settings.settings.associations.default)//yes, no, or ask, as the file says
	let ours = name => !!opens.value[name]?.executable && opens.value[name].executable.toLowerCase() == thisCopy(paths).toLowerCase()//the system would open this type with this very copy of ftorrent. By path on both platforms, though the mac records a default by bundle identifier: while a newer copy sits elsewhere, as a mounted disk image during a manual upgrade can, macOS may route ftorrent's types to it and the bar asks until the user drags it in and ejects. Known, brief, and accepted rather than another lookup
	let allOurs = computed(() => typeNames.every(ours))
	let anyOurs = computed(() => typeNames.some(ours))
	let disagrees = computed(() => installed.value && ((answer.value == 'yes' && !allOurs.value) || (answer.value == 'no' && anyOurs.value)))//the setting and the system tell different stories, which only the user can settle

	let running = Promise.resolve()//the last pass queued, so the next one waits for it
	function refresh(answering = '') { running = running.then(() => pass(answering)); return running }//a pass, after any still going; answering is the answer the user just gave, the one time anything gets written

	async function pass(answering) {//renew the offer, write the answer the user just gave if there is one, then read where things stand and decide the bar
		try {
			log(`associations: pass, answer ${answer.value}${answering ? ', just given' : ''}`)
			changed.value = await register(paths, answering)
			let {found, problems} = await whoOpens()
			opens.value = found
			log(`associations: opens ${typeNames.map(name => `${name} with ${opens.value[name]?.executable || 'nothing'}`).join(', ')}`)
			trouble.value = problems.join(', ')//blank when the system answered for all four
		} catch (error) {
			trouble.value = String(error)
			log(`associations: trouble, ${error}`)
		}
		let up = !closed && answer.value != 'no' && !allOurs.value//the one rule for the bar
		if (up != bannerUp.value) log(`associations: bar ${up ? 'up' : 'down'}`)
		bannerUp.value = up
	}

	async function start(startPaths) {//call once at startup, after the settings are read; on anything but an installed copy this returns at once and nothing is ever shown
		paths = startPaths
		installed.value = installedCopy(paths)
		if (!installed.value) return
		await refresh()
		await getCurrentWindow().onFocusChanged(({payload: focused}) => {//back from windows' Settings, or from anywhere a default might have changed
			if (!focused) return
			let carried = returning && answer.value == 'yes'//back from the Settings a yes given again opened, with that yes still the answer
			returning = false
			if (carried) log('associations: back from Settings, writing the fallbacks under the yes given again')
			refresh(carried ? 'yes' : '')
		})
	}

	async function choose(value) {//the user's answer, yes, no, or ask, from the banner or the Settings page
		if (!installed.value) return
		let again = platformName == 'Windows' && value == 'yes' && answer.value == 'yes' && !allOurs.value//a yes given again on windows, because ftorrent has lost a type since the last one; the essay says why that goes straight to Settings
		log(`associations: the user chose ${value}${again ? ' again' : ''}`)
		settings.settings.associations.default = value
		await settings.save()
		await refresh(again ? '' : value)//the moment anything is claimed or given back, except a yes given again, which writes when the user comes back from Settings
		if (again) returning = true
		if (value == 'yes' && platformName == 'Windows' && (again || !allOurs.value)) await openWindowsSettings()//a duel the fallbacks keep losing, or a saved choice of another program in the way; either way only the user can settle it, there
	}

	function dismiss() {//the banner's ×: gone for the rest of this run, and asked again next launch if there's still something to ask
		closed = true
		bannerUp.value = false
		log('associations: bar closed')
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
