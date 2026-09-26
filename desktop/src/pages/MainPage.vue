<script setup>
//./src/pages/MainPage.vue
import {ref, computed, watch, onMounted, onUnmounted} from 'vue'
import {storeToRefs} from 'pinia'
import {invoke} from '@tauri-apps/api/core'
import {useGreetStore} from '../stores/greet.js'
import {useSettingsStore} from '../stores/settings.js'
import {engineStatus} from '../engine.js'
import {instanceStatus} from '../instance.js'
import {associateStatus} from '../associate.js'

let {name} = storeToRefs(useGreetStore())//what the user typed, kept in the store so it's still here after a trip to the about page and back; storeToRefs hands back a writable ref, so v-model below works exactly as it did before
let greetMessage = ref('')//what rust sent back, shown beneath it; left as the component's own state on purpose, so it clears on navigation and the difference is visible side by side

//hand the name to the rust command named greet and show its answer; this round trip is the scaffold's proof that the webview can reach the native core, and it runs the same way in the dev window and the built app
async function greet() {
	greetMessage.value = await invoke('greet', {name: name.value})
}

//the note, a setting that does nothing except prove that settings work: type one, save it, quit, start again, and it's here, and in ftorrent.toml. The box holds a draft of its own so typing changes nothing until Save; a setting writes when the user acts, not on every keystroke
let store = useSettingsStore()//main.js loaded it before this page mounted, or is about to; the object is the same either way
let noteDraft = ref(store.settings.note.text)
watch(() => store.settings.note.text, text => { noteDraft.value = text })//when load fills in the saved note a moment after mount, the box follows
let noteSaved = ref(false)//true for a moment after Save, so the button can say so
async function saveNote() {
	store.settings.note.text = noteDraft.value
	await store.save()
	noteSaved.value = true
	setTimeout(() => { noteSaved.value = false }, 1500)
}

//get the default download folder ready, the way starting a torrent will; a stand-in for the add-torrent flow until there are torrents, so ftorrent never makes a folder at startup
let prepared = ref(false)//true for a moment after the button, so it can say so
async function prepareFolder() {
	let first = store.resolvedFolders[0]
	if (!first) return//no download folders in the settings at all
	try {
		await store.prepareFolder(first.path)
	} catch (error) {
		store.problems.push(`engine: ${error}`)//the engine isn't running, most likely, and its own status line says why
	}
	prepared.value = true
	setTimeout(() => { prepared.value = false }, 1500)
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

//what registration did at startup, asked for once; blank everywhere but an installed windows copy
let associations = ref('')
onMounted(async () => { associations.value = await associateStatus() })

//where everything is, as startup worked it out before this page existed; the settings store holds it, because the download folders resolve against it
let paths = computed(() => store.paths)
let pathsHeard = computed(() => engine.value?.ready?.paths?.data === paths.value?.data && !!paths.value?.data)//the engine sent back the data folder it was told, so the paths made the round trip
let foldersHeard = computed(() => {//and the download folders the page sent it, which are the ones this copy holds, in the same order, so the settings made the round trip too; an empty list counts, since a first run's default folder may not exist yet
	let sent = store.heldFolders
	let heard = engine.value?.folders?.folders
	return Array.isArray(heard) && heard.join('\n') == sent.join('\n')
})
let folderWords = {held: 'held', busy: 'in use by another copy of ftorrent', missing: 'not on this machine'}//how each state reads on the page; trouble reads as itself, reason and all
function folderState(path) {//how the resolved folder at this path stands, in words, or blank before the page has asked
	let state = store.folderStates[path]
	if (!state) return ''
	return `, ${folderWords[state] ?? state}`
}

//everything above as plain lines, in one box the user can copy from, since a status is most useful pasted into a message or an issue
let report = computed(() => {
	let lines = [engineLine.value]
	let p = paths.value
	if (p) {
		lines.push(`ftorrent is ${p.mode}${pathsHeard.value ? ', and the engine has its paths' : ''}`)
		lines.push(`program: ${p.location}`)
		lines.push(`data: ${p.data}`)
		lines.push(`settings: ${p.settings}${foldersHeard.value ? ', and the engine has its folders' : ''}`)
		for (let folder of store.resolvedFolders) lines.push(`downloads: ${folder.setting} → ${folder.path}${folderState(folder.path)}`)
		if (p.trouble) lines.push(p.trouble)
	}
	for (let problem of store.problems) lines.push(problem)
	if (associations.value) lines.push(associations.value)
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

		<form class="row" @submit.prevent="saveNote">
			<input id="note-input" v-model="noteDraft" placeholder="A note to yourself..." />
			<button type="submit">{{ noteSaved ? 'Saved' : 'Save as Setting' }}</button>
		</form>

		<div class="row">
			<button type="button" @click="prepareFolder">{{ prepared ? 'Prepared' : 'Prepare download folder' }}</button>
		</div>

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
