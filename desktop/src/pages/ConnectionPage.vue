<script setup>
import {ref, computed, nextTick} from 'vue'
import KeyValues from '../components/KeyValues.vue'
import {connectionRun, connectionFacts, sayRun} from '../connection.js'

let run = ref(null)//the latest run, null until the first Check
let facts = computed(() => connectionFacts(run.value))
let text = ref('')//every run since the page opened, oldest first
let running = ref(false)//so Check waits while a run is out
let box = ref(null)//the textarea, scrolled to the newest run as each one lands

async function check() {
	running.value = true
	run.value = await connectionRun()
	text.value += (text.value ? '\n\n' : '') + sayRun(run.value)//a blank line between runs
	running.value = false
	await nextTick()//once vue has put the new text in the box
	if (!box.value) return//the user left the tab while the run was out, and the box went with it
	box.value.scrollTop = box.value.scrollHeight
}

//the whole page as text, the facts and then the box, since what's on it is most useful pasted into a message or an issue
let copied = ref(false)//true for a moment after the button is pressed, so it can say so
async function copy() {
	await navigator.clipboard.writeText([...facts.value.map(([key, value]) => `${key}: ${value}`), '', text.value].join('\n'))
	copied.value = true
	setTimeout(() => { copied.value = false }, 1500)
}
</script>

<template>
	<!-- ./src/pages/ConnectionPage.vue -->
	<!-- ftorrent's connection to the network: Check at the top, the facts below it, as tall as they need, a box of text filling the middle, which scrolls on its own, and Copy at the bottom -->
	<main class="h-full flex flex-col gap-2">
		<button type="button" class="self-start" :disabled="running" @click="check">Check</button>
		<KeyValues :rows="facts" />
		<textarea ref="box" readonly :value="text" class="flex-1 min-h-0 resize-none font-mono wrap-anywhere"></textarea><!-- min-h-0 lets the box be shorter than its text, so the text scrolls inside it rather than stretching the page; wrap-anywhere because a path is one long word, and should wrap rather than scroll sideways -->
		<button type="button" class="self-start" @click="copy">{{ copied ? 'Copied' : 'Copy' }}</button>
	</main>
</template>
