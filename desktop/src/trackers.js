import {reactive} from 'vue'
import {Size, Time, tickToText} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go
import {netGet, netResolve, netLocal} from './net.js'
import {useIncomingStore} from './stores/incoming.js'
import {useSettingsStore} from './stores/settings.js'
import {log} from './log.js'

/*
The list panel, the Trackers tab: a check of right now, from this machine, of the trackers the public lists name. list.ftorrent.com checks the same trackers every hour from a server and keeps a record of uptime; this checks them once, when the user asks, and keeps only what answered, fastest first, so the trackers written into a new torrent are ones that answer this client today. Its focus is the reverse of the page's: how many milliseconds each tracker took to answer, not a calendar of hours.

A check runs in order. It saves the two lists the user may have edited, lists and always under [trackers] in ftorrent.toml, and asks net_local which families this machine has a route in, which are the only families it checks or shows. It fetches each list through net_get and merges them with always into one set of trackers, each named once in the spelling _normalize gives it. It looks up every name at once through net_resolve, since libtorrent looks names up one at a time and a slow name would add its wait to every reply time behind it, so a name that doesn't resolve, or has no address in a family, is decided before anything is announced. Then it announces to the rest from a session in the engine that only announces, up to inFlight at once, starting the next as each finishes, and shows each result as it arrives.

Each tracker is judged on its first try in each family: one that needs a second isn't one to put in a torrent. The engine passes up libtorrent's own alerts, each with libtorrent's time for it, so a reply time is the reply's moment minus the announce's, however late the poll that carried them. A UDP or HTTP tracker answers from each family's own address, which is what the session listens on; a WebSocket tracker gets one connection, over whichever family the system picks, so it has one result. When the check is done, the trackers that answered go into answered, fastest first, and the moment into checked. A check in which no tracker answered at all says more about this network than the trackers, a captive portal or blocked traffic, so it saves nothing and the last list stays.

The engine's session stays open once made: closing one waits out every open connection, minutes behind a dead WebSocket tracker, and the engine reads no other command while it waits. engine.py says more. Every tracker address here came from a list on the internet, so it's data and stays data: shown through Vue's interpolation, handed to net_resolve and the engine as a value, never put into markup.
*/

const inFlight = 20//announces out at once; a client downloading one torrent holds far more connections than this, and a dead tracker holds its place only until libtorrent gives up on it
const listLimit = 4*Size.mb//a list is a few hundred lines, so this much means something's wrong
const listSeconds = 15//how long fetching one list may take
const lookupLimit = 5*Time.second//how long one name may take to resolve before it counts as not resolving
const trackerLimit = 20*Time.second//how long a tracker may take to answer in every family it's announced in, past libtorrent's own 10 seconds for UDP and the 15 the engine gives HTTP, after which it's a timeout
const pollEvery = Time.second/10//how often a running check asks the engine for its alerts; the times come from libtorrent, so this sets only how soon each shows
const asleep = 30*Time.second//a gap this long between two ticks of that timer means the computer slept, or the system held the app's timers back, and every tracker still out would come back a timeout that says nothing about it
const checkPort = 49152 + Math.floor(Math.random()*16384)//the port the checking session listens on, picked once for this run of the app from the range IANA sets aside for this; announces carry it, and nothing connects to it

export const outcomeWords = {//each way a tracker can turn out in a family, in words for the panel; up shows its milliseconds instead
	waiting:     'Waiting',
	up:          'Up',
	dns:         "Name doesn't resolve",
	no_address:  'No address',
	timeout:     'Timed out',
	refused:     'Refused',
	cert:        "Certificate didn't verify",
	bad_reply:   'Bad reply',
	untried:     'Not tried',//the family a WebSocket tracker wasn't reached over, since libtorrent makes it one connection, over whichever family the system picks
	interrupted: 'Not finished',//still out when the computer slept
}

