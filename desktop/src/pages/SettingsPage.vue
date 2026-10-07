<script setup>
import {computed} from 'vue'
import {useSettingsStore} from '../stores/settings.js'
import {useAssociationsStore} from '../stores/associations.js'
import {useLoginStore} from '../stores/login.js'
import {useUpdateStore} from '../stores/update.js'
import {brandName} from '../brand.js'
import {themeWindow, fontWindow} from '../window.js'
import RadioGroup from '../components/RadioGroup.vue'
import {fontsOffered, settingsName, systemFace, loginWords, platformName} from '../settings.js'
import {sayAgo} from '../time.js'

let store = useSettingsStore()
let associations = useAssociationsStore()//the answer to whether ftorrent opens torrents and magnets, and whether this copy is installed, the only kind that may answer
let login = useLoginStore()//whether the user wants ftorrent to start at login, and whether the system agrees
let update = useUpdateStore()//the section that checks for a newer version

//the answers to each question on this page, as [value, words], in the order shown; the values are the ones settings.js checks
let associationChoices = [['yes', 'Yes'], ['no', 'No'], ['ask', 'Ask']]
let loginChoices       = [['yes', 'Yes'], ['no', 'No']]//the setting is true or false, and the group speaks in strings
let appearanceChoices  = [['light', 'Light'], ['dark', 'Dark'], ['system', 'System']]//the same three words on every platform
let fontChoices        = [//the words in three parts, so the template can set the face's own name in italics, and San Francisco reads as a typeface rather than the city
	['system',  systemFace ? {face: systemFace.face, trail: `, from ${systemFace.maker}, the system font`} : {lead: 'System font'}],//named on windows and the mac; linux's is whatever the desktop sets, so it isn't named
	['inter',   {face: 'Inter', trail: ', by Rasmus Andersson, cross-platform'}],
	['verdana', {face: 'Verdana', trail: ', vibing Windows XP'}],
].filter(([value]) => fontsOffered.includes(value))//verdana on windows and the mac, not linux; settings.js says why

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

async function chooseAutomatic(automatic) {
	store.settings.update.automatic = automatic
	await store.save()
	update.tick()//turned on when a check is due checks now, as startup would
}
let lastChecked = computed(() => {//worked out when the page opens or a check lands, and not counting up while it stays open
	let last = Date.parse(store.settings.update.last)
	return isNaN(last) ? '' : sayAgo(Date.now() - last)
})

</script>

<template>
	<!-- ./src/pages/SettingsPage.vue -->
	<!-- the settings a user changes from inside ftorrent, each written to ftorrent.toml as it changes -->
	<main>
		<h1>{{ settingsName }}</h1><!-- options on windows, settings elsewhere; settings.js says why -->

		<div>
			<RadioGroup :choices="associationChoices" :chosen="store.settings.associations.default" :disabled="!associations.installed" @choose="associations.choose">Open <em>.torrent</em> files and <em>magnet:</em> links with {{ brandName }}</RadioGroup><!-- the two people know; the answer covers ftorrent's own two as well, which the settings file names. A copy the installer didn't place never registers anything, and its choice is simply grayed. This page only records the answer: the bar is where ftorrent asks, and a yes there opens windows' own Settings when windows doesn't agree -->
		</div>

		<div v-if="platformName != 'Linux'"><!-- starting at login is a Windows and Mac feature, so linux doesn't show the question at all -->
			<RadioGroup :choices="loginChoices" :chosen="login.wanted ? 'yes' : 'no'" :disabled="!login.installed" @choose="value => login.choose(value == 'yes')">{{ loginWords.question }}</RadioGroup><!-- the user's answer, which only a click here changes, and which sets the system to match; off at the factory, asked about nowhere else, and grayed for any copy the installer didn't place -->
			<p v-if="login.differs"><a href="#" @click.prevent="login.openSettings">{{ loginWords.confirm }}</a></p><!-- the system says otherwise than the answer, whichever way, as when the user changed it in the system's own page; the link opens that page, Login Items & Extensions on a Mac and Startup Apps on Windows, and goes away once the two agree. Why they differ is in the log and the main page's report -->
		</div>

		<div v-if="update.shown"><!-- an installed copy, on a Mac or Windows -->
			<p><label class="flex w-fit items-center gap-2"><input type="checkbox" :checked="store.settings.update.automatic" @change="chooseAutomatic($event.target.checked)" /><span>Check automatically</span></label></p><!-- clickable on the box and its words only, like RadioGroup's answers -->
			<p><button :disabled="update.checking" @click="update.check">Check for Update</button></p>
			<p v-if="update.found">{{ update.found.version }}, {{ update.found.date }}</p>
			<p v-if="update.trouble">{{ update.trouble }}</p>
			<p v-if="lastChecked">Last checked {{ lastChecked }}</p>
		</div>

		<RadioGroup :choices="appearanceChoices" :chosen="store.settings.appearance.mode" @choose="chooseMode">Appearance</RadioGroup>
		<RadioGroup :choices="fontChoices"       :chosen="store.settings.appearance.font" @choose="chooseFont">UI Font
			<template #answer="{words}">{{ words.lead }}<em v-if="words.face">{{ words.face }}</em>{{ words.trail }}</template>
		</RadioGroup>
	</main>
</template>
