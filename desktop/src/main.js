import {createApp} from 'vue'
import {createPinia} from 'pinia'
import App from './App.vue'
import router from './router/index.js'
import {revealWindow, watchWindow} from './window.js'
import {pathsStatus} from './paths.js'
import {useSettingsStore} from './stores/settings.js'
import {useIncomingStore} from './stores/incoming.js'
import {useAssociationsStore} from './stores/associations.js'
import {useLoginStore} from './stores/login.js'
import {loginLaunch} from './login.js'
import {brandName} from './brand.js'
import {logStart, log} from './log.js'
import {platformName} from './settings.js'
import {getVersion} from '@tauri-apps/api/app'
import './style.css'

document.title = brandName//the page's own title, which tauri doesn't show, since rust names the window; set here so index.html doesn't spell the name a second time
if (import.meta.env.PROD) document.addEventListener('contextmenu', event => { if (!event.target.closest('input, textarea, [contenteditable]')) event.preventDefault() })//turn away the web view's own right-click menu, a browser's on every platform, with items like reload and print; text fields keep theirs for cut, copy, and paste, and a development build keeps it everywhere, for inspecting the page. window.rs turns off the browser's keys

let pinia = createPinia()//one pinia for the life of the app: this process opens a single window once and closes it once, so there is never a second store to keep in step
createApp(App).use(pinia).use(router).mount('#app')
useIncomingStore(pinia).start()//start taking what comes up from rust, the engine's lines and this copy's arrivals, for the life of the app; first, so the engine's ready is read as soon as it's there
startup().catch(error => useSettingsStore(pinia).problems.push(`startup: ${error}`))//read the settings, place and show the window, and lock the download folders, none of which anything before this point could do, because the page is what knows what a setting is. The catch is the one top gate for the page's startup: whatever escaped shows on the main page rather than vanishing as an unhandled rejection

//the page's half of starting up, after rust's half in setup: read the settings file, place the window where it remembers and show it, keep it told where the window is, and lock the download folders it names and hand the engine the ones this copy holds. Rust started the engine before the page existed and told it everything else; the folders wait for here because they are a setting, and only the page reads those
async function startup() {
	let store = useSettingsStore(pinia)//outside a component, a store needs the pinia handed to it
	let launch = {login: false, argument: ''}//whether the system started this copy at login, which keeps the window hidden
	try {
		launch = await loginLaunch()
		await store.load(await pathsStatus())//where everything is, and then the settings file in the data folder, if there is one
		let logPath = await logStart(store.settings.log.record, store.paths?.home)//as soon as the setting is known, so the lines rust and the page have said so far land in the file or go
		if (logPath) log(`${store.paths.mode} ${brandName} ${await getVersion()} on ${platformName}, at ${store.paths.location}, logging to ${logPath}`)//the first line from the page, saying which copy this file is about
	} finally {
		await revealWindow(store, launch.login)//rust made the window hidden; place it where the settings remember, or somewhere fresh when they hold nothing, couldn't be read, or this copy is portable, and show it, whatever happened above, unless the system started this copy at login
	}
	if (!store.paths?.settings) return//no data folder, which the platform should always give: no window place to record and no engine to tell, and the main page shows the trouble
	await useAssociationsStore(pinia).start(store.paths)//an installed copy tells the system what it can open, and asks the user about being the default; any other copy returns at once
	await useLoginStore(pinia).start(store.paths, launch.argument)//and reads whether the system starts it at login, to compare with the user's answer; any other copy returns at once
	await watchWindow(store)//after load, so recording the window's place lands in settings that are already filled in from the file
	try {
		await store.lockFolders()//lock each download folder that exists, leaving any another copy holds to it, and hand the engine the ones this copy holds
	} catch (error) {
		store.problems.push(`engine: ${error}`)//the engine isn't running, most likely, and its own status line says why
	}
}
