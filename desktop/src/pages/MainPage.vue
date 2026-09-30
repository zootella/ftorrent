<script setup>
import {ref, computed, onMounted, onUnmounted} from 'vue'
import {useSettingsStore} from '../stores/settings.js'
import {useIncomingStore} from '../stores/incoming.js'
import {engineStatus} from '../engine.js'
import {instanceStatus} from '../instance.js'
import {brandName} from '../brand.js'
import {useAssociationsStore} from '../stores/associations.js'

let store = useSettingsStore()//main.js loaded it before this page mounted, or is about to; the object is the same either way
let associations = useAssociationsStore()//what registration did and what windows opens each contested type with, lines for the report on an installed windows copy

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

//the engine process's status and this copy's lock, asked for once a second while this page is showing; what the engine said and what has arrived come from the incoming store, which takes them for the whole app
let incoming = useIncomingStore()
let engine = ref(null)
let instance = ref(null)//this copy's lock and handoff
let engineTimer = null
async function askEngine() { engine.value = await engineStatus(); instance.value = await instanceStatus() }
onMounted(() => { askEngine(); engineTimer = setInterval(askEngine, 1000) })
onUnmounted(() => clearInterval(engineTimer))
let engineLine = computed(() => {//one sentence about the engine, whatever state it is in
	let s = engine.value
	if (!s) return 'engine: asking…'
	if (s.trouble) return `engine: not started, ${s.trouble}`
	let r = incoming.ready
	if (s.running && r) return `engine: libtorrent ${r.libtorrent}, WebTorrent ${r.webtorrent ? 'on' : 'off'}, Python ${r.python}, pid ${s.pid}`
	if (s.running) return 'engine: starting…'
	return `engine: stopped${s.exit ? ', ' + s.exit : ''}`
})

//where everything is, as startup worked it out before this page existed; the settings store holds it, because the download folders resolve against it
let paths = computed(() => store.paths)
let pathsHeard = computed(() => incoming.ready?.paths?.data === paths.value?.data && !!paths.value?.data)//the engine sent back the data folder it was told, so the paths made the round trip
let foldersHeard = computed(() => {//and the download folders the page sent it, which are the ones this copy holds, in the same order, so the settings made the round trip too; an empty list counts, since a first run's default folder may not exist yet
	let sent = store.heldFolders
	let heard = incoming.folders
	return Array.isArray(heard) && heard.join('\n') == sent.join('\n')
})
let folderWords = {held: 'held', busy: `in use by another copy of ${brandName}`, missing: 'not on this machine'}//how each state reads on the page; trouble reads as itself, reason and all
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
		lines.push(`${brandName} is ${p.mode}${pathsHeard.value ? ', and the engine has its paths' : ''}`)
		lines.push(`program: ${p.location}`)
		lines.push(`data: ${p.data}`)
		lines.push(`settings: ${p.settings}${foldersHeard.value ? ', and the engine has its folders' : ''}`)
		for (let folder of store.resolvedFolders) lines.push(`downloads: ${folder.setting} → ${folder.path}${folderState(folder.path)}`)
		if (p.trouble) lines.push(p.trouble)
	}
	for (let problem of store.problems) lines.push(problem)
	lines.push(...associations.report)
	let i = instance.value
	if (i) {
		if (i.held) lines.push(`lock: held, ${i.lock}`)
		if (i.handoff) lines.push(`handoff: ${i.handoff}`)
		if (i.trouble) lines.push(i.trouble)
	}
	incoming.arrivals.forEach((arrival, n) => lines.push(`${n + 1}. ${arrival.from}: ${arrival.args.join(' ') || '(no arguments)'}`))
	for (let error of incoming.errors) lines.push(`engine said: ${error.message}${error.command !== undefined ? ', ' + JSON.stringify(error.command) : ''}`)
	let d = incoming.dropped
	if (d.engine > 0 || d.arrivals > 0) lines.push(`dropped because the page fell behind: ${d.engine} engine lines, ${d.arrivals} arrivals`)//zero always, unless something is wrong
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
	<main>
		<h1>{{ brandName }}</h1>

		<button type="button" @click="prepareFolder">{{ prepared ? 'Prepared' : 'Prepare download folder' }}</button>

		<div class="w-full">
			<textarea readonly :value="report" :rows="report.split('\n').length" class="block w-full mb-2 font-mono wrap-anywhere"></textarea><!-- wrap-anywhere because a path is one long word, and should wrap rather than push the window wider -->
			<button type="button" @click="copyReport">{{ copied ? 'Copied' : 'Copy' }}</button>
		</div>
	</main>
</template>
