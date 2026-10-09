<script setup>
import {ref, computed, onMounted, version as vueVersion} from 'vue'
import {getVersion, getTauriVersion} from '@tauri-apps/api/app'
import {version as viteVersion} from 'vite/package.json'//a build tool, so its version is read as this copy is built, and is the vite that built it
import {version as tailwindVersion} from 'tailwindcss/package.json'//the same for tailwind, which the build compiled the stylesheet with
import {brandName, brandHomepage} from '../brand.js'
import brandIcon from '../../src-tauri/icons/app-icon-wide.svg'//the mark cropped to the pill, from the file the icon studio writes beside the application icon's sources; vite inlines a file this small as a data uri, which the content security policy allows for images
import {windowWebviewVersion} from '../window.js'
import {webviewWords} from '../settings.js'
import {processId, processOpen} from '../process.js'
import {engineStatus} from '../engine.js'
import {useIncomingStore} from '../stores/incoming.js'

//the parts ftorrent is made of, and the version of each, asked of the part itself wherever it can answer at runtime; and for the two parts that are processes of ftorrent's own, this copy and the engine it started, the process id beside the version, so a running copy can be matched to the task manager and to its log file, which is named for the id
let incoming = useIncomingStore()//the engine's ready line names the libtorrent and python it runs on
let app = ref('')//ftorrent's own version, from tauri.conf.json
let pid = ref(0)//and its process id
let enginePid = ref(0)//the engine's, 0 until it's running
let tauri = ref('')
let webview = ref('')
let homepageWords = brandHomepage.replace(/\/$/, '')//the link's words are the whole address, https://ftorrent.com, without the trailing slash tauri.conf.json writes
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
		['libtorrent', withPid(libtorrent, enginePid.value)],//the engine's row, since the engine is the process that holds libtorrent
		['Web view',   `${webviewWords.engine} ${webview.value}, ${webviewWords.host}`],//the engine and its version, then the system component that carries it, so the number has a name on every platform
		['Python',     r.python ?? ''],
		['Tauri',      tauri.value],
		['Vite',       viteVersion],
		['Vue',        vueVersion],
		['Tailwind',   tailwindVersion],
	]
})
function withPid(version, id) { return version && id > 0 ? `${version}, pid ${id}` : version }//a version with its process id after it, once both are known
</script>

<template>
	<!-- ./src/pages/AboutPage.vue -->
	<!-- where ftorrent lives on the web, and the parts it's built from, each with its version -->
	<main>
		<dl class="grid grid-cols-[auto_1fr] gap-x-line">
			<template v-for="[name, version] in parts" :key="name">
				<dt class="text-muted">{{ name }}</dt>
				<dd>{{ version }}</dd>
			</template>
		</dl>
		<a :href="brandHomepage" @click.prevent="processOpen(brandHomepage)" class="block w-fit">{{ homepageWords }}</a><!-- the system's browser opens it, rather than the web view navigating away from ftorrent; a block as wide as its words, so it sits in the page's column like every other part -->
		<h1 class="font-brand text-brand text-title">{{ brandName }}</h1><!-- the name and the mark close the page, below the facts -->
		<img :src="brandIcon" alt="" class="w-mark" /><!-- as wide as the theme says and half as tall, the pill and nothing around it -->
	</main>
</template>
