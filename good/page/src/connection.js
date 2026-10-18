/*
What a web page can learn about the connection it runs on, asked from inside the browser's sandbox. The desktop client's Connection tab asks many of the same questions from a real socket and can tell more; this page asks what any page can, which is what a browser peer has to work with.

For each family, IPv4 and IPv6, the page asks two things. A reflector at ip4.ftorrent.com or ip6.ftorrent.com answers the address the request arrived from; each name has an address record in one family only, so the browser, which picks a family for itself, can only reach it over that one, and the pair of answers is which families reach the internet from here, over TCP.

And STUN, over UDP, which is what a peer connection travels on. A page has no socket of its own, but WebRTC gives it one at a remove: the page makes a peer connection, names STUN servers, and asks for a data channel, which starts ICE (Interactive Connectivity Establishment), the dance by which two WebRTC peers find a path to each other. The page runs only its first step, gathering: the browser opens a socket on each network interface, sometimes two or three on one address, as Safari does, sends each server a binding request from each, and hands back candidates, the addresses a peer might reach this browser at. A host candidate is a local address, which every mainstream browser now writes as a made-up mDNS name ending in .local, so a page can't read its own local addresses; a server-reflexive candidate is what a STUN server saw, the public address and port, with its local address and port written 0.0.0.0 or :: and 0 for the same reason. The WebRTC project's Trickle ICE sample is the page that lists these candidates as they arrive, and browser NAT checkers build on the same stream.

The page makes three peer connections at once. The first names our server alone, and its reflexive candidates are this browser's public address and port over UDP, which should match what the reflector saw over HTTPS; when it doesn't, something sends the two out apart: over IPv4, a carrier's translator with more than one public address of its own, or in either family a VPN, a proxy, or a relay such as iCloud Private Relay carrying one path and not the other. The second names Cloudflare's alone, which shows it answers from here. The third names both, and the browser asks both from the same sockets. A router keys the mapping it makes on the inside address and port, so when both servers see the same public port the two candidates are the same, and the browser, which drops a reflexive candidate that repeats one it already has, hands back one. The page can't see how many sockets the browser gathered from, but asking one server, each socket answers one candidate, so the first connection's count in a family is the number of sockets. When the third's count is the same, every socket kept one public port for both servers, endpoint-independent mapping, the kind a hole punch works through; when it's double, each got a new port for the second server, endpoint-dependent, the symmetric case, where it fails. The desktop client asks from one socket it chose itself, so it has no counting to do.

The box keeps more than the verdicts need: every candidate line the browser wrote, every error a STUN server's silence raised, and the browser's own record of each local candidate, from getStats. What one browser writes and another leaves out is itself a finding at this stage, and it all fits in one paste.

Every check gets checkSeconds, the same four the desktop client gives its own: a fetch over HTTPS is four round trips and a STUN exchange one, so four seconds leaves room for a satellite link and a lost packet, and only a path that silently drops what it's sent waits that long.
*/

const checkSeconds = 4//how long each check has to hear back, the essay above says why
const second = 1000//milliseconds
const noAnswer = `no answer in ${checkSeconds} seconds`
const reflectors = {4: 'https://ip4.ftorrent.com/', 6: 'https://ip6.ftorrent.com/'}
const stunOurs   = 'stun.ftorrent.com:3478'
const stunSecond = 'stun.cloudflare.com:3478'//Cloudflare's, on a separate network, as the second vantage until our own second server takes its place
const gatherings = [//the three peer connections, by what each asks
	{which: 'ours',   servers: [stunOurs]},
	{which: 'second', servers: [stunSecond]},
	{which: 'both',   servers: [stunOurs, stunSecond]},
]
const statsSkip = ['id', 'timestamp', 'type', 'transportId', 'isRemote']//the bookkeeping on every getStats record, the same on each, which the box leaves out

//just enough to tell an address of each family from anything else a server could answer
const looksLike = {
	4: /^\d{1,3}(\.\d{1,3}){3}$/,
	6: /^[0-9a-f]*:[0-9a-f:.]*$/i,
}

