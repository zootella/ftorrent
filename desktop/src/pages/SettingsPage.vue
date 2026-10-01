<script setup>
import {ref, watch} from 'vue'
import {useSettingsStore} from '../stores/settings.js'
import {useAssociationsStore} from '../stores/associations.js'
import {brandName} from '../brand.js'
import {themeWindow, fontWindow} from '../window.js'
import RadioGroup from '../components/RadioGroup.vue'
import {fontsOffered, settingsName, platformName, systemFace} from '../settings.js'

let store = useSettingsStore()
let associations = useAssociationsStore()//the answer to whether ftorrent opens torrents and magnets, and whether windows agrees

//the answers to each question on this page, as [value, words], in the order shown; the values are the ones settings.js checks
let associationChoices = [['yes', 'Yes'], ['no', 'No'], ['ask', `Ask when ${brandName} starts`]]
let appearanceChoices  = [['light', 'Light'], ['dark', 'Dark'], ['system', 'System']]//the same three words on every platform
let fontChoices        = [//the words in three parts, so the template can set the face's own name in italics, and San Francisco reads as a typeface rather than the city
	['system',  systemFace ? {face: systemFace.face, trail: `, from ${systemFace.maker}, the system font`} : {lead: 'System font'}],//named on windows and the mac; linux's is whatever the desktop sets, so it isn't named
	['inter',   {face: 'Inter', trail: ', by Rasmus Andersson, cross-platform'}],
	['verdana', {face: 'Verdana', trail: ', vibing Windows XP'}],
].filter(([value]) => fontsOffered.includes(value))//verdana only on windows; settings.js says why

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
		<h1>{{ settingsName }}</h1><!-- options on windows, settings elsewhere; settings.js says why -->

		<div>
			<RadioGroup :choices="associationChoices" :chosen="store.settings.associations.default" :disabled="!associations.installed" @choose="associations.choose">Open <em>.torrent</em> files and <em>magnet:</em> links with {{ brandName }}</RadioGroup><!-- the two people know; the answer covers ftorrent's own two as well, which the settings file names -->
			<!-- a copy the installer didn't place never registers anything, and its choice is simply grayed; an installed one on windows links to windows' own Settings, the one place a saved choice of another program can be changed -->
			<p v-if="associations.installed && associations.disagrees && platformName == 'Windows'"><a href="#" @click.prevent="associations.openWindowsSettings()">Update in Windows Settings</a></p>
		</div>

		<RadioGroup :choices="appearanceChoices" :chosen="store.settings.appearance.mode" @choose="chooseMode">Appearance</RadioGroup>
		<RadioGroup :choices="fontChoices"       :chosen="store.settings.appearance.font" @choose="chooseFont">UI Font
			<template #answer="{words}">{{ words.lead }}<em v-if="words.face">{{ words.face }}</em>{{ words.trail }}</template>
		</RadioGroup>

		<form class="flex gap-2" @submit.prevent="saveNote">
			<input v-model="noteDraft" placeholder="A note to yourself..." />
			<button type="submit">{{ noteSaved ? 'Saved' : 'Save as Setting' }}</button>
		</form>
	</main>
</template>
