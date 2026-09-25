<script setup>
//./src/pages/MainPage.vue
import {ref, computed, onMounted, onUnmounted} from 'vue'
import {storeToRefs} from 'pinia'
import {invoke} from '@tauri-apps/api/core'
import {useGreetStore} from '../stores/greet.js'
import {engineStatus} from '../engine.js'
import {pathsStatus} from '../paths.js'

let {name} = storeToRefs(useGreetStore())//what the user typed, kept in the store so it's still here after a trip to the about page and back; storeToRefs hands back a writable ref, so v-model below works exactly as it did before
let greetMessage = ref('')//what rust sent back, shown beneath it; left as the component's own state on purpose, so it clears on navigation and the difference is visible side by side

//hand the name to the rust command named greet and show its answer; this round trip is the scaffold's proof that the webview can reach the native core, and it runs the same way in the dev window and the built app
async function greet() {
	greetMessage.value = await invoke('greet', {name: name.value})
}

//the engine's status, asked for once a second while this page is showing; a fact from below that this one page displays for now, so it lives here rather than in a store
let engine = ref(null)
let engineTimer = null
async function askEngine() { engine.value = await engineStatus() }
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

		<p>{{ engineLine }}</p>

		<div v-if="paths" class="paths">
			<p v-if="paths.mode == 'translocated'">macOS is running ftorrent from a temporary copy at {{ paths.location }}, so it can't see its own folder. To fix it, quit ftorrent, run <code>xattr -dr com.apple.quarantine</code> on ftorrent.app where you put it, and open it again.</p>
			<template v-else>
				<p>ftorrent is {{ paths.mode }}{{ pathsHeard ? ', and the engine has its paths' : '' }}</p>
				<p>program: {{ paths.location }}</p>
				<p>data: {{ paths.data }}</p>
				<p v-for="folder in paths.download_folders" :key="folder.setting">downloads: {{ folder.setting }} → {{ folder.path }}</p>
			</template>
			<p v-if="paths.trouble">{{ paths.trouble }}</p>
		</div>
	</main>
</template>

<style scoped>
.paths p {
	margin: 0.2em 0;
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
