import {Time, sayTick} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go

/*
What a web page can learn about the connection it runs on, asked from inside the browser's sandbox. The desktop client's Connection tab asks many of the same questions from a real socket and can tell more; this page asks what any page can, which is what a browser peer has to work with.

For each family, IPv4 and IPv6, the page asks two things. A reflector at ip4.ftorrent.com or ip6.ftorrent.com answers the address the request arrived from; each name has an address record in one family only, so the browser, which picks a family for itself, can only reach it over that one, and the pair of answers is which families reach the internet from here, over TCP. moon, our second server, at another place on the internet, runs the same reflector at ip4.moon.ftorrent.com and ip6.moon.ftorrent.com, and the page asks it too: a family that reaches either server reaches the internet, and a family that reaches one and not the other has found trouble on our side rather than the visitor's, which the page says.

And STUN, over UDP, which is what a peer connection travels on. A page has no socket of its own, but WebRTC gives it one at a remove: the page makes a peer connection, names STUN servers, and asks for a data channel, which starts ICE (Interactive Connectivity Establishment), the dance by which two WebRTC peers find a path to each other. The page runs only its first step, gathering: the browser opens a socket on each network interface, sometimes two or three on one address, as Safari does, sends each server a binding request from each, and hands back candidates, the addresses a peer might reach this browser at. A host candidate is a local address, which every mainstream browser now writes as a made-up mDNS name ending in .local, so a page can't read its own local addresses; a server-reflexive candidate is what a STUN server saw, the public address and port, with its local address and port written 0.0.0.0 or :: and 0 for the same reason. The WebRTC project's Trickle ICE sample is the page that lists these candidates as they arrive, and browser NAT checkers build on the same stream.

The page makes four peer connections at once. The first names our server alone, and its reflexive candidates are this browser's public address and port over UDP, which should match what the reflector saw over HTTPS; when it doesn't, something sends the two out apart: over IPv4, a carrier's translator with more than one public address of its own, or in either family a VPN, a proxy, or a relay such as iCloud Private Relay carrying one path and not the other. The second names moon's alone and the third Cloudflare's alone, which shows each answers from here. The fourth names all three, and the browser asks all three from the same sockets. A router keys the mapping it makes on the inside address and port, so when every server sees the same public port the candidates are the same, and the browser, which drops a reflexive candidate that repeats one it already has, hands back one. The page can't see how many sockets the browser gathered from, but asking one server, each socket answers one candidate, so the first connection's count in a family is the number of sockets. When the fourth's count is the same, every socket kept one public port for every server, endpoint-independent mapping, the kind a hole punch works through; when it's one for each server that answered alone, three times the sockets with all three answering, each socket got a new port for each server, endpoint-dependent, the symmetric case, where it fails. The desktop client asks from one socket it chose itself, so it has no counting to do.

And the hole punch, the whole of ICE rather than its first step: whether this browser and a peer behind a NAT of its own can reach each other directly. moon runs three such peers, each in a network namespace of its own. Two sit behind NATs moon makes with Linux's own translation: the cone, which keeps a socket's port for every destination, and the symmetric, which picks a fresh port for each. The third has a public IPv6 address behind a stateful firewall that translates nothing, the shape of a home router's IPv6 side. For each, the page makes a peer connection with a data channel, gathers every candidate through our STUN server, and posts the offer to moon over HTTPS; moon hands it to the peer, which gathers through the same server and answers, and moon passes the answer back. That one exchange is all the signaling, the shape WHIP standardized, since the peer is always there to answer. Then both sides run ICE: each sends STUN binding requests from its candidates to the other's, a NAT or a firewall lets them in only once its socket has sent toward where they come from, and the first pair answered both ways carries DTLS and the data channel over it. When the channel opens, the peer sends one message naming the pair it chose, and the page reads its own side of the pair from getStats. moon keeps the offer's public candidates in the peer's family, this browser's addresses as STUN saw them, wherever they are, since a phone's carrier can send its TCP and its UDP out from different addresses, and says what it dropped; ICE's checks are STUN requests signed with the browser's own ICE password, which nothing but that browser's WebRTC answers, and the peer gives up after eight seconds. The page punches the two NATs over IPv4 and the firewall over IPv6, all three at once. Through the cone and not the symmetric is the common case, the hole punch demonstrated rather than inferred from STUN; through neither is a NAT here that changes the port for each destination, or a network or extension that holds WebRTC back; and through both is a NAT here that lets in a packet from an address it has sent to at any port.

The box keeps more than the verdicts need: every candidate line the browser wrote, every error a STUN server's silence raised, and the browser's own record of each local candidate, from getStats. What one browser writes and another leaves out is itself a finding at this stage, and it all fits in one paste.

Every check gets checkSeconds, the same four the desktop client gives its own: a fetch over HTTPS is four round trips and a STUN exchange one, so four seconds leaves room for a satellite link and a lost packet, and only a path that silently drops what it's sent waits that long.
*/

