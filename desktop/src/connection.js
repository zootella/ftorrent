import {getVersion} from '@tauri-apps/api/app'
import {brandName, urlHome} from './brand.js'
import {platformName} from './settings.js'
import {Time, sayMoment} from './time.js'
import {netGet, netLocal} from './net.js'

/*
The Connection tab's checks: what this machine and the network say about its connection, asked when the user presses Check and at no other time, so opening the tab asks nothing of anyone. good.ftorrent.com asks some of the same questions from a browser; that page is separate code, and what the two share is the servers they ask and what each server's answer means.

For each family, IPv4 and IPv6, a run asks two things. The system says which of the machine's addresses it would send from to reach an address beyond the local network, which takes no packet at all. And a reflector at ip4.ftorrent.com or ip6.ftorrent.com answers the address the request arrived from; each name has an address record of one family only, so the request can only travel over that family, and the pair of answers is which families reach the internet from here. Behind a home router the two IPv4 answers differ, since the router translates; on IPv6 they're usually the same address, since nothing does.

A check that asks once and waits for one answer gets checkSeconds to hear back. One round trip is tens of milliseconds, and a fetch over https is four, the name, the connection, the encryption, and the request, so around 200 milliseconds on a typical connection and around 600 across an ocean. The slowest ordinary connection, a satellite in geostationary orbit, has a round trip of 600 milliseconds, which puts a fetch near two and a half seconds, and a lost packet is sent again after a second. Four seconds covers all of those, and only a path that silently drops what it's sent waits that long, since no route, or a name with no address, fails at once whatever the limit. A check that waits longer by design gets its own time when it arrives. So a check ends one of three ways: an answer, in milliseconds; a failure that came back with its reason, like no route; or no answer in checkSeconds.

A run asks every check at once and waits for the last, so it takes as long as its slowest check, and the box lists the checks in the table's order whatever order they finish in, so two runs line up line for line. An error is an answer like any other: the box shows its text, since a user on this tab is under the hood. The box keeps every run since the tab opened, oldest first, each starting with a line that says when it ran, which build, and on which platform, so a box pasted into a message or an issue says where its lines came from.
*/

const checkSeconds = 4//how long a check that asks once has to hear back, the essay above says why
const noAnswer = `no answer in ${checkSeconds} seconds`
const reflectorLimit = 100//bytes; the reflector answers one address and a newline, at most 46

//just enough to tell an address of each family from anything else a server or the system could answer
const looksLike = {
	4: /^\d{1,3}(\.\d{1,3}){3}$/,
	6: /^[0-9a-f]*:[0-9a-f:.]*$/i,
}

//the checks a run makes, in the order the table and the box list them, each family's local address beside its public one. 192.0.2.1 and 2001:db8::1 are addresses set aside for documentation, by RFC 5737 and RFC 3849, so they name no one, and any address beyond the local network gets the same answer, the default route's
let checks = [
	_route(4, '192.0.2.1'),
	_reflect(4),
	_route(6, '2001:db8::1'),
	_reflect(6),
]

//a check of the address the system would send from toward an address of this family
function _route(family, toward) {
	return {family, fact: `Local IPv${family} address`, asked: `route to ${toward}`, ask: () => netLocal(toward)}
}

//a check of the address a reflector sees a request of this family arrive from
function _reflect(family) {
	let url = `https://ip${family}.${urlHome}/`
	return {family, fact: `Public IPv${family} address`, asked: `GET ${url}`, ask: () => netGet(url, reflectorLimit, checkSeconds)}
}

//ask every check once, and answer the run: when it started, this copy's version, and each check's result in the order above
export async function connectionRun() {
	let moment = Date.now()
	let version = await getVersion()
	let results = await Promise.all(checks.map(_ask))//all at once; Promise.all keeps the order it was given, whatever order they finish in
	return {moment, version, results}
}

//one check, timed, as {fact, asked, success, value or outcome, milliseconds}
async function _ask(check) {
	let start = performance.now()
	let v
	try {
		let answer = (await check.ask()).trim()//the reflector ends its answer with a newline
		if (looksLike[check.family].test(answer)) v = {success: true, value: answer}
		else v = {success: false, outcome: `not an IPv${check.family} address: ${answer.slice(0, 60)}`}
	} catch (error) {//the bottom gate: rust's refusal becomes an ordinary answer, with its text kept for the box
		v = {success: false, outcome: String(error)}
	}
	let milliseconds = Math.round(performance.now() - start)
	if (!v.success && milliseconds >= checkSeconds*Time.second) v.outcome = noAnswer//net_get gave up at checkSeconds, whatever words the http library chose for it; the clock here started first, so it reads at least as long
	return {fact: check.fact, asked: check.asked, ...v, milliseconds}
}

//the table's rows for a run, [fact, value] in the order above, blank before the first run
export function connectionFacts(run) {
	if (!run) return checks.map(check => [check.fact, ''])
	return run.results.map(r => [r.fact, _sayFact(r)])
}

//a result in the table's words: the address, no answer for a check that ran out of time, or none for one that failed
function _sayFact(r) {
	if (r.success) return r.value
	if (r.outcome == noAnswer) return 'no answer'
	return 'none'
}

//the box's lines for one run: when, which build, and which platform, then a line per check with what it asked, how long it took, and what came back
export function sayRun(run) {
	let width = Math.max(...run.results.map(r => r.asked.length))//the asked column as wide as its longest entry, so the times line up under each other
	let lines = [`${sayMoment(run.moment)}, ${brandName} ${run.version} on ${platformName}`]
	for (let r of run.results) lines.push(`${r.asked.padEnd(width)}  ${`${r.milliseconds} ms`.padStart(7)}  ${r.success ? r.value : r.outcome}`)//the answer last, since an error's text can run long
	return lines.join('\n')
}
