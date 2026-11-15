<script setup>
import {computed} from 'vue'
import {calendar, dayClass, sayDay, sayDate} from '../report.js'

let props = defineProps({
	days: String,//a family's days from trackers.json, oldest first, one character a day
	hour: Number,//the report's hour, whose day is the last character
})
let laid = computed(() => calendar(props.days, props.hour))
function monthOf(week) { return laid.value.months.find(month => month.week == week)?.name || '' }
let weekdays = ['', 'Mon', '', 'Wed', '', 'Fri', '']//every other row named, the way GitHub does, Sunday at the top
</script>

<template>
	<!-- a day's square and the gap between squares grow with the window, set once here as --cell and --gap, so six months of weeks fit a phone and fill a wide screen -->
	<div class="flex gap-[var(--gap)] text-[0.65rem] leading-none text-neutral-500 [--cell:8px] [--gap:2px] sm:[--cell:14px] sm:[--gap:3px] lg:[--cell:16px] lg:[--gap:4px] dark:text-neutral-400">
		<div class="flex flex-col gap-[var(--gap)] pr-1">
			<div class="h-4"></div>
			<div v-for="(name, i) in weekdays" :key="i" class="flex h-[var(--cell)] items-center">{{ name }}</div>
		</div>
		<div v-for="(week, w) in laid.weeks" :key="w" class="flex flex-col gap-[var(--gap)]">
			<div class="relative h-4"><span class="absolute left-0 whitespace-nowrap">{{ monthOf(w) }}</span></div>
			<template v-for="(cell, d) in week" :key="d">
				<div v-if="cell" class="size-[var(--cell)] rounded-[3px]" :class="dayClass(cell.character)" :data-tip="`${sayDate(cell.tick)}: ${sayDay(cell.character)}`" :aria-label="`${sayDate(cell.tick)}: ${sayDay(cell.character)}`"></div>
				<div v-else class="size-[var(--cell)]"></div>
			</template>
		</div>
	</div>
</template>
