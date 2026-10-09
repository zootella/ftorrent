<script setup>
import {useAssociationsStore} from './stores/associations.js'
import {brandName, brandDocs} from './brand.js'
import {settingsName} from './settings.js'
import {listen} from '@tauri-apps/api/event'
import {openUrl} from '@tauri-apps/plugin-opener'
import {useRouter} from 'vue-router'

let associations = useAssociationsStore()//the banner's question and answers; the store decides when it's up
let router = useRouter()
listen('menu', event => event.payload == 'help' ? openUrl(brandDocs) : router.push({name: event.payload}))//the menus' Options, Settings, and About, which rust passes up as the name of the route to open, and Help, which opens the documentation site in the system's browser; here in the shell, which lives as long as the app
</script>

<template>
	<!-- ./src/App.vue -->
	<!-- the shell every page sits inside: the banner when there's a question to ask, the navigation across the top, and the outlet the router fills with whichever page is current -->
	<!-- the question names the two types people know, .torrent and magnet:, while the answer covers all four and the bar is up if any of the four opens with another app; the same stance for the whole group, on purpose, as associate.js says -->
	<div v-if="associations.bannerUp" class="flex items-center gap-line px-line bg-line/40 border-b border-line">
		<span class="flex-1">Open <em>.torrent</em> files and <em>magnet:</em> links with {{ brandName }}?</span>
		<button type="button" @click="associations.choose('yes')">Yes</button>
		<button type="button" @click="associations.choose('no')">No</button>
		<button type="button" class="bg-transparent border-transparent hover:border-transparent text-heading leading-none" aria-label="Close" @click="associations.dismiss()">×</button><!-- a bare ×, so it reads as closing the bar rather than as a third answer -->
	</div>
	<nav class="flex gap-line px-line pt-line">
		<router-link to="/" active-class="font-bold">Main</router-link>
		<router-link to="/settings" active-class="font-bold">{{ settingsName }}</router-link>
		<router-link to="/about" active-class="font-bold">About</router-link>
	</nav>
	<router-view />
</template>
