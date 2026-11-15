import './style.css'
import {createApp} from 'vue'
import App from './App.vue'
import router from './router.js'
import {installTips} from './tooltip.js'

let report = null//the latest trackers.json, or null when it can't be had
try {
	let response = await fetch('/trackers.json')//from the page's own domain, so no cross-origin rules apply; the server marks it no-cache, so this is always the latest hour
	if (response.ok) report = await response.json()
} catch {}//no answer, or one that isn't JSON, reads the same as a missing file

let app = createApp(App)
app.provide('report', report)
app.use(router)
app.mount('#app')
installTips()