const checkSeconds = 4//how long each check has to hear back, the essay above says why
const noAnswer = `no answer in ${checkSeconds} seconds`
const reflectors = [//the two reflectors in each family, ours and moon's, at the names with one address record each
	{which: 'ours', family: 4, url: 'https://ip4.ftorrent.com/'},
	{which: 'ours', family: 6, url: 'https://ip6.ftorrent.com/'},
	{which: 'moon', family: 4, url: 'https://ip4.moon.ftorrent.com/'},
	{which: 'moon', family: 6, url: 'https://ip6.moon.ftorrent.com/'},
]
const stunOurs       = 'stun.ftorrent.com:3478'
const stunMoon       = 'moon.ftorrent.com:3478'//moon's, our second server, at another place on the internet
const stunCloudflare = 'stun.cloudflare.com:3478'//Cloudflare's, on a network that is neither of ours
const gatherings = [//the four peer connections, by what each asks
	{which: 'ours',       servers: [stunOurs]},
	{which: 'moon',       servers: [stunMoon]},
	{which: 'cloudflare', servers: [stunCloudflare]},
	{which: 'all',        servers: [stunOurs, stunMoon, stunCloudflare]},
]
const statsSkip = ['id', 'timestamp', 'type', 'transportId', 'isRemote']//the bookkeeping on every getStats record, the same on each, which the box leaves out
const punches = [['cone', 4], ['symmetric', 4], ['firewall', 6]]//moon's three namespaces, each with a peer in it, by the names moon's offer takes, and the family each punches in: its two NATs translate IPv4, and its firewall filters IPv6 and translates nothing
const punchNames = {cone: `moon's cone NAT`, symmetric: `moon's symmetric NAT`, firewall: `moon's IPv6 firewall`}//each namespace in the row's words
const offerSeconds = checkSeconds + 3//moon's peer gathers for up to three seconds before it answers, on top of the request's own time
const punchSeconds = 6//after the answer, how long ICE, DTLS, and the data channel have to open and the peer's message to arrive, which on a link with a long round trip is six round trips or so

//just enough to tell an address of each family from anything else a server could answer
const looksLike = {
	4: /^\d{1,3}(\.\d{1,3}){3}$/,
	6: /^[0-9a-f]*:[0-9a-f:.]*$/i,
}

//ask everything once, and answer the run: when it started, the browser it ran in, each result in the order the box lists them, and each gathering in full
export async function connectionRun() {
	let moment = Date.now()
	let [reflected, found, punched] = await Promise.all([//all at once; Promise.all keeps the order it was given, whatever order they finish in
		Promise.all(reflectors.map(_reflect)),
		Promise.all(gatherings.map(g => _gather(g.servers))),
		Promise.all(punches.map(_punch)),
	])
	let results = [...reflected]
	gatherings.forEach((g, i) => results.push(_reflexive(found[i], 4, g), _reflexive(found[i], 6, g)))
	results.push(...punched)
	return {moment, browser: navigator.userAgent, results, gatherings: gatherings.map((g, i) => ({...g, ...found[i]}))}
}