//ask everything once, and answer the run: when it started, the browser it ran in, each result in the order the box lists them, and each gathering in full
export async function connectionRun() {
	let moment = Date.now()
	let [reflect4, reflect6, ...found] = await Promise.all([//all at once; Promise.all keeps the order it was given, whatever order they finish in
		_reflect(4),
		_reflect(6),
		...gatherings.map(g => _gather(g.servers)),
	])
	let results = [reflect4, reflect6]
	gatherings.forEach((g, i) => results.push(_reflexive(found[i], 4, g), _reflexive(found[i], 6, g)))
	return {moment, browser: navigator.userAgent, results, gatherings: gatherings.map((g, i) => ({...g, ...found[i]}))}
}

//fetch a reflector, timed, as a result: {kind, family, asked, success, value or outcome, milliseconds}
async function _reflect(family) {
	let start = performance.now()
	let v
	try {
		let response = await fetch(reflectors[family], {cache: 'no-store', signal: AbortSignal.timeout(checkSeconds*second)})
		let text = (await response.text()).trim()//the reflector ends its answer with a newline
		if (!response.ok) v = {success: false, outcome: `the reflector answered ${response.status}`}
		else if (looksLike[family].test(text)) v = {success: true, value: text}
		else v = {success: false, outcome: `not an IPv${family} address: ${text.slice(0, 60)}`}
	} catch (error) {//the bottom gate: no route, a refused connection, or the timeout's abort, as an ordinary answer
		v = {success: false, outcome: error.name == 'TimeoutError' ? noAnswer : `${error.name}: ${error.message}`}
	}
	return {kind: 'reflect', family, asked: `GET ${reflectors[family]}`, ...v, milliseconds: Math.round(performance.now() - start)}
}

//make a peer connection naming these STUN servers, and collect everything its browser reports until gathering finishes or checkSeconds pass; answers {candidates, errors, stats, late, error, milliseconds}: every candidate with its line and when it came, every error a server raised, the browser's record of each local candidate, whether time ran out first, why WebRTC couldn't start, blank when it could, and how long the gathering ran
function _gather(servers) {
	return new Promise(resolve => {
		let found = {candidates: [], errors: [], stats: [], late: false, error: '', milliseconds: 0}
		let start = performance.now()
		let since = () => Math.round(performance.now() - start)//milliseconds since this gathering began
		let connection
		let done = false
		let timer = setTimeout(() => { found.late = true; finish() }, checkSeconds*second)
		async function finish() {
			if (done) return//the timer and the end of gathering can each arrive after the other
			done = true
			found.milliseconds = since()//how long the gathering ran
			clearTimeout(timer)
			if (connection) {
				found.stats = await _localStats(connection)//before closing, since a closed connection reports nothing
				connection.close()
			}
			resolve(found)
		}
		try {
			connection = new RTCPeerConnection({iceServers: [{urls: servers.map(server => `stun:${server}`)}]})
			connection.onicecandidate = event => {
				if (!event.candidate || !event.candidate.candidate) return finish()//null, or in some browsers a blank candidate first, means gathering is complete
				let line = event.candidate.candidate
				let c = _readCandidate(line) || {protocol: '', address: '', port: 0, type: ''}//a line we can't read still goes in the box as written
				found.candidates.push({...c, line, url: event.candidate.url || event.url || '', milliseconds: since()})//the STUN server it came from, in browsers that say
			}
			connection.onicecandidateerror = event => {//a server that didn't answer, or answered with an error
				found.errors.push({code: event.errorCode, text: event.errorText, url: event.url, address: event.address || '', port: event.port || 0, milliseconds: since()})
			}
			connection.createDataChannel('good')//a peer connection with nothing to send gathers nothing, so ask for a channel to give it a reason
			connection.createOffer().then(offer => connection.setLocalDescription(offer)).catch(error => { found.error = `${error.name}: ${error.message}`; finish() })//setting the offer as the local description is what starts gathering
		} catch (error) {//the bottom gate: a browser or an extension that turns WebRTC off
			found.error = `${error.name}: ${error.message}`
			finish()
		}
	})
}

