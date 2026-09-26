//./src/main.js
import {createApp} from 'vue'
import {createPinia} from 'pinia'
import App from './App.vue'
import router from './router/index.js'
import {revealWindow, watchWindow} from './window.js'
import {pathsStatus} from './paths.js'
import {useSettingsStore} from './stores/settings.js'
import './index.css'

let pinia = createPinia()//one pinia for the life of the app: this process opens a single window once and closes it once, so there is never a second store to keep in step
createApp(App).use(pinia).use(router).mount('#app')
revealWindow()//rust made the window hidden, at its remembered size and place; show it now that there's something in it to see
startup().catch(error => useSettingsStore(pinia).problems.push(`startup: ${error}`))//and read the settings, which nothing before this point could do, because the page is what knows what a setting is. The catch is the one top gate for the page's startup: whatever escaped shows on the main page rather than vanishing as an unhandled rejection

//the page's half of starting up, after rust's half in setup: read the settings file, keep it told where the window is, and lock the download folders it names and hand the engine the ones this copy holds. Rust started the engine before the page existed and told it everything else; the folders wait for here because they are a setting, and only the page reads those
async function startup() {
	let store = useSettingsStore(pinia)//outside a component, a store needs the pinia handed to it
	await store.load(await pathsStatus())//where everything is, and then the settings file in the data folder, if there is one
	if (!store.paths.settings) return//no data folder, which the platform should always give: no window place to record and no engine to tell, and the main page shows the trouble
	await watchWindow(store)//after load, so recording the window's place lands in settings that are already filled in from the file
	try {
		await store.lockFolders()//lock each download folder that exists, leaving any another copy holds to it, and hand the engine the ones this copy holds
	} catch (error) {
		store.problems.push(`engine: ${error}`)//the engine isn't running, most likely, and its own status line says why
	}
}