//one hole punch over WebRTC, to the peer behind one of moon's NATs: a peer connection with a data channel, every candidate gathered through our STUN server, the offer posted to moon, the peer's answer set, and then ICE between the two until the peer's message arrives over the channel or punchSeconds pass. Answers a result for the box, its value what the punch saw: this browser's reflexive candidates, what moon kept and dropped, the peer's candidates, its message, the pair this browser says carried the channel, and ICE's state at the end
async function _punch([nat, family]) {
	let start = performance.now()
	let since = () => Math.round(performance.now() - start)
	let url = `https://ip${family}.moon.ftorrent.com/offer?nat=${nat}`
	let r = {kind: 'punch', family, nat, asked: `WebRTC to moon's ${nat} peer`}
	let v = {mine: [], kept: [], dropped: 0, peer: [], peerKept: null, message: null, opened: 0, pair: '', state: ''}
	let connection
	try {
		connection = new RTCPeerConnection({iceServers: [{urls: `stun:${stunOurs}`}]})
		let channel = connection.createDataChannel('punch')
		let message = new Promise(resolve => { channel.onmessage = event => resolve(event.data) })//the peer's one message, which comes once the channel is open both ways
		let gathered = new Promise(resolve => { connection.onicecandidate = event => { if (!event.candidate || !event.candidate.candidate) resolve() } })//null, or a blank candidate, means gathering is complete
		await connection.setLocalDescription(await connection.createOffer())//which starts gathering
		await Promise.race([gathered, _wait(checkSeconds)])
		v.mine = _candidates(connection.localDescription.sdp).filter(c => c.type == 'srflx' && _family(c.address) == family).map(c => _sayEndpoint(c.address, c.port))
		let response = await fetch(url, {method: 'POST', body: connection.localDescription.sdp, headers: {'Content-Type': 'text/plain'}, signal: AbortSignal.timeout(offerSeconds*Time.second)})//text/plain, which a browser sends across origins without a preflight request first
		if (!response.ok) throw new Error(`moon answered ${response.status}, ${(await response.text()).trim()}`)
		let a = await response.json()
		let peer = _candidates(a.answer)
		let reflexive = peer.find(c => c.type == 'srflx')//the peer's public address and port, with the port it bound as its related port
		Object.assign(v, {caller: a.caller, kept: a.kept, dropped: a.dropped, peer: peer.map(c => `${c.type} ${_sayEndpoint(c.address, c.port)}`), peerKept: reflexive ? reflexive.port == reflexive.related : null})
		await connection.setRemoteDescription({type: 'answer', sdp: a.answer})
		let text = await Promise.race([message, _wait(punchSeconds)])
		if (text) { v.message = JSON.parse(text); v.opened = since() }
		v.pair = await _selectedPair(connection)
		v.state = connection.iceConnectionState
		connection.close()
		return {...r, success: true, value: v, milliseconds: since()}
	} catch (error) {//the bottom gate: WebRTC turned off, moon refusing or out of reach, or an answer that wouldn't set, as an ordinary answer
		if (connection) connection.close()
		return {...r, success: false, outcome: error.name == 'TimeoutError' ? `no answer from moon in ${offerSeconds} seconds` : `${error.name}: ${error.message}`, value: v, milliseconds: since()}
	}
}

//a promise that resolves to nothing after some seconds, for racing against one that might never resolve
function _wait(seconds) { return new Promise(resolve => setTimeout(resolve, seconds*Time.second)) }

//the candidates in a description, read from its a=candidate lines
function _candidates(sdp) {
	return sdp.split(/\r?\n/).filter(line => line.startsWith('a=candidate:')).map(line => _readCandidate(line.slice(2))).filter(Boolean)
}

