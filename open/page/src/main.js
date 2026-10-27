import './style.css'
import {createApp} from 'vue'
import App from './App.vue'

let page = null//the gauge's latest numbers, or null when they can't be had, and the page shows everything but the panel: the globe, the announce URLs, and the quote need none of them
try {
	let response = await fetch('/page.json')
	if (response.ok) page = await response.json()
} catch {}//no answer, or one that isn't JSON, reads the same as a missing file

let app = createApp(App)
app.provide('page', page)
app.mount('#app')
