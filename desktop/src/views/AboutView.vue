<script setup>
//./src/views/AboutView.vue
import {ref, onMounted} from 'vue'
import {getName, getVersion, getTauriVersion} from '@tauri-apps/api/app'

let name = ref('')//the productName from tauri.conf.json
let version = ref('')//the app version from there as well
let tauriVersion = ref('')//the version of tauri the binary was built against

//ask the rust core who we are, once, when the view first appears. These three commands live in tauri's core:app plugin, which the default capability already grants, so reaching them took no change to capabilities/default.json
onMounted(async () => {
	name.value = await getName()
	version.value = await getVersion()
	tauriVersion.value = await getTauriVersion()
})
</script>

<template>
	<main class="container">
		<h1>About</h1>
		<p>{{ name }} {{ version }}</p>
		<p>Built on Tauri {{ tauriVersion }}</p>
	</main>
</template>
