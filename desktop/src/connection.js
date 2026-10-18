import {getVersion} from '@tauri-apps/api/app'
import {brandName, urlHome} from './brand.js'
import {platformName} from './settings.js'
import {Time, sayMoment} from './time.js'
import {netGet, netLocal, netStun} from './net.js'

/*
The Connection tab's checks: what this machine and the network say about its connection, asked when the user presses Check and at no other time, so opening the tab asks nothing of anyone. good.ftorrent.com asks some of the same questions from a browser; that page is separate code, and what the two share is the servers they ask and what each server's answer means.

For each family, IPv4 and IPv6, a run asks three things. The system says which of the machine's addresses it would send from to reach an address beyond the local network, which takes no packet at all. A reflector at ip4.ftorrent.com or ip6.ftorrent.com answers the address the request arrived from; each name has an address record of one family only, so the request can only travel over that family, and the pair of answers is which families reach the internet from here. Behind a home router the two IPv4 answers differ, since the router translates; on IPv6 they're usually the same address, since nothing does.

And STUN, over UDP, which is what a peer connection travels on: our server and Cloudflare's, asked at once from one local port, the way Tailscale's netcheck asks its servers, each answering the address and port it saw the request arrive from. A router keys the mapping it makes on the inside address and port, so when both servers see the same public port, it keeps one mapping for every destination, endpoint-independent mapping, which hole punching needs; a new port for each server is endpoint-dependent, the symmetric case, where hole punching fails. The address STUN saw should be the one the reflector saw, and when it isn't, something carries one path and not the other, a VPN, a proxy, or a relay. On IPv6 these comparisons are of the /64, the network, since a machine can send from more than one of its temporary addresses, and a Mac sometimes from one it has already deprecated.

A check that asks once and waits for one answer gets checkSeconds to hear back. One round trip is tens of milliseconds, and a fetch over https is four, the name, the connection, the encryption, and the request, so around 200 milliseconds on a typical connection and around 600 across an ocean. The slowest ordinary connection, a satellite in geostationary orbit, has a round trip of 600 milliseconds, which puts a fetch near two and a half seconds, and a lost packet is sent again after a second. Four seconds covers all of those, and only a path that silently drops what it's sent waits that long, since no route, or a name with no address, fails at once whatever the limit. A check that waits longer by design gets its own time when it arrives. So a check ends one of three ways: an answer, in milliseconds; a failure that came back with its reason, like no route; or no answer in checkSeconds.

A run asks every check at once and waits for the last, so it takes as long as its slowest check, at most checkSeconds. The box lists the checks in the same order every time, whatever order they finish in, so two runs line up line for line, and a family's one STUN call shows as a line for each server it asked. An error is an answer like any other: the box shows its text, since a user on this tab is under the hood. The box keeps every run since the tab opened, oldest first, each starting with a line that says when it ran, which build, and on which platform, so a box pasted into a message or an issue says where its lines came from.
*/

const checkSeconds = 4//how long a check that asks once has to hear back, the essay above says why
const noAnswer = `no answer in ${checkSeconds} seconds`
const reflectorLimit = 100//bytes; the reflector answers one address and a newline, at most 46
const stunServers = [`stun.${urlHome}:3478`, 'stun.cloudflare.com:3478']//ours, then Cloudflare's, on a separate network, as the second vantage until our own second server takes its place

//192.0.2.1 and 2001:db8::1 are addresses set aside for documentation, by RFC 5737 and RFC 3849, so they name no one, and any address beyond the local network gets the same answer, the default route's
const beyond = {4: '192.0.2.1', 6: '2001:db8::1'}

//just enough to tell an address of each family from anything else a server or the system could answer
const looksLike = {
	4: /^\d{1,3}(\.\d{1,3}){3}$/,
	6: /^[0-9a-f]*:[0-9a-f:.]*$/i,
}

//the checks for one run, in the order the box lists them: for each family, its local address, its public address over HTTPS, and STUN
function _checks() {
	let checks = []
	for (let family of [4, 6]) checks.push(_route(family), _reflect(family), _stun(family))
	return checks
}

