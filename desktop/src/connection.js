import {reactive, ref, shallowRef, watch} from 'vue'
import {getVersion} from '@tauri-apps/api/app'
import {getCurrentWindow} from '@tauri-apps/api/window'
import {Data, Size, Time, sayDuration, saySize4} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go
import {brandName, urlHome} from './brand.js'
import {platformName} from './settings.js'
import {sayMoment} from './time.js'
import {netGet, netLocal, netStun, netResolve, netInterfaces, netNatpmp, netNatpmpMap, netPcpAnnounce, netPcpMap, netUpnpSearch, netUpnpDescription, netUpnpExternal, netUpnpMappings, netUpnpFirewall, netUpnpAddPort, netUpnpRemovePort, netUpnpAddPinhole, netUpnpRemovePinhole} from './net.js'
import {udpOpen, udpStun, udpSend, udpReceive, udpClose} from './udp.js'
import {processOutput, processElevate} from './process.js'
import {firewallProfiles, firewallRules} from './firewall.js'
import {engineStatus} from './engine.js'
import {dialogOpen} from './dialog.js'
import {downloadsFolder, pathsStatus, isInstalled} from './paths.js'
import {useIncomingStore} from './stores/incoming.js'
import {stick} from './log.js'

/*
The Connection tab's checks: what this machine and the network say about its connection, asked when the user presses Check and at no other time, so opening the tab asks nothing of anyone. good.ftorrent.com asks some of the same questions from a browser; that page is separate code, and what the two share is the servers they ask and what each server's answer means.

For each family, IPv4 and IPv6, a run asks three things. The system says which of the machine's addresses it would send from to reach an address beyond the local network, which takes no packet at all. A reflector at ip4.ftorrent.com or ip6.ftorrent.com answers the address the request arrived from; each name has an address record of one family only, so the request can only travel over that family, and the pair of answers is which families reach the internet from here. Behind a home router the two IPv4 answers differ, since the router translates; on IPv6 they're usually the same address, since nothing does.

And STUN, over UDP, which is what a peer connection travels on: our server, moon's, and Cloudflare's, three places on the internet, asked at once from one local port, the way Tailscale's netcheck asks its servers, each answering the address and port it saw the request arrive from. A router keys the mapping it makes on the inside address and port, so when every server sees the same public port, it keeps one mapping for every destination, endpoint-independent mapping, which hole punching needs; a new port for each server is endpoint-dependent, the symmetric case, where hole punching fails. The address STUN saw should be the one the reflector saw, and when it isn't, something carries one path and not the other, a VPN, a proxy, or a relay. On IPv6 these comparisons are of the /64, the network, since a machine can send from more than one of its temporary addresses, and a Mac sometimes from one it has already deprecated.

And filtering, the other half of whether a hole punch can work: which outside addresses the router lets reach a port it has mapped. RFC 4787 names three kinds. Endpoint-independent filtering lets in anyone once the socket has sent anything; address-dependent lets in only an address the socket has sent to, from any port there; and address-and-port-dependent only the exact address and port. Telling them apart takes a socket that stays open across several steps, and a server that sends from somewhere the socket hasn't, which is moon. Each of two tests opens a socket of its own through udp_open, asks one STUN server from it which public address and port it has, then asks moon over HTTPS to poke that port, three small UDP packets carrying a token the panel makes up, while it receives on the socket. The first test's socket asks our STUN server alone, so moon's poke comes from an address it has never sent to, and arriving is endpoint-independent filtering. The second's asks moon's STUN server, so moon's poke comes from the address it just sent to, at another port: arriving there and not in the first test is address-dependent, and held back there too is address-and-port-dependent. The panel names the public address STUN saw, and moon pokes there, since that isn't always where the request for a poke comes from: over IPv6 the request can leave from another of the machine's temporary addresses, and on cellular from another of the carrier's public IPv4 addresses. moon pokes only a public address, and the poke's answer names where it went, which the reading sets beside where STUN saw the socket. A firewall on this machine filters too, and the reading names it when it may be what held a poke back. The two tests need nothing from each other, so they run side by side, each family's beside the other's and beside every other check from the start of the run, and a receive that hears nothing waits its whole four seconds, so the run waits on the filtering last.

And the hole punch, which mapping and filtering decide between them: whether this machine and a peer behind a NAT of its own can reach each other directly, each sending to the other at once. moon runs three such peers, each in a network namespace of its own. Two sit behind NATs moon makes with Linux's own translation: the cone, which keeps a socket's port for every destination, and the symmetric, which picks a fresh port for each. For each, a socket opened for it alone asks our STUN server its public address and port, then asks moon over HTTPS to punch that address and port; the peer in the namespace named learns its own public address and port from the same STUN server, moon answers them, and the peer starts sending to the address and port the panel named, a small packet every fifth of a second for five seconds. The panel sends to the peer at the same pace while it listens, in rounds of a second, since udp_receive waits in whole seconds. A router lets a packet in only once its socket has sent toward where the packet comes from, so the packets get through only when both sides send, and only when each NAT shows the other side the port STUN saw. Each packet says whether the other side's have arrived, so this socket hears both directions: a packet of the peer's at all is the peer getting through, and one saying heard yes is this side getting through. When the peer's packets come from another port than the one moon answered, as the symmetric NAT sends them, the panel sends there too, as ICE does, which is how a router that lets in anything from an address it has sent to gets through even the symmetric NAT. Over IPv4 the panel punches through moon's two NATs, and over IPv6 through a third namespace, where the peer has a public address of its own behind a stateful firewall that translates nothing, the shape of a home router's IPv6 side; all three run beside every other check from the start of the run.

And, while a session listens, the inbound proof: whether a stranger can reach the engine at all, unasked, which is the question every number above leads to. moon probes the session's port in each family three ways at once, all to the address the request came from. A DHT ping, which the engine's DHT node answers with a pong that says where it saw moon; a uTP connection request, uTP being BitTorrent's transport over UDP, which the engine accepts with a torrent loaded and turns away without one, either way an answer; and a TCP dial with a BitTorrent handshake carrying the chosen torrent's info-hash, which the engine answers with its own handshake, or closes without a torrent, which still proves the connection opened. A plain UDP poke would prove nothing here, since libtorrent drops a packet it can't read without an alert or a reply. moon hears each answer itself and says so, and the engine posts an incoming connection for the dial and, with a torrent, for the uTP request, so the run waits one more poll of the engine and counts the connections in from moon's address. Nothing back from all three, with nothing refused, is a translator or a firewall dropping a stranger's packets, the measurement Transmission's port test has offered for years, here over both families and both transports.

And the machine itself, which takes no packet beyond a name lookup. net_interfaces lists every interface with every address on it, the ones a person finds in Settings and wonders about along with the rest, and each address gets a few words from the ranges the standards set aside: private, link-local, global, and the others. That one call says less than a person needs, so on a Mac and Linux the panel also runs the tools the system ships: ifconfig and ip for what kind each IPv6 address is, temporary, stable, from DHCPv6, or deprecated, and how long it lasts; route and ip route for the gateway in each family; and on a Mac networksetup for the name Settings gives each interface, like Wi-Fi. Windows says all of it through net_interfaces itself, which is just as well, since its tools print in the system's language. The interface carrying the default route is the one holding the address the system says it sends from, and the address the reflector saw over IPv6 is usually one of the machine's own, a temporary one.

The resolver answers three names. ipv4only.arpa has only IPv4 addresses by definition, RFC 7050, so an IPv6 answer for it means the network makes up IPv6 addresses for names that have none, DNS64, so an IPv6-only network can reach them through a translator, NAT64; where the IPv4 address sits inside the answer names the translator's prefix. ip4.ftorrent.com and ip6.ftorrent.com show the single-family records the reflector checks rest on. And an IPv4 address in the shared address space, 100.64.0.0/10, RFC 6598, on the interface carrying the default route says a carrier translates above the router; on any other interface it's a mesh VPN's, like Tailscale's, which borrows the range.

And the machine's firewall, which stands between a connection that reaches the machine and the engine listening for it. On a Mac the panel runs socketfilterfw five times, one read each, all answered without sudo: whether the firewall is on; block all, which when on refuses every incoming connection whatever the list says; stealth mode, which leaves probes to closed ports unanswered; the allowance for signed software, built-in and downloaded; and the list of programs the firewall has been asked about, each by its path and allowed or blocked. ftorrent is signed ad hoc rather than with an Apple-issued identity, so the allowance for signed software doesn't cover it or its engine, and whether each is admitted comes from the list. The panel looks there for this copy's executable, or the bundle holding it, since the list writes some programs as the .app, and for the engine's path, which the firewall asks about and lists on its own, so a development build's engine and an installed copy's are two entries. The firewall ships off on a Mac, and off is not a problem. On Windows the panel reads the firewall through its COM interface, INetFwPolicy2, without elevation and in numbers, where netsh prints in the system's language: each of the three profiles, Domain, Private, and Public, whether the current network is in it, whether the firewall is on for it, block all, and what it does to an incoming connection no rule names; and every rule naming this copy's executable or the engine's by path, the block rules the security alert writes before anyone answers included, since a block rule overrides an allow. The current network's profile gets its own words, since Windows starts every new network as Public unless someone says otherwise, a home network included, and most people have never seen which theirs is. The box puts the firewall's standing after the checks and before the interfaces, with the whole list under it on a Mac and the rules naming ftorrent and its engine on Windows. On Linux the panel reads the firewall as unknown, since ufw and nftables need root to read.

And two buttons for changing the firewall rather than reading it, each doing one thing, since the panel is where each piece is built and tested on its own. Firewall settings… opens the page where the person makes the exception: on a Mac System Settings at the Network pane, one row above Firewall, since a program can't add itself to that firewall, and on Windows Control Panel's Allow an app through Windows Firewall, which lists each program's rules with a box for each kind of network, and where Change settings, behind its own shield, lets the person check them. Write firewall rules…, on Windows for an installed copy, writes the rules itself in one run of netsh with an administrator's rights, which Windows asks the person to approve. netsh deletes every inbound rule naming this copy or its engine, the security alert's block rules among them, since a block wins over an allow. Then it adds an allow for each over TCP and UDP in every profile, with edge traversal left to the program, so libtorrent's request for Teredo doesn't raise the alert a second time. The box says what's about to happen before Windows asks, and reads the firewall again once netsh is done, which is the proof, since netsh's exit code reads 1 whenever a delete finds nothing to delete. netsh goes through PowerShell, so the paths travel in a command line, which is Unicode the whole way, rather than in a script file netsh would have to decode right, and as plain text, so the UAC prompt's details show exactly the commands the box lists. A Windows copy anywhere else doesn't get that button, since a rule names a path, and a portable copy or a build in the repository may move or vanish; the settings page serves it, since Allow another app there takes a program at any path.

And the router, asked once the first wave of checks has named the gateway, since the question goes to that address. NAT-PMP, Apple's protocol from 2005, RFC 6886, is two bytes to the gateway's port 5351 asking for its external address, answered with the address and the seconds since the router's mapping table started. PCP, the IETF's successor from 2013, RFC 6887, listens on the same port, and its ANNOUNCE is a 24-byte header that asks for nothing: a router that speaks PCP answers it with a result code and its epoch, and a router that speaks only NAT-PMP answers, in NAT-PMP's own form, that the version is unsupported, which says what the router speaks as surely as a success would. A router that speaks neither refuses the port, and the refusal comes back on both, where the machine's firewall lets it: Windows' admits only one kind of ICMP unreachable by default, not the port-unreachable a refusal is, so there a refused port reads as no answer, after the full four seconds. UPnP is the third way and the oldest, from 2001, with the widest support: its Internet Gateway Device profile describes a router, and a search multicast to 239.255.255.250 on port 1900 finds one, its device description gives the control address of its WAN connection service, and SOAP calls there read its external address and walk its port mappings by index until the router says the list has ended. The service type names the version, WANIPConnection:1 from 2001 or 2 from 2010, which added IPv6 firewall control as a service of its own, whose status read says whether the router's IPv6 firewall is on and admits pinholes; the panel reads that when the router offers the service and says when it doesn't. igd-next does all the talking in Rust and answers the gateway it found as plain data the page hands back for each read, so the search runs once and the page owns what follows. The description the search read those paths from also names the router, its friendly name, maker, and model, which igd-next keeps none of, so net_upnp_description fetches it again, over plain http on the local network as UPnP serves it, and the page reads the root device's fields into the UPnP row, in parentheses after the router's address. The three protocols are asked at once, once the gateway is known, and the four UPnP reads follow the search. Two readings sit on those facts. Which protocols the router speaks comes straight from what answered, with the firewall named when it may have held the search's answer back. And whether a carrier translates above the router rests on the external address the router reports, the one check a carrier's translator can't dodge: when it differs from the address the reflector saw, something between the router and the internet translates again, and when that address is in the shared address space, 100.64.0.0/10, it's a carrier's; a private one is more likely a second router, like a provider's modem. The shared address space on the default route and an endpoint-dependent mapping go in as reasons beside it. One case slips past the address check: address-plus-port sharing, where the carrier gives several customers one public address and each one's router a slice of its ports, so the router's own external address is the public one.

A check that asks once and waits for one answer gets checkSeconds to hear back. One round trip is tens of milliseconds, and a fetch over https is four, the name, the connection, the encryption, and the request, so around 200 milliseconds on a typical connection and around 600 across an ocean. The slowest ordinary connection, a satellite in geostationary orbit, has a round trip of 600 milliseconds, which puts a fetch near two and a half seconds, and a lost packet is sent again after a second. Four seconds covers all of those, and only a path that silently drops what it's sent waits that long, since no route, or a name with no address, fails at once whatever the limit. A check that waits longer by design gets its own time when it arrives. So a check ends one of three ways: an answer, in milliseconds; a failure that came back with its reason, like no route; or no answer in checkSeconds.

A run asks its first wave of checks at once and waits for the last, so that wave takes as long as its slowest check, at most checkSeconds. Then, for each STUN server whose name led somewhere, it asks the system which of the machine's addresses it would send from toward that server, which takes no packet, and over IPv6 can be a different one of the machine's addresses for each. That is the system's word and not a measurement: a Mac has sent HTTPS and STUN from a deprecated temporary address while saying, a moment later, that it would send from the current one, so the box gives both, and the address the server saw is on the line above. The box lists the checks in the same order every time, whatever order they finish in, so two runs line up line for line, and a family's one STUN call shows as a line for each server it asked, with everything net_stun saw of that server on the lines under it: where the request went, the address the system says it would send from, where the answer came from, the answer in hex, and each attribute in it by name with a few words on what it carries. An error is an answer like any other: the box shows its text, since a user on this tab is under the hood. The box keeps every run since the app started, oldest first, each starting with a line that says when it ran, which build, and on which platform, so a box pasted into a message or an issue says where its lines came from, and ends with every interface and every address on it, each with a few words on what it is.

And the engine's session, which runs for as long as the user lets it rather than for four seconds. Start opens libtorrent's session at a random port from the dynamic range, the range Transmission picks its random port from, listening on every address in both families and joining the DHT through our bootstrap node and two others, with port mapping and local service discovery off. libtorrent reads every address as each address the machine has, loopback and link-local included, and opens a TCP socket and a UDP one on each; from each one that reaches beyond the machine it runs a DHT node with its own routing table, announces to the trackers, and learns the address peers see. With a torrent file opened, it also joins that swarm in upload mode, announcing and taking connections but downloading nothing, so strangers who find it through the trackers and the DHT dial in. While it runs, the page asks the engine once a second for the alerts libtorrent has posted and for its counters. A session in a busy swarm posts a hundred alerts a second, most of them peers tried and dropped and DHT lookups, so the box shows each kind of alert once, the first to arrive, timed from Start by when the page heard it, up to a second after it happened; the table counts every alert by kind, and the box gets the counts when the session stops. A connection in is the exception, and gets a line every time, saying whether the session had sent to that address first, through a DHT lookup or a connection it tried, or whether it's unsolicited. The DHT's routine messages, its bootstrap queries and pings, post no alert, so unsolicited means as far as the alerts show. The table keeps the readings too: which sockets opened, how many nodes the routing tables hold, in all by libtorrent's counter and in each by asking each DHT node, with the address it runs from and its node ID, since libtorrent's own DHT stats describe only one table, the addresses peers say they see, and how many strangers connected in over each family and how many of those were unsolicited, counted by alert, and by libtorrent's own counter. Behind a home router with no port mapping, strangers still reach the session, over both families and over uTP, which shares the session's UDP port with the DHT: each DHT message the session sends opens the router for that node's replies, and a node also in the swarm connects in through the opening. Whether the router lets in a stranger who was never sent anything is its filtering, which these alerts can't settle and Good7 measures. The session lives in the engine, not the tab, so it keeps running while the user looks elsewhere, which is why the box, the last run, and the session's readings are kept here rather than in the page. For the same reason the whole panel as Copy takes it, the table and then the box, is stuck to stick.txt beside the log as it changes, at most once a second, so it can be read from outside ftorrent while a session runs, whichever tab is showing.

And port mapping, which Map and Unmap turn on and off in the running session: libtorrent's two mappers, which the session that downloads leaves off, since ftorrent maps its port with its own code, worked here by hand so a Check before Map and one after show what libtorrent's own mapping gets. The UPnP mapper searches for the router and asks its WAN connection service to forward the session's port, TCP and UDP, over IPv4 alone. The NAT-PMP mapper sends PCP's MAP to the gateway from every socket with a global address in either family, and falls back to NAT-PMP over IPv4 when the router answers that the version is unsupported, so over IPv6 its request is for a pinhole in the router's firewall, since nothing translates there. On macOS it finds no IPv6 gateway, since its reading of the routing table drops every IPv6 route there, so each IPv6 mapper closes without sending, and its row says why. Each lease is a minute, renewed at three quarters of that, and each mapper deletes what it mapped when it's turned off or the session stops. Every alert the mappers post gets a line in the box, the mappings, the failures, and the mappers' own log, since a router that never answers posts nothing else: the NAT-PMP mapper sends nine times over about eleven seconds and then waits two hours. The table reads each mapper on each local address from those lines: which protocol it spoke, which only the log says, since the alerts call PCP natpmp too; whether UPnP found the router; what the router mapped, and to which external port; whether the router took only a mapping with no lease, which libtorrent asks for when UPnP's answer is error 725, and which stays in the router until it's deleted; how many requests went and answers came; and why a mapper stopped, when its log says it did. A Check while the session runs probes its port as it always does, and says whether mapping was on.

And the mapping run, which Try mapping starts: the panel's own requests to the router, apart from libtorrent's, so the panel measures what the router allows on its own, whatever libtorrent can or can't ask. For each of the five combinations of protocol and family, UPnP, NAT-PMP, and PCP over IPv4, and UPnP and PCP over IPv6, since NAT-PMP has none, a trial opens a UDP socket and keeps it open throughout, and the five run side by side. moon pokes the address and port a mapping would forward before anything is asked; the trial asks the router to forward the socket's port, with a lease of a minute; moon pokes the port the router granted; the trial deletes the grant; and moon pokes a last time. Over IPv4 the pokes go to the router's external address, as UPnP's GetExternalIPAddress, NAT-PMP's address request, or PCP's MAP answer names it, or failing those the address the reflector saw, and over IPv6 to this machine's own address, the one the system sends from, which the pinhole names, since nothing translates there. PCP's ANNOUNCE goes to the IPv6 gateway first, so the run knows whether the router's IPv6 side speaks PCP at all, and every PCP answer goes in the box whole, as the router wrote it. So each combination answers four questions: whether the router speaks it, what it granted, whether a packet from outside arrives through the grant and was held back before it, and whether the delete closes it again. A router that takes UPnP mappings only with no lease answers error 725, and the trial asks again with none, which only its delete removes. The run changes the router, so it never runs on a Check, only when the person asks, and it goes in the box beside the Checks.

And what the system can change behind ftorrent's back, or hold up with a box of its own. A system box, the firewall asking about ftorrent or Local Network privacy asking to reach the router, takes focus, and a check running while it stands fails on the clock: the UPnP search, held back by the Mac firewall until the person answers, reads no answer in four seconds. So after a Check, the panel asks again the checks that stand alone and ended in no answer, on their own, when the window comes forward, since answering a box gives focus back, and on timers ten and twenty seconds on in case it doesn't, at most twice, as a run of their own in the box; what they answer folds into the Check's table, and a UPnP search that finds the router has its four reads follow. The steps of the filtering, the punches, and the inbound proof each belong to a sequence on one socket, so the panel never asks one alone. And whenever the window comes forward after a Check, the panel reads the firewall again, since the person may have just changed it in System Settings or Windows Settings, and says in the box when its standing or what it admits has changed. The panel never runs Try mapping again on its own, since it changes the router.
*/