//the candidate pair this browser says carries the connection, as the type and endpoint of each end, like srflx 203.0.113.42:52814 to srflx 198.51.100.7:46210, or blank when there's none. Chrome and Safari name it on the transport record, and Firefox marks the pair itself selected
async function _selectedPair(connection) {
	let stats = await connection.getStats()
	let all = [...stats.values()]
	let transport = all.find(s => s.type == 'transport' && s.selectedCandidatePairId)
	let pair = transport ? stats.get(transport.selectedCandidatePairId) : all.find(s => s.type == 'candidate-pair' && s.selected)
	if (!pair) return ''
	let end = id => {
		let c = stats.get(id)
		if (!c) return ''
		let address = c.address || c.ip//address in the standard, ip in browsers from before it
		return address ? `${c.candidateType} ${_sayEndpoint(address, c.port)}` : `${c.candidateType} port ${c.port}, its address hidden by the browser`//a browser can leave a host candidate's address out of its stats, its own so a page can't learn it, and in Safari the far end's too
	}
	return `${end(pair.localCandidateId)} to ${end(pair.remoteCandidateId)}`
}

//fetch a reflector, timed, as a result: {kind, which, family, asked, success, value or outcome, milliseconds}
async function _reflect({which, family, url}) {
	let start = performance.now()
	let v
	try {
		let response = await fetch(url, {cache: 'no-store', signal: AbortSignal.timeout(checkSeconds*Time.second)})
		let text = (await response.text()).trim()//the reflector ends its answer with a newline
		if (!response.ok) v = {success: false, outcome: `the reflector answered ${response.status}`}
		else if (looksLike[family].test(text)) v = {success: true, value: text}
		else v = {success: false, outcome: `not an IPv${family} address: ${text.slice(0, 60)}`}
	} catch (error) {//the bottom gate: no route, a refused connection, or the timeout's abort, as an ordinary answer
		v = {success: false, outcome: error.name == 'TimeoutError' ? noAnswer : `${error.name}: ${error.message}`}
	}
	return {kind: 'reflect', which, family, asked: `GET ${url}`, ...v, milliseconds: Math.round(performance.now() - start)}
}

