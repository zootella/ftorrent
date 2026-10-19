import {getVersion} from '@tauri-apps/api/app'
import {brandName, urlHome} from './brand.js'
import {platformName} from './settings.js'
import {Time, sayMoment} from './time.js'
import {netGet, netLocal, netStun, netResolve, netInterfaces} from './net.js'
import {processOutput} from './process.js'

/*
The Connection tab's checks: what this machine and the network say about its connection, asked when the user presses Check and at no other time, so opening the tab asks nothing of anyone. good.ftorrent.com asks some of the same questions from a browser; that page is separate code, and what the two share is the servers they ask and what each server's answer means.

For each family, IPv4 and IPv6, a run asks three things. The system says which of the machine's addresses it would send from to reach an address beyond the local network, which takes no packet at all. A reflector at ip4.ftorrent.com or ip6.ftorrent.com answers the address the request arrived from; each name has an address record of one family only, so the request can only travel over that family, and the pair of answers is which families reach the internet from here. Behind a home router the two IPv4 answers differ, since the router translates; on IPv6 they're usually the same address, since nothing does.

And STUN, over UDP, which is what a peer connection travels on: our server and Cloudflare's, asked at once from one local port, the way Tailscale's netcheck asks its servers, each answering the address and port it saw the request arrive from. A router keys the mapping it makes on the inside address and port, so when both servers see the same public port, it keeps one mapping for every destination, endpoint-independent mapping, which hole punching needs; a new port for each server is endpoint-dependent, the symmetric case, where hole punching fails. The address STUN saw should be the one the reflector saw, and when it isn't, something carries one path and not the other, a VPN, a proxy, or a relay. On IPv6 these comparisons are of the /64, the network, since a machine can send from more than one of its temporary addresses, and a Mac sometimes from one it has already deprecated.

And the machine itself, which takes no packet beyond a name lookup. net_interfaces lists every interface with every address on it, the ones a person finds in Settings and wonders about along with the rest, and each address gets a few words from the ranges the standards set aside: private, link-local, global, and the others. That one call says less than a person needs, so on a Mac and Linux the panel also runs the tools the system ships: ifconfig and ip for what kind each IPv6 address is, temporary, stable, from DHCPv6, or deprecated, and how long it lasts; route and ip route for the gateway in each family; and on a Mac networksetup for the name Settings gives each interface, like Wi-Fi. Windows says all of it through net_interfaces itself, which is just as well, since its tools print in the system's language. The interface carrying the default route is the one holding the address the system says it sends from, and the address the reflector saw over IPv6 is usually one of the machine's own, a temporary one.

The resolver answers three names. ipv4only.arpa has only IPv4 addresses by definition, RFC 7050, so an IPv6 answer for it means the network makes up IPv6 addresses for names that have none, DNS64, so an IPv6-only network can reach them through a translator, NAT64; where the IPv4 address sits inside the answer names the translator's prefix. ip4.ftorrent.com and ip6.ftorrent.com show the single-family records the reflector checks rest on. And an IPv4 address in the shared address space, 100.64.0.0/10, RFC 6598, on the interface carrying the default route says a carrier translates above the router; on any other interface it's a mesh VPN's, like Tailscale's, which borrows the range.

A check that asks once and waits for one answer gets checkSeconds to hear back. One round trip is tens of milliseconds, and a fetch over https is four, the name, the connection, the encryption, and the request, so around 200 milliseconds on a typical connection and around 600 across an ocean. The slowest ordinary connection, a satellite in geostationary orbit, has a round trip of 600 milliseconds, which puts a fetch near two and a half seconds, and a lost packet is sent again after a second. Four seconds covers all of those, and only a path that silently drops what it's sent waits that long, since no route, or a name with no address, fails at once whatever the limit. A check that waits longer by design gets its own time when it arrives. So a check ends one of three ways: an answer, in milliseconds; a failure that came back with its reason, like no route; or no answer in checkSeconds.

A run asks every check at once and waits for the last, so it takes as long as its slowest check, at most checkSeconds. Then, for each STUN server whose name led somewhere, it asks the system which of the machine's addresses it would send from toward that server, which takes no packet, and over IPv6 can be a different one of the machine's addresses for each. That is the system's word and not a measurement: a Mac has sent HTTPS and STUN from a deprecated temporary address while saying, a moment later, that it would send from the current one, so the box gives both, and the address the server saw is on the line above. The box lists the checks in the same order every time, whatever order they finish in, so two runs line up line for line, and a family's one STUN call shows as a line for each server it asked, with everything net_stun saw of that server on the lines under it: where the request went, the address the system says it would send from, where the answer came from, the answer in hex, and each attribute in it by name with a few words on what it carries. An error is an answer like any other: the box shows its text, since a user on this tab is under the hood. The box keeps every run since the tab opened, oldest first, each starting with a line that says when it ran, which build, and on which platform, so a box pasted into a message or an issue says where its lines came from, and ends with every interface and every address on it, each with a few words on what it is.
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

//the system's own tools, run on a Mac and Linux for what net_interfaces can't say there: each IPv6 address's kind and lifetimes, the gateway in each family, and on a Mac the name Settings gives each interface; Windows says all of it through net_interfaces
const tools = {
	macOS: [
		{tool: 'addresses', program: '/sbin/ifconfig', args: ['-L']},//-L adds each IPv6 address's lifetimes
		{tool: 'gateway4', program: '/sbin/route', args: ['-n', 'get', 'default']},
		{tool: 'gateway6', program: '/sbin/route', args: ['-n', 'get', '-inet6', 'default']},
		{tool: 'ports', program: '/usr/sbin/networksetup', args: ['-listallhardwareports']},
	],
	Linux: [
		{tool: 'addresses', program: 'ip', args: ['-o', 'address']},//-o puts each address on one line
		{tool: 'gateway4', program: 'ip', args: ['-4', 'route', 'show', 'default']},
		{tool: 'gateway6', program: 'ip', args: ['-6', 'route', 'show', 'default']},
	],
	Windows: [],
}[platformName]

//the names the resolver is asked: one with only IPv4 addresses by definition, and our two single-family names
const names = ['ipv4only.arpa', `ip4.${urlHome}`, `ip6.${urlHome}`]

//the checks for one run, in the order the box lists them: for each family, its local address, its public address over HTTPS, and STUN; then the machine's interfaces, the tools that say more about them, and the names
function _checks() {
	let checks = []
	for (let family of [4, 6]) checks.push(_route(family), _reflect(family), _stun(family))
	checks.push(_interfaces(), ...tools.map(_tool), ...names.map(_resolve))
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

//a check of the machine's interfaces and every address on each
function _interfaces() {
	return {kind: 'interfaces', asked: 'list interfaces', ask: () => netInterfaces(), read: list => ({success: true, value: list})}
}

//a check that runs one of the system's tools and keeps what it printed for the readers below; a tool that exits with an error fails with the first line of what it said
function _tool(t) {
	let read = output => output.code == 0 ? {success: true, value: output.out} : {success: false, outcome: [`exit ${output.code}`, (output.err || output.out).trim().split('\n')[0]].filter(Boolean).join(', ')}
	return {kind: 'tool', tool: t.tool, asked: [t.program, ...t.args].join(' '), ask: () => processOutput(t.program, t.args), read}
}

//a check of every address the system's resolver answers for a name
function _resolve(name) {
	return {kind: 'resolve', name, asked: `resolve ${name}`, ask: () => netResolve(name), read: list => ({success: true, value: list})}
}

//an answer that should be an address of this family
function _readAddress(family, text) {
	if (looksLike[family].test(text)) return {success: true, value: text}
	return {success: false, outcome: `not an IPv${family} address: ${text.slice(0, 60)}`}
}

//ask every check once, and answer the run: when it started, this copy's version, each check's result in the order the box lists them, and the machine's interfaces put together from them
export async function connectionRun() {
	let moment = Date.now()
	let version = await getVersion()
	let results = (await Promise.all(_checks().map(_ask))).flatMap(_perServer)//all at once; Promise.all keeps the order it was given, whatever order they finish in
	await Promise.all(results.filter(r => r.binding?.to).map(async r => { r.source = await _source(r.binding.to) }))//then the address this machine sends from toward each STUN server whose name led somewhere
	return {moment, version, results, machine: _machine(results)}
}

//the address the system would send from toward a server's address and port, like [2606:4700:49::]:3478, as {success, value or outcome}
async function _source(to) {
	let address = to.replace(/:\d+$/, '').replace(/^\[|\]$/g, '')//the address alone, without the port or an IPv6 address's brackets
	try {
		return {success: true, value: await netLocal(address)}
	} catch (error) {//the bottom gate, as in _ask
		return {success: false, outcome: String(error)}
	}
}

//one check, timed, as what the check says about itself, like its kind and what it asked, then {success, value or outcome, milliseconds}
async function _ask(check) {
	let {ask, read, ...about} = check
	let start = performance.now()
	let v
	try {
		let answer = await Promise.race([ask(), _late()])
		if (answer === late) v = {success: false, outcome: noAnswer}//strict, since the answer may be text, an object, or this
		else v = read(answer)
	} catch (error) {//the bottom gate: rust's refusal becomes an ordinary answer, with its text kept for the box
		v = {success: false, outcome: String(error)}
	}
	let milliseconds = Math.round(performance.now() - start)
	if (!v.success && milliseconds >= checkSeconds*Time.second) v.outcome = noAnswer//rust gave up at checkSeconds, whatever words it chose for it; the clock here started first, so it reads at least as long
	return {...about, ...v, milliseconds}
}

//a STUN result as a result for each server, since one call asks them all, with each server's own time from net_stun, which is the network's; any other result as it is
function _perServer(r) {
	if (r.kind != 'stun') return [r]
	return stunServers.map((server, i) => {
		let asked = `STUN ${server} from port ${r.port}`
		if (!r.success) return {...r, asked}//the call itself failed or ran out of time, for every server at once
		let b = {...r.value.servers[i], local: r.value.local}//this server's binding, with the local port every request went from, kept as binding whether or not the server answered, for the lines under it in the box
		if (!b.error) return {kind: 'stun', family: r.family, asked, success: true, value: b, binding: b, milliseconds: b.milliseconds}
		return {kind: 'stun', family: r.family, asked, success: false, outcome: b.milliseconds >= checkSeconds*Time.second ? noAnswer : b.error, binding: b, milliseconds: b.milliseconds}//the same three endings as every other check
	})
}

//what a check's answer is when it hasn't come a quarter second past checkSeconds: rust gives up at checkSeconds itself, so this only ever wins when a call stalls before its own clock starts, as a stuck name lookup would, and it keeps the run from waiting on it
const late = Symbol('late')
function _late() { return new Promise(resolve => setTimeout(() => resolve(late), checkSeconds*Time.second + Time.second/4)) }

//the table's rows for a run, five for each family and three about the machine, blank before the first run
export function connectionFacts(run) {
	let rows = []
	for (let family of [4, 6]) {
		let mine = kind => run ? run.results.filter(r => r.kind == kind && r.family == family) : []//this family's results of one kind, in the order asked, none before the first run
		let [route] = mine('route'), [reflect] = mine('reflect'), stuns = mine('stun')
		rows.push(
			[`Local IPv${family} address`,   run ? _sayLocal(route, run.machine)    : ''],
			[`Public IPv${family} address`,  run ? _sayPublic(reflect, run.machine) : ''],
			[`Public IPv${family} over UDP`, run ? _sayUdp(stuns, reflect)          : ''],
			[`IPv${family} mapping`,         run ? _sayMapping(stuns)               : ''],
			[`IPv${family} gateway`,         run ? _sayGateway(family, run.machine) : ''],
		)
	}
	rows.push(
		['Interfaces',           run ? _sayCount(run.machine)  : ''],
		['DNS64',                run ? _sayDns64(run.results)  : ''],
		['Shared address space', run ? _sayShared(run.machine) : ''],
	)
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

//an IPv6 address's first four groups, its /64, as text two spellings of one network share
function _sixtyFour(address) {
	return _groups(address).slice(0, 4).join(':')
}

//an IPv6 address as its eight groups of sixteen bits, as numbers, with any :: written out, so two spellings of one address come out the same
function _groups(address) {
	let dotted = address.match(/^(.*:)(\d+)\.(\d+)\.(\d+)\.(\d+)$/)//an IPv4 address written at the end, like 64:ff9b::192.0.0.170, which is two groups
	if (dotted) address = `${dotted[1]}${(dotted[2] << 8 | dotted[3]).toString(16)}:${(dotted[4] << 8 | dotted[5]).toString(16)}`
	let [head, tail] = address.includes('::') ? address.split('::').map(side => side ? side.split(':') : []) : [address.split(':'), []]
	return [...head, ...Array(8 - head.length - tail.length).fill('0'), ...tail].map(group => parseInt(group, 16))
}

//an address as text two spellings of it share, for finding it in a list
function _key(address) {
	if (!address.includes(':')) return address
	return _groups(address).join(':')
}

//whether two addresses are the same address, false when either is missing
function _same(a, b) {
	return Boolean(a && b) && _key(a) == _key(b)
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
	for (let r of run.results) {
		lines.push(`${r.asked.padEnd(width)}  ${`${r.milliseconds} ms`.padStart(7)}  ${_sayAnswer(r)}`)//the answer last, since an error's text can run long
		if (r.binding) lines.push(..._sayBinding(r))
	}
	lines.push(..._sayMachine(run.machine))
	return lines.join('\n')
}

//everything net_stun saw of one server, on lines under its own: where the request went, the address the system says it would send from, where the answer came from and what kind it is, the whole answer in hex, and each attribute in it
function _sayBinding(r) {
	let b = r.binding
	let lines = []
	if (b.to) lines.push(`  to ${b.to} from port ${b.local}; ${r.source.success ? `the system says it would send there from ${r.source.value}` : r.source.outcome}`)
	if (!b.answer) return lines
	lines.push(`  answer from ${b.from}, ${stunKinds[b.answer.slice(0, 4)] || `type ${b.answer.slice(0, 4)}`}: ${_sayHex(b.answer)}`)
	for (let a of _attributes(b.answer)) lines.push(`    ${a.type} ${a.name}  ${a.value}  ${a.note}`)
	return lines
}

//the kinds of message a binding request can be answered with, by the type in an answer's first two bytes, RFC 8489
const stunKinds = {
	'0101': 'binding success',
	'0111': 'binding error',
}

//STUN's attributes by type, from RFC 8489 and RFC 5780, each with its name and a few words on what it carries
const stunAttributes = {
	0x0001: ['MAPPED-ADDRESS',     'the address and port the server saw, unmasked, the 2003 form'],
	0x0008: ['MESSAGE-INTEGRITY',  'a keyed hash, for servers that check credentials'],
	0x0009: ['ERROR-CODE',         'why the server refused'],
	0x0020: ['XOR-MAPPED-ADDRESS', 'the address and port the server saw, masked'],
	0x8022: ['SOFTWARE',           `the server's software`],
	0x8023: ['ALTERNATE-SERVER',   'another server to ask instead'],
	0x8028: ['FINGERPRINT',        'a checksum over the message'],
	0x802b: ['RESPONSE-ORIGIN',    'the address the server answered from'],
	0x802c: ['OTHER-ADDRESS',      `the server's second address, for NAT behavior tests`],
}

//the attributes in a STUN answer from its hex, read past the twenty-byte header the way net.rs reads them for the mapped address: each a type, a length, and a value padded to four bytes; answered as {type, name, value, note} in the box's words
function _attributes(hex) {
	let bytes = (hex.match(/../g) || []).map(pair => parseInt(pair, 16))
	let attributes = []
	let at = 20//past the header
	while (at + 4 <= bytes.length) {
		let type = bytes[at] << 8 | bytes[at + 1]
		let size = bytes[at + 2] << 8 | bytes[at + 3]
		let [name, note] = stunAttributes[type] || ['unknown', 'not one this panel knows']
		attributes.push({type: `0x${type.toString(16).padStart(4, '0')}`, name, value: _sayValue(type, bytes.slice(at + 4, at + 4 + size)), note})
		at += 4 + Math.ceil(size/4)*4//each value is padded to a multiple of four bytes
	}
	return attributes
}

//an attribute's value in the box's words: SOFTWARE and an error's reason as the text they are, everything else in hex
function _sayValue(type, value) {
	let text = bytes => new TextDecoder().decode(new Uint8Array(bytes))
	if (type == 0x8022) return text(value)
	if (type == 0x0009 && value.length >= 4) return `${(value[2] & 7)*100 + value[3]} ${text(value.slice(4))}`//the hundreds in the low three bits of one byte and the rest in the next, then the reason
	return _sayHex(value.map(byte => byte.toString(16).padStart(2, '0')).join(''))
}

//hex in groups of four bytes, the way the RFCs print a STUN message
function _sayHex(hex) {
	return (hex.match(/.{1,8}/g) || []).join(' ')
}

//what came back, in the box's words: the address, the error's text, for STUN what the server saw and the answer's size, how many interfaces and lines a list or a tool gave, or a name's addresses with a few words on them
function _sayAnswer(r) {
	if (!r.success) return r.outcome
	if (r.kind == 'stun') {
		let b = r.value//the binding net_stun answered
		let said = `${_sayEndpoint(b.address, b.port)}, ${b.bytes} bytes`
		if (b.sends > 1) said += `, sent ${b.sends} times`
		return said
	}
	if (r.kind == 'interfaces') return `${r.value.length} interfaces, ${r.value.reduce((sum, i) => sum + i.addresses.length, 0)} addresses`
	if (r.kind == 'tool') return `${r.value.trim().split('\n').length} lines`//what it printed is read into the interfaces below
	if (r.kind == 'resolve') return `${r.value.join(', ')}; ${_sayResolved(r)}`
	return r.value
}

//a few words on what a name's addresses show
function _sayResolved(r) {
	let four = r.value.some(address => !address.includes(':')), six = r.value.some(address => address.includes(':'))
	if (r.name == 'ipv4only.arpa') return six ? 'IPv6 made up by DNS64' : 'IPv4 only, so no DNS64 here'
	if (r.name.startsWith('ip4.')) return six ? 'IPv6 too, made up by DNS64' : 'IPv4 only, which keeps its fetch on IPv4'
	return four ? 'IPv4 too' : 'IPv6 only, which keeps its fetch on IPv6'
}

//the machine's interfaces, put together from every check that says something about them: net_interfaces for the list, the tools for what it leaves out on a Mac and Linux, and the route and reflector checks for which address is which; as [{name, port, flags, routes, gateways, addresses: [{address, family, prefix, shared, kinds, notes}]}] in the system's order, empty when net_interfaces failed
function _machine(results) {
	let listed = results.find(r => r.kind == 'interfaces')
	if (!listed.success) return []
	let printed = tool => results.find(r => r.kind == 'tool' && r.tool == tool && r.success)?.value || ''//what a tool printed, blank when it failed or this platform runs none
	let more = (platformName == 'Linux' ? _readIp : _readIfconfig)(printed('addresses'))
	let ports = _readPorts(printed('ports'))
	let gateways = [_readGateway(printed('gateway4')), _readGateway(printed('gateway6'))]
	let answer = (kind, family) => results.find(r => r.kind == kind && r.family == family && r.success)?.value
	return listed.value.map(i => {
		let found = more[i.name] || {status: '', addresses: {}}
		let routes = [4, 6].filter(family => i.addresses.some(a => _same(a.address, answer('route', family))))//the families whose default route this interface carries, by the address the system sends from
		let addresses = i.addresses.map(a => {
			let family = a.address.includes(':') ? 6 : 4
			let extra = found.addresses[_key(a.address)] || {}
			let flags = [...a.flags, ...(extra.flags || [])].map(word => flagWords[word] ?? word).filter(Boolean)//a word with no entry shows as the system wrote it
			let preferred = _lifetime(a.preferred ?? extra.preferred), valid = _lifetime(a.valid ?? extra.valid)
			let kind = _sayKind(a.address)
			let shared = kind == 'shared address space'
			let notes = [kind]
			if (shared) notes.push(routes.includes(4) ? `a carrier's, so carrier-grade NAT is likely` : `a mesh VPN's, like Tailscale's`)//only the interface carrying the default route says anything about the carrier
			notes.push(...flags)
			if (preferred < Infinity || valid < Infinity) notes.push(`preferred ${_sayLifetime(preferred)}, valid ${_sayLifetime(valid)}`)
			if (_same(a.address, answer('route', family))) notes.push('the source toward the internet')
			if (_same(a.address, answer('reflect', family))) notes.push('the reflector saw this one')
			return {address: a.address, family, prefix: a.prefix, shared, kinds: [kind, ...flags].map(note => note.split(',')[0]), notes}
		})
		let named = gateways.filter(g => g.interface == i.name).map(g => g.gateway)
		return {name: i.name, port: ports[i.name] || i.description, flags: [...i.flags, found.status].filter(Boolean), routes, gateways: [...i.gateways, ...named], addresses}
	})
}

//a Mac's ifconfig -L, read for each interface's status and each IPv6 address's flags and lifetimes, as {name: {status, addresses: {key: {flags, preferred, valid}}}}
function _readIfconfig(text) {
	let interfaces = {}
	let current
	for (let line of text.split('\n')) {
		let start = line.match(/^(\S+): flags=/)//a line that starts an interface, like en1: flags=8863<UP,BROADCAST,...> mtu 1500
		if (start) { current = interfaces[start[1]] = {status: '', addresses: {}}; continue }
		if (!current) continue
		let status = line.match(/^\s+status: (\S+)/)//active or inactive, whether the link is there
		if (status) current.status = status[1]
		let six = line.match(/^\s+inet6 (\S+) prefixlen \d+(.*)$/)//like inet6 2001:db8:4a2c:100::9c35 prefixlen 64 dynamic pltime 255522 vltime 601122
		if (!six) continue
		let words = six[2].trim().split(/\s+/).filter(Boolean)
		let number = name => { let i = words.indexOf(name); return i < 0 ? undefined : words[i + 1] == 'infty' ? Infinity : Number(words[i + 1]) }
		let flags = words.filter((word, i) => !['pltime', 'vltime', 'scopeid'].includes(word) && !['pltime', 'vltime', 'scopeid'].includes(words[i - 1]))//the flags alone, without the three numbers and their names
		current.addresses[_key(six[1].replace(/%.*/, ''))] = {flags, preferred: number('pltime'), valid: number('vltime')}//a link-local address ends with % and its interface, which net_interfaces leaves off
	}
	return interfaces
}

