<script setup>
import {ref, computed, onMounted, version as vueVersion} from 'vue'
import {getVersion, getTauriVersion} from '@tauri-apps/api/app'
import {openUrl} from '@tauri-apps/plugin-opener'
import {version as viteVersion} from 'vite/package.json'//a build tool, so its version is read as this copy is built, and is the vite that built it
import {brandName, brandHomepage, brandHost} from '../brand.js'
import {windowWebviewVersion} from '../window.js'
import {processId} from '../process.js'
import {engineStatus} from '../engine.js'
import {useIncomingStore} from '../stores/incoming.js'

//the parts ftorrent is made of, and the version of each, asked of the part itself wherever it can answer at runtime; and for the two parts that are processes of ftorrent's own, this copy and the engine it started, the process id beside the version, so a running copy can be matched to the task manager and to its log file, which is named for the id
let incoming = useIncomingStore()//the engine's ready line names the libtorrent and python it runs on
let app = ref('')//ftorrent's own version, from tauri.conf.json
let pid = ref(0)//and its process id
let enginePid = ref(0)//the engine's, 0 until it's running
let tauri = ref('')
let webview = ref('')
onMounted(async () => {
	app.value = await getVersion()
	pid.value = await processId()
	tauri.value = await getTauriVersion()
	webview.value = await windowWebviewVersion().catch(error => `could not tell, ${error}`)//the platform may not say, and that's an answer worth showing
	let engine = await engineStatus()//asked once, as the page opens, like the versions; the main page is where the engine is watched
	if (engine.running) enginePid.value = engine.pid
})
let parts = computed(() => {
	let r = incoming.ready ?? {}//empty until the engine has said it's up
	let libtorrent = r.libtorrent ?? ''
	if (libtorrent && r.webtorrent) libtorrent += ', with WebTorrent'
	return [
		[brandName,    withPid(app.value, pid.value)],
		['Tauri',      tauri.value],
		['Web view',   webview.value],
		['Vue',        vueVersion],
		['Vite',       viteVersion],
		['libtorrent', withPid(libtorrent, enginePid.value)],//the engine's row, since the engine is the process that holds libtorrent
		['Python',     r.python ?? ''],
	]
})
function withPid(version, id) { return version && id > 0 ? `${version}, pid ${id}` : version }//a version with its process id after it, once both are known
</script>

<template>
	<!-- ./src/pages/AboutPage.vue -->
	<!-- where ftorrent lives on the web, and the parts it's built from, each with its version -->
	<main>
		<h1 class="font-brand text-brand">About {{ brandName }}</h1>
		<a :href="brandHomepage" @click.prevent="openUrl(brandHomepage)">{{ brandHost }}</a><!-- the system's browser opens it, rather than the web view navigating away from ftorrent -->
		<dl class="grid grid-cols-[auto_1fr] gap-x-4">
			<template v-for="[name, version] in parts" :key="name">
				<dt class="text-muted">{{ name }}</dt>
				<dd>{{ version }}</dd>
			</template>
		</dl>
	</main>
</template>