//a candidate's text, the line SDP writes, like candidate:842163049 1 udp 1677729535 203.0.113.42 50000 typ srflx raddr 0.0.0.0 rport 0, read into {protocol, address, port, type}, or false for a line that isn't one
function _readCandidate(text) {
	let words = text.split(' ')
	let typ = words.indexOf('typ')
	if (words.length < 8 || typ < 0) return false
	return {protocol: words[2].toLowerCase(), address: words[4], port: Number(words[5]), type: words[typ + 1]}
}

//the browser's own record of each local candidate, from getStats, which can say more than the candidate's line, like the server a candidate came from or the kind of network it was gathered on; answers each record as name=value text
async function _localStats(connection) {
	let records = []
	try {
		let report = await connection.getStats()
		report.forEach(s => {
			if (s.type == 'local-candidate') records.push(Object.entries(s).filter(([key]) => !statsSkip.includes(key)).map(([key, value]) => `${key}=${value}`).join(' '))
		})
	} catch (error) {//the bottom gate: a browser that won't report on a connection, as a line in the box
		records.push(`${error.name}: ${error.message}`)
	}
	return records
}

//one family's reflexive candidates from a gathering, as a result whose value is the list of distinct public endpoints, in the order they arrived
function _reflexive(found, family, g) {
	let host = found.candidates.filter(c => c.type == 'host' && c.protocol == 'udp').length//across the whole gathering, since a host candidate's mDNS name doesn't say which family it's in
	let r = {kind: 'stun', family, which: g.which, asked: `STUN ${g.servers.join(' and ')}`, host}
	let mine = found.candidates.filter(c => c.type == 'srflx' && _family(c.address) == family)
	let endpoints = [...new Set(mine.map(c => _sayEndpoint(c.address, c.port)))]
	if (found.error) return {...r, success: false, outcome: found.error, milliseconds: found.milliseconds}
	if (!mine.length) return {...r, success: false, outcome: found.late ? noAnswer : `no IPv${family} candidate`, milliseconds: found.milliseconds}
	return {...r, success: true, value: endpoints, candidates: mine, milliseconds: mine[0].milliseconds}
}

//4 or 6, by whether the address has colons in it
function _family(address) {
	if (address.includes(':')) return 6
	return 4
}

//the table's rows for a run, three for each family, blank before the first run
export function connectionFacts(run) {
	let rows = []
	for (let family of [4, 6]) {
		let mine = kind => run ? run.results.filter(r => r.kind == kind && r.family == family) : []//this family's results of one kind, in the order asked
		let [reflect] = mine('reflect'), [ours, second, both] = mine('stun')
		rows.push(
			[`Public IPv${family} address`,  run ? _sayFact(reflect)                : ''],
			[`Public IPv${family} over UDP`, run ? _sayUdp(ours, reflect)           : ''],
			[`IPv${family} mapping`,         run ? _sayMapping(ours, second, both) : ''],
		)
	}
	return rows
}

//a result in the table's words: the address, no answer for a check that ran out of time, or none for one that failed
function _sayFact(r) {
	if (!r.success && r.outcome == noAnswer) return 'no answer'
	if (!r.success) return 'none'
	if (r.kind == 'stun') return r.value[0]
	return r.value
}

//the address and port our STUN server saw, and a word when its network isn't the one the reflector saw over HTTPS
function _sayUdp(ours, reflect) {
	let said = _sayFact(ours)
	if (_apart(ours, reflect)) said += `, but HTTPS came from ${reflect.value}`
	return said
}

//true when UDP and HTTPS both answered in a family and left from different networks
function _apart(ours, reflect) {
	return ours.success && reflect.success && !_sameNetwork(ours.family, ours.candidates[0].address, reflect.value)
}

//what a disagreement between UDP and HTTPS most likely means, for the sentence under the verdict; the table states the two addresses, and this says what could put them apart
const apartMeaning = {
	4: `Over IPv4, UDP and HTTPS left from different addresses, so something sends them out apart: a carrier's translator with more than one public address, or a VPN, a proxy, or a relay such as iCloud Private Relay carrying one and not the other.`,
	6: `Over IPv6, UDP and HTTPS left from different networks, so a VPN, a proxy, or a relay such as iCloud Private Relay is carrying one and not the other.`,//nothing translates IPv6, so only a different path moves a connection to another network
}

