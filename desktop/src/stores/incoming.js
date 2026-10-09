import {ref} from 'vue'
import {defineStore} from 'pinia'
import {engineSend, engineTake} from '../engine.js'
import {instanceTake} from '../instance.js'
import {log} from '../log.js'

/*
What comes up from below, and the page's side of the road down to the engine. Rust holds two drained queues, the lines the engine writes and the requests that reach this copy, and knows nothing about what's in either; this store takes them, several times a second, and is where they start to mean something. The engine's lines are JSON the engine wrote, so they're parsed here, and of the events the page cares about so far, ready is kept as the engine said it, for the About page, and every one of them goes to the log. The arrivals are kept as a list, each marked launch for this copy's own command line, handoff for one a second launch carried in, or open for files and links macOS opened with this copy. Adding a torrent will read that list.

Taking runs from main.js for the life of the app, not from a page, so nothing waits in Rust just because a different page is showing. If a queue ever filled while the page wasn't taking, Rust drops the oldest and counts them, and the count ends up here, which says so in the log.
*/

const takeEvery = 250//milliseconds between takes; fast enough that a clicked magnet shows up at once, slow enough to cost nothing
const arrivalsKept = 100//the page's own history of arrivals, until adding a torrent is what reads them

export const useIncomingStore = defineStore('incoming', () => {
	let ready = ref(null)//the engine's ready event, once it has said it
	let arrivals = ref([])//what has reached this copy, oldest first

	function send(message) {//a command down the road to the engine, as an object; it becomes one line of json here
		return engineSend(JSON.stringify(message))
	}

	async function take() {//everything waiting in both queues, read into the store
		let lines = await engineTake()
		if (lines.dropped) log(`engine: ${lines.dropped} lines dropped because the page fell behind`)//never, unless something is wrong
		for (let line of lines.items) {
			let event
			try { event = JSON.parse(line) } catch { continue }//a line that isn't json isn't anything the page can use
			if (event.event == 'ready') {
				ready.value = event
				log(`engine: ready, libtorrent ${event.libtorrent}, WebTorrent ${event.webtorrent ? 'on' : 'off'}, Python ${event.python}, data folder ${event.paths?.data || 'none'}`)//the data folder as the engine heard it from init, so it made the round trip
			}
			else if (event.event == 'folders') log(`engine: has the folders ${event.folders?.join(', ') || 'none'}`)//the download folders the page sent, echoed back
			else if (event.event == 'error') log(`engine: error, ${event.message}${event.command !== undefined ? ', ' + JSON.stringify(event.command) : ''}`)
		}
		let reached = await instanceTake()
		if (reached.dropped) log(`arrivals: ${reached.dropped} dropped because the page fell behind`)//the same
		if (reached.items.length > 0) arrivals.value = [...arrivals.value, ...reached.items].slice(-arrivalsKept)
	}

	function start() {//take now and then on a timer, for the life of the app; called once from main.js
		let tick = () => take().catch(() => {})//a take that fails, which would mean rust is gone, has no one to tell; the next one tries again
		tick()
		setInterval(tick, takeEvery)
	}

	return {ready, arrivals, send, start}
})
