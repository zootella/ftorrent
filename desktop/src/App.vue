<script setup>
import {useAssociationsStore} from './stores/associations.js'
import {brandName} from './brand.js'

let associations = useAssociationsStore()//the banner's question and answers; the store decides when it's up
let sampleText = 'Sphinx of black quartz, judge my vow. AVATAR Wavy Tofu QGRSJ 0123456789 Il1| O0 rn m'//a line whose letters give a typeface away, the same as the menu lifecycle.rs puts before File on windows, so the system's menu text and the page's can be compared one above the other
let throwbackTexts = ['cygwin', 'Data', 'Documents', 'Documents and Settings', 'KPCMS', 'libtorrent', 'MSOCache', 'openssl', 'Perl', 'Program Files', 'RECYCLER', 'System Volume Information', 'WINDOWS']//the folders a windows xp explorer window shows, exactly as it lists them, for comparing the verdana choice with the real thing side by side
</script>

<template>
	<!-- ./src/App.vue -->
	<!-- the shell every page sits inside: the sample lines, the banner when there's a question to ask, the navigation across the top, and the outlet the router fills with whichever page is current -->
	<p class="pl-[7px] pt-4">{{ sampleText }}</p><!-- 7px rather than the page's 12, so it starts right under the same text in the menu bar -->
	<p v-for="text in throwbackTexts" :key="text" class="pl-[7px]">{{ text }}</p>
	<div v-if="associations.bannerUp" class="flex items-center gap-2 py-2 pr-2 pl-4 bg-accent/10 border-b border-accent/25">
		<span class="flex-1">Use {{ brandName }} for .torrent files and magnet links? Windows may ask you to confirm in its Settings.</span>
		<button type="button" @click="associations.choose('yes')">Yes</button>
		<button type="button" @click="associations.choose('no')">No</button>
		<button type="button" class="px-2 bg-transparent border-transparent hover:border-transparent text-xl leading-none" aria-label="Close" @click="associations.dismiss()">×</button><!-- a bare ×, so it reads as closing the bar rather than as a third answer -->
	</div>
	<nav class="flex gap-4 px-4 pt-4">
		<router-link to="/" active-class="font-bold">Main</router-link>
		<router-link to="/settings" active-class="font-bold">Settings</router-link>
		<router-link to="/about" active-class="font-bold">About</router-link>
	</nav>
	<router-view />
</template>