//Linux's ip -o address, read for each address's flags and lifetimes, in the same shape
function _readIp(text) {
	let interfaces = {}
	for (let line of text.split('\n')) {
		let m = line.match(/^\d+:\s+(\S+)\s+inet6?\s+([^\s/]+)\/\d+(.*)$/)//like 2: wlan0    inet6 2001:db8::1/64 scope global temporary dynamic \       valid_lft 86000sec preferred_lft 14000sec
		if (!m) continue
		let [, name, address, rest] = m
		let [before, after = ''] = rest.split('\\')//ip puts the lifetimes after a backslash
		let words = before.trim().split(/\s+/)
		let scope = words.indexOf('scope')
		let flags = scope < 0 ? [] : words.slice(scope + 2).filter(word => word != name)//the words after scope global or scope link, without the label an IPv4 line ends with
		let number = name => { let v = after.match(new RegExp(`${name} (\\S+)`))?.[1]; return v == 'forever' ? Infinity : v ? parseInt(v) : undefined }
		interfaces[name] ??= {status: '', addresses: {}}
		interfaces[name].addresses[_key(address)] = {flags, preferred: number('preferred_lft'), valid: number('valid_lft')}
	}
	return interfaces
}

//a Mac's networksetup -listallhardwareports, read as the name Settings gives each interface, like {en1: 'Wi-Fi'}
function _readPorts(text) {
	let ports = {}
	for (let [, port, device] of text.matchAll(/Hardware Port: (.+)\nDevice: (\S+)/g)) ports[device] = port
	return ports
}