const checkSeconds = 4//how long a check that asks once has to hear back, the essay above says why
const noAnswer = _sayNoAnswer(checkSeconds)
const reflectorLimit = 100//bytes; the reflector answers one address and a newline, at most 46
const descriptionLimit = 64*Size.kb//a router's UPnP device description is a few kilobytes, its list of icons included
const moonStun = `moon.${urlHome}:3478`//moon's, our second STUN server, at another place on the internet
const stunServers = [`stun.${urlHome}:3478`, moonStun, 'stun.cloudflare.com:3478']//ours, moon's, and Cloudflare's: three places on the internet, two of them ours
const pokePackets = 3//how many packets moon sends in a poke, as its Go service sets it, so a receive can stop once they're all in
const mappingLease = 60//seconds, how long each mapping the mapping run asks for lasts, so one a run leaves behind, if it stops partway, is gone from the router a minute later
const mappingDescription = `${brandName} mapping test`//the words a UPnP mapping carries, which a router's settings page shows beside it
const reportLimit = 8*Size.kb//bytes; moon's account of a poke, a ping, or a dial is one small JSON object, a ping's whole answer in hex among it
const pingSeconds = checkSeconds + 3//moon waits three seconds for a ping's answer, on top of the request's own time
const dialSeconds = checkSeconds + 6//and up to three to connect a dial and three more to hear back
const punches = [['cone', 4], ['symmetric', 4], ['firewall', 6]]//moon's three namespaces, each with a peer in it, by the names moon's punch takes, and the family each punches in: its two NATs translate IPv4, and its firewall filters IPv6 and translates nothing
const punchNames = {cone: `moon's cone NAT`, symmetric: `moon's symmetric NAT`, firewall: `moon's IPv6 firewall`}//each namespace in the row's words
const punchCallSeconds = checkSeconds + 3//moon's peer waits up to three seconds for its own STUN answer before moon answers, on top of the request's own time
const punchSeconds = 5//how long the panel sends and listens, the length of the peer's sending, which starts just before moon answers
const punchGap = Time.second/5//between the panel's packets, as between the peer's

//192.0.2.1 and 2001:db8::1 are addresses set aside for documentation, by RFC 5737 and RFC 3849, so they name no one, and any address beyond the local network gets the same answer, the default route's
const beyond = {4: '192.0.2.1', 6: '2001:db8::1'}

//just enough to tell an address of each family from anything else a server or the system could answer
const looksLike = {
	4: /^\d{1,3}(\.\d{1,3}){3}$/,
	6: /^[0-9a-f]*:[0-9a-f:.]*$/i,
}

const firewallTool = '/usr/libexec/ApplicationFirewall/socketfilterfw'//the Mac's command for its application firewall, which answers every read below without sudo; System Settings shows the same facts under Network, Firewall

//the system's own tools, run on a Mac and Linux for what net_interfaces can't say there: each IPv6 address's kind and lifetimes, the gateway in each family, and on a Mac the name Settings gives each interface, and the firewall's standing and its list; Windows says the interfaces through net_interfaces
const tools = {
	macOS: [
		{tool: 'addresses', program: '/sbin/ifconfig', args: ['-L']},//-L adds each IPv6 address's lifetimes
		{tool: 'gateway4', program: '/sbin/route', args: ['-n', 'get', 'default']},
		{tool: 'gateway6', program: '/sbin/route', args: ['-n', 'get', '-inet6', 'default']},
		{tool: 'ports', program: '/usr/sbin/networksetup', args: ['-listallhardwareports']},
		{tool: 'firewall', program: firewallTool, args: ['--getglobalstate']},//on or off, like Firewall is enabled. (State = 1)
		{tool: 'blockall', program: firewallTool, args: ['--getblockall']},//whether every incoming connection is refused, whatever the list says
		{tool: 'stealth', program: firewallTool, args: ['--getstealthmode']},//whether probes to closed ports go unanswered
		{tool: 'signed', program: firewallTool, args: ['--getallowsigned']},//two lines, for built-in and for downloaded signed software, which ftorrent, signed ad hoc, doesn't count as
		{tool: 'programs', program: firewallTool, args: ['--listapps']},//every program the firewall has been asked about, by path, each allowed or blocked
	],
	Linux: [
		{tool: 'addresses', program: 'ip', args: ['-o', 'address']},//-o puts each address on one line
		{tool: 'gateway4', program: 'ip', args: ['-4', 'route', 'show', 'default']},
		{tool: 'gateway6', program: 'ip', args: ['-6', 'route', 'show', 'default']},
	],
	Windows: [],
}[platformName]
const firewallTools = ['firewall', 'blockall', 'stealth', 'signed', 'programs']//the Mac's five firewall reads among the tools, which the window coming forward asks again
const firewallUnknown = {macOS: 'unknown', Linux: 'unknown, ufw and nftables need root to read', Windows: 'unknown'}[platformName]//the table's words for a firewall the panel can't read: a Mac's when socketfilterfw failed, Windows' when its COM interface did, and Linux's always

const firewallListName = platformName == 'Windows' ? 'Firewall rules' : 'Firewall list'//the table's name for the row about this copy and its engine: the firewall's list of programs on a Mac, and the rules naming them on Windows

//the names the resolver is asked: one with only IPv4 addresses by definition, and our two single-family names
const names = ['ipv4only.arpa', `ip4.${urlHome}`, `ip6.${urlHome}`]

//the checks for one run, in the order the box lists them: for each family, its local address, its public address over HTTPS, and STUN; then the machine's interfaces, the tools that say more about them, the names, and on Windows the firewall, which a Mac reads through its tools
function _checks(paths, engine) {
	let checks = []
	for (let family of [4, 6]) checks.push(_route(family), _reflect(family), _stun(family))
	checks.push(_interfaces(), ...tools.map(_tool), ...names.map(_resolve))
	if (platformName == 'Windows') checks.push(_firewallProfiles(), _firewallRules('ftorrent', paths.executable), _firewallRules('engine', engine.path))
	return checks
}

//a check of the Windows firewall's three profiles, through its COM interface
function _firewallProfiles() {
	return {kind: 'firewall-profiles', asked: 'Windows firewall profiles', ask: () => firewallProfiles(), read: list => ({success: true, value: list})}
}

