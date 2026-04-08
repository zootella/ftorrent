<script setup>
import { ref } from 'vue'

const props = defineProps({
	href: String,
	color: String,
})

const copied = ref(false)

async function copy() {
	await navigator.clipboard.writeText(props.href)
	copied.value = true
	setTimeout(() => { copied.value = false }, 1500)
}
</script>

<template>
	<div class="url-row" @click="copy">
		<span class="url-text">{{ href }}</span>
		<span class="url-copy">{{ copied ? 'copied' : 'copy' }}</span>
	</div>
</template>

<style scoped>
.url-row {
	display: flex;
	align-items: baseline;
	justify-content: center;
	gap: 0.5rem;
	cursor: pointer;
	font-family: 'Monaspace Radon', monospace;
	font-size: 1.3rem;
	font-weight: 300;
	color: var(--text);
}

.url-row:hover {
	color: v-bind(color);
	text-shadow: 0 0 2px v-bind(color);
}

.url-text {
	overflow-wrap: break-word;
	word-break: break-all;
}

.url-copy {
	font-family: 'Jura', sans-serif;
	font-size: 1rem;
	font-weight: var(--muted-weight);
	color: var(--text-muted);
	text-decoration: underline;
	margin-left: 1rem;
	white-space: nowrap;
	text-shadow: none;
}
</style>