//the default route's gateway and interface, from a Mac's route -n get default or Linux's ip route show default, as {gateway, interface}, or {} when there's no route or no gateway on it
function _readGateway(text) {
	let gateway = text.match(/gateway: (\S+)/), device = text.match(/interface: (\S+)/)
	if (gateway && device) return {gateway: gateway[1].replace(/%.*/, ''), interface: device[1]}//a link-local gateway ends with % and its interface
	let linux = text.match(/via (\S+) dev (\S+)/)//like default via 192.168.1.1 dev wlan0 proto dhcp metric 600
	if (linux) return {gateway: linux[1], interface: linux[2]}
	return {}
}

//a few words for each flag a system puts on an address, in that system's own vocabulary, since one word can mean different things on two of them: Linux's dynamic is any address with a lifetime, and a Mac's is one from DHCPv6; blank for a flag that says nothing a person needs
const flagWords = {
	macOS: {
		autoconf:   '',
		secured:    'stable, made from the network and a secret',
		temporary:  'temporary',
		dynamic:    'from DHCPv6',
		deprecated: 'deprecated, kept for connections already open',
		tentative:  'still being checked for a duplicate',
		duplicated: 'a duplicate, unused',
		detached:   'detached, its network is gone',
		optimistic: 'in use before its duplicate check ends',
	},
	Linux: {
		temporary:        'temporary',
		mngtmpaddr:       'stable, the one temporary addresses are made beside',
		'stable-privacy': 'stable, made from the network and a secret',
		dynamic:          '',
		noprefixroute:    '',
		deprecated:       'deprecated, kept for connections already open',
		tentative:        'still being checked for a duplicate',
		dadfailed:        'a duplicate, unused',
	},
	Windows: {
		'prefix dhcp':                 'from DHCP',
		'prefix router-advertisement': `made from the network's prefix`,
		'prefix manual':               'set by hand',
		'prefix well-known':           '',
		'prefix other':                '',
		'suffix dhcp':                 '',
		'suffix random':               'with a random suffix',
		'suffix link-layer':           'made from the hardware address',
		'suffix manual':               '',
		'suffix well-known':           '',
		'suffix other':                '',
		preferred:                     '',
		deprecated:                    'deprecated, kept for connections already open',
		tentative:                     'still being checked for a duplicate',
		duplicate:                     'a duplicate, unused',
	},
}[platformName]