//a check of the address the system would send from to reach beyond the local network. Each check is {kind, family, asked, ask, read}: what it is, what it asks in the box's words, the call that asks, and what reads the call's answer into {success, value or outcome}
function _route(family) {
	return {kind: 'route', family, asked: `route to ${beyond[family]}`, ask: () => netLocal(beyond[family]), read: text => _readAddress(family, text)}
}

//a check of the address a reflector sees a request of this family arrive from
function _reflect(family) {
	let url = `https://ip${family}.${urlHome}/`
	return {kind: 'reflect', family, asked: `GET ${url}`, ask: () => netGet(url, reflectorLimit, checkSeconds), read: text => _readAddress(family, text.trim())}//the reflector ends its answer with a newline
}

//a check of the address and port each STUN server sees a request of this family arrive from, all asked from one local port
function _stun(family) {
	let port = 49152 + Math.floor(Math.random()*16384)//from the dynamic range, RFC 6335, a new one each run, so a mapping the router made for the last run can't answer for this one
	return {kind: 'stun', family, port, asked: `STUN from port ${port}`, ask: () => netStun(stunServers, family, port, checkSeconds), read: bindings => ({success: true, value: bindings})}//net_stun has read each answer's bytes into an address already
}

//an answer that should be an address of this family
function _readAddress(family, text) {
	if (looksLike[family].test(text)) return {success: true, value: text}
	return {success: false, outcome: `not an IPv${family} address: ${text.slice(0, 60)}`}
}

//ask every check once, and answer the run: when it started, this copy's version, and each check's result in the order the box lists them
export async function connectionRun() {
	let moment = Date.now()
	let version = await getVersion()
	let results = await Promise.all(_checks().map(_ask))//all at once; Promise.all keeps the order it was given, whatever order they finish in
	return {moment, version, results: results.flatMap(_perServer)}
}

//one check, timed, as {kind, family, port, asked, success, value or outcome, milliseconds}
async function _ask(check) {
	let start = performance.now()
	let v
	try {
		let answer = await Promise.race([check.ask(), _late()])
		if (answer === late) v = {success: false, outcome: noAnswer}//strict, since the answer may be text, an object, or this
		else v = check.read(answer)
	} catch (error) {//the bottom gate: rust's refusal becomes an ordinary answer, with its text kept for the box
		v = {success: false, outcome: String(error)}
	}
	let milliseconds = Math.round(performance.now() - start)
	if (!v.success && milliseconds >= checkSeconds*Time.second) v.outcome = noAnswer//rust gave up at checkSeconds, whatever words it chose for it; the clock here started first, so it reads at least as long
	return {kind: check.kind, family: check.family, port: check.port, asked: check.asked, ...v, milliseconds}
}

//a STUN result as a result for each server, since one call asks them all, with each server's own time from net_stun, which is the network's; any other result as it is
function _perServer(r) {
	if (r.kind != 'stun') return [r]
	return stunServers.map((server, i) => {
		let asked = `STUN ${server} from port ${r.port}`
		if (!r.success) return {...r, asked}//the call itself failed or ran out of time, for every server at once
		let b = r.value.servers[i]//this server's binding
		if (!b.error) return {kind: 'stun', family: r.family, asked, success: true, value: {...b, local: r.value.local}, milliseconds: b.milliseconds}
		return {kind: 'stun', family: r.family, asked, success: false, outcome: b.milliseconds >= checkSeconds*Time.second ? noAnswer : b.error, milliseconds: b.milliseconds}//the same three endings as every other check
	})
}

//what a check's answer is when it hasn't come a quarter second past checkSeconds: rust gives up at checkSeconds itself, so this only ever wins when a call stalls before its own clock starts, as a stuck name lookup would, and it keeps the run from waiting on it
const late = Symbol('late')
function _late() { return new Promise(resolve => setTimeout(() => resolve(late), checkSeconds*Time.second + Time.second/4)) }

