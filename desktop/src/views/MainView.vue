<script setup>
//./src/views/MainView.vue
import {ref} from 'vue'
import {invoke} from '@tauri-apps/api/core'

let name = ref('')//what the user typed into the form
let greetMessage = ref('')//what rust sent back, shown beneath it

//hand the name to the rust command named greet and show its answer; this round trip is the scaffold's proof that the webview can reach the native core, and it runs the same way in the dev window and the built app
async function greet() {
	greetMessage.value = await invoke('greet', {name: name.value})
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
	</main>
</template>

<style scoped>
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