//a few words on the interfaces a system makes for itself, by name, for the ones Settings doesn't name
const interfaceWords = [
	[/^lo\d*$/,            'loopback, this machine only'],
	[/^utun\d+$/,          `a tunnel, for a VPN or one of macOS's own services`],
	[/^awdl\d+$/,          'Apple Wireless Direct Link, for AirDrop and Continuity'],
	[/^llw\d+$/,           'low-latency Wi-Fi, beside awdl0'],
	[/^gif\d+$/,           'a generic tunnel'],
	[/^stf\d+$/,           'a 6to4 tunnel'],
	[/^(docker\d+|br-)/,   'a Docker bridge'],
	[/^veth/,              `one end of a container's link`],
	[/^tailscale\d+$/,     'Tailscale'],
	[/^wg\d+$/,            'WireGuard'],
]

//what kind of address this is, by the ranges IANA's special-purpose registries set aside for each family, in a few words
function _sayKind(address) {
	if (!address.includes(':')) {
		let [a, b, c] = address.split('.').map(Number)
		if (a == 127) return 'loopback, this machine only'
		if (a == 10 || (a == 172 && b >= 16 && b < 32) || (a == 192 && b == 168)) return 'private'//RFC 1918
		if (a == 100 && b >= 64 && b < 128) return 'shared address space'//RFC 6598
		if (a == 169 && b == 254) return 'link-local, made up when no DHCP server answered'
		if (a == 192 && b == 0 && c == 0) return 'set aside for protocols, like DS-Lite'
		if ((a == 192 && b == 0 && c == 2) || (a == 198 && b == 51 && c == 100) || (a == 203 && b == 0 && c == 113)) return 'documentation'
		if (a >= 224 && a < 240) return 'multicast'
		if (a == 0 || a >= 240) return 'reserved'
		return 'public'
	}
	let g = _groups(address)
	if (g.every((group, i) => group == (i == 7 ? 1 : 0))) return 'loopback, this machine only'
	if ((g[0] & 0xffc0) == 0xfe80) return 'link-local, this link only'
	if ((g[0] & 0xfe00) == 0xfc00) return 'unique local, private to this network'
	if (g[0] == 0x64 && g[1] == 0xff9b) return `NAT64's well-known prefix`
	if (g[0] == 0x2001 && g[1] == 0) return 'Teredo, a tunnel through IPv4'
	if (g[0] == 0x2001 && g[1] == 0xdb8) return 'documentation'
	if (g[0] == 0x2002) return '6to4, a tunnel through IPv4'
	if ((g[0] & 0xff00) == 0xff00) return 'multicast'
	if ((g[0] & 0xe000) == 0x2000) return 'global'
	return 'reserved'
}

