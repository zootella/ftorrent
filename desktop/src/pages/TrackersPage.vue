<script setup>
import {ref, computed} from 'vue'
import {Time} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go
import KeyValues from '../components/KeyValues.vue'
import {trackers, trackersCheck, trackersPick, trackerFastest, outcomeWords, sayFrom, sayList} from '../trackers.js'
import {useSettingsStore} from '../stores/settings.js'
import {readTick, sayAgo} from '../time.js'

let store = useSettingsStore()
//the two lists, one to a line, for the user to edit before a check, which saves them: each box shows the setting until the user types in it, so a panel drawn before the settings finished loading still shows the user's own, and Check never saves factory values over them
let listsEdit = ref(null)//what the user has typed, or null for the setting as it is
let alwaysEdit = ref(null)
let listsText = computed({get: () => listsEdit.value ?? store.settings.trackers.lists.join('\n'), set: text => { listsEdit.value = text }})
let alwaysText = computed({get: () => alwaysEdit.value ?? store.settings.trackers.always.join('\n'), set: text => { alwaysEdit.value = text }})
async function checkNow() {
	await trackersCheck(listsText.value, alwaysText.value)
	listsEdit.value = null//saved now, so the boxes show the settings again, cleaned of any line that wasn't an address
	alwaysEdit.value = null
}

let facts = computed(() => {
	let saved = store.settings.trackers
	let rows = [['This machine', sayMachine()]]
	for (let list of trackers.lists) rows.push([sayList(list.url), list.error ? `Couldn't fetch, ${list.error}` : `${list.found} trackers`])
	rows.push(['Saved', saved.checked ? `${saved.answered.length} that answered, checked ${sayAgo(Date.now() - readTick(saved.checked))}` : 'Nothing checked yet'])
	return rows
})

function sayMachine() {//which families this machine can check, from the last check's look at its routes
	let families = trackers.families
	if (families.length == 0) return 'Found when you check'
	if (families.length == 2) return 'IPv4 and IPv6'
	return `IPv${families[0]} only, with no IPv${families[0] == 4 ? 6 : 4} route here`
}

let shown = computed(() => [...trackers.results].sort((a, b) => trackerFastest(a) - trackerFastest(b) || a.url.localeCompare(b.url)))//fastest first, and the rest by address
function cell(slot) {//a tracker's result in one family: its reply time when it answered, and otherwise what happened
	if (slot.outcome == 'up') return `${slot.ms} ms`
	return outcomeWords[slot.outcome]
}
function tip(slot) {//libtorrent's words for a failure, and how long it took, on hover
	if (!slot.message) return ''
	if (slot.ms > 0) return `${slot.message}, after ${slot.ms} ms`
	return slot.message
}

const presets = [['all', 'Everything'], ['fastest5', 'Fastest 5'], ['fastest10', 'Fastest 10'], ['fastest20', 'Fastest 20'], ['random20', 'Random 20']]
let chosen = ref('')//the trackers the last preset picked, as a list to paste
let copied = ref(false)//true for a moment after Copy, so it can say so
async function copy() {
	await navigator.clipboard.writeText(chosen.value)
	copied.value = true
	setTimeout(() => { copied.value = false }, 1.5*Time.second)
}
</script>

<template>
	<!-- ./src/pages/TrackersPage.vue -->
	<!-- the Trackers panel, list's half in the desktop client: Check and what it's doing at the top, the two lists to edit, the facts, the trackers fastest first, and the presets and the list they pick. The three boxes share the height the window gives them, each with a floor, so a taller window makes all three taller, and each scrolls on its own. The addresses are in the interface's own face, the way a browser's address bar shows them -->
	<main class="h-full flex flex-col gap-2">
		<div class="flex items-center gap-2">
			<button type="button" :disabled="trackers.running" @click="checkNow">Check</button>
			<span class="text-muted">{{ trackers.words }}</span>
		</div>
		<div class="flex-1 min-h-24 flex gap-2">
			<label class="flex-1 min-w-0 flex flex-col gap-1">
				<span class="text-muted">Lists</span>
				<textarea v-model="listsText" spellcheck="false" :disabled="trackers.running" class="flex-1 min-h-0 resize-none whitespace-pre"></textarea>
			</label>
			<label class="flex-1 min-w-0 flex flex-col gap-1">
				<span class="text-muted">Trackers</span>
				<textarea v-model="alwaysText" spellcheck="false" :disabled="trackers.running" class="flex-1 min-h-0 resize-none whitespace-pre"></textarea>
			</label>
		</div>
		<KeyValues :rows="facts" />
		<div class="flex-1 min-h-24 overflow-auto rounded-md border border-line"><!-- the table scrolls inside its box rather than stretching the panel -->
			<table class="w-full">
				<thead class="sticky top-0 bg-paper text-muted">
					<tr>
						<th class="px-2 text-left">Tracker</th>
						<th v-for="f in trackers.families" :key="f" class="px-2 text-right">IPv{{ f }}</th>
						<th class="px-2 text-left">From</th>
					</tr>
				</thead>
				<tbody>
					<tr v-for="entry in shown" :key="entry.url">
						<td class="px-2 wrap-anywhere">{{ entry.url }}</td><!-- an address from a list on the internet, shown as text through vue's interpolation and never as markup -->
						<td v-for="f in trackers.families" :key="f" class="px-2 text-right whitespace-nowrap" :class="entry.at[f].outcome == 'up' ? '' : 'text-muted'" :title="tip(entry.at[f])">{{ cell(entry.at[f]) }}</td>
						<td class="px-2 text-muted whitespace-nowrap">{{ [...new Set(entry.from.map(sayFrom))].join(', ') }}</td>
					</tr>
				</tbody>
			</table>
		</div>
		<div class="flex flex-wrap items-center gap-2">
			<button v-for="[preset, label] in presets" :key="preset" type="button" @click="chosen = trackersPick(preset)">{{ label }}</button>
			<button type="button" :disabled="!chosen" @click="copy">{{ copied ? 'Copied' : 'Copy' }}</button>
		</div>
		<textarea readonly :value="chosen" spellcheck="false" class="flex-1 min-h-24 resize-none whitespace-pre"></textarea>
	</main>
</template>
