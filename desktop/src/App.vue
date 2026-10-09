<script setup>
import {useAssociationsStore} from './stores/associations.js'
import {brandName, urlHelp} from './brand.js'
import {settingsName, platformName} from './settings.js'
import {listen} from '@tauri-apps/api/event'
import {processOpen} from './process.js'
import {menuEnable} from './menu.js'
import {log} from './log.js'
import {useRouter} from 'vue-router'

let associations = useAssociationsStore()//the banner's question and answers; the store decides when it's up
let router = useRouter()
listen('menu', event => event.payload == 'help' ? processOpen(`https://${urlHelp}`) : router.push({name: event.payload}))//the menus' Options, Settings, and About, which rust passes up as the name of the route to open, and Help, which opens the documentation site in the system's browser; here in the shell, which lives as long as the app

if (platformName == 'macOS') {//the mac's Edit menu, whose six items are the system's own, Cut, Copy, Paste and the rest, answered by the web view: they start gray, and the page lights each one while something on it can answer, the way a mac app's Edit menu reads. The page decides rather than AppKit because the menu library turns off AppKit's own asking, and because a torrent list will one day light Paste for a magnet link and Select All for its rows, which only the page can know
	let lit = {Undo: false, Redo: false, Cut: false, Copy: false, Paste: false, 'Select All': false}//as the menu starts, from lifecycle.rs
	function editMenu() {
		let field = document.activeElement
		let typing = !!field && field.matches('input:not([type=radio]):not([type=checkbox]), textarea, [contenteditable]')//a field that takes typing has focus
		let selected = (document.getSelection()?.toString() ?? '') != ''//text is selected, in a field or on the page
		let want = {Undo: typing, Redo: typing, Cut: typing && selected, Copy: selected, Paste: typing, 'Select All': typing}
		for (let [item, enabled] of Object.entries(want)) if (lit[item] != enabled) { lit[item] = enabled; menuEnable('Edit', item, enabled).catch(error => log(`menu: ${item}, ${error}`)) }//only what changed, since each call crosses to rust
	}
	for (let name of ['focusin', 'focusout', 'selectionchange']) document.addEventListener(name, editMenu)//focus arriving, focus leaving, and the selection changing are the three moments the answer can change
}
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