//a lifetime in seconds, with always as Infinity: the absence of one, and Windows' 4294967295
function _lifetime(seconds) {
	if (seconds == null || seconds >= 4294967295) return Infinity
	return seconds
}

//a lifetime in round words, like 4 minutes or 6 days
function _sayLifetime(seconds) {
	if (seconds == Infinity) return 'always'
	for (let [name, length] of [['day', Time.day], ['hour', Time.hour], ['minute', Time.minute]]) {
		let count = Math.floor(seconds*Time.second/length)
		if (count >= 1) return `${count} ${name}${count == 1 ? '' : 's'}`
	}
	return `${seconds} seconds`
}

//the interface an address is on and the address's entry, or undefined when it's on none of them
function _find(machine, address) {
	for (let i of machine) {
		let a = i.addresses.find(a => _same(a.address, address))
		if (a) return {i, a}
	}
}

//an interface by its name and, when Settings gives it one, that name too, like en1 (Wi-Fi)
function _sayInterface(i) {
	return `${i.name}${i.port ? ` (${i.port})` : ''}`
}

//the address the system sends from, with the interface it's on and a few words on what kind it is
function _sayLocal(route, machine) {
	let found = route.success && _find(machine, route.value)
	if (!found) return _sayFact(route)
	return `${route.value} on ${_sayInterface(found.i)}, ${found.a.kinds.join(', ')}`
}

