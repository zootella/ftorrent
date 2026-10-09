<script setup>
import {ref, computed, onMounted} from 'vue'
import KeyValues from '../components/KeyValues.vue'
import {netLocal} from '../net.js'

let local = ref('')//this machine's address on its own network, asked as the page opens
onMounted(async () => { local.value = await netLocal().catch(error => String(error)) })//no route out, as with no network, shows as the reason
let facts = computed(() => [['Local IP address', local.value]])
let text = 'Hello, Connection information and diagnostics panel!'

//the whole page as text, the facts and then the box, since what's on it is most useful pasted into a message or an issue
let copied = ref(false)//true for a moment after the button is pressed, so it can say so
async function copy() {
	await navigator.clipboard.writeText([...facts.value.map(([key, value]) => `${key}: ${value}`), '', text].join('\n'))
	copied.value = true
	setTimeout(() => { copied.value = false }, 1500)
}
</script>

<template>
	<!-- ./src/pages/ConnectionPage.vue -->
	<!-- ftorrent's connection to the network: facts at the top, as tall as they need, a box of text filling the middle, which scrolls on its own, and Copy at the bottom -->
	<main class="h-full flex flex-col gap-2">
		<KeyValues :rows="facts" />
		<textarea readonly :value="text" class="flex-1 min-h-0 resize-none font-mono wrap-anywhere"></textarea><!-- min-h-0 lets the box be shorter than its text, so the text scrolls inside it rather than stretching the page; wrap-anywhere because a path is one long word, and should wrap rather than scroll sideways -->
		<button type="button" class="self-start" @click="copy">{{ copied ? 'Copied' : 'Copy' }}</button>
	</main>
</template>
