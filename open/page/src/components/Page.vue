<script setup>
import { inject, ref } from 'vue'
import Space from './Space.vue'
import Panel from './Panel.vue'

const page = inject('page')

const urls = [
	'udp://open.ftorrent.com:443/announce',
	'https://open.ftorrent.com/announce',
	'wss://open.ftorrent.com',
]

const copied = ref(null)

async function copy(url) {
	await navigator.clipboard.writeText(url)
	copied.value = url
	setTimeout(() => { copied.value = null }, 1500)
}
</script>

<template>
	<Space class="banner" />

	<h1>open.ftorrent.com</h1>

	<div class="announce-urls">
		<div v-for="url in urls" :key="url" class="announce-row" @click="copy(url)">
			<span>{{ url }}</span>
			<span class="copy-link">{{ copied === url ? 'copied' : 'copy' }}</span>
		</div>
	</div>

	<Panel style="margin: 2rem" />
</template>

<style scoped>
.banner {
	height: 300px;
}

h1 {
	text-align: center;
	margin: 2rem;
}

.announce-urls {
	border: 1px solid transparent;
	border-radius: 6px;
	font-family: 'Monaspace Radon', monospace;
	font-size: 1.3rem;
	font-weight: 300;
	color: var(--text-muted);
	text-align: center;
	margin: 2rem;
}

.announce-row {
	display: flex;
	align-items: center;
	justify-content: center;
	gap: 0.5rem;
	cursor: pointer;
}

.announce-row:hover {
	font-weight: 700;
}

.copy-link {
	font-family: 'Jura', sans-serif;
	font-size: 1rem;
	font-weight: 700;
	color: #aaa;
	text-decoration: underline;
	margin-left: 1rem;
}
</style>
