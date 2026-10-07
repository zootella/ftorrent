import {ref, computed} from 'vue'
import {defineStore} from 'pinia'
import {getCurrentWindow} from '@tauri-apps/api/window'
import {openUrl} from '@tauri-apps/plugin-opener'
import {useSettingsStore} from './settings.js'
import {isInstalled} from '../paths.js'
import {loginRead, loginWrite, loginRemove, loginSettings} from '../login.js'
import {platformName} from '../settings.js'
import {log} from '../log.js'

/*
Whether ftorrent starts at login: the user's answer, in the login.start setting, beside what the system says, for the settings page to show; login.js is how each platform reads and writes, and why the two are kept apart. Off at the factory, asked about nowhere but the settings page, and only for an installed copy on Windows or the Mac, by isInstalled in paths.js, since the system would start whatever path the entry names.

The answer changes only when the user clicks it, so the page's buttons always show what the user chose. A click saves the answer, reads the system, puts the entry there for a yes only if there's none, or takes it away for a no only if there is one, and reads again after. A read also runs at startup and whenever the window comes back into focus, as when the user returns from the system's settings. When the system differs from the answer, whichever way, the page shows a link to the system's own page for it, and the link goes away once they agree.
*/

export const useLoginStore = defineStore('login', () => {
	let settings = useSettingsStore()
	let installed = ref(false)//an installed copy on windows or the mac, the only kind that may start at login; the question isn't shown on linux at all
	let system = ref('')//on, off, or absent, as the last read found it; blank before the first, or when it couldn't be read
	let trouble = ref('')//what went wrong in the last read or write, blank when nothing did, for the log and the main page's report
	let paths = null//where everything is, from startup
	let argument = ''//the argument a windows registration carries, from login.rs
	let writing = ref(0)//clicks whose write and the read after it are still under way, while the answer has moved and the system hasn't been asked again yet; a count, so a second quick click keeps the link held back until both are done

	let wanted = computed(() => settings.settings.login.start)//what the user answered
	let differs = computed(() => installed.value && writing.value == 0 && system.value != '' && wanted.value != (system.value == 'on'))//the page's cue for the link, held back during a click, so it never flashes up in the moment between the answer changing and the system being read

	let running = Promise.resolve()//the last read or write queued, so the next one waits for it
	function queue(work) { running = running.then(work).catch(error => log(`login: trouble, ${error}`)); return running }//caught here, so one that fails can't stop every one queued after it

	async function read() {
		try {
			let found = await loginRead(paths, argument)
			if (found != system.value) log(`login: the system says ${found}`)//a change, whoever made it, so the log shows a trip to the system's settings as well as a click here
			system.value = found
		} catch (error) {
			system.value = ''//unknown, which shows no link rather than a wrong one
			trouble.value = String(error)
			log(`login: could not read, ${error}`)
		}
	}

	function refresh() { return queue(async () => { trouble.value = ''; await read() }) }//read where things stand

	async function start(startPaths, launchArgument) {//call once at startup, after the settings are read; any other copy returns at once
		paths = startPaths
		argument = launchArgument
		installed.value = isInstalled(paths)
		if (!installed.value) return
		await refresh()
		await getCurrentWindow().onFocusChanged(({payload: focused}) => { if (focused) refresh() })//back from the system's settings, perhaps having changed it there
	}

	function choose(value) {//the user's answer from the settings page, true or false: saved, then the one time ftorrent writes to the system
		if (!installed.value) return
		settings.settings.login.start = value
		writing.value++
		return queue(async () => {
			log(`login: the user chose ${value ? 'yes' : 'no'}`)
			await settings.save()
			trouble.value = ''
			try {
				await read()//what the system says now, which may have changed since the last read, as when the user added ftorrent in System Settings a moment ago
				if (value && system.value == 'absent') { await loginWrite(paths, argument); log('login: wrote the entry') }//only when there's no entry: one there already, on or switched off, is the system's to keep as it is
				else if (!value && system.value != 'absent' && await loginRemove()) log('login: removed the entry')//and only when there is one
			} catch (error) {
				trouble.value = String(error)
				log(`login: trouble, ${error}`)
			}
			await read()
			writing.value--
			log(`login: the system now says ${system.value || 'nothing'}`)
		})
	}

	async function openSettings() {//the system's own page for it: Login Items & Extensions on the mac, Startup Apps on windows
		try {
			if (platformName == 'macOS') await loginSettings()
			else await openUrl('ms-settings:startupapps')
		} catch (error) {
			trouble.value = `could not open the system's settings, ${error}`
			log(`login: ${trouble.value}`)
		}
	}

	let report = computed(() => {//a line for the main page's report, none for a copy that can't start at login
		if (!installed.value) return []
		return [`login: ${wanted.value ? 'yes' : 'no'}, the system says ${system.value || 'nothing yet'}${trouble.value ? ', ' + trouble.value : ''}`]
	})

	return {installed, wanted, differs, report, start, choose, openSettings}
})
