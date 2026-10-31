<script setup>
import {ref, computed, watch, nextTick, onMounted} from 'vue'
import {Time} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go
import KeyValues from '../components/KeyValues.vue'
import {connectionRun, connectionRetries, connectionFacts, connectionText, sayRun, lastRun, firewallButtons, firewallSettings, firewallWrite, mappingRun, mappingFacts, sayMappingRun, lastMappingRun, box, boxAdd, session, sessionFacts, sessionOpen, sessionStart, sessionMapping, sessionStop} from '../connection.js'

let facts = computed(() => [...connectionFacts(lastRun.value), ...mappingFacts(lastMappingRun.value), ...sessionFacts()])
let running = ref(false)//so Check, Try mapping, and Write firewall rules… wait while any one's run is out
let boxElement = ref(null)//the textarea

async function check() {
	running.value = true
	lastRun.value = await connectionRun()
	boxAdd(sayRun(lastRun.value), 'run')
	running.value = false
	connectionRetries()//ask again what heard nothing, when the window comes forward or on the two timers, in case a system box held it
}

//ask the router for each mapping, prove each with moon, and undo it, which changes the router for a minute, so it runs only when asked
async function tryMapping() {
	running.value = true
	lastMappingRun.value = await mappingRun()
	boxAdd(sayMappingRun(lastMappingRun.value), 'run')
	running.value = false
}

//write this copy's Windows firewall rules, behind Windows' own prompt, which changes the machine, so the other runs wait
async function writeFirewallRules() {
	running.value = true
	try { await firewallWrite() }
	finally { running.value = false }//even when the run couldn't start, so the buttons don't stay greyed
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
	setTimeout(() => { copied.value = false }, 1.5*Time.second)
}
</script>

<template>
	<!-- ./src/pages/ConnectionPage.vue -->
	<!-- ftorrent's connection to the network: Check and the session's controls at the top, the facts below them, as tall as they need, a box of text filling the middle, which scrolls on its own, and Copy at the bottom -->
	<main class="h-full flex flex-col gap-2">
		<div class="flex gap-2">
			<button type="button" :disabled="running" @click="check">Check</button>
			<button type="button" :disabled="running" @click="tryMapping">Try mapping</button>
			<button type="button" @click="sessionOpen">Open…</button>
			<button type="button" :disabled="session.running" @click="sessionStart">Start</button>
			<button type="button" :disabled="!session.running || session.mapping" @click="sessionMapping(true)">Map</button>
			<button type="button" :disabled="!session.running || !session.mapping" @click="sessionMapping(false)">Unmap</button>
			<button type="button" :disabled="!session.running" @click="sessionStop">Stop</button>
			<button v-if="firewallButtons.settings" type="button" @click="firewallSettings">Firewall settings…</button><!-- the page where the person makes the exception by hand: on a Mac System Settings' Network pane, with Firewall a row below, and firewallPane in connection.js says why it stops at Network; on Windows Control Panel's Allow an app through Windows Firewall -->
			<button v-if="firewallButtons.write" type="button" :disabled="running" @click="writeFirewallRules">Write firewall rules…</button><!-- on Windows, for an installed copy, netsh replacing the rules naming this copy and its engine with allow rules, behind one UAC prompt -->
		</div>
		<KeyValues :rows="facts" />
		<textarea ref="boxElement" readonly :value="box" class="flex-1 min-h-0 resize-none font-mono wrap-anywhere"></textarea><!-- min-h-0 lets the box be shorter than its text, so the text scrolls inside it rather than stretching the page; wrap-anywhere because a path is one long word, and should wrap rather than scroll sideways -->
		<button type="button" class="self-start" @click="copy">{{ copied ? 'Copied' : 'Copy' }}</button>
	</main>
</template>
