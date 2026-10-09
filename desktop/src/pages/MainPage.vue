<script setup>
import {ref} from 'vue'
import {useSettingsStore} from '../stores/settings.js'
import {brandName} from '../brand.js'
import {log} from '../log.js'

let store = useSettingsStore()//main.js loaded it before this page mounted, or is about to; the object is the same either way

//get the default download folder ready, the way starting a torrent will; a stand-in for the add-torrent flow until there are torrents, so ftorrent never makes a folder at startup
let prepared = ref(false)//true for a moment after the button, so it can say so
async function prepareFolder() {
	let first = store.resolvedFolders[0]
	if (!first) return//no download folders in the settings at all
	try {
		await store.prepareFolder(first.path)
	} catch (error) {
		log(`engine: ${error}`)//the engine isn't running, most likely, and its own line in the log says why
	}
	prepared.value = true
	setTimeout(() => { prepared.value = false }, 1500)
}
</script>

<template>
	<main>
		<h1>{{ brandName }}</h1>

		<button type="button" @click="prepareFolder">{{ prepared ? 'Prepared' : 'Prepare download folder' }}</button>
	</main>
</template>