//the address a reflector saw, and a word when it's one of the machine's own, which over IPv4 means nothing translates
function _sayPublic(reflect, machine) {
	let found = reflect.success && _find(machine, reflect.value)
	if (!found) return _sayFact(reflect)
	return `${reflect.value}, this machine's own, ${found.a.kinds.join(', ')}`
}

//the gateway the default route in a family goes through, and the interface it's on
function _sayGateway(family, machine) {
	if (!machine.length) return 'unknown'
	let i = machine.find(i => i.routes.includes(family))
	if (!i) return 'none'//no default route in this family
	let gateway = i.gateways.find(g => g.includes(':') == (family == 6))
	if (!gateway) return `none named, the route goes through ${_sayInterface(i)}`//a tunnel's route has no gateway, and a tool that failed names none
	return `${gateway} on ${_sayInterface(i)}`
}

//how many interfaces, and how many hold an address
function _sayCount(machine) {
	if (!machine.length) return 'unknown'
	return `${machine.length}, ${machine.filter(i => i.addresses.length).length} with addresses`
}

//whether the network makes up IPv6 addresses for names that have none, by the answer for ipv4only.arpa, and the translator's prefix when it does
function _sayDns64(results) {
	let r = results.find(r => r.kind == 'resolve' && r.name == 'ipv4only.arpa')
	if (!r.success) return 'unknown'
	let prefixes = [...new Set(r.value.filter(address => address.includes(':')).map(_nat64Prefix))]
	if (!prefixes.length) return 'none'
	return `yes, NAT64 prefix ${prefixes.join(' and ')}`
}