export const trackers = reactive({//the check as it stands, for the panel to draw
	running: false,
	words: '',//one line on what the check is doing, or how it ended
	families: [],//the families this machine has a route in, from the last check, 4 and 6
	lists: [],//each list as fetched: {url, found, error}
	results: [],//each tracker: {url, from, ws, at: {4: {outcome, ms, message}, 6: {...}}}, with at holding only the families this machine has
	started: 0,//when the check began, for how long it took
})

//a tracker's announce address in one spelling, so a tracker two lists write differently is checked once, the same rule list.ftorrent.com uses: scheme and host in lowercase, without the port the scheme implies; blank for anything that isn't a tracker address of a kind libtorrent speaks
function _normalize(text) {
	let m = text.trim().match(/^(udp|https?|wss?):\/\/(\[[0-9a-f:.]+\]|[^\s/:?#[\]]+)(?::(\d{1,5}))?(\/[^\s?#]*)?(\?[^\s#]*)?$/i)
	if (!m) return ''
	let scheme = m[1].toLowerCase()
	let port = m[3] ? Number(m[3]) : 0
	if (m[3] && !(port >= 1 && port <= 65535)) return ''
	let implied = {http: 80, ws: 80, https: 443, wss: 443}[scheme]
	return `${scheme}://${m[2].toLowerCase()}${port && port != implied ? ':' + port : ''}${m[4] ?? ''}${m[5] ?? ''}`
}

//the non-blank lines of a box or a list, trimmed, leaving out comments
function _lines(text) {
	return text.split(/\r?\n/).map(line => line.trim()).filter(line => line && !line.startsWith('#'))
}

//where a tracker came from, in a word, for the panel's From column: a list on GitHub by the account that keeps it, like ngosang, and any other by its site's name, the label before the top-level domain, like ftorrent for list.ftorrent.com and newtrackon for newtrackon.com; yours for a tracker from the second box
export function sayFrom(source) {
	if (source == 'always') return 'yours'
	let m = source.match(/^https:\/\/([^/]+)\/([^/]+)/)
	if (!m) return source
	if (m[1] == 'raw.githubusercontent.com') return m[2]
	let labels = m[1].split('.')
	return labels.length > 1 ? labels[labels.length - 2] : m[1]
}

//a list's address made shorter, for the facts: without its scheme, or for a file on GitHub, the account and the file, so ngosang's two lists read apart
export function sayList(url) {
	let m = url.match(/^https:\/\/raw\.githubusercontent\.com\/([^/]+)\/.*\/([^/]+)$/)
	return m ? `${m[1]} ${m[2]}` : url.replace(/^https:\/\//, '')
}

//the fastest reply a tracker gave in any family, in milliseconds, or Infinity when it answered in none, for putting the fastest first
export function trackerFastest(entry) {
	let times = Object.values(entry.at).filter(slot => slot.outcome == 'up').map(slot => slot.ms)
	if (times.length == 0) return Infinity
	return Math.min(...times)
}

//the addresses of the trackers this check found up, fastest first
function _upFastest() {
	return trackers.results.filter(entry => trackerFastest(entry) < Infinity).sort((a, b) => trackerFastest(a) - trackerFastest(b)).map(entry => entry.url)
}

//the trackers a preset picks, as a list to paste, one tracker to a line with a blank line between, so each is a tier of its own and a client that announces to every tier, as qBittorrent does, announces to each: from this check's results when there are some, and otherwise from answered, the last check's
export function trackersPick(preset) {
	let up = _upFastest()
	let from = up.length > 0 ? up : useSettingsStore().settings.trackers.answered
	let chosen = []
	if (preset == 'all') chosen = from
	else if (preset == 'fastest5') chosen = from.slice(0, 5)
	else if (preset == 'fastest10') chosen = from.slice(0, 10)
	else if (preset == 'fastest20') chosen = from.slice(0, 20)
	else if (preset == 'random20') chosen = _shuffle(from).slice(0, 20)
	else throw new Error(`no preset named ${preset}`)//a bug in the panel that asked
	if (chosen.length == 0) return ''
	return chosen.join('\n\n') + '\n'
}

//a copy of a list in a random order, every order as likely as any other, which sorting by a random comparison doesn't give
function _shuffle(list) {
	let copy = [...list]
	for (let i = copy.length - 1; i > 0; i--) {
		let j = Math.floor(Math.random()*(i + 1))//a place from the front through i, every one as likely, which is Fisher and Yates's shuffle
		let held = copy[i]
		copy[i] = copy[j]
		copy[j] = held
	}
	return copy
}

//how a failed announce ended, from the error libtorrent reports, as [outcome, words]: the same reading list.ftorrent.com gives, where its category says what kind of thing went wrong and its value which thing
function _classify(e) {
	if (e.category == 'http') return ['bad_reply', `HTTP ${e.status}`]//a web server answered with an error status
	if (e.reason) return ['bad_reply', e.reason]//the tracker's own words for refusing
	if (e.category == 'asio.netdb') return ['dns', e.message]
	if (e.category == 'libtorrent' && e.value == 202) return ['dns', e.message]//a name in another script, which libtorrent refuses, since it can be made to look like a different name
	if (e.category == 'libtorrent' && e.value == 180) return ['no_address', e.message]//the name has no address in this family
	if (e.category == 'asio.ssl.stream') return ['refused', e.message]//the encrypted connection was cut off partway
	if (e.category == 'asio.ssl') return ['cert', e.message]
	if (e.category == 'system' && /CONNREFUSED|CONNRESET|EPIPE/.test(e.name)) return ['refused', e.message]//something answered and turned the announce away, by the name of the error, which the platform's numbers would hide
	if (e.category == 'system' || (e.category == 'libtorrent' && e.value == 36)) return ['timeout', e.message]//nothing answered: timed out, or no route
	return ['bad_reply', e.message]//something answered, and not with a tracker's reply
}

//the engine's events this module hears, while a check is running; the incoming store hands every event to each listener for the life of the app, so this is added once
let heardOnce = false
let byTracker = new Map()//tracker address to its result, for an announce the engine hasn't answered with a hash yet
let byHash = new Map()//info hash to the result it was announced for
let waiters = []//[test, resolve] for each event a check is waiting on
let onAlert = null//what a running check does with each tracker alert

function _heard(event) {//one engine event, as the incoming store hands it over: an answer a check is waiting on, the hash an announce went out under, or a tracker alert for the running check
	for (let waiter of [...waiters]) if (waiter[0](event)) { waiters = waiters.filter(w => w != waiter); waiter[1](event) }
	if (event.event == 'check_added') {
		let entry = byTracker.get(event.tracker)
		if (entry) { entry.hash = event.hash; byHash.set(event.hash, entry) }
	}
	else if (event.event == 'check_alert' && onAlert) onAlert(event)
}

function _waitFor(test, limit) {//the first engine event that passes test, or false when none has after limit
	return new Promise(resolve => {
		let waiter = [test, resolve]
		waiters.push(waiter)
		setTimeout(() => { if (waiters.includes(waiter)) { waiters = waiters.filter(w => w != waiter); resolve(false) } }, limit)
	})
}

function _lookup(host) {//every address a name resolves to, or an error, within lookupLimit; a host written as an address is its own answer
	if (/^\d+\.\d+\.\d+\.\d+$/.test(host)) return Promise.resolve({addresses: [host]})
	if (host.startsWith('[')) return Promise.resolve({addresses: [host.slice(1, -1)]})
	let late = new Promise(resolve => setTimeout(() => resolve({error: `no answer from DNS in ${lookupLimit/Time.second} seconds`}), lookupLimit))
	return Promise.race([netResolve(host).then(addresses => ({addresses}), error => ({error: String(error)})), late])//a slow lookup finishes on Rust's thread on its own; the check stops waiting for it
}

//check the trackers: the lists and always as the user left them in the two boxes, which this saves to the settings first
export async function trackersCheck(listsText, alwaysText) {
	if (trackers.running) return
	let settings = useSettingsStore()
	let incoming = useIncomingStore()
	if (!heardOnce) { incoming.listen(_heard); heardOnce = true }
	trackers.running = true
	trackers.started = Date.now()
	trackers.results = []
	trackers.lists = []
	try {
		//the two lists, kept in the settings as the user left them, each line that isn't an address of its kind left out
		let lists = _lines(listsText).filter(url => /^https:\/\/[^\s/?#]+/i.test(url))
		let always = [...new Set(_lines(alwaysText).map(_normalize).filter(Boolean))]
		settings.settings.trackers.lists = lists
		settings.settings.trackers.always = always
		await settings.save()

		//which families this machine can check, from the address it would send from toward each; connecting a UDP socket sends nothing
		trackers.words = "Finding this machine's addresses"
		let [four, six] = await Promise.allSettled([netLocal('192.0.2.1'), netLocal('2001:db8::1')])//addresses set aside for documentation, which name no one
		let local = {}
		if (four.status == 'fulfilled') local[4] = four.value
		if (six.status == 'fulfilled') local[6] = six.value
		trackers.families = Object.keys(local).map(Number)
		if (trackers.families.length == 0) { trackers.words = 'This machine has no route to the internet, so there is nothing to check'; return }

		//every list at once, and always beside them, into one set of trackers
		trackers.words = `Fetching ${lists.length} lists`
		trackers.lists = lists.map(url => ({url, found: 0, error: ''}))
		let fetched = await Promise.all(lists.map(async (url, i) => {
			try {
				let found = [...new Set(_lines(await netGet(url, listLimit, listSeconds)).map(_normalize).filter(Boolean))]
				trackers.lists[i].found = found.length
				return found
			} catch (error) {
				trackers.lists[i].error = String(error)
				return []
			}
		}))
		let from = new Map()//tracker address to the sources that name it
		let add = (url, source) => from.set(url, [...(from.get(url) ?? []), source])
		fetched.forEach((found, i) => found.forEach(url => add(url, lists[i])))
		always.forEach(url => add(url, 'always'))
		trackers.results = [...from].map(([url, sources]) => ({url, from: sources, ws: /^wss?:/.test(url), hash: '', started: 0, sent: {}, done: false, at: Object.fromEntries(trackers.families.map(f => [f, {outcome: 'waiting', ms: 0, message: ''}]))}))

		//every name at once, deciding the trackers that can't be reached in a family before announcing
		let hostOf = url => url.match(/^\w+:\/\/(\[[^\]]+\]|[^/:?#]+)/)[1]
		let hosts = [...new Set(trackers.results.map(entry => hostOf(entry.url)))]
		trackers.words = `Looking up ${hosts.length} names`
		let answers = new Map(await Promise.all(hosts.map(async host => [host, await _lookup(host)])))
		for (let entry of trackers.results) {
			let answer = answers.get(hostOf(entry.url))
			for (let f of trackers.families) {
				if (answer.error) entry.at[f] = {outcome: 'dns', ms: 0, message: answer.error}
				else if (!answer.addresses.some(a => a.includes(':') == (f == 6))) entry.at[f] = {outcome: 'no_address', ms: 0, message: `no IPv${f} address`}
			}
		}

		//the engine's session, on each family's own address
		let made = incoming.send({command: 'check_session', addresses: trackers.families.map(f => local[f]), port: checkPort})
		let answered = await _waitFor(e => e.event == 'check_session' || (e.event == 'error' && e.command == 'check_session'), 5*Time.second)
		await made
		if (!answered || answered.event == 'error') { trackers.words = `The engine couldn't start checking${answered ? ', ' + answered.message : ''}`; return }

		//announce, up to inFlight at once, each judged on its first outcome in each family it's announced in
		let send = message => incoming.send(message).catch(error => log(`trackers: sending ${message.command}, ${error}`))//many commands a second, none of which the check waits on, so a failure goes to the log
		let queue = trackers.results.filter(entry => Object.values(entry.at).some(slot => slot.outcome == 'waiting'))
		let live = 0
		let poller = null
		let interrupted = false//the computer slept partway through, so what's still out can't be judged, and nothing is saved
		let lastTick = Date.now()
		await new Promise(finished => {
			let complete = entry => {//a WebSocket tracker is done at its one outcome, and the rest at one in every family
				let slots = Object.values(entry.at)
				if (entry.ws) return slots.some(slot => slot.outcome != 'waiting')
				return slots.every(slot => slot.outcome != 'waiting')
			}
			let finish = entry => {//done with one tracker: forget its torrent, and start the next
				entry.done = true
				for (let slot of Object.values(entry.at)) if (slot.outcome == 'waiting') slot.outcome = entry.ws ? 'untried' : 'timeout'
				if (entry.hash) send({command: 'check_remove', hash: entry.hash})
				live--
				fill()
			}
			let fill = () => {
				while (live < inFlight && queue.length > 0) {
					let entry = queue.shift()
					live++
					entry.started = Date.now()
					byTracker.set(entry.url, entry)
					send({command: 'check_add', tracker: entry.url})
				}
				let waiting = trackers.results.filter(entry => !entry.done && entry.started).length
				let up = trackers.results.filter(entry => trackerFastest(entry) < Infinity).length
				trackers.words = `Announcing: ${up} up, ${waiting} waiting, ${queue.length} to go`
				if (live == 0 && queue.length == 0) { clearInterval(poller); onAlert = null; finished() }
			}
			onAlert = e => {
				let entry = byHash.get(e.hash)
				if (!entry || entry.done || !entry.at[e.family]) return
				if (e.kind == 'announce') { entry.sent[e.family] ??= e.time; return }
				if (e.kind != 'reply' && e.kind != 'error') return
				let slot = entry.at[e.family]
				if (slot.outcome != 'waiting') return//its first outcome in this family is the one kept, and a family decided before announcing stays decided
				let ms = Math.max(0, Math.round(e.time - (entry.sent[e.family] ?? entry.started)))
				if (e.kind == 'reply') entry.at[e.family] = {outcome: 'up', ms, message: ''}
				else { let [outcome, message] = _classify(e); entry.at[e.family] = {outcome, ms, message} }
				if (complete(entry)) finish(entry)
			}
			poller = setInterval(() => {
				if (Date.now() - lastTick > asleep) {//stop here, leaving what had answered on the panel, rather than time everything out at once
					interrupted = true
					queue.length = 0
					for (let entry of trackers.results) {
						if (entry.done || !entry.started) continue//decided already, or never announced
						entry.done = true
						for (let slot of Object.values(entry.at)) if (slot.outcome == 'waiting') slot.outcome = 'interrupted'
						if (entry.hash) send({command: 'check_remove', hash: entry.hash})
					}
					live = 0
					fill()
					return
				}
				lastTick = Date.now()
				send({command: 'check_poll'})
				for (let entry of trackers.results) if (!entry.done && entry.started && Date.now() - entry.started > trackerLimit) {//no answer in time, in a family still waiting
					for (let slot of Object.values(entry.at)) if (slot.outcome == 'waiting') Object.assign(slot, {outcome: 'timeout', ms: trackerLimit, message: `no answer in ${trackerLimit/Time.second} seconds`})
					finish(entry)
				}
			}, pollEvery)
			fill()
		})
		byTracker.clear()
		byHash.clear()

		//what answered, fastest first, into the settings, unless nothing did
		let up = _upFastest()
		let seconds = Math.round((Date.now() - trackers.started)/Time.second)
		if (interrupted) {
			trackers.words = 'The computer slept during the check, so it stopped, and the last list stays'
		} else if (up.length == 0) {
			trackers.words = `No tracker answered in ${seconds} seconds, which points at this network rather than the trackers, so the last list stays`
		} else {
			settings.settings.trackers.answered = up
			settings.settings.trackers.checked = tickToText(Date.now())
			await settings.save()
			trackers.words = `${up.length} of ${trackers.results.length} trackers answered, in ${seconds} seconds`
		}
		log(`trackers: checked ${trackers.results.length} over IPv${trackers.families.join(' and IPv')} in ${seconds} seconds, ${up.length} answered${interrupted ? ', stopped by sleep' : ''}`)
	} catch (error) {
		trackers.words = `The check stopped: ${error}`
		log(`trackers: the check stopped, ${error}`)
	} finally {
		trackers.running = false
	}
}