//make a peer connection naming these STUN servers, and collect everything its browser reports until gathering finishes or checkSeconds pass; answers {candidates, errors, stats, late, error, milliseconds}: every candidate with its line and when it came, every error a server raised, the browser's record of each local candidate, whether time ran out first, why WebRTC couldn't start, blank when it could, and how long the gathering ran
function _gather(servers) {
	return new Promise(resolve => {
		let found = {candidates: [], errors: [], stats: [], late: false, error: '', milliseconds: 0}
		let start = performance.now()
		let since = () => Math.round(performance.now() - start)//milliseconds since this gathering began
		let connection
		let done = false
		let timer = setTimeout(() => { found.late = true; finish() }, checkSeconds*Time.second)
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

//a candidate's text, the line SDP writes, like candidate:842163049 1 udp 1677729535 203.0.113.42 50000 typ srflx raddr 0.0.0.0 rport 0, read into {protocol, address, port, type, related}, or false for a line that isn't one; related is the rport, the local port a reflexive candidate was gathered from, which browsers write as 0 and moon's peer writes as it is
function _readCandidate(text) {
	let words = text.split(' ')
	let typ = words.indexOf('typ')
	if (words.length < 8 || typ < 0) return false
	let rport = words.indexOf('rport')
	return {protocol: words[2].toLowerCase(), address: words[4], port: Number(words[5]), type: words[typ + 1], related: rport > 0 ? Number(words[rport + 1]) : 0}
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

//the table's rows for a run, four for each family, blank before the first run
export function connectionFacts(run) {
	let rows = []
	for (let family of [4, 6]) {
		let mine = kind => run ? run.results.filter(r => r.kind == kind && r.family == family) : []//this family's results of one kind, in the order asked
		let [reflect, moon] = mine('reflect'), stuns = mine('stun'), [ours] = stuns
		rows.push(
			[`Public IPv${family} address`,  run ? _sayPublic(reflect, moon) : ''],
			[`Public IPv${family} over UDP`, run ? _sayUdp(ours, reflect)    : ''],
			[`IPv${family} mapping`,         run ? _sayMapping(stuns)        : ''],
		)
		rows.push([`IPv${family} hole punch`, run ? _sayPunch(run, family) : ''])
	}
	return rows
}

//one family's hole punch as the page saw it, _readPunched's sentence for each of moon's namespaces in that family; what the punches mean is in the essay at the top, not the row
function _sayPunch(run, family) {
	return punches.filter(([nat, f]) => f == family).map(([nat]) => _readPunched(run.results.find(r => r.kind == 'punch' && r.nat == nat)).said).join(' ')
}

//one punch as a sentence: moon's NAT or firewall, with whether it kept the port of moon's peer behind it, the port the peer's answer says it bound against the one STUN saw; then whether the data channel opened, and how soon. Or why the punch couldn't run, as when the offer held no candidate in that family for moon to keep
function _readPunched(r) {
	let v = r.value
	let couldnt = why => ({said: `The punch to moon's peer behind ${punchNames[r.nat]} couldn't run: ${why}.`})
	if (!r.success) return couldnt(r.outcome)
	if (!v.kept.length) return couldnt(`this browser's offer held no public IPv${r.family} candidate`)
	let head = v.peerKept == null ? `moon's peer sits behind ${punchNames[r.nat]}` : `${punchNames[r.nat]} ${v.peerKept ? 'kept' : 'changed'} the port of moon's peer behind it`//moon's peer named as moon's every time, so it can't be read as this browser
	if (v.message) return {said: `${head}, and a data channel opened between that peer and this browser in ${v.opened} ms.`}
	return {said: `${head}, and no data channel opened between that peer and this browser in ${punchSeconds} seconds.`}
}

//a result in the table's words: the address, no answer for a check that ran out of time, or none for one that failed
function _sayFact(r) {
	if (!r.success && r.outcome == noAnswer) return 'no answer'
	if (!r.success) return 'none'
	if (r.kind == 'stun') return r.value[0]
	return r.value
}

//the address our reflector saw, and moon's beside it when moon saw another network or only one of the two answered
function _sayPublic(reflect, moon) {
	if (reflect.success && moon.success && !_sameNetwork(reflect.family, reflect.value, moon.value)) return `${reflect.value}, but moon saw ${moon.value}`
	if (!reflect.success && moon.success) return `${moon.value} by moon, ${_sayFact(reflect)} from ours`
	if (reflect.success && !moon.success) return `${reflect.value}, ${_sayFact(moon)} from moon`
	return _sayFact(reflect)//both saw the same network, or neither answered
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
		let reflect = run.results.find(r => r.kind == 'reflect' && r.family == family && r.which == 'ours')
		let ours = run.results.find(r => r.kind == 'stun' && r.family == family && r.which == 'ours')
		if (_apart(ours, reflect)) sentences.push(apartMeaning[family])
	}
	return sentences.join(' ')
}

//whether the router keeps one public port for every destination, by count: our server alone gets one candidate from each socket the browser gathered from, and the servers asked together from the same sockets get the same count when every socket kept its port, and one for each server for each socket when each got a new one. A server counts only when it answered alone, or the same count could mean it never answered at all, and the verdict says how many did when one didn't
function _sayMapping(stuns) {
	let ours = stuns.find(r => r.which == 'ours'), all = stuns.find(r => r.which == 'all')
	let alone = stuns.filter(r => r.which != 'all')
	let answered = alone.filter(r => r.success).length//ours among them
	if (!ours.success || !all.success || answered < 2) return 'unknown'
	let sockets = ours.value.length
	let said = 'unknown'
	if (all.value.length == sockets) said = 'endpoint-independent'
	else if (all.value.length == answered*sockets) said = 'endpoint-dependent'
	if (said != 'unknown' && answered < alone.length) said += `, by ${answered} of ${alone.length} servers`
	return said
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

//the dual-stack verdict from the reflectors: which families reach the internet from here, by either server answering
export function sayVerdict(run) {
	let [v4, v6] = [4, 6].map(family => run.results.some(r => r.kind == 'reflect' && r.family == family && r.success))
	if (v4 && v6) return 'You have dual-stack, both IPv4 and IPv6.'
	if (v4) return 'You have IPv4 only.'
	if (v6) return 'You have IPv6 only.'
	return 'Neither family reached us. Something is blocking the connection.'
}

//a sentence for each family where one of our two servers answered and the other didn't, which puts the trouble on our side rather than the visitor's; blank when both or neither answered in every family
export function sayServers(run) {
	let sentences = []
	for (let family of [4, 6]) {
		let [reflect, moon] = run.results.filter(r => r.kind == 'reflect' && r.family == family)
		if (moon.success && !reflect.success) sentences.push(`Over IPv${family}, moon answered and our main server didn't, so the trouble is ours, not your connection's.`)
		if (reflect.success && !moon.success) sentences.push(`Over IPv${family}, our main server answered and moon didn't, so the trouble is moon's, not your connection's.`)
	}
	return sentences.join(' ')
}

//the box's lines for one run: when, and in which browser, then a line per check with what it asked, how long it took, and what came back
export function sayRun(run) {
	let width = Math.max(...run.results.map(r => r.asked.length))//the asked column as wide as its longest entry, so the times line up under each other
	let lines = [`${_sayMoment(run.moment)}, good.ftorrent.com`, run.browser]
	for (let r of run.results) {
		lines.push(`${r.asked.padEnd(width)}  ${`${r.milliseconds} ms`.padStart(7)}  ${_sayAnswer(r)}`)//the answer last, since an error's text can run long
		if (r.kind == 'punch') lines.push(..._sayPunchLines(r.value))
	}
	for (let g of run.gatherings) lines.push('', ..._sayGathering(g))//then everything each peer connection reported
	return lines.join('\n')
}

//a tick as the box writes it, like 2026oct23 Fri 12:53p 49.123s, on the visitor's own clock: the minute as a person says it, with a and p for am and pm, then the seconds to the millisecond; the s at the end isn't a tag, since ergo has none for a lone s, so it's written as it is
function _sayMoment(tick) {
	let local = tick - new Date(tick).getTimezoneOffset()*Time.minute//shifted by the browser's offset from UTC at that moment, daylight saving included, since sayTick writes every tick as it falls in UTC
	return sayTick(local, 'YYYYmmmD Ddd h12:mma ss.ttts')
}

//what came back, in the box's words: the address or the error's text, and for STUN each reflexive candidate the browser handed back, then how many host candidates the gathering made
function _sayAnswer(r) {
	if (r.kind == 'reflect' && r.success) return r.value
	if (r.kind == 'reflect') return r.outcome
	if (r.kind == 'punch') return _readPunched(r).said
	let said = r.outcome
	if (r.success) said = `IPv${r.family} ${r.candidates.map(c => _sayEndpoint(c.address, c.port)).join(', ')}`
	return `${said}; ${r.host} host`
}

//the lines under a punch in the box: this browser's reflexive candidates in the offer, the ones moon kept and how many it dropped, the peer's candidates in its answer, the pair this browser says carried the channel, ICE's state at the end, and the peer's message as it came
function _sayPunchLines(v) {
	let lines = [`  offer: reflexive ${v.mine.join(', ') || 'none'}; moon kept ${v.kept.length}, dropped ${v.dropped}`]
	for (let k of v.kept) lines.push(`    kept ${k}`)
	lines.push(`  answer: ${v.peer.join(', ') || 'no candidates'}`)
	lines.push(`  pair: ${v.pair || 'none'}; ICE ${v.state || 'not started'}`)
	if (v.message) lines.push(`  message: ${JSON.stringify(v.message)}`)
	return lines
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
