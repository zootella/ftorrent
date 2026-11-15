<script setup>
import {inject} from 'vue'
import {Time} from '@zootella/ergo'
import {lastHours, outcomes, outcomeOrder, outcomeDot, sayOutcome, families, share, sayHour, saySpan} from '../report.js'

let report = inject('report')

let hours = lastHours(report)
let total = report.trackers.length + report.cut.length
let letters = outcomeOrder.filter(letter => hours.counts.ipv4[letter] || hours.counts.ipv6[letter])//the outcomes this hour had, from up to gone

let passes = Object.entries(families).filter(([key]) => report.passes[key].checked).map(([key, name]) => `${saySpan(report.passes[key].seconds*Time.second)} over ${name}`)
let missing = Object.entries(families).filter(([key]) => !report.passes[key].checked).map(([key, name]) => `${name} has no reading this hour.`)
let timing = [`The ${sayHour(report.hour)} pass took ${passes.join(' and ')}.`, ...missing].join(' ')

let head = 'text-xs font-medium text-neutral-500 dark:text-neutral-400'
</script>

<template>
	<div class="rounded-xl border border-neutral-200 p-5 dark:border-neutral-800">
		<h2 class="mb-4 font-semibold">Last hour</h2>
		<div class="space-y-3">
			<div v-for="(name, key) in families" :key="key" class="flex items-center gap-3">
				<span class="w-9 shrink-0 text-xs text-neutral-500 dark:text-neutral-400">{{ name }}</span>
				<div class="flex h-2.5 flex-1 gap-[2px] overflow-hidden rounded-full">
					<template v-for="letter in letters" :key="letter">
						<div v-if="hours.counts[key][letter]" :class="outcomeDot(letter)" :style="{width: share(hours.counts[key][letter], total)}" :data-tip="`${name}: ${hours.counts[key][letter]} ${outcomes[letter].word}`"></div>
					</template>
				</div>
			</div>
		</div>
		<table class="mt-4 w-full text-sm tabular-nums">
			<thead>
				<tr :class="head">
					<th class="pb-2 text-left font-medium"></th>
					<th v-for="(name, key) in families" :key="key" class="pb-2 text-right font-medium">{{ name }}</th>
				</tr>
			</thead>
			<tbody>
				<tr v-for="letter in letters" :key="letter">
					<td class="py-1"><span class="mr-2 inline-block size-2 rounded-full" :class="outcomeDot(letter)"></span>{{ sayOutcome(letter) }}</td>
					<td v-for="(name, key) in families" :key="key" class="py-1 text-right">{{ hours.counts[key][letter] || '' }}</td>
				</tr>
			</tbody>
		</table>
		<p class="mt-4 text-sm text-neutral-500 dark:text-neutral-400">{{ hours.both }} trackers answered over both IPv4 and IPv6. {{ timing }}</p>
	</div>
</template>
