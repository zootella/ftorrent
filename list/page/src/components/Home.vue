<script setup>
import {inject, computed, nextTick, onMounted} from 'vue'
import {useRoute} from 'vue-router'
import {horizonPhrase, families, findExtra, listText, splitUrl, schemes, hasAddress, lastHour, outcomeDot, sayOutcome, sayUptime, trackerPath, trackerUrl} from '../report.js'
import {useQuery} from '../query.js'
import Copy from './Copy.vue'
import Summary from './Summary.vue'
import Tracker from './Tracker.vue'

let report = inject('report')
let route = useRoute()
let {read, set: setQuery, clear} = useQuery()

//the tracker the address names, whose details open at the foot of this same page, or blank; choosing a tracker changes the address and draws its details below without moving the page, so a reader can stay where they are and click from tracker to tracker to compare them, and the address bookmarks the whole view
let opened = computed(() => route.name == 'tracker' ? trackerUrl(route.params.scheme, route.params.rest) : '')
onMounted(() => {//a bookmark or a shared link to a tracker is the one time the page moves itself, down to the details it was opened for
	if (opened.value) nextTick(() => document.getElementById('tracker')?.scrollIntoView())
})

let criterion = report.criterion
let percent = Math.round(criterion.uptime*100)//0.95 as 95
let text = listText(report)//trackers.txt, built from the same hour the rest of the page shows, for the copy button
let urls = report.trackers.map(entry => entry.url)//the same list, one tracker at a time, for the box to draw
let fileUrl = report.downstream.find(url => url.endsWith('.txt'))

//the filters over the trackers, each a value in the address that query.js reads and writes
let show = computed(() => read('show'))//blank for the listed trackers, or up for those up last hour, or cut, or all
let schemesChosen = computed(() => read('scheme').split(',').filter(s => schemes.includes(s)))//the kinds chosen, any number of them, written in the address like scheme=https,wss; none chosen shows every kind
let family = computed(() => read('family'))//blank for any, or ipv4 or ipv6 for a tracker with an address there, or both for one with an address in each
let words = computed(() => read('q'))//text the URL must contain
let filtered = computed(() => show.value || schemesChosen.value.length || family.value || words.value)

let everyone = [...report.trackers, ...report.cut]
let upNow = everyone.filter(entry => lastHour(entry.ipv4) == 'U' || lastHour(entry.ipv6) == 'U')//up last hour, over either family
let tab = computed(() => {//the trackers the first control picks, before the others narrow them
	if (show.value == 'up') return upNow
	if (show.value == 'cut') return report.cut
	if (show.value == 'all') return everyone
	return report.trackers
})
let schemeCounts = computed(() => {//how many of the tab's trackers are each kind, for the buttons
	let counts = {}
	for (let entry of tab.value) {
		let s = splitUrl(entry.url).scheme
		counts[s] = (counts[s] || 0) + 1
	}
	return counts
})

function reaches(entry) {//whether a tracker passes the family filter: it has an address in the family, or, with Up last hour chosen, it was up there
	let fits = column => show.value == 'up' ? lastHour(column) == 'U' : hasAddress(column, criterion.horizon)
	let four = fits(entry.ipv4)
	let six = fits(entry.ipv6)
	if (family.value == 'ipv4') return four
	if (family.value == 'ipv6') return six
	if (family.value == 'both') return four && six
	return true
}
let shown = computed(() => {//in the report's order, the most dependable first
	let lower = words.value.trim().toLowerCase()
	return tab.value.filter(entry =>
		(!schemesChosen.value.length || schemesChosen.value.includes(splitUrl(entry.url).scheme)) &&
		reaches(entry) &&
		(!lower || entry.url.includes(lower)))//URLs are lowercase already
})
function tagTitle(entry) {//what a tracker's tag says on hover: each family's last hour and its uptime over the criterion's horizon
	return Object.entries(families).map(([key, name]) => `${name}: ${sayOutcome(lastHour(entry[key]))} last hour${sayUptime(entry[key][criterion.horizon]) ? `, ${sayUptime(entry[key][criterion.horizon])} over ${horizonPhrase(criterion.horizon)}` : ''}`).join('\n')
}