//a check of every Windows firewall rule naming one of this copy's two programs by path, which is ftorrent or engine
function _firewallRules(which, path) {
	return {kind: 'firewall-rules', which, asked: `Windows firewall rules for ${path.split(/[\\/]/).pop()}`, ask: () => firewallRules(path), read: list => ({success: true, value: list})}
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

//the filtering check for one family, as two tests side by side, every step of each a result for the box: the first test's socket asks our STUN server alone, and moon pokes the public address and port it saw, from moon's own address, which that socket has never sent to; the second's asks moon's STUN server, and moon pokes it from that same address and another port. Neither needs anything from the other, so neither waits for the other. _sayFiltering reads which pokes arrived
async function _filtering(family) {
	let tests = await Promise.all([[1, stunServers[0]], [2, moonStun]].map(([test, server]) => _filterTest(family, test, server)))
	return tests.flat()//the first test's steps, then the second's
}

//one filtering test, on a socket opened for it alone and kept open across its steps: ask one STUN server from it, then have moon poke the public address and port the server saw, while the socket listens
async function _filterTest(family, test, server) {
	let port = 49152 + Math.floor(Math.random()*16384)//from the dynamic range, a new one each run, as the STUN check's
	let opened
	try {
		opened = await udpOpen(family, port)
	} catch (error) {//the bottom gate, as in _ask: a port already taken, or a family the machine has no address in
		return [{kind: 'filter-open', family, test, asked: `open UDP port ${port}`, success: false, outcome: String(error), milliseconds: 0}]
	}
	let stun = await _ask({kind: 'filter-stun', family, test, asked: `STUN ${server} from port ${port}, kept open`, ask: () => udpStun(opened.socket, [server], checkSeconds), read: _readBinding})
	let url = stun.success ? `https://ip${family}.moon.${urlHome}/poke?port=${stun.value.port}&address=${stun.value.address}&token=${Data({random: 4}).base16()}` : ''//the public address and port STUN saw, which moon pokes even when the request leaves from another address, written as it is, since a global address is digits, hex, dots, and colons, all of which a query takes as they are; and a token of our own, so a packet that arrives says which poke it came from
	let [poke, receive] = await Promise.all([//at once, so the socket is already listening when the poke's packets arrive
		_ask({kind: 'poke', family, test, asked: `GET ${url || `https://ip${family}.moon.${urlHome}/poke`}`, ask: () => url ? netGet(url, reportLimit, checkSeconds) : Promise.reject('no public port from STUN to poke'), read: _readPoke}),
		_ask({kind: 'receive', family, test, asked: `receive on port ${port}`, ask: () => url ? udpReceive(opened.socket, pokePackets, checkSeconds) : Promise.reject('no poke to wait for'), read: list => ({success: true, value: list})}),
	])
	udpClose(opened.socket)
	return [stun, poke, receive]
}

//what a held socket's STUN call answered for its one server, read like one server's binding from net_stun: the binding, with the local port, when the server answered, and its error when it didn't
function _readBinding(bindings) {
	let b = {...bindings.servers[0], local: bindings.local}
	if (b.error) return {success: false, outcome: b.error}
	return {success: true, value: b}
}

//moon's account of a poke, one JSON object, {to, token, packets, sent, from_port, bytes, error}: where it sent, the token the packets carry, how many it meant to send and did, the port they left from, each one's size, and what stopped it; a poke that sent nothing fails with moon's reason
function _readPoke(text) {
	let a = JSON.parse(text)
	if (a.sent == 0) return {success: false, outcome: `moon sent nothing${a.error ? `, ${a.error}` : ''}`}
	return {success: true, value: a}
}

//one hole punch, against the peer behind one of moon's two NATs, on a socket opened for it alone and kept open across its steps: ask our STUN server from it which public address and port it has; ask moon to punch that port, which hands it to the peer, and the peer answers its own public address and port and starts sending; then send to the peer while listening. Each step is a result for the box, and _sayPunch reads them
async function _punch([nat, family]) {
	let port = 49152 + Math.floor(Math.random()*16384)//from the dynamic range, a new one each run, as the STUN check's
	let opened
	try {
		opened = await udpOpen(family, port)
	} catch (error) {//the bottom gate, as in _ask
		return [{kind: 'punch-open', family, nat, asked: `open UDP port ${port}`, success: false, outcome: String(error), milliseconds: 0}]
	}
	let stun = await _ask({kind: 'punch-stun', family, nat, asked: `STUN ${stunServers[0]} from port ${port}, kept open`, ask: () => udpStun(opened.socket, [stunServers[0]], checkSeconds), read: _readBinding})//our server rather than moon's, so the socket hasn't sent to moon's address before the punch, as a peer meeting a stranger wouldn't have
	let token = Data({random: 4}).base16()//carried by both sides' packets, so one that arrives says which punch it belongs to
	let url = stun.success ? `https://ip${family}.moon.${urlHome}/punch?nat=${nat}&port=${stun.value.port}&address=${stun.value.address}&token=${token}` : ''//the public address and port STUN saw, where the peer sends even when the request leaves from another address
	let call = await _ask({kind: 'punch', family, nat, asked: `GET ${url || `https://ip${family}.moon.${urlHome}/punch?nat=${nat}`}`, ask: () => url ? netGet(url, reportLimit, punchCallSeconds) : Promise.reject('no public port from STUN to punch from'), read: _readPunch, seconds: punchCallSeconds})
	let exchange = await _ask({kind: 'punch-exchange', family, nat, asked: `punch ${call.success ? call.value.mapped : `moon's ${nat} peer`} from port ${port}`, ask: () => call.success ? _exchange(opened.socket, call.value.mapped, token) : Promise.reject('no peer to punch'), read: v => ({success: true, value: v}), seconds: punchSeconds + 1})
	udpClose(opened.socket)
	return [stun, call, exchange]
}

//moon's account of a punch, one JSON object, {nat, to, token, local_port, mapped, stun_milliseconds, packets, gap_milliseconds, error}: the NAT, where its peer sends, the token its packets carry, the port the peer bound inside its namespace and the public address and port our STUN server saw it at, and how many packets it sends how far apart; a peer that learned no public address sends nothing, and fails with moon's reason
function _readPunch(text) {
	let a = JSON.parse(text)
	if (!a.mapped) return {success: false, outcome: `moon's ${a.nat} peer sent nothing, ${a.error}`}
	return {success: true, value: a}
}

//the punch's two halves at once, for punchSeconds: send to the peer every punchGap, "punch <token> <n> heard <yes or no>", while listening in rounds of a second. A packet carrying the token is the peer's, which every packet after says was heard, and when it comes from somewhere other than where the panel is sending, that address joins the ones sent to. Answers how many packets went, to where, and every datagram that arrived, timed from the start, and a send's error when one failed
async function _exchange(socket, peer, token) {
	let start = performance.now()
	let targets = [peer]//where to send, the peer's public address and port first
	let heard = 'no'//whether a packet of the peer's has arrived, which every packet after says
	let sent = 0, failed = '', arrived = []
	let sending = true
	let sender = (async () => {//the sending half, on its own loop, so the receives below don't wait on it
		while (sending) {
			for (let to of targets) {
				sent++
				try { await udpSend(socket, to, Data({text: `punch ${token} ${sent} heard ${heard}`}).base16()) } catch (error) { failed = String(error) }//a send that fails is lost like any packet, and its error is kept for the box
			}
			await new Promise(resolve => setTimeout(resolve, punchGap))
		}
	})()
	try {
		for (let round = 0; round < punchSeconds; round++) {
			let began = performance.now() - start
			for (let d of await udpReceive(socket, 1000, 1)) {//a second of whatever arrives, past any count the peer could send
				arrived.push({...d, milliseconds: Math.round(began + d.milliseconds)})//timed from the punch's start rather than the round's
				if (!_datagramText(d).startsWith(`punch ${token} `)) continue//a STUN answer that came late, or anything else
				heard = 'yes'
				if (!targets.includes(d.from)) targets.push(d.from)
			}
		}
	} finally {//a receive that fails stops the sending too, rather than leaving it running for the life of the app
		sending = false
		await sender
	}
	return {sent, targets, arrived, failed}
}

//the inbound proof for one family, while a session listens: moon pings the engine's port with a DHT ping and with a uTP connection request, and dials it over TCP with a BitTorrent handshake carrying the chosen torrent's info-hash, or a random one with no torrent, all three at once, each a result for the box; _sayInbound reads them
function _inbound(family, port, infohash) {
	let base = `https://ip${family}.moon.${urlHome}`
	let probe = (kind, path, seconds) => ({kind, family, asked: `GET ${base}${path}`, ask: () => netGet(base + path, reportLimit, seconds), read: text => ({success: true, value: JSON.parse(text)}), seconds})//moon's report, whatever it found, is the answer; only the request failing is a failure
	return Promise.all([
		_ask(probe('ping-dht', `/ping?port=${port}&kind=dht`, pingSeconds)),
		_ask(probe('ping-utp', `/ping?port=${port}&kind=utp`, pingSeconds)),
		_ask(probe('dial', `/dial?port=${port}${infohash ? `&infohash=${infohash}` : ''}`, dialSeconds)),
	])
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

//a check of the router's external address and epoch by NAT-PMP, asked of the gateway once the first wave has named it, or failing at once when no IPv4 default route names one
function _natpmp(gateway) {
	let asked = gateway ? `NAT-PMP to ${gateway}:5351` : 'NAT-PMP to the gateway'
	let ask = () => gateway ? netNatpmp(gateway, checkSeconds) : Promise.reject('no IPv4 gateway to ask')
	return {kind: 'natpmp', asked, ask, read: answer => ({success: true, value: answer})}//net_natpmp answers {address, epoch}, or rejects with why
}

//a check of what the router answers PCP's ANNOUNCE, asked of the gateway beside NAT-PMP
function _pcp(gateway, family = 4, scope = 0, name = '') {
	let asked = gateway ? `PCP ANNOUNCE to ${_sayGatewayPort(gateway, family, name)}` : 'PCP ANNOUNCE to the gateway'
	let ask = () => gateway ? netPcpAnnounce(gateway, scope, checkSeconds) : Promise.reject(`no IPv${family} gateway to ask`)
	return {kind: 'pcp', family, asked, ask, read: answer => ({success: true, value: answer})}//net_pcp_announce answers {version, result, epoch, sends, bytes, answer} for any answer, PCP's or NAT-PMP's, or rejects with why
}

const upnpSearch = '239.255.255.250:1900'//the SSDP multicast address and port, where every UPnP device on the local network listens for a search

//a check that searches the local network for a router by UPnP, answering the gateway igd-next found, which the four reads below take back
function _upnpSearch() {
	return {kind: 'upnp', asked: `UPnP search to ${upnpSearch}`, ask: () => netUpnpSearch(checkSeconds), read: gateway => ({success: true, value: gateway})}
}

//the four UPnP reads of a gateway the search found: its device description, its external address, its port mappings, and its IPv6 firewall's status, each failing at once when there's no gateway to ask, and the last when the gateway offers no IPv6 firewall control service, so the box has the same lines whatever the router
function _upnpReads(gateway) {
	let none = () => Promise.reject('no gateway found by the search')//made only when asked, so a rejection nobody takes never exists
	let where = gateway ? ` at ${gateway.address}` : ''
	return [
		{kind: 'upnp-description', asked: gateway ? `GET http://${gateway.address}${gateway.root}` : 'GET the UPnP device description', ask: () => gateway ? netUpnpDescription(gateway, descriptionLimit, checkSeconds) : none(), read: _readDevice},
		{kind: 'upnp-external', asked: `UPnP GetExternalIPAddress${where}`, ask: () => gateway ? netUpnpExternal(gateway) : none(), read: address => _readAddress(4, address)},
		{kind: 'upnp-mappings', asked: `UPnP GetGenericPortMappingEntry${where}`, ask: () => gateway ? netUpnpMappings(gateway) : none(), read: found => ({success: true, value: found})},
		{kind: 'upnp-firewall', asked: `UPnP GetFirewallStatus${where}`, ask: () => !gateway ? none() : gateway.firewall ? netUpnpFirewall(gateway) : Promise.reject('the router offers no WANIPv6FirewallControl service'), read: status => ({success: true, value: status})},
	]
}

//a UPnP device description, read for the root device's own fields, each blank where the description leaves it out: its friendly name, maker, model name, number, and description, and the address of its settings page; the devices nested inside it, like the WAN connection device, are passed over
function _readDevice(text) {
	let doc = new DOMParser().parseFromString(text, 'text/xml')
	let device = [...doc.documentElement.children].find(e => e.localName == 'device')//the root device, a direct child of the document's root element
	if (!device || doc.getElementsByTagName('parsererror').length) return {success: false, outcome: `not a device description: ${text.slice(0, 60)}`}
	let field = name => [...device.children].find(e => e.localName == name)?.textContent.trim() || ''//a direct child only, so a nested device's fields don't stand in for the root's
	return {success: true, value: {friendly: field('friendlyName'), manufacturer: field('manufacturer'), model: field('modelName'), number: field('modelNumber'), description: field('modelDescription'), url: field('presentationURL')}}
}

//a router in a few words from its description, like NETGEAR R7000 or ASUS RT-AX88U: its maker and model, a word the description repeats said once, and its friendly name where it gives no model; with details, the description's fields after that
function _sayDevice(d, details) {
	let words = [d.manufacturer, d.model || d.friendly, d.number].filter(Boolean).filter((word, i, all) => all.indexOf(word) == i)
	let said = words.join(' ') || 'unnamed'
	if (details) said += `; friendly name "${d.friendly}", model "${d.model}", number "${d.number}", description "${d.description}"${d.url ? `, settings page ${d.url}` : ''}`
	return said
}

//PCP's result codes, RFC 6887, each with its name and a few words; a NAT-PMP answer carries its own, where 1 is also unsupported version
const pcpResults = {
	0:  ['SUCCESS',                 'the router speaks PCP'],
	1:  ['UNSUPP_VERSION',          'the router speaks an older version'],
	2:  ['NOT_AUTHORIZED',          'the router refuses this client'],
	3:  ['MALFORMED_REQUEST',       `the router couldn't read the request`],
	4:  ['UNSUPP_OPCODE',           `the router doesn't do this operation`],
	5:  ['UNSUPP_OPTION',           `the router doesn't know an option sent`],
	6:  ['MALFORMED_OPTION',        `the router couldn't read an option sent`],
	7:  ['NETWORK_FAILURE',         'the router has no working connection'],
	8:  ['NO_RESOURCES',            'the router is out of room'],
	9:  ['UNSUPP_PROTOCOL',         `the router doesn't map this transport protocol`],
	10: ['USER_EX_QUOTA',           'this client has used its share of mappings'],
	11: ['CANNOT_PROVIDE_EXTERNAL', `the router can't give the external port or address asked for`],
	12: ['ADDRESS_MISMATCH',        `the address in the request isn't the packet's source`],
	13: ['EXCESSIVE_REMOTE_PEERS',  'too many remote peers in one request'],
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

//ask every check once, and answer the run: when it started, this copy's version, each check's result in the order the box lists them, and the machine's interfaces and its firewall put together from them
export async function connectionRun() {
	let moment = Date.now()
	let filtering = Promise.all([4, 6].map(_filtering))//each family's two tests, on sockets of their own, beside every other check from the start, since a poke that never arrives waits its whole four seconds
	let punching = Promise.all(punches.map(_punch))//and the three punches, which take five seconds of sending after moon answers
	let probing = session.running ? Promise.all([4, 6].map(family => _inbound(family, session.port, session.torrent?.v1))) : null//and with a session listening, moon's probes of its port, also from the start
	let [version, paths, engine] = await Promise.all([getVersion(), pathsStatus(), engineStatus()])//this copy's version, and where this copy and its engine are, which the firewall lists programs by
	let results = (await Promise.all(_checks(paths, engine).map(_ask))).flatMap(_perServer)//all at once; Promise.all keeps the order it was given, whatever order they finish in
	await Promise.all(results.filter(r => r.binding?.to).map(async r => { r.source = await _source(r.binding.to) }))//then the address this machine sends from toward each STUN server whose name led somewhere
	let machine = _machine(results)
	let gateway = _gateway(4, machine)
	results.push(...await Promise.all([_natpmp(gateway), _pcp(gateway), _upnpSearch()].map(_ask)))//then the router, at the gateway the tools named for the two on port 5351, and by search for UPnP, all three at once
	let found = results.find(r => r.kind == 'upnp')
	results.push(...await Promise.all(_upnpReads(found.success ? found.value : null).map(_ask)))//then the reads of the gateway the search found
	results.push(...(await filtering).flat())//and last the filtering, IPv4's two tests and then IPv6's
	results.push(...(await punching).flat())//and the punches, the cone's steps and then the symmetric's
	let inbound = false//whether the inbound proof ran, which takes a session listening
	let probed = probing ? {port: session.port, mapping: session.mapping} : null//and the session it probed, at the moment the probes went out: its port, and whether its port mappers were on
	if (probing) {
		results.push(...(await probing).flat())//and the inbound proof, IPv4's three probes and then IPv6's
		await new Promise(resolve => setTimeout(resolve, pollEvery + Time.second/4))//one more poll of the engine, so the alerts the probes caused are in before they're counted
		inbound = true
	}
	return {moment, version, paths, engine, results, machine, inbound, probed, arrivals: arrivals.filter(a => a.moment >= moment), firewall: _firewall(results, paths, engine)}//the connections in since the run began, which the inbound proof looks for moon's among
}

//the address the system would send from toward a server's address and port, like [2606:4700:49::]:3478, as {success, value or outcome}
async function _source(to) {
	try {
		return {success: true, value: await netLocal(_addressOf(to))}
	} catch (error) {//the bottom gate, as in _ask
		return {success: false, outcome: String(error)}
	}
}

//one check, timed, as what the check says about itself, like its kind and what it asked, then {success, value or outcome, milliseconds}
async function _ask(check) {
	let {ask, read, seconds = checkSeconds, ...about} = check//a check that waits longer by design names its own seconds
	let start = performance.now()
	let v
	try {
		let answer = await Promise.race([ask(), _late(seconds)])
		if (answer === late) v = {success: false, outcome: _sayNoAnswer(seconds)}//strict, since the answer may be text, an object, or this
		else v = read(answer)
	} catch (error) {//the bottom gate: rust's refusal becomes an ordinary answer, with its text kept for the box
		v = {success: false, outcome: String(error)}
	}
	let milliseconds = Math.round(performance.now() - start)
	if (!v.success && milliseconds >= seconds*Time.second) v.outcome = _sayNoAnswer(seconds)//rust gave up at its seconds, whatever words it chose for it; the clock here started first, so it reads at least as long
	return {...about, ...v, milliseconds, again: () => _ask(check)}//the same check asked once more, which a check that stands on its own can be
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

//what a check's answer is when it hasn't come a quarter second past its seconds: rust gives up at those seconds itself, so this only ever wins when a call stalls before its own clock starts, as a stuck name lookup would, and it keeps the run from waiting on it
const late = Symbol('late')
function _late(seconds) { return new Promise(resolve => setTimeout(() => resolve(late), seconds*Time.second + Time.second/4)) }

//no answer in some seconds, the words noAnswer is for checkSeconds
function _sayNoAnswer(seconds) { return `no answer in ${seconds} seconds` }

//the kinds of check a Check can ask again on their own, since each stands alone: the ones its first wave asks, and the router's, which wait for the gateway the first wave names. A step of the filtering, the punches, or the inbound proof belongs to a sequence on one socket, and asking one again alone would mean nothing
const standalone = ['route', 'reflect', 'stun', 'interfaces', 'tool', 'resolve', 'firewall-profiles', 'firewall-rules', 'natpmp', 'pcp', 'upnp']
const retryAt = [10*Time.second, 20*Time.second]//after a Check, when to ask again what heard nothing, if the window hasn't come forward first, and then no more
let retryNext = null//the latest Check's next retry, which the window coming forward takes first
let retryTimers = []

//after a Check whose standalone checks heard nothing, arm the cadence that waits out a system box: ask them again when the window comes forward, since answering a box gives focus back, and on timers ten and twenty seconds on, at most twice in all, and stop once nothing is left unheard
export function connectionRetries() {
	_watchFocus()
	retryTimers.forEach(clearTimeout)
	retryTimers = []
	retryNext = null
	let moment = lastRun.value?.moment
	if (!moment || !_unheard(lastRun.value).length) return
	let left = retryAt.length
	let go = async when => {
		if (left == 0 || lastRun.value?.moment != moment) return//spent, or a newer Check has replaced this one
		left--
		retryNext = left > 0 ? go : null
		await _retry(when)
		if (!_unheard(lastRun.value).length) { retryTimers.forEach(clearTimeout); retryNext = null }//everything answered, so the rest are needless
	}
	retryNext = go
	retryTimers = retryAt.map(after => setTimeout(() => go(`${after/Time.second} seconds after the Check`), after))
}

//the latest Check's standalone checks that ended in no answer, each with where it sits in the run
function _unheard(run) {
	return run.results.map((r, i) => [r, i]).filter(([r]) => standalone.includes(r.kind) && r.outcome == noAnswer && r.again)
}

//ask the unheard checks again, as a run of its own in the box, and fold what they answer into the latest Check, so the table shows it; a UPnP search that finds the router now has its four reads follow, since they waited on it
async function _retry(when) {
	let run = lastRun.value
	let unheard = _unheard(run)
	if (!unheard.length) return
	let moment = Date.now()
	let again = await Promise.all(unheard.map(([r]) => r.again()))
	let results = [...run.results]
	unheard.forEach(([r, i], n) => { results[i] = again[n] })
	let search = again.find(r => r.kind == 'upnp')
	let reads = []
	if (search?.success) {
		reads = await Promise.all(_upnpReads(search.value).map(_ask))
		for (let read of reads) { let i = results.findIndex(r => r.kind == read.kind); if (i >= 0) results[i] = read }
	}
	if (lastRun.value?.moment != run.moment) return//a newer Check replaced this one meanwhile, and its answers stand
	lastRun.value = {...run, results, machine: _machine(results), firewall: _firewall(results, run.paths, run.engine)}
	let lines = [`${sayMoment(moment)}, asking again ${when} what heard nothing in the Check of ${sayMoment(run.moment)}`]
	let width = Math.max(...[...again, ...reads].map(r => r.asked.length))
	for (let r of [...again, ...reads]) lines.push(..._sayResult(r, width))
	boxAdd(lines.join('\n'), 'run')
}

//read the firewall again whenever the window comes forward, since the person may have changed it in System Settings or Windows Settings while it was away, and say so in the box when its standing or what it admits has changed
async function _rereadFirewall() {
	let run = lastRun.value
	if (!run) return
	let reads = run.results.map((r, i) => [r, i]).filter(([r]) => r.again && (r.kind.startsWith('firewall-') || (r.kind == 'tool' && firewallTools.includes(r.tool))))
	if (!reads.length) return
	let again = await Promise.all(reads.map(([r]) => r.again()))
	if (lastRun.value?.moment != run.moment) return//a newer Check read it meanwhile
	let results = [...lastRun.value.results]
	reads.forEach(([r, i], n) => { results[i] = again[n] })
	let firewall = _firewall(results, run.paths, run.engine)
	let before = `${_sayFirewall(lastRun.value.firewall)}; ${_sayFirewallList(lastRun.value.firewall)}`
	let after = `${_sayFirewall(firewall)}; ${_sayFirewallList(firewall)}`
	lastRun.value = {...lastRun.value, results, firewall}
	if (after != before) boxAdd(`${sayMoment(Date.now())}, the firewall changed while the window was away: ${after}`, 'session')
}

//hear the window coming forward, once: the unheard checks' next retry goes first, then the panel reads the firewall again
let watchingFocus = false
async function _watchFocus() {
	if (watchingFocus) return
	watchingFocus = true
	await getCurrentWindow().onFocusChanged(async ({payload: focused}) => {
		if (!focused || firewallChanging) return//the UAC prompt gives focus back as netsh starts, so a read then could catch the rules half changed; Write firewall rules… reads them itself once netsh is done
		if (retryNext) await retryNext('when the window came forward')
		await _rereadFirewall()
	})
}

//the mapping run, which Try mapping starts: this machine's address and gateway in each family, the public IPv4 address a reflector sees, and the router by UPnP's search; then the router's external address by UPnP and by NAT-PMP, and PCP's ANNOUNCE to the IPv6 gateway; then a trial for each of the five combinations of protocol and family, all side by side. Each trial changes the router for a minute and undoes it, which is why the run waits for the person to ask
export async function mappingRun() {
	let moment = Date.now()
	let version = await getVersion()
	let first = await Promise.all([_route(4), _route(6), _reflect(4), _interfaces(), ...tools.filter(t => t.tool.startsWith('gateway')).map(_tool), _upnpSearch()].map(_ask))//the address the system sends from in each family, which a mapping names inside; the interfaces and the tools that name each family's gateway, which a Check reads the same way; and the router by UPnP
	let [route4, route6] = [4, 6].map(family => first.find(r => r.kind == 'route' && r.family == family))
	let reflect4 = first.find(r => r.kind == 'reflect'), search = first.find(r => r.kind == 'upnp')
	let machine = _machine(first)
	let gateway4 = _gateway(4, machine), gateway6 = _gateway(6, machine)
	let link = machine.find(i => i.routes.includes(6))//the interface carrying IPv6's default route, which the link-local gateway is on
	let scope = link?.index ?? 0, name = link?.name ?? ''
	let gateway = search.success ? search.value : null
	let second = await Promise.all([
		{kind: 'upnp-external', asked: `UPnP GetExternalIPAddress${gateway ? ` at ${gateway.address}` : ''}`, ask: () => gateway ? netUpnpExternal(gateway) : Promise.reject('no UPnP gateway to ask'), read: address => _readAddress(4, address)},
		_natpmp(gateway4),
		_pcp(gateway6, 6, scope, name),
	].map(_ask))//the router's external address by UPnP and by NAT-PMP, where moon reaches a mapping over IPv4, and whether the router's IPv6 side speaks PCP before the run asks it for anything
	let [external, natpmp] = second
	let public4 = reflect4.success ? reflect4.value : ''//where moon pokes over IPv4 when a protocol doesn't say, which is the address the internet sees
	let own6 = route6.success ? route6.value : ''//and over IPv6, this machine's own address, the one a pinhole opens to
	let trials = await Promise.all([
		_trial({combination: 'upnp4',   family: 4, address: external.success ? external.value : public4,       ..._upnpPort(gateway, route4)}),
		_trial({combination: 'natpmp4', family: 4, address: natpmp.success ? natpmp.value.address : public4,   ..._natpmpPort(gateway4, natpmp)}),
		_trial({combination: 'pcp4',    family: 4, address: public4,                                           ..._pcpMap(gateway4, 4, 0, '', '0.0.0.0', '0.0.0.0')}),
		_trial({combination: 'upnp6',   family: 6, address: own6,                                              ..._upnpPinhole(gateway, route6)}),
		_trial({combination: 'pcp6',    family: 6, address: own6,                                              ..._pcpMap(gateway6, 6, scope, name, own6, '::')}),
	])
	return {moment, version, results: [...first, ...second, ...trials.flat()]}
}

//UPnP's mapping over IPv4, as a trial takes it: AddPortMapping of the socket's port at the same port outside, to this machine's address, and DeletePortMapping. A router that takes only mappings with no lease answers error 725, which igd-next words as permanent tests for, and the trial asks again with no lease, which only the delete removes
function _upnpPort(gateway, route) {
	let internal = port => route.success ? `${route.value}:${port}` : ''
	return {
		add: (port, lease) => ({kind: 'map-add', asked: `UPnP AddPortMapping UDP ${port} to ${internal(port) || 'this machine'} ${lease > 0 ? `for ${lease} seconds` : 'with no lease'}`, ask: () => {
			if (!gateway) return Promise.reject('no UPnP gateway to ask')
			if (!route.success) return Promise.reject('no IPv4 address of this machine to map to')
			return netUpnpAddPort(gateway, 'UDP', port, internal(port), lease, mappingDescription).then(() => ({internal: port, port, lease}))
		}, read: v => ({success: true, value: v})}),
		permanent: outcome => /only supports permanent leases/.test(outcome),
		remove: grant => ({kind: 'map-remove', asked: `UPnP DeletePortMapping UDP ${grant?.port ?? ''}`.trimEnd(), ask: () => grant ? netUpnpRemovePort(gateway, 'UDP', grant.port) : Promise.reject('nothing granted to delete'), read: () => ({success: true, value: 'deleted'})}),
	}
}

//UPnP's pinhole over IPv6, as a trial takes it: AddPinhole to this machine's address and the socket's port, from any remote host and port, and DeletePinhole by the number the router gave it; igd-next refuses before sending anything when the router offers no IPv6 firewall control service
function _upnpPinhole(gateway, route) {
	let internal = port => route.success ? `[${route.value}]:${port}` : ''
	return {
		add: (port, lease) => ({kind: 'pinhole-add', asked: `UPnP AddPinhole UDP ${internal(port) || 'to this machine'} for ${lease} seconds`, ask: () => {
			if (!gateway) return Promise.reject('no UPnP gateway to ask')
			if (!route.success) return Promise.reject('no IPv6 address of this machine to open a pinhole to')
			return netUpnpAddPinhole(gateway, 'UDP', internal(port), lease).then(id => ({internal: port, port, lease, id}))
		}, read: v => ({success: true, value: v})}),
		remove: grant => ({kind: 'pinhole-remove', asked: `UPnP DeletePinhole ${grant?.id ?? ''}`.trimEnd(), ask: () => grant ? netUpnpRemovePinhole(gateway, grant.id) : Promise.reject('nothing granted to delete'), read: () => ({success: true, value: 'deleted'})}),
	}
}

//NAT-PMP's mapping over IPv4, as a trial takes it: the natpmp crate's mapping request for the socket's port, suggesting the same port outside, and the same request with a lifetime and external port of nothing, which deletes it. Its answer names no address, so the grant takes the one NAT-PMP's address request answered
function _natpmpPort(gateway, natpmp) {
	let at = gateway ? ` at ${gateway}:5351` : ''
	return {
		add: (port, lease) => ({kind: 'natpmp-add', asked: `NAT-PMP map UDP ${port} for ${lease} seconds${at}`, ask: () => gateway ? netNatpmpMap(gateway, 'UDP', port, port, lease, checkSeconds) : Promise.reject('no IPv4 gateway to ask'), read: m => ({success: true, value: {internal: m.internal, port: m.external, lease: m.lifetime, epoch: m.epoch, address: natpmp.success ? natpmp.value.address : ''}})}),
		remove: grant => ({kind: 'natpmp-remove', asked: grant ? `NAT-PMP map UDP ${grant.internal} for 0 seconds${at}, the delete` : 'NAT-PMP delete', ask: () => grant ? netNatpmpMap(gateway, 'UDP', grant.internal, 0, 0, checkSeconds) : Promise.reject('nothing granted to delete'), read: () => ({success: true, value: 'deleted'})}),
	}
}

//PCP's MAP, as a trial takes it, over either family: a request for the socket's port from this machine's address from, suggesting the same port outside and suggested, zeros for no preference, carrying a nonce made for the trial, and the same request with a lifetime of nothing, which deletes it. Over IPv6 it goes to the link-local gateway on the interface scope numbers, from the machine's own global address, since the request's source is the address the pinhole opens to
function _pcpMap(gateway, family, scope, name, from, suggested) {
	let nonce = Data({random: 12}).base16()//the router keys the mapping on it, so the delete carries the same one
	let to = gateway ? _sayGatewayPort(gateway, family, name) : 'the gateway'
	let why = ''
	if (!gateway) why = `no IPv${family} gateway to ask`
	else if (!from) why = `no IPv${family} address of this machine to map to`
	return {
		add: (port, lease) => ({kind: 'pcp-add', asked: `PCP MAP UDP ${port} for ${lease} seconds to ${to}`, ask: () => why ? Promise.reject(why) : netPcpMap(gateway, scope, from, 'UDP', port, port, suggested, lease, nonce, checkSeconds), read: _readPcpMap}),
		remove: grant => ({kind: 'pcp-remove', asked: grant ? `PCP MAP UDP ${grant.internal} for 0 seconds to ${to}, the delete` : 'PCP delete', ask: () => grant ? netPcpMap(gateway, scope, from, 'UDP', grant.internal, 0, suggested, 0, nonce, checkSeconds) : Promise.reject('nothing granted to delete'), read: m => { let v = _readPcpMap(m); return v.success ? {success: true, value: 'deleted'} : v }}),
	}
}

//what a router answered PCP's MAP, as a grant: success with what it assigned, or a refusal in PCP's words with the whole answer, or NAT-PMP's answer that it speaks only NAT-PMP
function _readPcpMap(m) {
	if (m.version != 2) return {success: false, outcome: `NAT-PMP answered, result ${m.result}${m.result == 1 ? ', unsupported version, so the router speaks NAT-PMP only' : ''}`}
	if (m.result != 0) return {success: false, outcome: `refused, result ${_sayPcpResult(m.result)}; ${saySize4(m.bytes)}: ${_sayHex(m.answer)}`}
	return {success: true, value: {internal: m.internal, port: m.external, lease: m.lifetime, epoch: m.epoch, address: m.address, bytes: m.bytes, answer: m.answer}}
}

//one trial of one combination, on a socket opened for it alone and kept open throughout: moon pokes the address and port a mapping would forward before the trial asks for anything, the request goes to the router, moon pokes the port it granted, the delete goes, and moon pokes a last time, so the three pokes are the before, the during, and the after. Each step is a result for the box, and _sayTrial reads them
async function _trial(t) {
	let port = 49152 + Math.floor(Math.random()*16384)//from the dynamic range, a new one each run, as the STUN check's
	let about = {combination: t.combination, family: t.family, port}
	let opened
	try {
		opened = await udpOpen(t.family, port)
	} catch (error) {//the bottom gate, as in _ask
		return [{...about, kind: 'trial-open', asked: `open UDP port ${port}`, success: false, outcome: String(error), milliseconds: 0}]
	}
	let steps = await _trialPoke(about, 'before', opened.socket, t.address, port, 'no address to poke')
	let adds = [await _ask({...about, step: 'add', ...t.add(port, mappingLease)})]
	if (!adds[0].success && t.permanent?.(adds[0].outcome)) adds.push(await _ask({...about, step: 'add', ...t.add(port, 0)}))//a router that takes only mappings with no lease, asked again with none, so the trial still proves the mapping and deletes it
	let grant = adds.at(-1).success ? adds.at(-1).value : null
	let target = grant?.address && !['0.0.0.0', '::'].includes(grant.address) ? grant.address : t.address//the external address the grant names, where a protocol's answer names one, and otherwise the one the trial was given
	steps.push(...adds, ...await _trialPoke(about, 'granted', opened.socket, target, grant?.port, 'nothing granted to poke through'))
	steps.push(await _ask({...about, step: 'remove', ...t.remove(grant)}), ...await _trialPoke(about, 'released', opened.socket, target, grant?.port, 'nothing granted to poke through'))
	udpClose(opened.socket)
	return steps
}

//moon's poke at one address and port at one stage of a trial, while the trial's socket listens, both results for the box; with no address or no port to poke, both fail with why, so the box has the same lines whatever happened
async function _trialPoke(about, stage, socket, address, port, why) {
	let token = Data({random: 4}).base16()//so a packet that arrives says which poke it came from
	let url = address && port ? `https://ip${about.family}.moon.${urlHome}/poke?port=${port}&address=${address}&token=${token}` : ''
	return Promise.all([//at once, so the socket is already listening when the poke's packets arrive
		_ask({...about, stage, token, kind: 'poke', asked: `GET ${url || `https://ip${about.family}.moon.${urlHome}/poke`}`, ask: () => url ? netGet(url, reportLimit, checkSeconds) : Promise.reject(why), read: _readPoke}),
		_ask({...about, stage, token, kind: 'receive', asked: `receive on port ${about.port}`, ask: () => url ? udpReceive(socket, pokePackets, checkSeconds) : Promise.reject(why), read: list => ({success: true, value: list})}),
	])
}

//the table's rows for the mapping run, one for each combination, blank before the first run
export function mappingFacts(run) {
	return [
		['UPnP over IPv4',    run ? _sayTrial(run, 'upnp4')   : ''],
		['NAT-PMP over IPv4', run ? _sayTrial(run, 'natpmp4') : ''],
		['PCP over IPv4',     run ? _sayTrial(run, 'pcp4')    : ''],
		['UPnP over IPv6',    run ? _sayTrial(run, 'upnp6')   : ''],
		['PCP over IPv6',     run ? _sayTrial(run, 'pcp6')    : ''],
	]
}

//what one trial found, in the four answers each combination takes: whether the router answered the request, what it granted, whether moon's poke arrived through the grant and was held back before it, and whether the delete took it away, which the last poke being held back again shows
function _sayTrial(run, combination) {
	let steps = run.results.filter(r => r.combination == combination)
	let open = steps.find(r => r.kind == 'trial-open')
	if (open) return `unknown, ${open.outcome}`
	let at = (kind, stage) => steps.find(r => r.kind == kind && r.stage == stage)
	let arrived = stage => { let r = at('receive', stage); return r.success && r.value.some(d => _datagramText(d).startsWith(`poke ${r.token}`)) }//moon's packets, by the token this poke carried
	let before = `not sent with nothing asked, ${at('poke', 'before').outcome}`
	if (at('poke', 'before').success) before = arrived('before') ? 'arrived with nothing asked' : 'held back with nothing asked'
	let adds = steps.filter(r => r.step == 'add')
	let add = adds.at(-1)
	if (!add.success) return `not granted, ${add.outcome}; moon's poke ${before}`
	let g = add.value
	let granted = `granted UDP ${g.internal}`
	if (g.port != g.internal) granted += ` at external port ${g.port}`//a router may assign another port than the one suggested
	granted += g.lease > 0 ? ` for ${g.lease} seconds` : ' with no lease'
	if (adds.length > 1) granted += `, after the router refused a lease of ${mappingLease} seconds`
	let said = [granted]
	let during = at('poke', 'granted')
	if (!during.success) said.push(`unknown whether it forwards, ${during.outcome}`)
	else said.push(arrived('granted') ? `moon's poke arrived through it, and was ${before}` : `moon's poke didn't arrive through it, and was ${before}`)
	let remove = steps.find(r => r.step == 'remove')
	if (!remove.success) said.push(`the delete failed, ${remove.outcome}`)
	else if (at('poke', 'released').success) said.push(arrived('released') ? 'deleted, and moon\'s poke still arrives' : 'deleted, and moon\'s poke is held back again')
	else said.push('deleted')
	return said.join('; ')
}

//the box's lines for one mapping run: when, which build, and which platform, then a line per step as a Check's run has them
export function sayMappingRun(run) {
	let width = Math.max(...run.results.map(r => r.asked.length))
	let lines = [`${sayMoment(run.moment)}, ${brandName} ${run.version} on ${platformName}, asking the router for mappings`]
	for (let r of run.results) lines.push(..._sayResult(r, width))
	return lines.join('\n')
}

export const lastMappingRun = shallowRef(null)//the latest mapping run, kept here like the last Check, null until the first

//the table's rows for a run, eight for each family, five about the machine, and five about the router, blank before the first run
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
			[`IPv${family} filtering`,       run ? _sayFiltering(run, family)       : ''],
		)
		rows.push([`IPv${family} hole punch`, run ? _sayPunch(run, family) : ''])
		rows.push(
			[`IPv${family} inbound`,         run ? _sayInbound(run, family)         : ''],
			[`IPv${family} gateway`,         run ? _sayGateway(family, run.machine) : ''],
		)
	}
	rows.push(
		['Interfaces',           run ? _sayCount(run.machine)         : ''],
		['DNS64',                run ? _sayDns64(run.results)         : ''],
		['Shared address space', run ? _sayShared(run.machine)        : ''],
		['Firewall',             run ? _sayFirewall(run.firewall)     : ''],
		[firewallListName,       run ? _sayFirewallList(run.firewall) : ''],
		['NAT-PMP',              run ? _sayNatpmp(run.results)        : ''],
		['PCP',                  run ? _sayPcp(run.results)           : ''],
		['UPnP',                 run ? _sayUpnp(run.results)          : ''],
		['Router speaks',        run ? _saySpeaks(run)                : ''],
		['Carrier-grade NAT',    run ? _sayCarrier(run)               : ''],
	)
	return rows
}

//which of the three protocols the router speaks, read from the three asks: UPnP from the search, with the firewall named when it may have held the answer back, and NAT-PMP and PCP from what came back on port 5351, where NAT-PMP's own answer to PCP's request counts as NAT-PMP too
function _saySpeaks(run) {
	let result = kind => run.results.find(r => r.kind == kind)
	let natpmp = result('natpmp'), pcp = result('pcp'), search = result('upnp'), f = run.firewall
	let said = []
	if (search.success) said.push(`UPnP IGD version ${search.value.service.split(':').pop()}`)
	else if (search.outcome == noAnswer && f.known && f.on == 'on' && !f.ftorrent.listed) said.push(`no answer to the UPnP search, which the firewall may have held back, since ${brandName} isn't on its list`)
	else said.push(`no UPnP, ${search.outcome == noAnswer ? 'no answer to the search' : search.outcome}`)
	let port = []//what answered on port 5351
	if (pcp.success && pcp.value.version == 2) port.push('PCP')
	if (natpmp.success || (pcp.success && pcp.value.version == 0)) port.push('NAT-PMP')
	if (port.length) said.push(port.join(' and '))
	else if (natpmp.outcome == noAnswer && pcp.outcome == noAnswer) said.push('no answer on port 5351')
	else said.push(`not NAT-PMP or PCP, ${natpmp.outcome}`)
	return said.join('; ')
}

//whether a carrier translates above the router, as a likelihood with its reasons: the router's external address against the one the internet sees, and what kind of address it is; the shared address space on the default route; and the mapping verdict
function _sayCarrier(run) {
	let result = (kind, family) => run.results.find(r => r.kind == kind && (!family || r.family == family))
	let reflect = result('reflect', 4), natpmp = result('natpmp'), external = result('upnp-external')
	let router = natpmp.success ? natpmp.value.address : external.success ? external.value : ''//the external address as the router reports it, by either protocol that answered
	let reasons = []
	let likely = ''//likely, possible, or unlikely, and blank until a reason settles one
	if (!router) reasons.push('the router reported no external address')
	else if (!reflect.success) reasons.push(`the reflector saw no IPv4 address to set the router's ${router} against`)
	else if (router == reflect.value) { likely = 'unlikely'; reasons.push(`the router's external address ${router} is the one the internet sees`) }
	else {
		reasons.push(`the router's external address ${router} isn't the one the internet sees, ${reflect.value}, so something translates again above the router`)
		let kind = _sayKind(router)
		if (kind == 'shared address space') { likely = 'likely'; reasons.push(`and it's in the shared address space, 100.64.0.0/10, which carriers use behind their translators`) }
		else if (kind == 'private') { likely = 'possible'; reasons.push(`and it's private, which fits a second router, like a provider's modem, as well as a carrier using private addresses`) }
		else likely = 'likely'
	}
	if (run.machine.some(i => i.routes.includes(4) && i.addresses.some(a => a.shared))) { likely = 'likely'; reasons.push('an address on the default route is in the shared address space') }
	if (_sayMapping(run.results.filter(r => r.kind == 'stun' && r.family == 4)).startsWith('endpoint-dependent')) reasons.push(`the IPv4 mapping is endpoint-dependent, which fits a carrier's translator, though some home routers do the same`)
	return `${likely || 'unknown'}: ${reasons.join('; ')}`
}

//what the router said to UPnP, for the table: the service and its version at the gateway's address, with the router's maker and model from its description in parentheses, the way an interface's name follows an address; then its external address, how many mappings it holds, and its IPv6 firewall, or why the search found nothing
function _sayUpnp(results) {
	let result = kind => results.find(r => r.kind == kind)
	let search = result('upnp')
	if (!search.success) return search.outcome == noAnswer ? 'no answer' : `none, ${search.outcome}`
	let description = result('upnp-description'), external = result('upnp-external'), mappings = result('upnp-mappings'), firewall = result('upnp-firewall')
	let said = [`${_sayService(search.value.service)} at ${search.value.address}${description.success ? ` (${_sayDevice(description.value)})` : ''}`]
	said.push(external.success ? `external address ${external.value}` : `external address ${_sayFact(external)}`)
	said.push(mappings.success ? `${mappings.value.mappings.length} mappings` : `mappings ${_sayFact(mappings)}`)
	said.push(!search.value.firewall ? 'no IPv6 firewall control' : firewall.success ? `IPv6 firewall ${firewall.value.enabled ? 'on' : 'off'}, pinholes ${firewall.value.pinholes ? 'allowed' : 'not allowed'}` : `IPv6 firewall ${_sayFact(firewall)}`)
	return said.join(', ')
}

//a UPnP service type by its name and version, like WANIPConnection:1 from urn:schemas-upnp-org:service:WANIPConnection:1
function _sayService(type) {
	return type.split(':').slice(-2).join(':')
}

//what the router said to PCP's ANNOUNCE, for the table: a PCP answer by its result code's words and the epoch, NAT-PMP's answer that the version is unsupported, or why there's no answer
function _sayPcp(results) {
	let r = results.find(r => r.kind == 'pcp')
	if (!r.success) return r.outcome == noAnswer ? 'no answer' : `none, ${r.outcome}`
	let v = r.value
	if (v.version == 0) return `none, the router answered in NAT-PMP's form that version 2 is unsupported, so it speaks NAT-PMP only; epoch ${_sayLifetime(v.epoch)}`
	return `${_sayPcpResult(v.result)}; epoch ${_sayLifetime(v.epoch)}`
}

//a PCP result code with its name and a few words, like 0 SUCCESS, the router speaks PCP
//a gateway's port 5351 as the box writes it, an IPv6 one in brackets with the interface it's on, like [fe80::1%en1]:5351
function _sayGatewayPort(gateway, family, name) {
	if (family == 6) return `[${gateway}${name ? `%${name}` : ''}]:5351`
	return `${gateway}:5351`
}

function _sayPcpResult(code) {
	let [name, note] = pcpResults[code] || ['unknown', 'not a code this panel knows']
	return `${code} ${name}, ${note}`
}

//what the router said to NAT-PMP, for the table: its external address and how long since its mapping table started, or why there's no answer
function _sayNatpmp(results) {
	let r = results.find(r => r.kind == 'natpmp')
	if (!r.success) return r.outcome == noAnswer ? 'no answer' : `none, ${r.outcome}`
	return `external address ${r.value.address}, epoch ${_sayLifetime(r.value.epoch)}`
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

//whether the STUN servers that answered saw one public port for the one local port, which takes two answers at least, and says how many answered when one didn't
function _sayMapping(stuns) {
	let answered = stuns.filter(r => r.success)
	if (answered.length < 2) return 'unknown'
	let [a] = answered.map(r => r.value)//the first answer, which every other is set against
	let said = 'endpoint-independent'
	if (answered.some(r => r.value.port != a.port || !_sameNetwork(r.family, r.value.address, a.address))) said = 'endpoint-dependent'
	else if (a.port == a.local) said = 'endpoint-independent, port kept'//the router left the port as the machine chose it, or nothing translates at all
	if (answered.length < stuns.length) said += `, by ${answered.length} of ${stuns.length} servers`
	return said
}

//which of moon's two pokes reached its socket, read as the filtering, RFC 4787's three kinds. The first came from an address its socket had never sent to, so its arriving is endpoint-independent filtering. The second came from the address its socket had just sent to, at another port, so its being held back is address-and-port-dependent, and its arriving where the first didn't is address-dependent. A poke moon sent somewhere other than where STUN saw the socket says nothing about the socket, and the reading says where it went; and a firewall on this machine filters too, so the reading names it when it may be what held a poke back
function _sayFiltering(run, family) {
	let steps = run.results.filter(r => r.family == family && ['filter-open', 'filter-stun', 'poke', 'receive'].includes(r.kind))
	let [first, second] = [1, 2].map(test => _readPoked(steps.filter(r => r.test == test)))
	let said = 'unknown'
	if (first.arrived) said = 'endpoint-independent'
	else if (second.arrived === false) said = 'address-and-port-dependent'//held back even from the address the socket had just sent to, whatever the first test said; strict, since undefined means the test couldn't say
	else if (first.arrived === false && second.arrived) said = 'address-dependent'
	else if (first.arrived === false) said = 'not endpoint-independent'
	else if (second.arrived) said = 'endpoint-independent or address-dependent'
	let notes = [first.why, second.why].filter(Boolean)
	if (first.arrived && second.arrived === false) notes.push(`the second poke didn't arrive though the first did, which no kind of filtering does, so its packets were most likely lost`)
	let holding = _sayHolding(run.firewall, 'ftorrent')
	if (holding && said != 'endpoint-independent') notes.push(holding)
	return [said, ...notes].join('; ')
}

//one poke's test, its STUN, poke, and receive steps, read as whether the poke's packets reached the socket: {arrived} true or false, or {why} when the test can't say, as when its socket didn't open and the test is that one step
function _readPoked(steps) {
	if (steps[0].kind == 'filter-open') return {why: `the socket ${steps[0].outcome}`}
	let [stun, poke, receive] = steps
	if (!stun.success) return {why: `STUN from the socket ${stun.outcome}`}
	if (!poke.success) return {why: `the poke ${poke.outcome}`}
	let to = _addressOf(poke.value.to)
	if (!_same(to, stun.value.address)) return {why: `moon poked ${to}, but STUN saw the socket at ${stun.value.address}`}//moon pokes the address the panel names, so this is a guard on what moon reports
	if (!receive.success) return {why: `the receive ${receive.outcome}`}
	return {arrived: receive.value.some(d => _datagramText(d).startsWith(`poke ${poke.value.token} `))}//a packet of this poke's, by its token, rather than one left over from the first poke or a STUN answer that came late
}

//one family's hole punch as the panel saw it, _readPunched's sentence for each of moon's namespaces in that family, and the firewall here named when nothing got through any and it may be what held the packets back; what the punches mean is in the essay at the top, not the row
function _sayPunch(run, family) {
	let punched = punches.filter(([nat, f]) => f == family).map(([nat]) => _readPunched(nat, run.results.filter(r => r.nat == nat)))
	let said = punched.map(p => p.said)
	let holding = _sayHolding(run.firewall, 'ftorrent')
	if (punched.every(p => p.none) && holding) said.push(`${holding[0].toUpperCase()}${holding.slice(1)}.`)//the firewall here, when nothing got through any and it may be what held the packets back
	return said.join(' ')
}

//one punch's steps as a sentence: moon's NAT or firewall, with whether it kept the port of moon's peer behind it, the port the peer bound against the one STUN saw; then which way packets got through, when the first of the peer's arrived, and where from when that wasn't where STUN saw it. Both ways is a packet of the peer's saying it had heard this machine's, and nothing from the peer can't say whether this machine's reached it. Answers {said}, with {none} when nothing arrived; or the reason the punch couldn't run
function _readPunched(nat, steps) {
	let couldnt = why => ({said: `The punch to moon's peer behind ${punchNames[nat]} couldn't run: ${why}.`})
	if (steps[0].kind == 'punch-open') return couldnt(`the socket ${steps[0].outcome}`)
	let [stun, call, exchange] = steps
	if (!stun.success) return couldnt(`STUN from the socket ${stun.outcome}`)
	if (!call.success) return couldnt(`the punch ${call.outcome}`)
	let to = _addressOf(call.value.to)
	if (!_same(to, stun.value.address)) return couldnt(`moon's peer punched ${to}, but STUN saw this machine's socket at ${stun.value.address}`)//a guard on what moon reports, as the poke's
	if (!exchange.success) return couldnt(exchange.outcome)
	let v = call.value
	let kept = Number(v.mapped.split(':').pop()) == v.local_port
	let head = `${punchNames[nat]} ${kept ? 'kept' : 'changed'} the port of moon's peer behind it`//moon's peer named as moon's every time, so it can't be read as this machine
	let ours = exchange.value.arrived.filter(d => _datagramText(d).startsWith(`punch ${v.token} `))
	if (!ours.length) return {said: `${head}, and nothing from that peer reached this machine.`, none: true}
	let first = ours[0]
	let from = first.from == v.mapped ? '' : `, from ${first.from} rather than the ${v.mapped} STUN saw`
	if (ours.some(d => _datagramText(d).endsWith(' heard yes'))) return {said: `${head}, and packets between that peer and this machine got through both ways, the first in ${first.milliseconds} ms${from}.`}
	return {said: `${head}, and packets from that peer reached this machine, the first in ${first.milliseconds} ms${from}, but none said this machine's had reached it.`}
}

//a few words when a firewall on this machine may be what held a packet back from one of this copy's two programs, which is ftorrent, whose socket the filtering test listens on, or engine, whose port the inbound proof probes: a Mac's when it's on and blocks all, or doesn't list the program as allowed; Windows' when the current network's profile is on and blocks all, or a rule blocks the program coming in over UDP there, or none allows it; blank otherwise, and when the firewall can't be read. What it held back is them, the packets, unless the caller names something else
function _sayHolding(f, which, what = 'them') {
	if (!f.known) return ''
	let program = f[which]
	let name = which == 'engine' ? program.path.split(/[\\/]/).pop() : brandName//the engine by its file name, as the firewall lists it
	if (f.profiles) {
		let current = f.profiles.filter(p => p.current && p.enabled)
		if (!current.length) return ''
		let names = current.map(p => p.name)
		let ruled = (action, protocol) => program.rules.some(r => r.enabled && r.action == action && r.direction == 'in' && [protocol, 'any'].includes(r.protocol) && r.profiles.some(profile => names.includes(profile)))//whether a rule of this action covers this protocol coming in on the current profile
		let protocols = which == 'engine' ? ['UDP', 'TCP'] : ['UDP']//the filtering test listens over UDP alone, and the inbound proof reaches the engine over both
		if (current.some(p => p.block_all)) return `the Windows firewall may be what held ${what} back, since it blocks all incoming on ${names.join(' and ')}`
		if (protocols.some(protocol => ruled('block', protocol))) return `the Windows firewall may be what held ${what} back, since a rule blocks ${name} coming in on ${names.join(' and ')}`
		let unallowed = protocols.filter(protocol => !ruled('allow', protocol))
		if (unallowed.length) return `the Windows firewall may be what held ${what} back, since no rule lets ${name} in over ${unallowed.join(' or ')} on ${names.join(' and ')}`
		return ''
	}
	if (f.on != 'on') return ''
	if (f.blockAll == 'on') return `the Mac firewall may be what held ${what} back, since it blocks all incoming connections`
	if (!program.listed?.allowed) return `the Mac firewall may be what held ${what} back, since ${name} isn't allowed on its list`
	return ''
}

//whether a stranger reached the engine over this family, from moon's three probes of its port while a session listened: a pong, a uTP answer, or a connection the dial opened each proves it; a refusal says the packets reached a machine where nothing took them; and nothing back from all three is a translator or a firewall dropping them unasked. Then how many connections in the engine counted from moon's address while the probes ran, and a firewall on this machine named when it may be what dropped them
function _sayInbound(run, family) {
	if (!run.inbound) return 'unknown, no session listening'
	let probe = kind => run.results.find(r => r.kind == kind && r.family == family)
	let dht = probe('ping-dht'), utp = probe('ping-utp'), dial = probe('dial')
	let failed = [dht, utp, dial].filter(r => !r.success)
	if (failed.length) return `unknown, ${failed.map(r => r.outcome).join('; ')}`//a request to moon failed, so a probe never went out
	let reached = []
	if (dht.value.answered) reached.push('DHT ping answered')
	if (utp.value.answered) reached.push(utp.value.utp == 'STATE' ? 'uTP accepted' : `uTP turned away with ${utp.value.utp}`)
	if (dial.value.connected) reached.push(dial.value.handshake ? 'handshake exchanged' : 'TCP connection opened')
	let said = ''
	if (reached.length) said = `reachable: ${reached.join(', ')}`
	else if (dht.value.refused || utp.value.refused || /refused/.test(dial.value.error || '')) said = 'not reachable: refused, the packets reached a machine where nothing took them'
	else said = 'not reachable: nothing came back to either ping, and the dial opened no connection'
	let moon = run.results.filter(r => r.kind == 'stun' && r.binding?.server == moonStun && r.binding.from).map(r => _addressOf(r.binding.from))//moon's own addresses, where its STUN answers came from, which its probes leave from too
	let fromMoon = run.arrivals.filter(a => moon.some(address => _same(address, a.address)) && a.address.includes(':') == (family == 6))
	let notes = [`the engine counted ${fromMoon.length} connection${fromMoon.length == 1 ? '' : 's'} in from moon${fromMoon.length ? `, over ${[...new Set(fromMoon.map(a => a.socket))].join(' and ')}` : ''}`, `port mapping ${run.probed.mapping ? 'on' : 'off'}`]
	let holding = _sayHolding(run.firewall, 'engine')
	if (!reached.length && holding) notes.push(holding)
	return [said, ...notes].join('; ')
}

//a datagram's bytes as text, blank for bytes that aren't all printable, like a STUN answer's
function _datagramText(d) {
	let bytes = Data({base16: d.data}).array()
	if (!bytes.every(b => b >= 0x20 && b < 0x7f)) return ''
	return new TextDecoder().decode(bytes)
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

//the address alone from an address and port written the way a URL writes them, like 2606:4700:49:: from [2606:4700:49::]:3478
function _addressOf(endpoint) {
	return endpoint.replace(/:\d+$/, '').replace(/^\[|\]$/g, '')//the port off the end, and then an IPv6 address's brackets
}

//an address and a port the way a URL writes them, with brackets around an IPv6 address so its colons don't run into the port's
function _sayEndpoint(address, port) {
	if (address.includes(':')) return `[${address}]:${port}`
	return `${address}:${port}`
}

//the box's lines for one run: when, which build, and which platform, then a line per check with what it asked, how long it took, and what came back, then the firewall and the interfaces
export function sayRun(run) {
	let width = Math.max(...run.results.map(r => r.asked.length))//the asked column as wide as its longest entry, so the times line up under each other
	let header = `${sayMoment(run.moment)}, ${brandName} ${run.version} on ${platformName}`
	if (run.probed) header += `, session on port ${run.probed.port} with port mapping ${run.probed.mapping ? 'on' : 'off'}`//a run that probed a session names it, and whether its port was mapped
	let lines = [header]
	for (let r of run.results) lines.push(..._sayResult(r, width))
	lines.push(..._sayFirewallLines(run.firewall), ..._sayMachine(run.machine))
	return lines.join('\n')
}

//one result's lines in the box: what it asked, padded to width, how long it took, and what came back, then whatever it carries that takes lines of its own under it
function _sayResult(r, width) {
	let lines = [`${r.asked.padEnd(width)}  ${`${r.milliseconds} ms`.padStart(7)}  ${_sayAnswer(r)}`]//the answer last, since an error's text can run long
	if (r.binding) lines.push(..._sayBinding(r))
	if (r.kind == 'receive' && r.success) lines.push(..._sayReceived(r))
	if (r.kind == 'punch-exchange' && r.success) lines.push(..._sayReceived({value: r.value.arrived}))
	if (r.success && ['upnp', 'upnp-mappings'].includes(r.kind)) lines.push(..._sayUpnpLines(r))
	return lines
}

//moon's report of a ping, in the box's words: where it went and from which of moon's ports, then the answer, a pong with the node's id and where the node saw moon, or a uTP packet's type; a refusal; an error; or how many sends went unanswered
function _sayPing(v) {
	let said = `${v.to} from moon's port ${v.from_port}: `
	if (v.answered && v.dht) return said + `pong in ${v.milliseconds} ms from node ${v.dht.id}, which saw moon at ${v.dht.saw}`
	if (v.answered) return said + `${v.utp} in ${v.milliseconds} ms, ${v.utp == 'STATE' ? 'accepted, and moon reset it' : 'turned away'}`
	if (v.refused) return said + `refused, after ${v.sent} send${v.sent == 1 ? '' : 's'}`
	if (v.error) return said + v.error
	return said + `no answer to ${v.sent} send${v.sent == 1 ? '' : 's'} in ${v.milliseconds} ms`
}

//moon's report of a dial, in the box's words: where it went, then the handshake that came back and the client it names, a close with nothing, what else came back, or why no connection opened
function _sayDial(v) {
	let said = `${v.to}: `
	if (!v.connected) return said + `no connection in ${v.milliseconds} ms, ${v.error}`
	said += `connected in ${v.milliseconds} ms from moon's port ${v.from_port}, `
	if (v.handshake) return said + `handshake back from peer id ${v.handshake.peer_id}${v.handshake.infohash == v.infohash ? '' : ', for another torrent'}`
	if (v.closed) return said + `then closed with ${v.received} bytes back, which a client without the torrent does`
	return said + `${v.received} bytes back${v.error ? `, then ${v.error}` : ''}`
}

//each datagram a receive heard, on lines under its own: where it came from, when, its size, and what it carried, as text when it's all printable, like a poke's, and in hex when it isn't, like a STUN answer that came late
function _sayReceived(r) {
	return r.value.map(d => `  from ${d.from} at ${d.milliseconds} ms, ${saySize4(d.bytes)}: ${_datagramText(d) ? `"${_datagramText(d)}"` : _sayHex(d.data)}`)
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
	let bytes = Data({base16: hex}).array()//the answer's bytes, from the lowercase base16 net.rs writes
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
	let text = bytes => new TextDecoder().decode(bytes)
	if (type == 0x8022) return text(value)
	if (type == 0x0009 && value.length >= 4) return `${(value[2] & 7)*100 + value[3]} ${text(value.slice(4))}`//the hundreds in the low three bits of one byte and the rest in the next, then the reason
	return _sayHex(Data({array: value}).base16())
}

//hex in groups of four bytes, the way the RFCs print a STUN message
function _sayHex(hex) {
	return (hex.match(/.{1,8}/g) || []).join(' ')
}

//what came back, in the box's words: the address, the error's text, for STUN what the server saw and the answer's size, how many interfaces and lines a list or a tool gave, or a name's addresses with a few words on them
function _sayAnswer(r) {
	if (!r.success) return r.outcome
	if (r.kind == 'stun' || r.kind == 'filter-stun' || r.kind == 'punch-stun') {
		let b = r.value//the binding net_stun or udp_stun answered
		let said = `${_sayEndpoint(b.address, b.port)}, ${saySize4(b.bytes)}`
		if (b.sends > 1) said += `, sent ${b.sends} times`
		return said
	}
	if (r.kind == 'poke') return `${r.value.sent} of ${r.value.packets} packets to ${r.value.to} from port ${r.value.from_port}, carrying ${r.value.token}${r.value.error ? `, then ${r.value.error}` : ''}`
	if (r.kind == 'punch') return `moon's ${r.value.nat} peer bound port ${r.value.local_port} inside its namespace, and STUN saw it at ${r.value.mapped} in ${r.value.stun_milliseconds} ms; ${r.value.packets} packets ${r.value.gap_milliseconds} ms apart to ${r.value.to}, carrying ${r.value.token}`
	if (r.kind == 'punch-exchange') return `${r.value.sent} packets sent to ${r.value.targets.join(' and ')}, ${r.value.arrived.length} datagram${r.value.arrived.length == 1 ? '' : 's'} arrived${r.value.failed ? `; a send failed, ${r.value.failed}` : ''}`
	if (r.kind == 'ping-dht' || r.kind == 'ping-utp') return _sayPing(r.value)
	if (r.kind == 'dial') return _sayDial(r.value)
	if (r.kind == 'receive') return r.value.length > 0 ? `${r.value.length} datagram${r.value.length == 1 ? '' : 's'}` : `nothing in ${checkSeconds} seconds`
	if (r.kind == 'interfaces') return `${r.value.length} interfaces, ${r.value.reduce((sum, i) => sum + i.addresses.length, 0)} addresses`
	if (r.kind == 'tool') { let lines = r.value.trim().split('\n'); return lines.length <= 2 ? lines.join('; ') : `${lines.length} lines` }//what it printed, when that's a line or two, like each of the firewall's reads; the count when it's a screen, read into the interfaces below
	if (r.kind == 'resolve') return `${r.value.join(', ')}; ${_sayResolved(r)}`
	if (r.kind == 'firewall-profiles') return r.value.map(p => `${p.name} ${p.enabled ? 'on' : 'off'}${p.current ? ', current' : ''}`).join('; ')
	if (r.kind == 'firewall-rules') return `${r.value.length} rules`//each on its own line in the firewall section below
	if (r.kind == 'natpmp') return `external address ${r.value.address}, epoch ${r.value.epoch} seconds, ${_sayLifetime(r.value.epoch)} since the router's mapping table started`
	if (r.kind == 'pcp') {
		let v = r.value
		let said = v.version == 2 ? `PCP answered, result ${_sayPcpResult(v.result)}` : `NAT-PMP answered a version 2 request, result ${v.result}${v.result == 1 ? ' unsupported version, so the router speaks NAT-PMP only' : ''}`
		said += `, epoch ${v.epoch} seconds, ${saySize4(v.bytes)}: ${_sayHex(v.answer)}`
		if (v.sends > 1) said += `, sent ${v.sends} times`
		return said
	}
	if (r.kind == 'upnp') {
		let g = r.value
		return `${g.service} at ${g.address}, description at ${g.root}, control at ${g.control}, ${Object.keys(g.actions).length} actions; IPv6 firewall control ${g.firewall ? `at ${g.firewall}` : 'not offered'}`
	}
	if (r.kind == 'upnp-description') return _sayDevice(r.value, true)
	if (r.kind == 'upnp-mappings') return `${r.value.mappings.length} mappings; ${r.value.ended}`
	if (r.kind == 'upnp-firewall') return `IPv6 firewall ${r.value.enabled ? 'on' : 'off'}, inbound pinholes ${r.value.pinholes ? 'allowed' : 'not allowed'}`
	if (r.kind == 'map-add') return r.value.lease > 0 ? `granted for ${r.value.lease} seconds` : 'granted with no lease'
	if (r.kind == 'pinhole-add') return `granted for ${r.value.lease} seconds, pinhole ${r.value.id}`
	if (r.kind == 'natpmp-add') return `granted external port ${r.value.port} for ${r.value.lease} seconds, epoch ${r.value.epoch} seconds`
	if (r.kind == 'pcp-add') return `granted ${r.value.address}, external port ${r.value.port}, for ${r.value.lease} seconds, epoch ${r.value.epoch} seconds; ${saySize4(r.value.bytes)}: ${_sayHex(r.value.answer)}`
	return r.value
}

//the lines under a UPnP check in the box: the actions the router's service answers, under the search, and each port mapping the walk found, under the walk
function _sayUpnpLines(r) {
	if (r.kind == 'upnp') return [`  actions: ${Object.keys(r.value.actions).sort().join(', ')}`]
	return r.value.mappings.map(m => `  ${m.protocol} ${m.external} to ${m.internal}${m.remote ? ` from ${m.remote} only` : ''}, ${m.enabled ? 'enabled' : 'disabled'}, ${m.lifetime > 0 ? `${_sayLifetime(m.lifetime)} left` : 'stays'}${m.description ? `, "${m.description}"` : ''}`)
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
	let more = {macOS: _readIfconfig, Linux: _readIp}[platformName]?.(printed('addresses')) ?? {}//windows runs no tool for this, since net_interfaces reads it all from the system
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
		return {name: i.name, index: i.index, port: ports[i.name] || i.description, flags: [...i.flags, found.status].filter(Boolean), routes, gateways: [...i.gateways, ...named], addresses}
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
		'prefix router-advertisement': '',//made from the prefix the router advertises, as most IPv6 addresses are, so blank like a Mac's autoconf
		'prefix manual':               'set by hand',
		'prefix well-known':           '',
		'prefix other':                '',
		'suffix dhcp':                 '',
		'suffix random':               'temporary',//only a temporary address has a random suffix origin, the one ipconfig labels Temporary IPv6 Address
		'suffix link-layer':           'stable',//the stable address, ipconfig's plain IPv6 Address; Windows says link-layer even though by default it makes this suffix up at random rather than from the hardware address
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

//the gateway the default route in a family goes through, like 192.168.1.1, or blank with no default route in that family or none named on it
function _gateway(family, machine) {
	let i = machine.find(i => i.routes.includes(family))
	return i?.gateways.find(g => g.includes(':') == (family == 6)) || ''
}

//the gateway the default route in a family goes through, and the interface it's on
function _sayGateway(family, machine) {
	if (!machine.length) return 'unknown'
	let i = machine.find(i => i.routes.includes(family))
	if (!i) return 'none'//no default route in this family
	let gateway = _gateway(family, machine)
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

//the machine's firewall, put together from its reads and this copy's paths, known false on Linux and when the first read failed. On a Mac, from the five socketfilterfw reads: whether it's on, block all, stealth mode, and the allowance for built-in and for downloaded signed software, each on, off, or blank when the read failed or used words the panel doesn't know; every program on its list as {path, allowed}; and for this copy of ftorrent and for its engine, the path looked for and the list's entry for it, when there is one. On Windows, from its COM interface: the three profiles as firewall_profiles answers them, and for each of ours its path, every rule naming it, and why the rules couldn't be read, blank when they could
function _firewall(results, paths, engine) {
	let profiles = results.find(r => r.kind == 'firewall-profiles')
	if (profiles) {//Windows
		if (!profiles.success) return {known: false}
		let ours = (which, path) => { let r = results.find(r => r.kind == 'firewall-rules' && r.which == which); return {path, rules: r.success ? r.value : [], failed: r.success ? '' : r.outcome} }
		return {known: true, profiles: profiles.value, ftorrent: ours('ftorrent', paths.executable), engine: ours('engine', engine.path), installed: isInstalled(paths)}//and whether Write firewall rules… can write this copy's rules
	}
	let printed = tool => results.find(r => r.kind == 'tool' && r.tool == tool && r.success)?.value || ''//what a read printed, blank when it failed or this platform has none
	if (!printed('firewall')) return {known: false}
	let signed = printed('signed').split('\n')
	let programs = _readPrograms(printed('programs'))
	return {
		known: true,
		on: _readOn(printed('firewall')),
		blockAll: _readOn(printed('blockall')),
		stealth: _readOn(printed('stealth')),
		signedBuiltIn: _readOn(signed.find(line => line.includes('built-in')) || ''),
		signedDownloaded: _readOn(signed.find(line => line.includes('downloaded')) || ''),
		programs,
		ftorrent: {path: paths.executable, listed: _listed(programs, paths.executable)},
		engine: {path: engine.path, listed: _listed(programs, engine.path)},
	}
}

//a socketfilterfw read as on or off, by the word in what it printed, like Firewall is enabled. (State = 1), Firewall has block all state set to disabled., or Firewall stealth mode is off; blank for words it doesn't use
function _readOn(text) {
	if (/\b(enabled|on)\b/i.test(text)) return 'on'
	if (/\b(disabled|off)\b/i.test(text)) return 'off'
	return ''
}

//a Mac's socketfilterfw --listapps, read as every program on the firewall's list, as [{path, allowed}]: each is a numbered line with the path, then a line saying (Allow incoming connections) or (Block incoming connections)
function _readPrograms(text) {
	let programs = []
	for (let [, path, verb] of text.matchAll(/^\d+ : (.+?)\s*\n\s*\((\w+) incoming connections\)/gm)) programs.push({path, allowed: verb == 'Allow'})
	return programs
}

//the list's entry for the program at this path, or undefined when it isn't listed: by the path itself, or by its bundle, since the firewall lists some programs as the .app holding them, and asks about a helper inside a bundle, like the engine, on its own
function _listed(programs, path) {
	return programs.find(p => p.path == path || (p.path.endsWith('.app') && path.startsWith(`${p.path}/Contents/MacOS/`)))
}

//the firewall's standing for the table. On a Mac: on or off, then block all, stealth mode, and which signed software it allows on its own. On Windows: the current network's profile first, then the other two, each on or off with block all and an allow-by-default when set. Or unknown, with why on Linux
function _sayFirewall(f) {
	if (!f.known) return firewallUnknown
	if (f.profiles) {
		let say = p => `${p.name} ${p.enabled ? 'on' : 'off'}${p.block_all ? ', blocking all incoming' : ''}${p.inbound == 'allow' ? ', incoming allowed by default' : ''}`
		let current = f.profiles.filter(p => p.current), others = f.profiles.filter(p => !p.current)
		return [current.length ? `${current.map(say).join(' and ')}, the current network's profile` : 'no current profile', ...others.map(say)].join('; ')
	}
	let signed = []
	if (f.signedBuiltIn == 'on') signed.push('built-in')
	if (f.signedDownloaded == 'on') signed.push('downloaded')
	return [
		f.on || 'unknown',
		`block all ${f.blockAll || 'unknown'}`,
		`stealth mode ${f.stealth || 'unknown'}`,
		signed.length ? `allows ${signed.join(' and ')} signed software` : 'allows no signed software on its own',
	].join('; ')
}

//this copy of ftorrent and its engine as the firewall knows them, the engine by its file name: on a Mac each allowed, blocked, or not listed, and on Windows each rule naming it in a few words
function _sayFirewallList(f) {
	if (!f.known) return 'unknown'//the row above says why
	let engineName = f.engine.path.split(/[\\/]/).pop()
	if (f.profiles) {
		let said = `${brandName} ${_sayRules(f.ftorrent)}, ${engineName} ${_sayRules(f.engine)}`
		if (![f.ftorrent, f.engine].every(program => _letsIn(program, f.profiles))) return `${said}; ${f.installed ? "Write firewall rules… removes any block rules and allows both on every network, with an administrator's approval, and " : ''}Firewall settings… opens the page where a person allows them by hand`
		return said
	}
	let said = `${brandName} ${_sayListed(f.ftorrent)}, ${engineName} ${_sayListed(f.engine)}`
	if (f.on != 'on') return said//off, which lets everything in whatever the list says
	if (f.blockAll == 'on') return `${said}; block all refuses every program whatever the list says, and System Settings turns it off under Network, Firewall, Options`
	if (!f.ftorrent.listed?.allowed || !f.engine.listed?.allowed) return `${said}; System Settings sets a program to allow under Network, Firewall, Options, and Firewall settings… opens Network there; the firewall lists ${engineName} once it has asked about it`//the Mac has no way for a program to add itself, so the person makes the exception, and a Check after reads it
	return said
}

//which of the two firewall buttons are shown: Firewall settings… on a Mac and Windows, which have a page for it, and Write firewall rules… on Windows for an installed copy alone; the essay above has why no other Windows copy gets that one
export const firewallButtons = reactive({settings: platformName == 'macOS' || platformName == 'Windows', write: false})
if (platformName == 'Windows') pathsStatus().then(paths => { firewallButtons.write = isInstalled(paths) }, () => {})

let firewallChanging = false//while Write firewall rules…'s run of netsh is out, so the window coming forward doesn't read the rules half changed

//Write firewall rules…, on Windows: say in the box what's about to happen, have netsh replace every inbound rule naming this copy or its engine with allow rules, behind one UAC prompt, and read the firewall again after, into the box and the table
export async function firewallWrite() {
	let [paths, engine] = await Promise.all([pathsStatus(), engineStatus()])
	let commands = [[brandName, paths.executable], [`${brandName} engine`, engine.path]].flatMap(([name, path]) => [
		['advfirewall', 'firewall', 'delete', 'rule', 'name=all', 'dir=in', `program=${path}`],//every inbound rule naming the program, whatever it's called, the alert's two among them
		...['TCP', 'UDP'].map(protocol => ['advfirewall', 'firewall', 'add', 'rule', `name=${name}`, 'dir=in', 'action=allow', `program=${path}`, `protocol=${protocol}`, 'profile=any', 'edge=deferapp']),//every profile, and edge traversal left to the program's own socket
	])
	let netsh = `${paths.system}\\netsh.exe`//by its full path, from the folder Windows keeps its own programs in, rather than found by a search
	let script = commands.map(words => `& ${_quotePowerShell(netsh)} ${words.map(_quotePowerShell).join(' ')}`).join('; ')//one line, since the whole script travels as one argument
	boxAdd([`${sayMoment(Date.now())}, Write firewall rules…: asking Windows for an administrator's approval to run netsh, which replaces every inbound rule naming this copy of ${brandName} or its engine with allow rules over TCP and UDP on every network`, ...commands.map(words => `  netsh ${words.map(_sayWord).join(' ')}`)].join('\n'), 'run')
	firewallChanging = true
	let outcome
	try {
		let code = await processElevate(`${paths.system}\\WindowsPowerShell\\v1.0\\powershell.exe`, ['-NoProfile', '-NonInteractive', '-Command', script])//the script as plain text, which the UAC prompt's details show as it is
		outcome = `netsh ran, and PowerShell exited with code ${code}`
	} catch (error) {//the user declined the prompt, or the run couldn't start
		outcome = String(error)
	}
	let results = await Promise.all([_firewallProfiles(), _firewallRules('ftorrent', paths.executable), _firewallRules('engine', engine.path)].map(_ask))
	firewallChanging = false
	let run = lastRun.value
	if (run) {//fold the new reads into the last Check, so the table shows the firewall as it is now
		let fresh = run.results.map(r => r.kind.startsWith('firewall-') ? results.find(n => n.kind == r.kind && n.which == r.which) ?? r : r)
		lastRun.value = {...run, results: fresh, firewall: _firewall(fresh, run.paths, run.engine)}
	}
	boxAdd([`${sayMoment(Date.now())}, Write firewall rules…: ${outcome}; the firewall read again:`, ..._sayFirewallLines(_firewall(results, paths, engine)).slice(1)].join('\n'), 'run')//without the blank line the lines start with in a Check
}

//a word as PowerShell will pass it on exactly: as it is when it's only letters, digits, and the few marks a path or a netsh setting uses, and otherwise in single quotes, where nothing is expanded and a quote is written twice. Never in double quotes, so the script can travel inside the one pair the command line puts around it
function _quotePowerShell(word) {
	if (/^[\w=.:\\-]+$/.test(word)) return word
	return `'${word.replace(/'/g, "''")}'`
}

//a word of a netsh command as the box shows it, in double quotes when it has a space, as a person would type it
function _sayWord(word) {
	return word.includes(' ') ? `"${word}"` : word
}

//whether a program's Windows firewall rules let a connection in over TCP and UDP on the current network: no inbound block rule that's on, since a block wins over an allow, and an inbound allow rule that's on for each protocol in every profile the machine's networks are in now; rules that couldn't be read let nothing in as far as the panel knows
function _letsIn(program, profiles) {
	let current = profiles.filter(p => p.current).map(p => p.name)
	let inbound = program.rules.filter(r => r.direction == 'in' && r.enabled)
	if (program.failed || inbound.some(r => r.action == 'block')) return false
	return ['TCP', 'UDP'].every(protocol => inbound.some(r => r.action == 'allow' && (r.protocol == protocol || r.protocol == 'any') && current.every(name => r.profiles.includes(name))))
}

//a program's Windows firewall rules in a few words each, like allow in TCP on Public, or no rules, or why they couldn't be read
function _sayRules(program) {
	if (program.failed) return `unknown, ${program.failed}`
	if (!program.rules.length) return 'no rules'
	return program.rules.map(r => `${r.action} ${r.direction} ${r.protocol} on ${r.profiles.join(' and ')}${r.enabled ? '' : ', off'}`).join(', ')
}

//a program's standing on the list, in a word
function _sayListed(program) {
	if (!program.listed) return 'not listed'
	return program.listed.allowed ? 'allowed' : 'blocked'
}

//the firewall, the box's lines between the checks and the interfaces: its standing, then on a Mac every program on its list by path, allowed or blocked, with this copy of ftorrent and its engine marked where they appear and named where they don't, and on Windows each rule naming this copy or its engine, with its action, direction, protocol, ports, profiles, and name
function _sayFirewallLines(f) {
	if (!f.known) return []
	let ours = [[f.ftorrent, `this copy of ${brandName}`], [f.engine, `this copy's engine`]]
	let lines = ['', `Firewall ${_sayFirewall(f)}`]
	if (f.profiles) {
		for (let [o, name] of ours) {
			if (o.failed) lines.push(`  unknown   ${o.path}; ${name}, ${o.failed}`)
			else if (!o.rules.length) lines.push(`  no rules  ${o.path}; ${name}`)
			for (let r of o.rules) lines.push(`  ${r.action.padEnd(5)} ${r.direction.padEnd(3)} ${r.protocol.padEnd(3)} ${(r.ports == '*' ? 'any ports' : r.ports || 'no ports').padEnd(11)} ${r.profiles.join(' and ').padEnd(26)} "${r.name}"${r.enabled ? '' : ', off'}; ${name}`)//the firewall writes * for any port, and nothing for a protocol without ports
		}
		return lines
	}
	for (let p of f.programs) {
		let mine = ours.find(([o]) => o.listed == p)
		lines.push(`  ${(p.allowed ? 'allowed' : 'blocked').padEnd(10)}  ${p.path}${mine ? `; ${mine[1]}` : ''}`)
	}
	for (let [o, name] of ours) if (!o.listed) lines.push(`  ${'not listed'.padEnd(10)}  ${o.path}; ${name}`)
	return lines
}

//every run and every line of the session since the app started, oldest first, kept here rather than in the page so it outlasts a visit to the tab
export const box = ref('')
let boxLast = ''//what the box's last addition was, a run or a line of the session, so a run sits apart from the lines around it

//add a run, set apart by a blank line on each side, or one line of the session, to the box
export function boxAdd(text, kind) {
	if (box.value) box.value += kind == 'run' || boxLast == 'run' ? '\n\n' : '\n'
	box.value += text
	boxLast = kind
}

export const lastRun = shallowRef(null)//the latest Check, kept here like the box, null until the first

const pollEvery = 1*Time.second//how often a running session is asked for its news, the beat most BitTorrent clients keep; whether faster is better is an open question

//the engine's session as the page knows it, read from the events it sends up
export const session = reactive({
	running: false,
	port: 0,
	started: 0,//when Start was pressed, which the box's lines are timed from
	torrent: null,//the file chosen, {path, name, v1, v2, size, files} once the engine has read it, or {path, error}
	sockets: [],//each listen result, {address, socket, error}
	bootstrapped: false,//whether the DHT has said it has its first nodes
	allNodes: null,//and how many all the tables hold together, libtorrent's dht.dht_nodes counter, which sums them
	tables: [],//each DHT node the session runs, one for each address it runs the DHT from, as {address, id, nodes}: the address, the node's ID in hex, and how many live nodes its routing table holds, null until the first count arrives; the counts add up to allNodes
	external: [],//each address peers and the DHT say they see, by libtorrent's vote, which it holds for each address it listens on
	incoming: {ipv4: 0, ipv6: 0},//connections strangers opened to the session, counted by alert, by family
	unsolicited: {ipv4: 0, ipv6: 0},//and those from an address no earlier alert shows the session sending to
	mapping: false,//whether libtorrent's two port mappers are on, as the engine last said
	mappers: {},//each mapper on each local address it maps from, keyed by both, as _mapper reads them; each Map starts them again
	hasIncoming: null,//libtorrent's own counter, net.has_incoming_connections, 1 once any has arrived from beyond the local network; it's set before libtorrent decides whether to keep the connection, and the alert comes only once it has, so a session with no torrent, which turns every connection away, can read yes with none by alert
	kinds: {},//how many of each kind of alert the session has posted, by type
})
let poller = 0//the interval asking a running session for its news
let contacted = new Set()//every address the session's alerts show it sending to, a DHT lookup or a connection it tried, since Start
let arrivals = []//every connection in since Start, as {address, socket, moment}, for the inbound proof to find moon's among
let listening = false//whether this module has asked to hear every engine event yet

//hear every engine event from now on, once
function _listen() {
	if (listening) return
	useIncomingStore().listen(_heard)
	listening = true
}

//a command down the road to the engine, with its failure, like an engine that isn't running, as a line in the box
function _send(message) {
	useIncomingStore().send(message).catch(error => boxAdd(`engine: ${error}`, 'session'))
}

//the system's open box, for a torrent file; the engine reads the one chosen, which stays chosen until the app quits
export async function sessionOpen() {
	_listen()
	let [path] = await dialogOpen({start: await downloadsFolder(), kind: 'Torrent files', extensions: ['torrent']})//one torrent file, from Downloads
	if (!path) return//cancelled
	session.torrent = {path}
	_send({command: 'read', path})
}

//start the session at a random port, join the chosen torrent's swarm if there is one, and ask for news once a second
export function sessionStart() {
	_listen()
	let port = 49152 + Math.floor(Math.random()*16384)//from the dynamic range, RFC 6335, a new one each Start; a setting may later let a person choose one, with random staying the default
	Object.assign(session, {running: true, port, started: Date.now(), sockets: [], bootstrapped: false, allNodes: null, tables: [], external: [], mapping: false, mappers: {}, incoming: {ipv4: 0, ipv6: 0}, unsolicited: {ipv4: 0, ipv6: 0}, hasIncoming: null, kinds: {}})
	contacted = new Set()
	arrivals = []
	boxAdd(`${sayMoment(session.started)}, session on port ${port}`, 'session')
	_send({command: 'session', port})
	if (session.torrent?.v1 || session.torrent?.v2) _send({command: 'add', path: session.torrent.path})//only a file the engine has read as a torrent
	poller = setInterval(() => { _send({command: 'stats'}); _send({command: 'tables'}); _send({command: 'poll'}) }, pollEvery)
}

//turn libtorrent's two port mappers on or off in the running session, which Map and Unmap do; the engine says when it has, and the alerts that follow say what the router did
export function sessionMapping(on) {
	_send({command: 'mapping', on})
}

//stop asking for news, take what's waiting, and stop the session
export function sessionStop() {
	clearInterval(poller)
	_send({command: 'poll'})//the engine handles lines in order, so this takes the last alerts before the session goes
	_send({command: 'stop'})
	session.running = false
	session.mapping = false//a session going closes its mappers, which send their deletes on the way out
}

//one event from the engine, read into the session's readings and the box
function _heard(event) {
	if (event.event == 'read') {
		session.torrent = event
		boxAdd(`read ${event.path}: ${event.name}, ${event.files} file${event.files == 1 ? '' : 's'}, ${saySize4(event.size)}`, 'session')
	} else if (event.event == 'added') {
		boxAdd(`${_since()}  joined the swarm of ${event.name} in upload mode`, 'session')
	} else if (event.event == 'tables') {
		session.tables = event.tables.map(t => ({...t, nodes: session.tables.find(known => known.id == t.id)?.nodes ?? null}))//each table keeps its last count until this round's arrives on a later poll
	} else if (event.event == 'mapping') {
		session.mapping = event.on
		if (event.on) session.mappers = {}//each Map reads the mappers from nothing, so the table shows this round
		boxAdd(`${_since()}  port mapping ${event.on ? 'on, with leases of a minute' : 'off, each mapper deleting what it mapped'}`, 'session')
	} else if (event.event == 'stopped') {
		let kinds = _sayKinds()
		boxAdd(`${_since()}  session stopped${kinds ? `; alerts by kind, ${kinds}` : ''}`, 'session')
	} else if (event.event == 'error' && ['read', 'add', 'session', 'mapping'].includes(event.command)) {
		if (event.command == 'read') session.torrent = {path: event.path, error: event.message}
		boxAdd(`engine: ${event.message}`, 'session')
	} else if (event.event == 'alert') {
		_alert(event)
	}
}

//a few words on the alerts a person most needs read for them
const alertWords = {
	listen_succeeded:    'a socket is open',
	listen_failed:       "a socket couldn't open",
	dht_bootstrap:       'the DHT has its first nodes',
	external_ip:         'the address peers say they see',
	incoming_connection: 'a connection in',
	tracker_reply:       'a tracker answered with peers',
	dht_reply:           'the DHT answered with peers',
	tracker_error:       "a tracker didn't answer",
	peer_connect:        'connected to a peer',
	peer_disconnected:   'a peer connection ended',
	alerts_dropped:      'libtorrent dropped alerts nobody took in time',
	portmap:             'the router mapped the port',
	portmap_error:       "a mapping didn't happen",
}

const everyLine = ['incoming_connection', 'portmap', 'portmap_error', 'portmap_log']//the kinds of alert that get a line every time rather than only the first: a connection in, since who came in is what the tab is here to show, and the port mappers, since each line is one request or answer between the session and the router, and the router's exact answer is the finding

//one alert: the counters and the DHT's table go into the readings, since the page asks for them every second; every other alert is counted by kind, and the first of each kind also gets a line in the box
function _alert(a) {
	if (a.type == 'session_stats') { session.hasIncoming = a.values['net.has_incoming_connections']; session.allNodes = a.values['dht.dht_nodes']; return }
	if (a.type == 'session_stats_header') return//the counters' names, once, which the values already carry
	if (a.type == 'dht_live_nodes') { let t = session.tables.find(t => t.id == a.id); if (t) t.nodes = a.nodes; return }//one table's count, asked for every second like the stats
	if (a.type == 'listen_succeeded') session.sockets.push({address: a.address, socket: a.socket})
	if (a.type == 'listen_failed') session.sockets.push({address: a.address, socket: a.socket, error: a.error})
	if (a.type == 'dht_bootstrap') session.bootstrapped = true
	if (a.type == 'external_ip' && !session.external.includes(a.address)) session.external.push(a.address)
	if ((a.type == 'peer_connect' && a.outgoing) || a.type == 'dht_outgoing_get_peers') contacted.add(a.address)
	if (a.type.startsWith('portmap') && a.transport) _mapper(a)
	let note = a.type == 'incoming_connection' ? _incoming(a) : ''
	session.kinds[a.type] = (session.kinds[a.type] || 0) + 1
	if (session.kinds[a.type] > 1 && !everyLine.includes(a.type)) return//a kind already in the box is counted, and only the table shows the count
	let line = `${_since()}  ${a.type}  ${a.message.replace(/\r\n?/g, '\n').trimEnd().replace(/\n/g, '\n' + ' '.repeat(10))}`//a UPnP mapper's log line can carry a whole HTTP request or answer, whose lines are indented under the first
	if (alertWords[a.type]) line += `; ${alertWords[a.type]}`
	if (note) line += `; ${note}`
	if (a.fault) line += `; the engine couldn't read its fields, ${a.fault}`
	boxAdd(line, 'session')
}

//one connection in, counted by family, and the few words on where it came from: the session itself, from another of its addresses; an address the session had sent to first, which a router lets in as an answer; or unsolicited
function _incoming(a) {
	if (!a.address) return ''//an alert whose fields the engine couldn't read, which its line says, and which isn't counted
	let family = a.address.includes(':') ? 'ipv6' : 'ipv4'
	session.incoming[family]++
	arrivals.push({address: a.address, socket: a.socket, moment: Date.now()})
	if (session.sockets.some(s => s.address == a.address)) return 'the session itself, from another of its addresses'
	if (contacted.has(a.address)) return 'the session had sent to this address first'
	session.unsolicited[family]++
	return 'unsolicited, no earlier alert shows the session sending to this address'
}

//one port mapper's alert, read into its readings: the requests it sent and the answers that came back, counted from its log, since a router that never answers posts no other alert; which protocol the NAT-PMP mapper spoke, PCP or NAT-PMP, which only its log says, since the alerts call both natpmp; whether UPnP's search found the router; what the router mapped, or why a mapping didn't happen; and why the mapper itself stopped, which only its log says too
function _mapper(a) {
	let key = `${a.transport} ${a.local}`
	if (!session.mappers[key]) session.mappers[key] = {transport: a.transport, local: a.local, sent: 0, answered: 0, deletes: 0, deleted: 0, version: '', found: false, permanent: false, mapped: {}, error: '', unmapped: '', stopped: ''}
	let m = session.mappers[key]
	let log = a.type == 'portmap_log' ? a.message : ''
	if (a.transport == 'natpmp') {
		let version = log.match(/transport: (\S+)/)?.[1]//PCP or NAT-PMP, on each request and each answer
		if (version) m.version = version
		if (/==> port map .*action: add/.test(log)) m.sent++
		if (/==> port map .*action: delete/.test(log)) m.deletes++
		if (/<== port map/.test(log)) m.answered++
	} else {
		if (/found rootdevice/.test(log)) m.found = true
		if (/sending: .*#AddPortMapping"/s.test(log)) m.sent++//each request as it goes, the first and every renewal, by the action its SOAP header names
		if (/sending: .*#DeletePortMapping"/s.test(log)) m.deletes++
		if (/\]: map response/.test(log)) m.answered++
		if (/\]: unmap response/.test(log)) m.deleted++
		if (/code: 725/.test(log)) m.permanent = true//the router takes only mappings with no lease, so libtorrent asks again for one that lasts until it's deleted
	}
	let failed = log.match(/\]: (failed to .*)/)?.[1]//a mapper that can't start says so in its log and closes, like failed to find default route for "en1" and the address, which the row already names
	if (failed) m.stopped = failed.replace(` ${a.local}`, '')
	if (a.type == 'portmap') { m.mapped[a.protocol] = a.port; m.error = '' }
	if (a.type == 'portmap_error' && session.mapping) m.error = a.error
	if (a.type == 'portmap_error' && !session.mapping) m.unmapped = a.error//an error while the mappers close is about a delete: libtorrent reports every UPnP delete's answer with an error code, and a router's answer of success carries none, so libtorrent's stand-in for none, -1, comes through as unknown UPnP error (-1)
}

//how long since Start, as the box's lines begin
function _since() {
	return sayDuration(Date.now() - session.started, '#.# s').padStart(8)//seconds to a tenth, rounded down, however many
}

//the table's rows for the session and the torrent file, which the alerts keep current while it runs, and which hold the last session's readings after Stop, until the next Start
export function sessionFacts() {
	let t = session.torrent
	let ran = session.started > 0//a session has run since the app started, running now or stopped
	return [
		['Torrent file',         t?.path || 'none'],
		['Info-hash',            t?.error || [t?.v1, t?.v2 && `v2 ${t.v2}`].filter(Boolean).join(', ')],
		['Session',              session.running ? `running on port ${session.port}` : ran ? `stopped, last on port ${session.port}` : 'stopped'],
		['Listening',            _saySockets()],
		['DHT',                  ran ? `${session.allNodes ?? 0} nodes in all, ${session.bootstrapped ? 'started' : 'starting'}` : ''],
		...session.tables.map(t => ['DHT table', _sayTable(t)]),
		['External address',     session.external.join(', ') || (session.running ? 'none yet' : '')],
		['Port mapping',         _sayPortMapping()],
		...Object.values(session.mappers).map(m => ['Port mapper', _sayMapper(m)]),
		['Incoming connections', ran ? _sayIncoming() : ''],
		['Alerts',               _sayKinds()],
	]
}

//one DHT node's table: the node's ID, how many live nodes it holds, and the address it runs from, with what kind of address that is when the last Check listed it; the ID and the count first, since they're the same width row to row, so the rows line up
function _sayTable(t) {
	let found = lastRun.value && _find(lastRun.value.machine, t.address)
	let kinds = found ? `, ${found.a.kinds.join(', ')}` : ''
	let nodes = t.nodes == null ? 'counting' : `${t.nodes} nodes`
	return `ID ${t.id}, ${nodes}, ${t.address}${kinds}`
}

//whether libtorrent's port mappers are on in the session, blank before one has run
function _sayPortMapping() {
	if (session.started == 0) return ''
	return session.mapping ? 'on, leases of a minute' : 'off'
}

//one port mapper on one local address: which protocol it spoke, whether UPnP found the router, what the router mapped and to which external port, whether the mapping is permanent, why one didn't happen, why the mapper stopped, and how many requests it sent and answers came back, with the firewall named when UPnP's search may have been held back by it
function _sayMapper(m) {
	let plural = (n, word) => `${n} ${word}${n == 1 ? '' : 's'}`
	let said = []
	if (m.transport == 'natpmp' && m.version) said.push(`speaking ${m.version}`)
	if (m.transport == 'upnp') said.push(m.found ? 'found the router' : 'no router found yet')
	let mapped = Object.entries(m.mapped).map(([protocol, port]) => `${protocol.toUpperCase()} to external port ${port}`)
	if (mapped.length) said.push(`mapped ${mapped.join(' and ')}`)
	if (m.permanent) said.push('permanent, since the router takes no lease')
	if (m.error) said.push(`not mapped, ${m.error}`)
	if (m.stopped) said.push(`stopped, ${m.stopped}`)
	said.push(`${plural(m.sent, 'request')} and ${plural(m.answered, 'answer')}`)
	if (m.deletes > 0) said.push(`${plural(m.deletes, 'delete')} sent${m.transport == 'upnp' ? ` and ${m.deleted} answered` : ''}`)
	if (m.unmapped) said.push(`libtorrent then posted ${m.unmapped}, which it does for a delete the router answered without an error code`)
	let holding = m.transport == 'upnp' && !m.found && lastRun.value ? _sayHolding(lastRun.value.firewall, 'engine', "the router's answer") : ''
	if (holding) said.push(holding)
	return `${m.transport == 'upnp' ? 'UPnP' : 'NAT-PMP'} mapper from ${m.local}: ${said.join(', ')}`
}

//each socket the session opened, by kind and address, and any that failed with why
function _saySockets() {
	if (!session.sockets.length) return session.running ? 'none yet' : ''
	return session.sockets.map(s => `${s.socket.toUpperCase()} on ${s.address}${s.error ? ` failed, ${s.error}` : ''}`).join(', ')
}

//strangers who connected in, by alert and family, and whether libtorrent's own counter agrees any did
function _sayIncoming() {
	let {ipv4, ipv6} = session.incoming
	let unsolicited = session.unsolicited
	return `${ipv4 + ipv6} by alert, ${ipv4} over IPv4 with ${unsolicited.ipv4} unsolicited and ${ipv6} over IPv6 with ${unsolicited.ipv6} unsolicited; ${session.hasIncoming ? 'yes' : 'none'} by libtorrent's counter, which also counts the ones it turned away`
}

//every alert the session has posted, counted by kind, most first; kept after Stop, until the next Start
function _sayKinds() {
	return Object.entries(session.kinds).sort((a, b) => b[1] - a[1]).map(([type, count]) => `${count} ${type}`).join(', ')
}

//the whole panel as text, the table's rows and then the box, which Copy puts on the clipboard and stick puts on the disk
export function connectionText() {
	return [...[...connectionFacts(lastRun.value), ...mappingFacts(lastMappingRun.value), ...sessionFacts()].map(([key, value]) => `${key}: ${value}`), '', box.value].join('\n')
}

//stick the panel as it starts, blank, and then a second after anything on it changes, so a busy second of alerts is one write, and the write carries all of it
let sticking = 0
watch(connectionText, () => {
	if (sticking) return//one already waiting, which will take the newest text when it goes
	sticking = setTimeout(() => {
		sticking = 0
		stick(connectionText())
	}, 1*Time.second)
}, {immediate: true})