//the NAT64 prefix in an IPv6 address made up for ipv4only.arpa, found by where 192.0.0.170 or 192.0.0.171 sits inside it, at one of the six places RFC 6052 allows, like 64:ff9b::/96
function _nat64Prefix(address) {
	let bytes = _groups(address).flatMap(group => [group >> 8, group & 0xff])
	for (let length of [96, 64, 56, 48, 40, 32]) {
		let embedded = bytes.filter((byte, i) => i >= length/8 && i != 8).slice(0, 4).join('.')//the four bytes after the prefix, passing over byte 8, which RFC 6052 keeps zero
		if (embedded == '192.0.0.170' || embedded == '192.0.0.171') {
			let groups = []
			for (let i = 0; i < 16; i += 2) groups.push(i < length/8 ? bytes[i] << 8 | (i + 1 < length/8 ? bytes[i + 1] : 0) : 0)
			return `${_sayIpv6(groups)}/${length}`
		}
	}
	return 'unknown'
}

//eight groups as an IPv6 address the usual way, the longest run of zero groups written as ::
function _sayIpv6(groups) {
	let [start, run] = [-1, 0]//where the longest run of zero groups starts, and how long it is
	for (let i = 0; i < 8; i++) {
		let n = 0
		while (i + n < 8 && groups[i + n] == 0) n++
		if (n > run) [start, run] = [i, n]
	}
	let hex = groups.map(group => group.toString(16))
	if (run < 2) return hex.join(':')
	return `${hex.slice(0, start).join(':')}::${hex.slice(start + run).join(':')}`
}

