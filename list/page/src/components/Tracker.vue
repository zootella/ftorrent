<script setup>
import {inject, computed} from 'vue'
import {useRoute} from 'vue-router'
import {Time} from '@zootella/ergo'
import {findTracker, findExtra, sayExtra, sayUptime, noAddress, lastHour, outcomes, sayOutcome, outcomeDot, families, horizonHead, horizonPhrase, sayDate, dayTick, saySpan, splitUrl, dayLegend, dayClass} from '../report.js'
import Calendar from './Calendar.vue'
import Copy from './Copy.vue'

let props = defineProps({
	url: String,//the tracker the address names
})
let report = inject('report')
let route = useRoute()
let close = computed(() => ({path: '/', query: route.query}))//back to the summary alone, keeping its filters

let found = computed(() => findTracker(report, props.url))//its entry, and whether it's listed, or false when this hour's report doesn't name it
let entry = computed(() => found.value.entry)
let parts = computed(() => splitUrl(props.url))
let criterion = report.criterion
let horizons = computed(() => Object.keys(entry.value.ipv4))//h first, in the report's order
let extra = computed(() => {
	let seen = findExtra(report, props.url)
	return seen ? sayExtra(report, seen, entry.value) : []
})
let reason = computed(() => {//why trackers.txt leaves it out, from the criterion at the top of the report
	if (extra.value.length) return 'trackers.txt leaves it out: its replies list peers that can\'t exist.'
	return `trackers.txt leaves it out: it was up in under ${Math.round(criterion.uptime*100)}% of its hours over the last ${horizonPhrase(criterion.horizon)}, in both families.`
})
let sources = computed(() => report.upstream.filter(source => source.lists && entry.value.upstream.includes(source.name)))//each public list that names it, with the page it lives at; list.ftorrent.com's own candidates aren't a list a reader would know

function details(key) { return entry.value.details?.[key] || {} }//what else the report knows about a family: the interval the tracker last asked for, the up hours that took a second try, and its last warning
function upHours(key) { return entry.value[key][criterion.horizon].U || 0 }

//one family's hours as table rows: a row for each outcome it had in any horizon, in the report's order
function rows(key) {
	let column = entry.value[key]
	let letters = Object.keys(outcomes).filter(letter => horizons.value.some(horizon => column[horizon][letter]))
	return letters.map(letter => ({letter, word: sayOutcome(letter), hours: horizons.value.map(horizon => column[horizon][letter] || '')}))
}

let certificate = computed(() => entry.value.details?.certificate)
let daysLeft = computed(() => certificate.value?.expires ? Math.floor((dayTick(certificate.value.expires) - report.hour)/Time.day) : 0)//from the report's hour, negative once it has run out
</script>

