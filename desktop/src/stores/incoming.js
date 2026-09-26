import {ref} from 'vue'
import {defineStore} from 'pinia'
import {engineSend, engineTake} from '../engine.js'
import {instanceTake} from '../instance.js'

/*
What comes up from below, and the page's side of the road down to the engine. Rust holds two drained queues, the lines the engine writes and the requests that reach this copy, and knows nothing about what's in either; this store takes them, several times a second, and is where they start to mean something. The engine's lines are JSON the engine wrote, so they're parsed here, and the events the page cares about so far, ready and the folders echo, are kept as the engine last said them. The arrivals are kept as a list, each marked launch for this copy's own command line or handoff for one a second launch carried in. Adding a torrent will read that list.

Taking runs from main.js for the life of the app, not from a page, so nothing waits in Rust just because a different page is showing. If a queue ever filled while the page wasn't taking, Rust drops the oldest and counts them, and the count ends up here, where the main page shows it.
*/

const takeEvery = 250//milliseconds between takes; fast enough that a clicked magnet shows up at once, slow enough to cost nothing
const arrivalsKept = 100//the page's own history of arrivals, until adding a torrent is what reads them
const errorsKept = 20//and of error lines from the engine

export const useIncomingStore = defineStore('incoming', () => {
	let ready = ref(null)//the engine's ready event, once it has said it
	let folders = ref(null)//the download folders the engine last echoed back, once it has
	let errors = ref([])//error lines from the engine, most recent last
	let arrivals = ref([])//what has reached this copy, oldest first
	let dropped = ref({engine: 0, arrivals: 0})//lines and arrivals rust had to drop because the page fell behind; zero, always, unless something is wrong

	function send(message) {//a command down the road to the engine, as an object; it becomes one line of json here
		return engineSend(JSON.stringify(message))
	}

	async function take() {//everything waiting in both queues, read into the store
		let lines = await engineTake()
		dropped.value.engine += lines.dropped
		for (let line of lines.items) {
			let event
			try { event = JSON.parse(line) } catch { continue }//a line that isn't json isn't anything the page can use
			if (event.event == 'ready') ready.value = event
			else if (event.event == 'folders') folders.value = event.folders
			else if (event.event == 'error') errors.value = [...errors.value, event].slice(-errorsKept)
		}
		let reached = await instanceTake()
		dropped.value.arrivals += reached.dropped
		if (reached.items.length > 0) arrivals.value = [...arrivals.value, ...reached.items].slice(-arrivalsKept)
	}

	function start() {//take now and then on a timer, for the life of the app; called once from main.js
		let tick = () => take().catch(() => {})//a take that fails, which would mean rust is gone, has no one to tell; the next one tries again
		tick()
		setInterval(tick, takeEvery)
	}

	return {ready, folders, errors, arrivals, dropped, send, start}
})