//a sentence for each family whose UDP and HTTPS left from different networks, blank when every family agrees
export function sayApart(run) {
	let sentences = []
	for (let family of [4, 6]) {
		let reflect = run.results.find(r => r.kind == 'reflect' && r.family == family)
		let ours = run.results.find(r => r.kind == 'stun' && r.family == family && r.which == 'ours')
		if (_apart(ours, reflect)) sentences.push(apartMeaning[family])
	}
	return sentences.join(' ')
}

//whether the router keeps one public port for every destination, by count: our server alone gets one candidate from each socket the browser gathered from, and both servers asked from the same sockets get the same count when every socket kept its port, and twice as many when each got a new one
function _sayMapping(ours, second, both) {
	if (!ours.success || !second.success || !both.success) return 'unknown'//Cloudflare's has to answer on its own, or the same count could mean it never answered at all
	let sockets = ours.value.length
	if (both.value.length == sockets)   return 'endpoint-independent'
	if (both.value.length == 2*sockets) return 'endpoint-dependent'
	return 'unknown'
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

//the dual-stack verdict from the two reflectors: which families reach the internet from here
export function sayVerdict(run) {
	let [v4, v6] = run.results.filter(r => r.kind == 'reflect').map(r => r.success)
	if (v4 && v6) return 'You have dual-stack, both IPv4 and IPv6.'
	if (v4) return 'You have IPv4 only.'
	if (v6) return 'You have IPv6 only.'
	return 'Neither family reached us. Something is blocking the connection.'
}

//the box's lines for one run: when, and in which browser, then a line per check with what it asked, how long it took, and what came back
export function sayRun(run) {
	let width = Math.max(...run.results.map(r => r.asked.length))//the asked column as wide as its longest entry, so the times line up under each other
	let lines = [`${new Date(run.moment).toISOString().replace(/\.\d+Z$/, 'Z')}, good.ftorrent.com`, run.browser]
	for (let r of run.results) lines.push(`${r.asked.padEnd(width)}  ${`${r.milliseconds} ms`.padStart(7)}  ${_sayAnswer(r)}`)//the answer last, since an error's text can run long
	for (let g of run.gatherings) lines.push('', ..._sayGathering(g))//then everything each peer connection reported
	return lines.join('\n')
}

//what came back, in the box's words: the address or the error's text, and for STUN each reflexive candidate the browser handed back, then how many host candidates the gathering made
function _sayAnswer(r) {
	if (r.kind == 'reflect' && r.success) return r.value
	if (r.kind == 'reflect') return r.outcome
	let said = r.outcome
	if (r.success) said = `IPv${r.family} ${r.candidates.map(c => _sayEndpoint(c.address, c.port)).join(', ')}`
	return `${said}; ${r.host} host`
}

//one peer connection in full: what it asked and how it ended, then each candidate line as the browser wrote it, when it came, and the server it names, each error a server raised, and the browser's record of each local candidate
function _sayGathering(g) {
	let ending = `${g.candidates.length} candidates in ${g.milliseconds} ms`
	if (g.late) ending += `, stopped at ${checkSeconds} seconds`
	if (g.error) ending = `couldn't start, ${g.error}`
	let lines = [`Gathering from ${g.servers.join(' and ')}: ${ending}`]
	let at = milliseconds => `${milliseconds} ms`.padStart(7)//the time column, lined up like the checks above
	for (let c of g.candidates) {
		let line = `${at(c.milliseconds)}  ${c.line}`
		if (c.url) line += `, from ${c.url}`
		lines.push(line)
	}
	for (let e of g.errors) {
		let line = `${at(e.milliseconds)}  error ${e.code} ${e.text}, from ${e.url}`
		if (e.address) line += `, local ${_sayEndpoint(e.address, e.port)}`
		lines.push(line)
	}
	for (let s of g.stats) lines.push(`  stats  ${s}`)
	return lines
}