//the three controls above the trackers, each a row of choices in one rounded tray, the chosen ones raised: what each choice sets in the address, its label, and how many trackers it holds, where a count helps; a choice with a count of zero can't be chosen, which the template asks with === 0, since a choice like Any has no count at all. The kinds of tracker can be chosen together, so a reader can build a list of their own, like only the encrypted ones; the other two trays pick one
let showChoices = computed(() => [
	{value: '', label: 'Listed good', count: report.trackers.length},
	{value: 'up', label: 'Up last hour', count: upNow.length},
	{value: 'cut', label: 'Not listed', count: report.cut.length},
	{value: 'all', label: 'All', count: everyone.length},
])
let schemeChoices = computed(() => [{value: '', label: 'Any'}, ...schemes.map(s => ({value: s, label: s.toUpperCase(), count: schemeCounts.value[s] || 0}))])
let familyChoices = [{value: '', label: 'Any'}, {value: 'ipv4', label: 'IPv4'}, {value: 'ipv6', label: 'IPv6'}, {value: 'both', label: 'Dual stack'}]
let controls = computed(() => [
	{key: 'show', chosen: [show.value], choices: showChoices.value},
	{key: 'scheme', chosen: schemesChosen.value.length ? schemesChosen.value : [''], choices: schemeChoices.value, together: true},
	{key: 'family', chosen: [family.value], choices: familyChoices},
])
function choose(control, value) {//a tray that picks one sets the value; the kinds tray turns one kind on or off, and Any clears them all
	if (!control.together || !value) return setQuery({[control.key]: value})
	let next = schemesChosen.value.includes(value) ? schemesChosen.value.filter(s => s != value) : [...schemesChosen.value, value]
	setQuery({scheme: schemes.filter(s => next.includes(s)).join(',')})//in a steady order, so the same choice always writes the same address
}

let shownText = computed(() => shown.value.map(entry => entry.url).join('\n\n') + '\n')//the trackers showing, as a list to paste, in trackers.txt's form
let segment = 'rounded-md px-3 py-1 text-sm font-medium whitespace-nowrap transition-colors disabled:cursor-default disabled:opacity-40'
let segmentOn = 'bg-white text-neutral-900 shadow-sm dark:bg-neutral-700 dark:text-white'
let segmentOff = 'text-neutral-500 hover:text-neutral-900 dark:text-neutral-400 dark:hover:text-white'
</script>

