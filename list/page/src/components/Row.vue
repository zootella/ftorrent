<script setup>
import {computed} from 'vue'
import {useRouter} from 'vue-router'
import {splitUrl, trackerPath, lastHour, outcomeDot, sayOutcome, sayUptime, noAddress, families} from '../report.js'

let props = defineProps({
	entry: Object,//one tracker from trackers.json
	horizon: String,//the horizon whose uptime the row shows, the one trackers.txt judges by
	extra: Boolean,//whether its replies were seen listing peers that can't exist
})
let router = useRouter()
let parts = computed(() => splitUrl(props.entry.url))
let path = computed(() => trackerPath(props.entry.url))
</script>

<template>
	<tr class="cursor-pointer hover:bg-neutral-50 dark:hover:bg-neutral-900" @click="router.push(path)">
		<td class="px-3 py-2.5 sm:px-4">
			<RouterLink :to="path" class="flex min-w-0 flex-col items-start gap-1 sm:flex-row sm:items-center sm:gap-3" @click.stop><!-- on a phone the label sits above the name, which gets the column's whole width -->
				<span class="w-12 shrink-0 rounded bg-neutral-100 py-0.5 text-center text-[0.65rem] font-semibold uppercase tracking-wide text-neutral-600 dark:bg-neutral-800 dark:text-neutral-300">{{ parts.scheme }}</span>
				<span class="min-w-0 break-all"><span class="font-medium">{{ parts.host }}</span><span class="hidden text-neutral-400 sm:inline dark:text-neutral-500">{{ parts.rest }}</span></span>
				<span v-if="extra" class="shrink-0 rounded bg-amber-100 px-1.5 py-0.5 text-[0.65rem] font-semibold text-amber-800 dark:bg-amber-900/50 dark:text-amber-300">extra peers</span>
			</RouterLink>
		</td>
		<td v-for="(name, key) in families" :key="key" class="whitespace-nowrap px-3 py-2.5 text-right tabular-nums sm:px-4">
			<span class="inline-flex items-center gap-2" :title="`${name} last hour: ${sayOutcome(lastHour(entry[key]))}`">
				<span v-if="noAddress(entry[key], horizon)" class="text-xs text-neutral-400 dark:text-neutral-500"><span class="hidden sm:inline">no address</span><span class="sm:hidden">—</span></span>
				<span v-else>{{ sayUptime(entry[key][horizon]) }}</span>
				<span class="size-2 rounded-full" :class="outcomeDot(lastHour(entry[key]))"></span>
			</span>
		</td>
	</tr>
</template>
