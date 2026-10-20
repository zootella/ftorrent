<script setup>
import {ref, computed, watch, nextTick, onMounted} from 'vue'
import KeyValues from '../components/KeyValues.vue'
import {connectionRun, connectionFacts, connectionText, sayRun, lastRun, box, boxAdd, session, sessionFacts, sessionOpen, sessionStart, sessionStop} from '../connection.js'

let facts = computed(() => [...connectionFacts(lastRun.value), ...sessionFacts()])
let running = ref(false)//so Check waits while a run is out
let boxElement = ref(null)//the textarea

async function check() {
	running.value = true
	lastRun.value = await connectionRun()
	boxAdd(sayRun(lastRun.value), 'run')
	running.value = false
}

//when the box grows, follow its newest line, unless the user has scrolled up to read, which a session's line a second would otherwise keep taking away
watch(box, async () => {
	let element = boxElement.value
	let following = !element || element.scrollHeight - element.scrollTop - element.clientHeight < 40//at the bottom, or near enough, before the new text arrives
	await nextTick()//once vue has put the new text in the box
	if (following && boxElement.value) boxElement.value.scrollTop = boxElement.value.scrollHeight//the user may have left the tab while the run was out, and the box gone with it
})

onMounted(async () => {
	await nextTick()
	if (boxElement.value) boxElement.value.scrollTop = boxElement.value.scrollHeight//back on the tab, at the newest line
})

//the whole page as text, the facts and then the box, since what's on it is most useful pasted into a message or an issue
let copied = ref(false)//true for a moment after the button is pressed, so it can say so
async function copy() {
	await navigator.clipboard.writeText(connectionText())
	copied.value = true
	setTimeout(() => { copied.value = false }, 1500)
}
</script>

<template>
	<!-- ./src/pages/ConnectionPage.vue -->
	<!-- ftorrent's connection to the network: Check and the session's controls at the top, the facts below them, as tall as they need, a box of text filling the middle, which scrolls on its own, and Copy at the bottom -->
	<main class="h-full flex flex-col gap-2">
		<div class="flex gap-2">
			<button type="button" :disabled="running" @click="check">Check</button>
			<button type="button" @click="sessionOpen">Open…</button>
			<button type="button" :disabled="session.running" @click="sessionStart">Start</button>
			<button type="button" :disabled="!session.running" @click="sessionStop">Stop</button>
		</div>
		<KeyValues :rows="facts" />
		<textarea ref="boxElement" readonly :value="box" class="flex-1 min-h-0 resize-none font-mono wrap-anywhere"></textarea><!-- min-h-0 lets the box be shorter than its text, so the text scrolls inside it rather than stretching the page; wrap-anywhere because a path is one long word, and should wrap rather than scroll sideways -->
		<button type="button" class="self-start" @click="copy">{{ copied ? 'Copied' : 'Copy' }}</button>
	</main>
</template>