<template>
	<div class="space-y-10">
		<section class="space-y-2">
			<h1 class="text-3xl font-semibold tracking-tight sm:text-4xl">Public trackers that work</h1>
			<p class="text-neutral-600 dark:text-neutral-400">Every hour, list.ftorrent.com announces to each public BitTorrent and WebTorrent tracker, over IPv4 and IPv6 separately, and rewrites <a :href="fileUrl" class="font-medium text-neutral-900 underline decoration-neutral-300 underline-offset-2 hover:decoration-neutral-900 dark:text-neutral-100 dark:decoration-neutral-600 dark:hover:decoration-neutral-100">trackers.txt</a> with the ones up in at least {{ percent }}% of the hours over the last {{ horizonPhrase(criterion.horizon) }}. This hour, that's {{ criterion.listed }}.</p>
		</section>

		<!-- the page opens on two boxes side by side, the list to copy and the last hour, stacked on a narrow window -->
		<div class="grid items-start gap-4 lg:grid-cols-2">
			<section class="overflow-hidden rounded-xl border border-neutral-200 dark:border-neutral-800">
				<div class="flex items-center justify-between gap-4 px-4 py-3">
					<div class="min-w-0">
						<div class="font-medium">trackers.txt</div>
						<div class="text-sm text-neutral-500 dark:text-neutral-400">Paste these into your client. A blank line between trackers puts each in its own tier, so the client announces to all of them.</div>
					</div>
					<Copy :text="text" primary />
				</div>
				<!-- the list exactly as trackers.txt writes it, a blank line between trackers; each blank line is its own block, which the stylesheet holds to a sliver, so the list reads compact on the page, the same text copies with its blank lines, and with no stylesheet the box shows the file as it is -->
				<pre class="max-h-72 overflow-auto border-y border-neutral-200 bg-neutral-50 px-4 py-3 font-radon text-[0.8rem] font-light leading-6 [font-variant-ligatures:none] dark:border-neutral-800 dark:bg-neutral-900"><template v-for="(url, i) in urls" :key="url"><span v-if="i > 0" class="block h-1 overflow-hidden">{{ '\n' }}</span>{{ url + '\n' }}</template></pre>
				<div class="flex flex-wrap items-center gap-x-3 gap-y-2 px-4 py-3 text-sm">
					<span class="text-neutral-500 dark:text-neutral-400">Or point your client at</span>
					<code class="break-all font-radon font-light [font-variant-ligatures:none]">{{ fileUrl }}</code>
					<Copy :text="fileUrl" />
				</div>
			</section>

			<Summary />
		</div>

		<section id="trackers" class="scroll-mt-4 space-y-4">
			<div class="flex flex-wrap items-center gap-3">
				<div v-for="control in controls" :key="control.key" class="inline-flex max-w-full overflow-x-auto rounded-lg bg-neutral-100 p-1 dark:bg-neutral-900">
					<button
						v-for="choice in control.choices"
						:key="choice.value"
						type="button"
						:disabled="choice.count === 0"
						:class="[segment, control.chosen.includes(choice.value) ? segmentOn : segmentOff]"
						@click="choose(control, choice.value)">
						{{ choice.label }}<span v-if="choice.count != null" class="ml-1.5 tabular-nums text-neutral-400">{{ choice.count }}</span>
					</button>
				</div>
				<input
					type="search"
					placeholder="Find a tracker"
					:value="words"
					@input="setQuery({q: $event.target.value})"
					class="w-full rounded-lg border border-neutral-200 bg-transparent px-3 py-1.5 text-sm outline-none focus:border-neutral-400 sm:ml-auto sm:w-64 dark:border-neutral-800 dark:focus:border-neutral-600" />
			</div>

			<div class="flex flex-wrap items-center gap-x-3 gap-y-2 text-sm text-neutral-500 dark:text-neutral-400">
				<span v-if="show == 'cut'">These fell under {{ percent }}% over the last {{ horizonPhrase(criterion.horizon) }} in both families, or listed peers that can't exist.</span>
				<span>Showing {{ shown.length }} of {{ tab.length }}.</span>
				<Copy v-if="shown.length" :text="shownText" :label="`Copy these ${shown.length}`" />
				<button v-if="filtered" type="button" class="underline underline-offset-2 hover:text-neutral-900 dark:hover:text-white" @click="clear">Clear</button>
			</div>

			<!-- the trackers as a cloud of tags, wrapping like a paragraph, each with two dots for its last hour over IPv4 and IPv6; a tag links to the tracker's own address, keeping the filters, and its details open below -->
			<div class="flex flex-wrap gap-1.5">
				<RouterLink
					v-for="entry in shown"
					:key="entry.url"
					:to="{path: trackerPath(entry.url), query: route.query}"
					:data-tip="tagTitle(entry)"
					class="inline-flex max-w-full items-center gap-2 rounded-md border px-2 py-1 font-radon text-[0.8rem] font-light [font-variant-ligatures:none]"
					:class="entry.url == opened ? 'border-neutral-900 bg-neutral-900 text-white dark:border-white dark:bg-white dark:text-neutral-900' : 'border-neutral-200 hover:border-neutral-400 dark:border-neutral-800 dark:hover:border-neutral-600'">
					<span class="inline-flex shrink-0 gap-0.5"><span v-for="(name, key) in families" :key="key" class="size-1.5 rounded-full" :class="outcomeDot(lastHour(entry[key]))"></span></span>
					<span class="break-all">{{ entry.url }}</span>
					<span v-if="findExtra(report, entry.url)" class="shrink-0 font-sans text-[0.65rem] font-semibold text-amber-600 dark:text-amber-400">Extra peers</span>
				</RouterLink>
			</div>
			<p v-if="!shown.length" class="text-sm text-neutral-500">No trackers match.</p>
			<p class="text-xs text-neutral-500 dark:text-neutral-400">The two dots on each tracker are its last hour over IPv4 and IPv6.</p>
		</section>

		<section v-if="opened" id="tracker" class="scroll-mt-4 border-t border-neutral-200 pt-10 dark:border-neutral-800">
			<Tracker :url="opened" />
		</section>
	</div>
</template>
