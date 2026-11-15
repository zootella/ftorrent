<script setup>
import {computed} from 'vue'
import {stripBackground} from '../report.js'

let props = defineProps({
	days: String,//a family's days from trackers.json, oldest first, one character a day; a single character draws one day, for the legend
})
let style = computed(() => ({
	width: `calc(${props.days.length} * var(--day))`,
	backgroundImage: stripBackground(props.days),
}))
</script>

<template>
	<span class="strip" :style="style"></span>
</template>

<style scoped>
.strip {
	display: inline-block;
	height: 1em;
	vertical-align: middle;
	border-bottom: 1px solid var(--line); /* a line under every day, so the strip shows its whole width where nothing was checked */
}
</style>
