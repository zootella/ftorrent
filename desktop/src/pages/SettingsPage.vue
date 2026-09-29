<script setup>
import {ref, watch} from 'vue'
import {useSettingsStore} from '../stores/settings.js'
import {useAssociationsStore} from '../stores/associations.js'
import {brandName} from '../brand.js'

let store = useSettingsStore()
let associations = useAssociationsStore()//the answer to whether ftorrent opens torrents and magnets, and whether windows agrees

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
			<label for="associations-default" class="mr-2">Open .torrent files and magnet links with {{ brandName }}</label>
			<select id="associations-default" :value="store.settings.associations.default" :disabled="!associations.installed" @change="associations.choose($event.target.value)">
				<option value="yes">Yes</option>
				<option value="no">No</option>
				<option value="ask">Ask when {{ brandName }} starts</option>
			</select>
			<!-- a copy the installer didn't place never registers anything, so it says why the choice is grayed; an installed one links to windows' own Settings when that's the only place left to settle a difference between this setting and windows -->
			<p v-if="!associations.installed" class="text-muted">Only {{ brandName }} installed on Windows sets up file types and links, so far.</p>
			<p v-else-if="associations.disagrees"><a href="#" @click.prevent="associations.openWindowsSettings()">Update in Windows Settings</a></p>
		</div>

		<form class="flex gap-2" @submit.prevent="saveNote">
			<input v-model="noteDraft" placeholder="A note to yourself..." />
			<button type="submit">{{ noteSaved ? 'Saved' : 'Save as Setting' }}</button>
		</form>
	</main>
</template>
