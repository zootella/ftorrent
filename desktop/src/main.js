import {createApp} from 'vue'
import {createPinia} from 'pinia'
import App from './App.vue'
import router from './router/index.js'
import {revealWindow, watchWindow} from './window.js'
import {pathsStatus} from './paths.js'
import {useSettingsStore} from './stores/settings.js'
import {useIncomingStore} from './stores/incoming.js'
import {associate} from './associate.js'
import './index.css'

let pinia = createPinia()//one pinia for the life of the app: this process opens a single window once and closes it once, so there is never a second store to keep in step
createApp(App).use(pinia).use(router).mount('#app')
useIncomingStore(pinia).start()//start taking what comes up from rust, the engine's lines and this copy's arrivals, for the life of the app; first, so the engine's ready is read as soon as it's there
startup().catch(error => useSettingsStore(pinia).problems.push(`startup: ${error}`))//read the settings, place and show the window, and lock the download folders, none of which anything before this point could do, because the page is what knows what a setting is. The catch is the one top gate for the page's startup: whatever escaped shows on the main page rather than vanishing as an unhandled rejection

//the page's half of starting up, after rust's half in setup: read the settings file, place the window where it remembers and show it, keep it told where the window is, and lock the download folders it names and hand the engine the ones this copy holds. Rust started the engine before the page existed and told it everything else; the folders wait for here because they are a setting, and only the page reads those
async function startup() {
	let store = useSettingsStore(pinia)//outside a component, a store needs the pinia handed to it
	try {
		await store.load(await pathsStatus())//where everything is, and then the settings file in the data folder, if there is one
	} finally {
		await revealWindow(store.settings)//rust made the window hidden; place it where the settings remember, or somewhere fresh when they hold nothing or couldn't be read, and show it, whatever happened above
	}
	if (!store.paths?.settings) return//no data folder, which the platform should always give: no window place to record and no engine to tell, and the main page shows the trouble
	await associate(store.paths)//on windows, an installed copy tells the system what it can open; everywhere else this returns at once
	await watchWindow(store)//after load, so recording the window's place lands in settings that are already filled in from the file
	try {
		await store.lockFolders()//lock each download folder that exists, leaving any another copy holds to it, and hand the engine the ones this copy holds
	} catch (error) {
		store.problems.push(`engine: ${error}`)//the engine isn't running, most likely, and its own status line says why
	}
}