//the table's rows for a run, four for each family, blank before the first run
export function connectionFacts(run) {
	let rows = []
	for (let family of [4, 6]) {
		let mine = kind => run ? run.results.filter(r => r.kind == kind && r.family == family) : []//this family's results of one kind, in the order asked, none before the first run
		let [route] = mine('route'), [reflect] = mine('reflect'), stuns = mine('stun')
		rows.push(
			[`Local IPv${family} address`,   run ? _sayFact(route)            : ''],
			[`Public IPv${family} address`,  run ? _sayFact(reflect)          : ''],
			[`Public IPv${family} over UDP`, run ? _sayUdp(stuns, reflect)    : ''],
			[`IPv${family} mapping`,         run ? _sayMapping(stuns)         : ''],
		)
	}
	return rows
}

//a result in the table's words: the address, no answer for a check that ran out of time, or none for one that failed
function _sayFact(r) {
	if (!r.success && r.outcome == noAnswer) return 'no answer'
	if (!r.success) return 'none'
	if (r.kind == 'stun') return _sayEndpoint(r.value.address, r.value.port)
	return r.value
}

//the address and port our STUN server saw, and a word when its network isn't the one the reflector saw over HTTPS
function _sayUdp([ours], reflect) {
	let said = _sayFact(ours)
	if (ours.success && reflect.success && !_sameNetwork(ours.family, ours.value.address, reflect.value)) said += `, but HTTPS came from ${reflect.value}`
	return said
}

//whether the two STUN servers saw one public port for the one local port
function _sayMapping([first, second]) {
	if (!first.success || !second.success) return 'unknown'
	let [a, b] = [first.value, second.value]
	if (a.port != b.port || !_sameNetwork(first.family, a.address, b.address)) return 'endpoint-dependent'
	if (a.port == a.local) return 'endpoint-independent, port kept'//the router left the port as the machine chose it, or nothing translates at all
	return 'endpoint-independent'
}

//whether two addresses of a family are on one network: the same address for IPv4, and the same /64 for IPv6, where a machine holds several addresses in its network at once
function _sameNetwork(family, a, b) {
	if (family == 4) return a == b
	return _sixtyFour(a) == _sixtyFour(b)
}

//an IPv6 address's first four groups of eight, its /64, with any :: written out and leading zeros dropped, so two spellings of one network compare equal
function _sixtyFour(address) {
	let groups = address.split(':')
	if (address.includes('::')) {
		let [head, tail] = address.split('::').map(side => side ? side.split(':') : [])
		groups = [...head, ...Array(8 - head.length - tail.length).fill('0'), ...tail]
	}
	return groups.slice(0, 4).map(group => parseInt(group, 16)).join(':')
}

//an address and a port the way a URL writes them, with brackets around an IPv6 address so its colons don't run into the port's
function _sayEndpoint(address, port) {
	if (address.includes(':')) return `[${address}]:${port}`
	return `${address}:${port}`
}

//the box's lines for one run: when, which build, and which platform, then a line per check with what it asked, how long it took, and what came back
export function sayRun(run) {
	let width = Math.max(...run.results.map(r => r.asked.length))//the asked column as wide as its longest entry, so the times line up under each other
	let lines = [`${sayMoment(run.moment)}, ${brandName} ${run.version} on ${platformName}`]
	for (let r of run.results) lines.push(`${r.asked.padEnd(width)}  ${`${r.milliseconds} ms`.padStart(7)}  ${_sayAnswer(r)}`)//the answer last, since an error's text can run long
	return lines.join('\n')
}

//what came back, in the box's words: the address, the error's text, or for STUN what the server saw and the answer's size
function _sayAnswer(r) {
	if (!r.success) return r.outcome
	if (r.kind != 'stun') return r.value
	let b = r.value//the binding net_stun answered
	let said = `${_sayEndpoint(b.address, b.port)}, ${b.bytes} bytes`
	if (b.sends > 1) said += `, sent ${b.sends} times`
	return said
}
