<script setup>
import {ref, watch} from 'vue'
import {useSettingsStore} from '../stores/settings.js'
import {useAssociationsStore} from '../stores/associations.js'
import {brandName} from '../brand.js'
import {themeWindow, fontWindow} from '../window.js'
import RadioGroup from '../components/RadioGroup.vue'
import {fontsOffered} from '../settings.js'

let store = useSettingsStore()
let associations = useAssociationsStore()//the answer to whether ftorrent opens torrents and magnets, and whether windows agrees

//the answers to each question on this page, as [value, words], in the order shown; the values are the ones settings.js checks
let associationChoices = [['yes', 'Yes'], ['no', 'No'], ['ask', `Ask when ${brandName} starts`]]
let appearanceChoices  = [['light', 'Light'], ['dark', 'Dark'], ['system', 'System']]//the order the mac's own appearance setting uses
let fontChoices        = [['system', 'System'], ['inter', 'Inter'], ['verdana', 'Verdana']].filter(([value]) => fontsOffered.includes(value))//verdana only on windows; settings.js says why

//light or dark, and the typeface, each taking effect at once and written to the file, the way a setting picked from its answers is, with no Save to press
async function chooseMode(mode) {
	store.settings.appearance.mode = mode
	await themeWindow(mode)
	await store.save()
}
async function chooseFont(font) {
	store.settings.appearance.font = font
	await fontWindow(font)
	await store.save()
}

//the note, a setting that does nothing except prove that settings work: type one, save it, quit, start again, and it's here, and in ftorrent.toml. The box holds a draft of its own so typing changes nothing until Save; a setting writes when the user acts, not on every keystroke
let noteDraft = ref(store.settings.note.text)
watch(() => store.settings.note.text, text => { noteDraft.value = text })//when load fills in the saved note a moment after mount, the box follows
let noteSaved = ref(false)//true for a moment after Save, so the button can say so
async function saveNote() {
	store.settings.note.text = noteDraft.value
	await store.save()
	noteSaved.value = true
	setTimeout(() => { noteSaved.value = false }, 1500)
}
</script>

<template>
	<!-- ./src/pages/SettingsPage.vue -->
	<!-- the settings a user changes from inside ftorrent, each written to ftorrent.toml as it changes -->
	<main>
		<h1>Settings</h1>

		<div>
			<RadioGroup :choices="associationChoices" :chosen="store.settings.associations.default" :disabled="!associations.installed" @choose="associations.choose">Open .torrent files and magnet links with {{ brandName }}</RadioGroup>
			<!-- a copy the installer didn't place never registers anything, so it says why the choice is grayed; an installed one links to windows' own Settings when that's the only place left to settle a difference between this setting and windows -->
			<p v-if="!associations.installed" class="text-muted">Only {{ brandName }} installed on Windows sets up file types and links, so far.</p>
			<p v-else-if="associations.disagrees"><a href="#" @click.prevent="associations.openWindowsSettings()">Update in Windows Settings</a></p>
		</div>

		<RadioGroup :choices="appearanceChoices" :chosen="store.settings.appearance.mode" @choose="chooseMode">Appearance</RadioGroup>
		<RadioGroup :choices="fontChoices"       :chosen="store.settings.appearance.font" @choose="chooseFont">Font</RadioGroup>

		<form class="flex gap-2" @submit.prevent="saveNote">
			<input v-model="noteDraft" placeholder="A note to yourself..." />
			<button type="submit">{{ noteSaved ? 'Saved' : 'Save as Setting' }}</button>
		</form>
	</main>
</template>
