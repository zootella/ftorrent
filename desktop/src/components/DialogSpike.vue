<script setup>
import {spikeBoxes, spikeRun, spikeStatus} from '../spike.js'
</script>

<template>
	<!-- ./src/components/DialogSpike.vue -->
	<!-- a button for each way spike.js puts up a system box, and a line below saying what the last one answered -->
	<div class="flex flex-col items-start gap-line">
		<div class="flex flex-wrap gap-line">
			<button v-for="b in spikeBoxes" :key="b.label" type="button" @click="spikeRun(b.label, b.box)">{{ b.label }}</button>
		</div>
		<p class="wrap-anywhere">{{ spikeStatus || 'No box yet' }}</p><!-- wrap-anywhere because a path is one long word -->
	</div>
</template>
