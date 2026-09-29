<script setup>
import {ref, computed, onMounted, version as vueVersion} from 'vue'
import {getVersion, getTauriVersion} from '@tauri-apps/api/app'
import {openUrl} from '@tauri-apps/plugin-opener'
import {version as viteVersion} from 'vite/package.json'//a build tool, so its version is read as this copy is built, and is the vite that built it
import {brandName, brandHomepage} from '../brand.js'
import {windowWebviewVersion} from '../window.js'
import {useIncomingStore} from '../stores/incoming.js'

let homepageName = new URL(brandHomepage).host//ftorrent.com, the address the way a person says it

//the parts ftorrent is made of, and the version of each, asked of the part itself wherever it can answer at runtime
let incoming = useIncomingStore()//the engine's ready line names the libtorrent and python it runs on
let app = ref('')//ftorrent's own version, from tauri.conf.json
let tauri = ref('')
let webview = ref('')
onMounted(async () => {
	app.value = await getVersion()
	tauri.value = await getTauriVersion()
	webview.value = await windowWebviewVersion().catch(error => `could not tell, ${error}`)//the platform may not say, and that's an answer worth showing
})
let parts = computed(() => {
	let r = incoming.ready ?? {}//empty until the engine has said it's up
	let libtorrent = r.libtorrent ?? ''
	if (libtorrent && r.webtorrent) libtorrent += ', with WebTorrent'
	return [
		[brandName,    app.value],
		['Tauri',      tauri.value],
		['Web view',   webview.value],
		['Vue',        vueVersion],
		['Vite',       viteVersion],
		['libtorrent', libtorrent],
		['Python',     r.python ?? ''],
	]
})
</script>

<template>
	<!-- ./src/pages/AboutPage.vue -->
	<!-- where ftorrent lives on the web, and the parts it's built from, each with its version -->
	<main>
		<h1 class="font-brand">About {{ brandName }}</h1>
		<a :href="brandHomepage" @click.prevent="openUrl(brandHomepage)">{{ homepageName }}</a><!-- the system's browser opens it, rather than the web view navigating away from ftorrent -->
		<dl class="grid grid-cols-2 gap-x-4 text-left">
			<template v-for="[name, version] in parts" :key="name">
				<dt class="text-right text-muted">{{ name }}</dt>
				<dd>{{ version }}</dd>
			</template>
		</dl>
	</main>
</template>