//whether an IPv4 address on the interface carrying the default route is in the shared address space, which says a carrier translates above the router
function _sayShared(machine) {
	if (!machine.length) return 'unknown'
	let found = machine.flatMap(i => i.addresses.filter(a => a.shared).map(a => ({i, a})))
	let routed = found.find(f => f.i.routes.includes(4))
	if (routed) return `${routed.a.address} on ${_sayInterface(routed.i)}, so carrier-grade NAT is likely`
	if (found.length) return `none on the default route, only on ${[...new Set(found.map(f => f.i.name))].join(' and ')}, a mesh VPN's`
	return 'none'
}

//the machine's interfaces, the box's last lines: each with its flags, what it is, and the routes and gateways it carries, then every address on it with its prefix length and a few words on it
function _sayMachine(machine) {
	if (!machine.length) return []
	let width = Math.max(...machine.flatMap(i => i.addresses.map(a => `${a.address}/${a.prefix}`.length)))//the address column as wide as its longest entry, so the notes line up
	let lines = ['']
	for (let i of machine) {
		let about = [i.flags.join(', ')]
		if (!i.port) about.push(interfaceWords.find(([pattern]) => pattern.test(i.name))?.[1])
		if (i.routes.length) about.push(`the default route for ${i.routes.map(family => `IPv${family}`).join(' and ')}`)
		if (i.gateways.length) about.push(`gateway ${i.gateways.join(' and ')}`)
		if (!i.addresses.length) about.push('no addresses')
		lines.push(`${_sayInterface(i)}: ${about.filter(Boolean).join('; ')}`)
		for (let a of i.addresses) lines.push(`  ${`${a.address}/${a.prefix}`.padEnd(width)}  ${a.notes.join(', ')}`)
	}
	return lines
}
