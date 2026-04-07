<script setup>
const memory = 5083136
const served = 12345
const downtime = 0

function fmt(n) {
	return n.toLocaleString('en-US').replace(/,/g, '\u2009')
}
</script>

<template>
	<div>
		<div>Memory</div>
		<div>{{ fmt(memory) }}</div>
		<div>Served</div>
		<div>{{ fmt(served) }}</div>
		<div>Downtime</div>
		<div>{{ fmt(downtime) }}</div>
	</div>
</template>
