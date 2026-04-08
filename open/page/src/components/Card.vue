<script setup>
import { inject, ref } from 'vue'
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
	<div>
		<h1>open.ftorrent.com</h1>

		<div class="announce-urls">
			<div v-for="url in urls" :key="url" class="announce-row" @click="copy(url)">
				<span>{{ url }}</span>
				<span class="copy-link">{{ copied === url ? 'copied' : 'copy' }}</span>
			</div>
		</div>

		<Panel class="m-6" />

		<pre>{{ JSON.stringify(page, null, '\t') }}</pre>
	</div>
</template>

<style scoped>
h1 {
	text-align: center;
}

.announce-urls {
	border: 1px solid transparent;
	border-radius: 6px;
	font-family: 'Monaspace Radon', monospace;
	font-size: 1.3rem;
	font-weight: 300;
	color: var(--text-muted);
	padding: 1rem;
	text-align: center;
	margin: 0.5rem 1.5rem;
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
