<script setup>
defineProps({
	rows: Array,//the facts, as [key, value] pairs, in the order shown
})
</script>

<template>
	<!-- ./src/components/KeyValues.vue -->
	<!-- facts as keys and values side by side, the keys in a column as wide as the longest one; as tall as its rows, so whatever follows moves down when there are more -->
	<dl class="grid grid-cols-[auto_1fr] gap-x-line">
		<template v-for="[key, value] in rows" :key="key">
			<dt class="text-muted">{{ key }}</dt>
			<dd>{{ value }}</dd>
		</template>
	</dl>
</template>
