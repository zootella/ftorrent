<script setup>
import {ref, computed, nextTick, onMounted} from 'vue'
import {Time} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go
import {connectionRun, connectionFacts, sayRun, sayVerdict, sayApart} from '../connection.js'

let run = ref(null)//the latest run, null until the first one comes back
let facts = computed(() => connectionFacts(run.value))
let verdict = computed(() => run.value ? sayVerdict(run.value) : '')
let apart = computed(() => run.value ? sayApart(run.value) : '')//what it likely means when UDP and HTTPS left from different networks, blank when they didn't
let text = ref('')//every run since the page opened, oldest first
let running = ref(false)//so Check waits while a run is out
let box = ref(null)//the textarea, scrolled to the newest run as each one lands

async function check() {
	running.value = true
	run.value = await connectionRun()
	text.value += (text.value ? '\n\n' : '') + sayRun(run.value)//a blank line between runs
	running.value = false
	await nextTick()//once vue has put the new text in the box
	box.value.scrollTop = box.value.scrollHeight
}
onMounted(check)//visiting the page is the request, so the first run starts as it opens

//everything on the page as text, the facts, the verdict and the sentence under it, and the box, since it's most useful pasted into a message or an issue
let copied = ref(false)//true for a moment after the button is pressed, so it can say so
async function copy() {
	let lines = facts.value.map(([key, value]) => `${key}: ${value}`)
	lines.push('', verdict.value)
	if (apart.value) lines.push(apart.value)
	lines.push('', text.value)
	await navigator.clipboard.writeText(lines.join('\n'))
	copied.value = true
	setTimeout(() => { copied.value = false }, 1.5*Time.second)
}
</script>

<template>
	<main>
		<h1>good</h1>
		<dl class="facts">
			<template v-for="[key, value] in facts" :key="key">
				<dt>{{ key }}</dt>
				<dd>{{ value }}</dd>
			</template>
		</dl>
		<p class="verdict">{{ verdict }}</p>
		<p v-if="apart">{{ apart }}</p>
		<button type="button" :disabled="running" @click="check">Check</button>
		<textarea ref="box" readonly :value="text"></textarea>
		<button type="button" @click="copy">{{ copied ? 'Copied' : 'Copy' }}</button>
	</main>
</template>

<style scoped>
main {
	min-height: 100dvh; /* at least the window, which moves with a phone's toolbars, so on a tall screen the box below takes whatever height is left, and on a short one the page grows and scrolls */
	padding: 1rem;
	display: flex;
	flex-direction: column;
	gap: 1rem;
}

h1 {
	font-size: 2rem;
	font-weight: 600;
}

.facts {
	display: grid;
	grid-template-columns: auto 1fr;
	column-gap: 1.5rem;
}

@media (max-width: 40rem) {
	.facts {
		grid-template-columns: 1fr; /* on a phone each name sits above its value, so a long address gets the whole width */
	}

	dd {
		margin-bottom: 0.5rem; /* a gap between one fact and the next name */
	}
}

dt {
	color: var(--text-muted);
}

dd {
	overflow-wrap: anywhere;
}

.verdict {
	font-weight: 600;
}

button {
	align-self: flex-start;
	border: 1px solid var(--line);
	border-radius: 0.375rem;
	padding: 0.25rem 0.75rem;
}

button:enabled:hover {
	border-color: var(--text);
}

button:disabled {
	color: var(--text-muted);
}

textarea {
	flex: 1; /* all the height the rest of the page leaves */
	min-height: 12rem; /* about nine lines at the least, so on a phone the box stays usable and the page scrolls instead */
	padding: 0.5rem;
	border: 1px solid var(--line);
	border-radius: 0.375rem;
	background: transparent;
	color: inherit;
	font: inherit;
	font-size: 0.8rem;
	white-space: pre-wrap; /* the padded columns line up as written, and a line too long for the box wraps */
	overflow-wrap: anywhere; /* even inside an IPv6 address, which has no space to wrap at */
	overflow-x: hidden;
	resize: none; /* the window sets its size */
}
</style>