<template>
	<div v-if="found" class="space-y-8">
		<header class="space-y-3">
			<div class="flex flex-wrap items-center gap-2 text-xs font-semibold">
				<RouterLink :to="close" class="order-last ml-auto text-sm font-normal text-neutral-500 hover:text-neutral-900 dark:text-neutral-400 dark:hover:text-white">Close</RouterLink>
				<span class="rounded bg-neutral-100 px-1.5 py-0.5 uppercase tracking-wide text-neutral-600 dark:bg-neutral-800 dark:text-neutral-300">{{ parts.scheme }}</span>
				<span v-if="found.listed" class="rounded bg-emerald-100 px-1.5 py-0.5 text-emerald-800 dark:bg-emerald-900/50 dark:text-emerald-300">Listed good</span>
				<span v-else class="rounded bg-neutral-100 px-1.5 py-0.5 text-neutral-600 dark:bg-neutral-800 dark:text-neutral-300">Not listed</span>
			</div>
			<h1 class="break-all text-2xl font-semibold tracking-tight sm:text-3xl">{{ parts.host }}</h1>
			<div class="flex flex-wrap items-center gap-3">
				<code class="break-all font-radon text-sm font-light text-neutral-600 [font-variant-ligatures:none] dark:text-neutral-300">{{ url }}</code>
				<Copy :text="url" />
			</div>
			<p class="text-sm text-neutral-500 dark:text-neutral-400">
				list.ftorrent.com has checked it hourly since {{ sayDate(entry.since) }}<template v-if="sources.length">, and
				<template v-for="(source, i) in sources" :key="source.name"><a :href="source.home" class="underline decoration-neutral-300 underline-offset-2 hover:text-neutral-900 dark:decoration-neutral-600 dark:hover:text-white">{{ source.name }}</a>{{ i < sources.length - 2 ? ', ' : i == sources.length - 2 ? ' and ' : '' }}</template>
				{{ sources.length == 1 ? 'names' : 'name' }} it</template>.
			</p>
		</header>

		<div v-if="!found.listed" class="space-y-1 rounded-xl border border-amber-200 bg-amber-50 p-4 text-sm text-amber-900 dark:border-amber-900/60 dark:bg-amber-950/40 dark:text-amber-200">
			<p>{{ reason }}</p>
			<p v-for="line in extra" :key="line">{{ line }}</p>
		</div>

		<!-- the families stacked, each in three columns on a wide screen: its numbers in a column of fixed width, so both calendars start at the same place and their weeks line up, then its calendar, then its details and its hours -->
		<section v-for="(name, key) in families" :key="key" class="grid gap-6 rounded-xl border border-neutral-200 p-5 lg:grid-cols-[16rem_auto_1fr] lg:gap-10 dark:border-neutral-800">
			<div class="space-y-4">
				<div class="flex items-center justify-between gap-3">
					<h2 class="font-semibold">{{ name }}</h2>
					<span class="inline-flex items-center gap-1.5 text-sm text-neutral-500 dark:text-neutral-400">
						<span class="size-2 rounded-full" :class="outcomeDot(lastHour(entry[key]))"></span>
						{{ sayOutcome(lastHour(entry[key])) }} last hour
					</span>
				</div>
				<div v-if="noAddress(entry[key], criterion.horizon)">
					<div class="text-2xl font-semibold text-neutral-400 dark:text-neutral-500">No {{ name }} address</div>
					<p class="mt-1 text-sm text-neutral-500 dark:text-neutral-400">The tracker's name has no {{ name }} address, so there's nothing to reach over {{ name }}.</p>
				</div>
				<template v-else>
					<div>
						<div class="text-4xl font-semibold tabular-nums tracking-tight">{{ sayUptime(entry[key][criterion.horizon]) || '—' }}</div>
						<div class="text-sm text-neutral-500 dark:text-neutral-400">Up over the last {{ horizonPhrase(criterion.horizon) }}</div>
					</div>
					<dl class="flex flex-wrap gap-x-5 gap-y-2">
						<div v-for="horizon in horizons.filter(h => h != 'h' && h != criterion.horizon)" :key="horizon">
							<dt class="text-xs text-neutral-500 dark:text-neutral-400">{{ horizonHead(horizon) }}</dt>
							<dd class="font-medium tabular-nums">{{ sayUptime(entry[key][horizon]) || '—' }}</dd>
						</div>
					</dl>
				</template>
			</div>

			<div class="overflow-x-auto">
				<Calendar :days="entry.days[key]" :hour="report.hour" />
			</div>

			<div class="space-y-4 text-sm">
				<dl v-if="details(key).interval || details(key).retried?.[criterion.horizon] || details(key).warning" class="grid grid-cols-[auto_1fr] gap-x-4 gap-y-1">
					<template v-if="details(key).interval">
						<dt class="text-neutral-500 dark:text-neutral-400">Interval</dt>
						<dd>Asks clients to announce every {{ saySpan(details(key).interval) }}</dd>
					</template>
					<template v-if="details(key).retried?.[criterion.horizon]">
						<dt class="text-neutral-500 dark:text-neutral-400">Second tries</dt>
						<dd>{{ details(key).retried[criterion.horizon] }} of {{ upHours(key) }} up hours over the last {{ horizonPhrase(criterion.horizon) }}</dd>
					</template>
					<template v-if="details(key).warning">
						<dt class="text-neutral-500 dark:text-neutral-400">Warning</dt>
						<dd class="break-words">{{ details(key).warning }}</dd>
					</template>
				</dl>
				<div class="overflow-x-auto">
					<table class="text-xs tabular-nums">
						<thead class="text-neutral-500 dark:text-neutral-400">
							<tr>
								<th class="py-1 pr-4 text-left font-medium">Hours</th>
								<th v-for="horizon in horizons" :key="horizon" class="px-2 py-1 text-right font-medium">{{ horizonHead(horizon) }}</th>
							</tr>
						</thead>
						<tbody>
							<tr v-for="row in rows(key)" :key="row.letter">
								<td class="whitespace-nowrap py-1 pr-4"><span class="mr-1.5 inline-block size-2 rounded-full" :class="outcomeDot(row.letter)"></span>{{ row.word }}</td>
								<td v-for="(hours, i) in row.hours" :key="i" class="px-2 py-1 text-right">{{ hours }}</td>
							</tr>
						</tbody>
					</table>
				</div>
			</div>
		</section>

		<div class="flex flex-wrap items-center gap-x-4 gap-y-2 text-xs text-neutral-500 dark:text-neutral-400">
			<span>Each square is a day, UTC, Sunday at the top of each week:</span>
			<span v-for="item in dayLegend" :key="item.character" class="inline-flex items-center gap-1.5"><span class="size-3 rounded-[3px]" :class="dayClass(item.character)"></span>{{ item.word }}</span>
		</div>

		<section v-if="certificate" class="rounded-xl border border-neutral-200 p-5 dark:border-neutral-800">
			<h2 class="mb-4 font-semibold">Certificate</h2>
			<p v-if="certificate.error" class="break-words text-sm text-red-600 dark:text-red-400">{{ certificate.error }}</p>
			<dl v-else class="flex flex-wrap gap-x-12 gap-y-4 text-sm">
				<div>
					<dt class="text-xs text-neutral-500 dark:text-neutral-400">Issuer</dt>
					<dd class="font-medium">{{ certificate.issuer }}</dd>
				</div>
				<div>
					<dt class="text-xs text-neutral-500 dark:text-neutral-400">Issued</dt>
					<dd class="font-medium">{{ sayDate(dayTick(certificate.issued)) }}</dd>
				</div>
				<div>
					<dt class="text-xs text-neutral-500 dark:text-neutral-400">Expires</dt>
					<dd class="font-medium">{{ sayDate(dayTick(certificate.expires)) }}</dd>
					<dd class="text-xs" :class="daysLeft < 0 ? 'text-red-600 dark:text-red-400' : daysLeft < 14 ? 'text-amber-600 dark:text-amber-400' : 'text-neutral-500 dark:text-neutral-400'">{{ daysLeft < 0 ? `Ran out ${-daysLeft} days ago` : `In ${daysLeft} days` }}</dd>
				</div>
			</dl>
		</section>
	</div>

	<div v-else class="space-y-4">
		<RouterLink :to="close" class="text-sm text-neutral-500 hover:text-neutral-900 dark:text-neutral-400 dark:hover:text-white">Close</RouterLink>
		<p>This hour's report doesn't name <code class="break-all font-radon font-light [font-variant-ligatures:none]">{{ url }}</code>. A tracker leaves the report when no list names it anymore.</p>
	</div>
</template>
