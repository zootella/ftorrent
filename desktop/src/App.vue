<script setup>
import {useAssociationsStore} from './stores/associations.js'
import {brandName} from './brand.js'
import {settingsName} from './settings.js'
import {listen} from '@tauri-apps/api/event'
import {useRouter} from 'vue-router'

let associations = useAssociationsStore()//the banner's question and answers; the store decides when it's up
let router = useRouter()
listen('menu', event => router.push({name: event.payload}))//the windows menu bar's Tools, Options and Help, About, which rust passes up as the name of the route to open; here in the shell, which lives as long as the app
</script>

<template>
	<!-- ./src/App.vue -->
	<!-- the shell every page sits inside: the banner when there's a question to ask, the navigation across the top, and the outlet the router fills with whichever page is current -->
	<div v-if="associations.bannerUp" class="flex items-center gap-2 py-2 pr-2 pl-4 bg-accent/10 border-b border-accent/25">
		<span class="flex-1">Use {{ brandName }} for .torrent files and magnet links? Windows may ask you to confirm in its Settings.</span>
		<button type="button" @click="associations.choose('yes')">Yes</button>
		<button type="button" @click="associations.choose('no')">No</button>
		<button type="button" class="px-2 bg-transparent border-transparent hover:border-transparent text-xl leading-none" aria-label="Close" @click="associations.dismiss()">×</button><!-- a bare ×, so it reads as closing the bar rather than as a third answer -->
	</div>
	<nav class="flex gap-4 px-4 pt-4">
		<router-link to="/" active-class="font-bold">Main</router-link>
		<router-link to="/settings" active-class="font-bold">{{ settingsName }}</router-link>
		<router-link to="/about" active-class="font-bold">About</router-link>
	</nav>
	<router-view />
</template>
