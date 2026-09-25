<script setup>
//./src/pages/MainPage.vue
import {ref, computed, onMounted, onUnmounted} from 'vue'
import {storeToRefs} from 'pinia'
import {invoke} from '@tauri-apps/api/core'
import {useGreetStore} from '../stores/greet.js'
import {engineStatus} from '../engine.js'
import {pathsStatus} from '../paths.js'
import {instanceStatus} from '../instance.js'

let {name} = storeToRefs(useGreetStore())//what the user typed, kept in the store so it's still here after a trip to the about page and back; storeToRefs hands back a writable ref, so v-model below works exactly as it did before
let greetMessage = ref('')//what rust sent back, shown beneath it; left as the component's own state on purpose, so it clears on navigation and the difference is visible side by side

//hand the name to the rust command named greet and show its answer; this round trip is the scaffold's proof that the webview can reach the native core, and it runs the same way in the dev window and the built app
async function greet() {
	greetMessage.value = await invoke('greet', {name: name.value})
}

//the engine's status, asked for once a second while this page is showing; a fact from below that this one page displays for now, so it lives here rather than in a store
let engine = ref(null)
let instance = ref(null)//this copy's lock and handoff, and the requests that have reached it
let engineTimer = null
async function askEngine() { engine.value = await engineStatus(); instance.value = await instanceStatus() }//the instance rides the same timer, so a request handed over from a second launch shows up within a second
onMounted(() => { askEngine(); engineTimer = setInterval(askEngine, 1000) })
onUnmounted(() => clearInterval(engineTimer))
let engineLine = computed(() => {//one sentence about the engine, whatever state it is in
	let s = engine.value
	if (!s) return 'engine: asking…'
	if (s.trouble) return `engine: not started, ${s.trouble}`
	if (s.running && s.ready) return `engine: libtorrent ${s.ready.libtorrent}, WebTorrent ${s.ready.webtorrent ? 'on' : 'off'}, Python ${s.ready.python}, pid ${s.pid}`
	if (s.running) return 'engine: starting…'
	return `engine: stopped${s.exit ? ', ' + s.exit : ''}`
})

//where everything is, asked for once, since startup worked it out before this page existed and none of it changes while the app runs
let paths = ref(null)
onMounted(async () => { paths.value = await pathsStatus() })
let pathsHeard = computed(() => engine.value?.ready?.paths?.data === paths.value?.data && !!paths.value?.data)//the engine sent back the data folder it was told, so the paths made the round trip

//everything above as plain lines, in one box the user can copy from, since a status is most useful pasted into a message or an issue
let report = computed(() => {
	let lines = [engineLine.value]
	let p = paths.value
	if (p) {
		if (p.mode == 'translocated') {
			lines.push(`macOS is running ftorrent from a temporary copy at ${p.location}, so it can't see its own folder. To fix it, quit ftorrent, run xattr -dr com.apple.quarantine on ftorrent.app where you put it, and open it again.`)
		} else {
			lines.push(`ftorrent is ${p.mode}${pathsHeard.value ? ', and the engine has its paths' : ''}`)
			lines.push(`program: ${p.location}`)
			lines.push(`data: ${p.data}`)
			for (let folder of p.download_folders) lines.push(`downloads: ${folder.setting} → ${folder.path}`)
		}
		if (p.trouble) lines.push(p.trouble)
	}
	let i = instance.value
	if (i) {
		if (i.held) lines.push(`lock: held, ${i.lock}`)
		if (i.handoff) lines.push(`handoff: ${i.handoff}`)
		if (i.trouble) lines.push(i.trouble)
		i.requests.forEach((request, n) => lines.push(`${n + 1}. ${request.from}: ${request.args.join(' ') || '(no arguments)'}`))
	}
	return lines.join('\n')
})
let copied = ref(false)//true for a moment after the button is pressed, so it can say so
async function copyReport() {
	await navigator.clipboard.writeText(report.value)
	copied.value = true
	setTimeout(() => { copied.value = false }, 1500)
}
</script>

<template>
	<main class="container">
		<h1>Welcome to ftorrent, powered by Tauri + Vue</h1>

		<div class="row">
			<a href="https://vite.dev" target="_blank">
				<img src="/vite.svg" class="logo vite" alt="Vite logo" />
			</a>
			<a href="https://tauri.app" target="_blank">
				<img src="/tauri.svg" class="logo tauri" alt="Tauri logo" />
			</a>
			<a href="https://vuejs.org/" target="_blank">
				<img src="../assets/vue.svg" class="logo vue" alt="Vue logo" />
			</a>
		</div>
		<p>Click on the Tauri, Vite, and Vue logos to learn more.</p>

		<form class="row" @submit.prevent="greet">
			<input id="greet-input" v-model="name" placeholder="Enter a name..." />
			<button type="submit">Greet</button>
		</form>
		<p>{{ greetMessage }}</p>

		<div class="report">
			<textarea readonly :value="report" :rows="report.split('\n').length"></textarea>
			<button type="button" @click="copyReport">{{ copied ? 'Copied' : 'Copy' }}</button>
		</div>
	</main>
</template>

<style scoped>
.report {
	text-align: left;
}

.report textarea {
	display: block;
	width: 100%;
	box-sizing: border-box;/* the border and padding inside the width, so full width means the page's width and no more */
	border: 1px solid #888;
	padding: 0.5em;
	margin-bottom: 0.5em;
	font-family: ui-monospace, monospace;
	font-size: 0.8em;
	resize: vertical;
	overflow-wrap: anywhere;/* a path is one long word, and should wrap rather than push the window wider */
}

.logo.vite:hover {
	filter: drop-shadow(0 0 2em #747bff);
}

.logo.tauri:hover {
	filter: drop-shadow(0 0 2em #24c8db);
}

.logo.vue:hover {
	filter: drop-shadow(0 0 2em #249b73);
}
</style>
