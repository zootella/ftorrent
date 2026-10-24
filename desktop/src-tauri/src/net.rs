use std::collections::HashMap;
use std::fs::{self, File};
use std::io::{Read, Write};
use std::net::{IpAddr, Ipv4Addr, Ipv6Addr, SocketAddr, ToSocketAddrs, UdpSocket};
use std::time::{Duration, Instant};
use serde::{Deserialize, Serialize};
use tauri::command;
use zootella_ergo::{size, Data, Sha256};
use crate::run_blocking;

/*
Requests to the web for the page: net_get fetches one https address, and either answers its body as text or, given a path, saves the body to that file and answers its SHA-256. What to fetch, where to save it, and what the answer means, is the page's. net_local answers the address this machine would send from to reach another, which takes no request at all. net_stun asks STUN servers what address and port requests from one socket on this machine arrived from. net_resolve asks the system's resolver for every address a name has. And net_interfaces lists the machine's network interfaces with every address on each.

The interfaces come from a different call on each kind of system. A mac and linux list them through getifaddrs, one entry for each address on each interface and one more for each interface's hardware, which is how an interface with no address still makes the list. A mac shortens an IPv4 netmask to the bytes that aren't zero, so 255.255.255.0 arrives in a record seven bytes long, three of them the mask, and reading a whole sockaddr_in there reads past its end. Windows lists them through GetAdaptersAddresses, which also carries what getifaddrs doesn't: each adapter's gateways, and for each address where its prefix and its suffix came from and how long it lasts. On a mac and linux those come from the system's own tools, which the page runs and reads.

ureq makes the request, a blocking call on run_blocking's pool like the disk commands, with its default TLS: rustls, checking certificates against Mozilla's roots compiled into the program. The answer is treated as coming from anywhere: https only, no redirect followed, the whole request bounded by the page's seconds, and the body by its limit, counted as it arrives and again after gzip is undone. A redirect comes back as its own body, which the page's check turns away; a 4xx or 5xx is an error.

STUN is written out by hand, since a binding request is small enough to read whole, RFC 8489. The request is a twenty-byte header and nothing more: its type, the length of what follows, which is zero, the magic cookie, and twelve random bytes the answer has to repeat. The answer carries the same header and then attributes, each a type, a length, and a value padded to four bytes, and the one that matters is XOR-MAPPED-ADDRESS, the address and port the server saw, masked by the cookie, and for IPv6 by the transaction id as well, so a router rewriting addresses it finds inside packets can't mistake it for one. UDP keeps no promises, so the request goes out again at STUN's intervals, half a second and then doubling, until an answer comes or the page's seconds pass.

net_stun asks every server it's given at once, from one socket, which is how Tailscale's netcheck asks its servers too, and it's the one command here that takes a list. A router keys the mapping it makes on the inside address and port, so the question of whether it keeps one public port for every destination can only be asked from one socket, and asking every server from that socket at the same moment answers it in one wait rather than one wait per server. Each request carries a transaction id of its own, which is how an answer finds the server it came from, and the socket stays unconnected, since it speaks to more than one address. Each answer also comes back whole, in hex beside the address it arrived from, so the page can show every attribute in it and not only the mapped address read here.

net_natpmp asks a router for its external address by NAT-PMP, RFC 6886, through the natpmp crate: a two-byte request to the gateway on port 5351, answered with the address and an epoch, the seconds since the router's mapping table started, which a program watches to learn that the router restarted and its mappings are gone. The crate binds a socket connected to the gateway, so a router refusing the port comes back as a refusal on the socket rather than as silence, and that refusal is itself a finding, since PCP listens on the same port. The crate's socket is nonblocking and its own, with nothing to wait on, so the command polls it every few milliseconds until the deadline, as the crate's own examples do.

net_pcp_announce speaks PCP, RFC 6887, the IETF's successor to NAT-PMP on the same port, and writes it by hand, since no crate sends the one message that asks for nothing. ANNOUNCE is the 24-byte header alone: the version, 2, the opcode, 0, a lifetime of nothing, and the client's own address, which a PCP server checks against the packet's source and refuses as a mismatch when they differ, so the socket is connected to the gateway first and the address the system picked for it is the one written. A router that speaks PCP answers with its result code and epoch. A router that speaks only NAT-PMP reads the version byte, 2 where it expects 0, and answers in its own eight-byte form that the version is unsupported, which is what RFC 6887 tells a client to expect and fall back on, so the command answers that as it is, version and all, and leaves the reading to the page. The request goes again at the intervals net_stun uses rather than PCP's own, whose first wait is three seconds, written for a client that will hold a mapping for hours, where this is one question with a few seconds to answer it.

The four net_upnp commands speak UPnP's Internet Gateway Device profile through igd-next, the crate iroh's port mapper and libp2p build their UPnP on. Finding a router is three exchanges: a search, a few lines of text multicast over UDP to 239.255.255.250 on port 1900, which a router answers from its own address with the location of its device description; that description, fetched over plain HTTP from the router, which names the control address of its WAN connection service and the service's type, whose last number is the version of the profile; and that service's own description, which lists the actions it answers. Then each question is a SOAP call to the control address: GetExternalIPAddress, GetGenericPortMappingEntry for one mapping by its index, until the router says the index is past the end, which is how UPnP lists mappings, and GetFirewallStatus on the IPv6 firewall control service, where a router has one. igd-next does the talking and the parsing, and net_upnp_search answers the gateway it found as plain data, every field igd-next needs to speak to it again, which the page hands back to each of the three reads, so the search happens once and the page owns what's asked after it and in what order. igd-next's SOAP calls carry a time limit of their own, ten seconds, which nothing here can shorten, so a read of a router that has stopped answering outlasts the page's clock, and the page says so first. What igd-next drops on the way, the SERVER line of the search's answer and the friendly name, maker, and model in the description, waits for another crate.
*/

const STUN_COOKIE: u32 = 0x2112A442;//the magic cookie every STUN message since RFC 5389 carries in bytes 4 through 8

/// What one STUN server saw, or why it didn't answer, as net_stun answers it for each server it asked
#[derive(Serialize)]
pub struct Binding {
	pub server:       String,//the server, as the page named it
	pub to:           String,//the address and port its name led to in the family asked for, like 162.159.207.0:3478, or blank when it has none
	pub address:      String,//the address the server saw the request come from, like 203.0.113.42, or blank with no answer
	pub port:         u16,//and the port, which a router may have changed from the local one
	pub milliseconds: u64,//from the first send until this server's answer, or until giving up on it, including any wait before sending again
	pub sends:        u32,//how many times the request went to this server, 1 when nothing was lost
	pub bytes:        usize,//the size of the answer, against the request's 20
	pub from:         String,//the address and port the answer arrived from, blank with no answer
	pub answer:       String,//the whole answer in lowercase hex, header and attributes, blank with no answer
	pub error:        String,//why there's no answer, blank when there is one
}

/// What net_stun answers: the local port every request went from, and each server's binding, in the order asked
#[derive(Serialize)]
pub struct Bindings {
	pub local:   u16,//the one asked for, or the system's pick when that was 0
	pub servers: Vec<Binding>,
}

/// One network interface, as net_interfaces answers it
#[derive(Serialize)]
pub struct Interface {
	pub name:        String,//like en1 on a mac and wlan0 on linux, and on windows the name Settings shows, like Wi-Fi
	pub description: String,//on windows the adapter's own name, often its hardware's; blank elsewhere
	pub index:       u32,//the system's number for it, which a link-local address is scoped by
	pub flags:       Vec<String>,//the system's words for what it is and its state: up, running, loopback and the rest of the interface flags on a mac and linux, and on windows its kind and its operational status
	pub addresses:   Vec<Address>,
	pub gateways:    Vec<String>,//on windows the gateways it routes through; empty elsewhere, where the route tools say
}

/// One address on an interface
#[derive(Serialize)]
pub struct Address {
	pub address:   String,//like 192.168.1.23 or fe80::1c2b:3a4d:5e6f:7081, without the interface a link-local one is scoped to
	pub prefix:    u8,//the length of the network it's on, like 24 or 64
	pub flags:     Vec<String>,//on windows where its prefix and suffix came from and its state, like prefix router-advertisement, suffix random, and preferred; empty elsewhere, where the interface tools say
	pub valid:     Option<u32>,//on windows the seconds it stays valid, 4294967295 for always; none elsewhere
	pub preferred: Option<u32>,//and the seconds it stays preferred for new connections
}

/// What a router answered NAT-PMP's public address request, as net_natpmp answers it
#[derive(Serialize)]
pub struct PublicAddress {
	pub address: String,//the router's external IPv4 address as the router reports it, like 203.0.113.42
	pub epoch:   u32,//seconds since the router's mapping table started, which resets when the router does
}

/// What a router answered PCP's ANNOUNCE, as net_pcp_announce answers it
#[derive(Serialize)]
pub struct Announce {
	pub version: u8,//2 from a router that speaks PCP; 0 from one that speaks only NAT-PMP, which answers in its own form
	pub result:  u16,//PCP's result code, 0 for success, or NAT-PMP's, 1 for unsupported version
	pub epoch:   u32,//seconds since the router's mapping table started, in both protocols' answers
	pub sends:   u32,//how many times the request went out, 1 when nothing was lost
	pub bytes:   usize,//the size of the answer, 24 for PCP's header and 8 for NAT-PMP's
	pub answer:  String,//the whole answer in lowercase hex
}

/// A router found by net_upnp_search, as the page holds it between commands and hands it back to each read: everything igd-next needs to speak to it again
#[derive(Serialize, Deserialize)]
pub struct UpnpGateway {
	pub address:  String,//the address and port the router serves its description and control from, like 192.168.1.1:49152
	pub root:     String,//the path of its device description there
	pub control:  String,//the control path of its WAN connection service, where the SOAP calls go
	pub schema:   String,//the path of that service's own description
	pub service:  String,//the service type, like urn:schemas-upnp-org:service:WANIPConnection:1, whose last number is the version of the profile the router speaks
	pub actions:  HashMap<String, Vec<String>>,//each action the service answers and the arguments it takes, from its description
	pub firewall: Option<String>,//the control path of its WANIPv6FirewallControl service, or None from a router that offers none
}

impl UpnpGateway {
	/// igd-next's gateway from the page's copy of it, the fields the search filled put back
	fn gateway(&self) -> Result<igd_next::Gateway, String> {
		Ok(igd_next::Gateway {
			addr: self.address.parse().map_err(|e: std::net::AddrParseError| e.to_string())?,
			root_url: self.root.clone(),
			control_url: self.control.clone(),
			control_schema_url: self.schema.clone(),
			control_schema: self.actions.clone(),
			service_type: self.service.clone(),
			ipv6_firewall_control_url: self.firewall.clone(),
		})
	}
}

/// One port mapping a router holds, as net_upnp_mappings answers each
#[derive(Serialize)]
pub struct Mapping {
	pub protocol:    String,//TCP or UDP
	pub external:    u16,//the port on the router's outside the mapping forwards from
	pub internal:    String,//the address and port inside the network it forwards to, like 192.168.1.23:51413
	pub remote:      String,//the one remote host the mapping admits, blank for any, which is the usual
	pub enabled:     bool,
	pub description: String,//the words the program that made the mapping left on it
	pub lifetime:    u32,//seconds until the router drops it, 0 for one that stays until deleted
}

/// What net_upnp_mappings answers: every mapping the walk found, and how the walk ended
#[derive(Serialize)]
pub struct Mappings {
	pub mappings: Vec<Mapping>,
	pub ended:    String,//the router saying the list has ended, which is the usual, or the error that stopped the walk
}

/// A router's IPv6 firewall as its WANIPv6FirewallControl service reports it, as net_upnp_firewall answers
#[derive(Serialize)]
pub struct Firewall {
	pub enabled:  bool,//whether the router's IPv6 firewall is on
	pub pinholes: bool,//whether it admits requests to open inbound pinholes
}

/// One server's part in a net_stun: where it is, the request made for it, when it's next due, and what has come of it
struct StunAsk {
	to:      Option<SocketAddr>,//the server's address in the family asked for, None when its name has none
	request: [u8; 20],
	next:    Instant,//when to send it next
	wait:    Duration,//STUN's interval after that send, doubled each time
	done:    bool,//answered, refused, or given up on
	binding: Binding,
}

/// Fetch this https address, giving up after seconds and refusing a body longer than limit bytes; with no save path, answer the body as text, and with one, write the body to that file and answer its SHA-256 in lowercase hex
#[command]
pub async fn net_get(url: String, limit: u64, seconds: u64, save: Option<String>) -> Result<String, String> {
	run_blocking(move || {
		let agent: ureq::Agent = ureq::Agent::config_builder()
			.https_only(true)//never plain http, even if a redirect or a caller asked for it
			.max_redirects(0)//answer a redirect as it is, rather than following it somewhere else
			.timeout_global(Some(Duration::from_secs(seconds)))//the whole request, connecting through the last byte
			.build()
			.into();
		let mut response = agent.get(&url).call().map_err(|e| e.to_string())?;
		let mut body = response.body_mut().with_config().limit(limit + 1).reader()//ureq's limit counts the bytes as they arrive, before gzip is undone; one past ours, since its reader fails any read once the limit is used up, even the last one that would only find the end
			.take(limit + 1);//and take counts them after, one past limit so the checks below can tell a body exactly limit long from a longer one
		if let Some(path) = save { return save_body(&mut body, &path, limit) }
		let mut text = String::new();
		body.read_to_string(&mut text).map_err(|e| e.to_string())?;
		if text.len() as u64 > limit { return Err(format!("the answer ran past {limit} bytes")) }//an error rather than a cut-off body
		Ok(text)
	}).await
}

/// The address this machine would send from to reach the address toward, IPv4 or IPv6: like 192.168.1.23 toward an IPv4 address beyond a home network, and the machine's own global address toward an IPv6 one; nothing is sent
#[command]
pub async fn net_local(toward: IpAddr) -> Result<String, String> {
	run_blocking(move || {
		let any = if toward.is_ipv4() { "0.0.0.0:0" } else { "[::]:0" };//any free port, on no interface in particular until the connect below picks one, in the family of the address asked about
		let socket = UdpSocket::bind(any).map_err(|e| e.to_string())?;
		socket.connect((toward, 9)).map_err(|e| e.to_string())?;//connecting a udp socket sends nothing; the system only looks up its route, which fixes the socket's source address, or fails with no route, as with no network or no ipv6. Port 9 is discard's, though nothing reaches it
		Ok(socket.local_addr().map_err(|e| e.to_string())?.ip().to_string())
	}).await
}

/// Send a STUN binding request to each of servers, names and ports like stun.ftorrent.com:3478, all at once from one socket on the local port given, or one the system picks with 0, over IPv4 or IPv6 as family says, 4 or 6; send each again at STUN's intervals until it answers or seconds pass, and answer what each server saw
#[command]
pub async fn net_stun(servers: Vec<String>, family: u8, port: u16, seconds: u64) -> Result<Bindings, String> {
	run_blocking(move || {
		let any: IpAddr = if family == 6 { Ipv6Addr::UNSPECIFIED.into() } else { Ipv4Addr::UNSPECIFIED.into() };//no interface in particular, in the family asked for
		let socket = UdpSocket::bind((any, port)).map_err(|e| e.to_string())?;
		let local = socket.local_addr().map_err(|e| e.to_string())?.port();

		let mut asks = Vec::new();
		for server in &servers {
			let mut binding = Binding {server: server.clone(), to: String::new(), address: String::new(), port: 0, milliseconds: 0, sends: 0, bytes: 0, from: String::new(), answer: String::new(), error: String::new()};
			let to = match server.to_socket_addrs() {
				Ok(mut addresses) => addresses.find(|address| address.is_ipv6() == (family == 6)),//the name's first address in the family asked for, so a name with records in both can be asked over either
				Err(e) => { binding.error = e.to_string(); None }
			};
			if let Some(to) = to { binding.to = to.to_string() }
			if to.is_none() && binding.error.is_empty() { binding.error = format!("{server} has no IPv{family} address") }
			let mut request = [0u8; 20];
			request[0..2].copy_from_slice(&0x0001u16.to_be_bytes());//a binding request, with bytes 2 and 3 left zero, since nothing follows the header
			request[4..8].copy_from_slice(&STUN_COOKIE.to_be_bytes());
			getrandom::fill(&mut request[8..20]).map_err(|e| e.to_string())?;//the transaction id, a new one for each server
			asks.push(StunAsk {to, request, next: Instant::now(), wait: Duration::from_millis(500), done: to.is_none(), binding});
		}

		let start = Instant::now();
		let deadline = start + Duration::from_secs(seconds);
		for ask in asks.iter_mut() { ask.next = start }//every server is due its first send now, together
		let mut answer = [0u8; 1500];//more than any binding answer, which fits in one unfragmented packet
		while Instant::now() < deadline {
			let now = Instant::now();
			for ask in asks.iter_mut().filter(|ask| !ask.done && ask.next <= now) {//every server due a send, its first or again
				let Some(to) = ask.to else { continue };
				match socket.send_to(&ask.request, to) {
					Ok(_) => { ask.binding.sends += 1; ask.next = now + ask.wait; ask.wait *= 2 }
					Err(e) => { ask.binding.error = e.to_string(); ask.binding.milliseconds = start.elapsed().as_millis() as u64; ask.done = true }
				}
			}
			let Some(next) = asks.iter().filter(|ask| !ask.done).map(|ask| ask.next).min() else { break };//every server has answered or failed
			let left = next.min(deadline).saturating_duration_since(Instant::now());//listen until the next send is due
			if left.is_zero() { continue }
			socket.set_read_timeout(Some(left)).map_err(|e| e.to_string())?;
			let (count, source) = match socket.recv_from(&mut answer) {
				Ok(received) => received,
				Err(e) if matches!(e.kind(), std::io::ErrorKind::WouldBlock | std::io::ErrorKind::TimedOut) => continue,//the read timed out, which unix says as would block and windows as timed out
				Err(e) if matches!(e.kind(), std::io::ErrorKind::ConnectionReset | std::io::ErrorKind::ConnectionRefused) => continue,//windows reports an icmp refusal from any one server as an error on the whole socket, naming no server, so the asking goes on and that server runs out of time
				Err(e) => return Err(e.to_string()),
			};
			for ask in asks.iter_mut().filter(|ask| !ask.done) {//the server whose transaction id the answer repeats; anything else, a stray packet or an answer to an earlier send already counted, matches none and is passed over
				match stun_read(&answer[..count], &ask.request) {
					Ok(None) => continue,
					Ok(Some((address, mapped))) => { ask.binding.address = address.to_string(); ask.binding.port = mapped }
					Err(e) => ask.binding.error = e,
				}
				ask.binding.bytes = count;//a refusal is an answer too, kept whole like a success
				ask.binding.from = source.to_string();
				ask.binding.answer = Data::from_array(&answer[..count]).base16();
				ask.binding.milliseconds = start.elapsed().as_millis() as u64;
				ask.done = true;
				break
			}
		}
		for ask in asks.iter_mut().filter(|ask| !ask.done) {
			ask.binding.error = format!("no answer in {seconds} seconds");
			ask.binding.milliseconds = start.elapsed().as_millis() as u64;
		}
		Ok(Bindings {local, servers: asks.into_iter().map(|ask| ask.binding).collect()})
	}).await
}

/// Send NAT-PMP's public address request, two bytes, to this gateway on port 5351, sending it again at NAT-PMP's intervals until the router answers or seconds pass, and answer the router's external address and epoch; an error when the router refuses the port, answers a result code, or doesn't answer in time
#[command]
pub async fn net_natpmp(gateway: Ipv4Addr, seconds: u64) -> Result<PublicAddress, String> {
	run_blocking(move || {
		let deadline = Instant::now() + Duration::from_secs(seconds);
		let mut client = natpmp::Natpmp::new_with(gateway).map_err(|e| e.to_string())?;//a udp socket connected to the gateway's port 5351, so a refusal of that port comes back on the socket
		client.send_public_address_request().map_err(|e| e.to_string())?;
		loop {
			match client.read_response_or_retry() {//what has arrived, or the request sent again once the crate's interval has passed, 250 milliseconds and then doubling
				Ok(natpmp::Response::Gateway(answer)) => return Ok(PublicAddress {address: answer.public_address().to_string(), epoch: answer.epoch()}),
				Ok(other) => return Err(format!("the router answered a mapping rather than its address: {other:?}")),//not an answer to this request
				Err(natpmp::Error::NATPMP_TRYAGAIN) => {//nothing yet, and nothing to wait on but the clock
					if Instant::now() >= deadline { return Err(format!("no answer in {seconds} seconds")) }
					std::thread::sleep(Duration::from_millis(10));
				}
				Err(natpmp::Error::NATPMP_ERR_NOGATEWAYSUPPORT) => return Err("the gateway refused port 5351".into()),//an icmp port unreachable from the gateway, which the crate reads as a gateway without NAT-PMP, and the page reads as neither NAT-PMP nor PCP, since both listen there
				Err(e) => return Err(e.to_string()),//a result code the router answered, or trouble with the socket
			}
		}
	}).await
}

/// Send PCP's ANNOUNCE, a 24-byte header asking for nothing, to this gateway on port 5351 over IPv4, sending it again at the intervals net_stun uses until the router answers or seconds pass, and answer what came back whole: a PCP answer with its result code and epoch, or NAT-PMP's unsupported-version answer from a router that speaks only that; an error when the router refuses the port or doesn't answer in time
#[command]
pub async fn net_pcp_announce(gateway: Ipv4Addr, seconds: u64) -> Result<Announce, String> {
	run_blocking(move || {
		let socket = UdpSocket::bind((Ipv4Addr::UNSPECIFIED, 0)).map_err(|e| e.to_string())?;
		socket.connect((gateway, 5351)).map_err(|e| e.to_string())?;//connected, so the system settles the source address the request has to carry, and so the gateway refusing the port comes back on the socket
		let IpAddr::V4(source) = socket.local_addr().map_err(|e| e.to_string())?.ip() else { return Err("the socket has no IPv4 address".into()) };
		let mut request = [0u8; 24];
		request[0] = 2;//PCP's version; byte 1 is ANNOUNCE, opcode 0 with the response bit clear, bytes 2 and 3 are reserved, and bytes 4 through 7 are the lifetime asked for, none
		request[18..20].copy_from_slice(&[0xff, 0xff]);
		request[20..24].copy_from_slice(&source.octets());//bytes 8 through 23 are the client's own address, an IPv4 one written as the IPv4-mapped IPv6 address ::ffff:a.b.c.d, which a PCP server checks against the packet's source

		let start = Instant::now();
		let deadline = start + Duration::from_secs(seconds);
		let mut next = start;//when to send next, now for the first
		let mut wait = Duration::from_millis(500);//STUN's interval after that send, doubled each time, the essay above says why not PCP's own
		let mut sends = 0;
		let mut answer = [0u8; 1500];
		while Instant::now() < deadline {
			let now = Instant::now();
			if next <= now {
				if let Err(e) = socket.send(&request) {
					if matches!(e.kind(), std::io::ErrorKind::ConnectionRefused | std::io::ErrorKind::ConnectionReset) { return Err("the gateway refused port 5351".into()) }//a refusal the socket kept from the last send, handed back on this one
					return Err(e.to_string())
				}
				sends += 1; next = now + wait; wait *= 2
			}
			let left = next.min(deadline).saturating_duration_since(Instant::now());//listen until the next send is due
			if left.is_zero() { continue }
			socket.set_read_timeout(Some(left)).map_err(|e| e.to_string())?;
			let count = match socket.recv(&mut answer) {
				Ok(count) => count,
				Err(e) if matches!(e.kind(), std::io::ErrorKind::WouldBlock | std::io::ErrorKind::TimedOut) => continue,//the read timed out, which unix says as would block and windows as timed out
				Err(e) if matches!(e.kind(), std::io::ErrorKind::ConnectionRefused | std::io::ErrorKind::ConnectionReset) => return Err("the gateway refused port 5351".into()),//an icmp port unreachable from the gateway, which windows reports as a reset: neither PCP nor NAT-PMP, since both listen there
				Err(e) => return Err(e.to_string()),
			};
			let bytes = &answer[..count];
			if count < 8 || bytes[1] != 0x80 { continue }//not an answer to ANNOUNCE, which both protocols write as the response bit over opcode 0, 128
			let (version, result, epoch) = match bytes[0] {
				2 if count >= 24 => (2, bytes[3] as u16, u32::from_be_bytes([bytes[8], bytes[9], bytes[10], bytes[11]])),//PCP: a reserved byte, the result code, the lifetime, then the epoch
				0 => (0, u16::from_be_bytes([bytes[2], bytes[3]]), u32::from_be_bytes([bytes[4], bytes[5], bytes[6], bytes[7]])),//NAT-PMP: a two-byte result code, then the epoch
				_ => continue,//some other version, which neither protocol has
			};
			return Ok(Announce {version, result, epoch, sends, bytes: count, answer: Data::from_array(bytes).base16()})
		}
		Err(format!("no answer in {seconds} seconds"))
	}).await
}

/// Search the local network for a router by UPnP, multicasting to 239.255.255.250:1900 and waiting up to seconds for an answer whose device description names a WAN connection service, and answer that gateway as the page will hand it back; an error when nothing answered in time, or when what answered couldn't be read
#[command]
pub async fn net_upnp_search(seconds: u64) -> Result<UpnpGateway, String> {
	run_blocking(move || {
		let mut options = igd_next::SearchOptions::default();//the IPv4 multicast search from any address, which is how a home router is found
		options.timeout = Some(Duration::from_secs(seconds));//for the whole search, the description fetches included
		options.single_search_timeout = Some(Duration::from_secs(seconds));//and for each wait on the socket within it
		let g = igd_next::search_gateway(options).map_err(|e| e.to_string())?;
		Ok(UpnpGateway {address: g.addr.to_string(), root: g.root_url, control: g.control_url, schema: g.control_schema_url, service: g.service_type, actions: g.control_schema, firewall: g.ipv6_firewall_control_url})
	}).await
}

/// Ask this gateway its external address, GetExternalIPAddress, and answer it as text
#[command]
pub async fn net_upnp_external(gateway: UpnpGateway) -> Result<String, String> {
	run_blocking(move || Ok(gateway.gateway()?.get_external_ip().map_err(|e| e.to_string())?.to_string())).await
}

/// Walk this gateway's port mappings by index, GetGenericPortMappingEntry from 0 up, until the router says the index is past the end or something else stops the walk, and answer every mapping found with how the walk ended
#[command]
pub async fn net_upnp_mappings(gateway: UpnpGateway) -> Result<Mappings, String> {
	run_blocking(move || {
		let g = gateway.gateway()?;
		let mut mappings = Vec::new();
		let ended = loop {
			if mappings.len() >= 1000 { break "stopped at 1000, in case the router never ends its list".to_string() }
			match g.get_generic_port_mapping_entry(mappings.len() as u32) {
				Ok(m) => mappings.push(Mapping {protocol: m.protocol.to_string(), external: m.external_port, internal: format!("{}:{}", m.internal_client, m.internal_port), remote: m.remote_host, enabled: m.enabled, description: m.port_mapping_description, lifetime: m.lease_duration}),
				Err(igd_next::GetGenericPortMappingEntryError::SpecifiedArrayIndexInvalid) => break "the router said the list has ended".to_string(),//error 713, the end of the list, the way UPnP ends it
				Err(e) => break e.to_string(),//an error in place of the end, which some routers answer instead, and which the page shows as it is
			}
		};
		Ok(Mappings {mappings, ended})
	}).await
}

/// Ask this gateway's IPv6 firewall control service whether the firewall is on and whether it admits pinholes, GetFirewallStatus; an error from a gateway that offers no such service
#[command]
pub async fn net_upnp_firewall(gateway: UpnpGateway) -> Result<Firewall, String> {
	run_blocking(move || {
		let status = gateway.gateway()?.get_firewall_status().map_err(|e| e.to_string())?;
		Ok(Firewall {enabled: status.firewall_enabled, pinholes: status.inbound_pinhole_allowed})
	}).await
}

/// This gateway's device description, the XML the search read its control paths from, fetched again and answered as text, since igd-next keeps none of what else it says: the router's friendly name, maker, model, and the address of its settings page. Over plain http, which net_get refuses: UPnP serves the description that way alone, on the local network, at the address the search's answer came from. A rejection past seconds or limit bytes
#[command]
pub async fn net_upnp_description(gateway: UpnpGateway, limit: u64, seconds: u64) -> Result<String, String> {
	run_blocking(move || {
		let agent: ureq::Agent = ureq::Agent::config_builder()
			.max_redirects(0)//answer a redirect as it is, rather than following it somewhere else
			.timeout_global(Some(Duration::from_secs(seconds)))//the whole request, connecting through the last byte
			.build()
			.into();
		let mut response = agent.get(&format!("http://{}{}", gateway.address, gateway.root)).call().map_err(|e| e.to_string())?;//the description's path at the router's address and port, as the search's answer named them
		let mut body = response.body_mut().with_config().limit(limit + 1).reader().take(limit + 1);//one past limit twice, as net_get counts it, so a body exactly limit long reads whole and a longer one is told apart
		let mut text = String::new();
		body.read_to_string(&mut text).map_err(|e| e.to_string())?;
		if text.len() as u64 > limit { return Err(format!("the answer ran past {limit} bytes")) }//an error rather than a cut-off body
		Ok(text)
	}).await
}

/// Every address the system's resolver answers for this name, IPv4 and IPv6 alike, in the order it gives them
#[command]
pub async fn net_resolve(name: String) -> Result<Vec<String>, String> {
	run_blocking(move || {
		let addresses = (name.as_str(), 0).to_socket_addrs().map_err(|e| e.to_string())?;//getaddrinfo, the resolver every program on the machine asks; the port is a placeholder it requires
		Ok(addresses.map(|address| address.ip().to_string()).collect())
	}).await
}

/// Every network interface on this machine, with every address on each, in the order the system lists them
#[command]
pub async fn net_interfaces() -> Result<Vec<Interface>, String> {
	run_blocking(interfaces).await
}

/// The interfaces from getifaddrs, gathered by name, since it lists each address as an entry of its own
#[cfg(unix)]
fn interfaces() -> Result<Vec<Interface>, String> {
	let mut head: *mut libc::ifaddrs = std::ptr::null_mut();
	if unsafe { libc::getifaddrs(&mut head) } != 0 { return Err(std::io::Error::last_os_error().to_string()) }
	let mut list: Vec<Interface> = Vec::new();
	let mut at = head;
	while !at.is_null() {//nothing in here returns early, so the list is always freed below
		let entry = unsafe { &*at };
		at = entry.ifa_next;
		let name = unsafe { std::ffi::CStr::from_ptr(entry.ifa_name) }.to_string_lossy().into_owned();
		let i = match list.iter().position(|interface| interface.name == name) {
			Some(i) => i,
			None => {
				let index = unsafe { libc::if_nametoindex(entry.ifa_name) };
				list.push(Interface {name, description: String::new(), index, flags: unix_flags(entry.ifa_flags), addresses: Vec::new(), gateways: Vec::new()});
				list.len() - 1
			}
		};
		if let Some(address) = unix_address(entry.ifa_addr, entry.ifa_netmask) { list[i].addresses.push(address) }
	}
	unsafe { libc::freeifaddrs(head) };
	Ok(list)
}

/// An interface's flags in the system's own words, the ones that say what it is and whether it's working
#[cfg(unix)]
fn unix_flags(flags: libc::c_uint) -> Vec<String> {
	let words = [(libc::IFF_UP, "up"), (libc::IFF_RUNNING, "running"), (libc::IFF_LOOPBACK, "loopback"), (libc::IFF_POINTOPOINT, "point-to-point"), (libc::IFF_BROADCAST, "broadcast"), (libc::IFF_MULTICAST, "multicast")];
	words.iter().filter(|(bit, _)| flags & *bit as libc::c_uint != 0).map(|(_, word)| word.to_string()).collect()
}

/// The IPv4 or IPv6 address in one getifaddrs entry with its prefix length, or None for an entry that holds a hardware address or nothing
#[cfg(unix)]
fn unix_address(address: *const libc::sockaddr, mask: *const libc::sockaddr) -> Option<Address> {
	if address.is_null() { return None }
	let ip = match unsafe { (*address).sa_family } as i32 {
		libc::AF_INET => IpAddr::from(unsafe { (*(address as *const libc::sockaddr_in)).sin_addr.s_addr }.to_ne_bytes()),//s_addr holds the address in network order, the order its bytes lie in memory
		libc::AF_INET6 => IpAddr::from(unsafe { (*(address as *const libc::sockaddr_in6)).sin6_addr.s6_addr }),
		_ => return None,
	};
	Some(Address {address: ip.to_string(), prefix: mask_length(mask, ip.is_ipv6()), flags: Vec::new(), valid: None, preferred: None})
}

/// The number of one bits in a getifaddrs netmask, which is the prefix length
#[cfg(unix)]
fn mask_length(mask: *const libc::sockaddr, v6: bool) -> u8 {
	if mask.is_null() { return 0 }
	let (start, end) = if v6 { (8, 24) } else { (4, 8) };//where the address's bytes sit in a sockaddr_in6 and a sockaddr_in
	#[cfg(target_os = "macos")]
	let end = end.min(unsafe { (*mask).sa_len } as usize);//a mac keeps only the bytes up to the last one that isn't zero, and says how many in sa_len, the essay above has it
	let bytes = unsafe { std::slice::from_raw_parts(mask as *const u8, end) };
	bytes.get(start..).unwrap_or_default().iter().map(|byte| byte.count_ones() as u8).sum()
}

/// The interfaces from GetAdaptersAddresses, with each one's gateways, and where each address came from and how long it lasts
#[cfg(windows)]
fn interfaces() -> Result<Vec<Interface>, String> {
	use windows::Win32::NetworkManagement::IpHelper::{GetAdaptersAddresses, GAA_FLAG_INCLUDE_GATEWAYS, IP_ADAPTER_ADDRESSES_LH};
	use windows::Win32::Networking::WinSock::AF_UNSPEC;
	let mut size: u32 = 15 * zootella_ergo::size::KB as u32;//the size Microsoft suggests starting with, which holds most machines' list
	let mut buffer: Vec<u64>;//of u64, so the structures written into it are aligned
	loop {
		buffer = vec![0; (size as usize).div_ceil(8)];
		let result = unsafe { GetAdaptersAddresses(AF_UNSPEC.0 as u32, GAA_FLAG_INCLUDE_GATEWAYS, None, Some(buffer.as_mut_ptr() as *mut IP_ADAPTER_ADDRESSES_LH), &mut size) };
		if result == 0 { break }//NO_ERROR
		if result != 111 { return Err(format!("GetAdaptersAddresses failed with error {result}")) }//anything but ERROR_BUFFER_OVERFLOW, after which size holds the room it needs, so the loop asks again
	}
	let mut list = Vec::new();
	let mut adapter = buffer.as_ptr() as *const IP_ADAPTER_ADDRESSES_LH;
	while !adapter.is_null() {//a linked list inside buffer, which outlives it
		let a = unsafe { &*adapter };
		adapter = a.Next;
		let mut addresses = Vec::new();
		let mut unicast = a.FirstUnicastAddress;
		while !unicast.is_null() {
			let u = unsafe { &*unicast };
			unicast = u.Next;
			let Some(ip) = windows_ip(u.Address.lpSockaddr) else { continue };
			let flags = vec![
				format!("prefix {}", windows_word(u.PrefixOrigin.0, &["other", "manual", "well-known", "dhcp", "router-advertisement"])),//NL_PREFIX_ORIGIN
				format!("suffix {}", windows_word(u.SuffixOrigin.0, &["other", "manual", "well-known", "dhcp", "link-layer", "random"])),//NL_SUFFIX_ORIGIN
				windows_word(u.DadState.0, &["invalid", "tentative", "duplicate", "deprecated", "preferred"]),//NL_DAD_STATE
			];
			addresses.push(Address {address: ip.to_string(), prefix: u.OnLinkPrefixLength, flags, valid: Some(u.ValidLifetime), preferred: Some(u.PreferredLifetime)});
		}
		let mut gateways = Vec::new();
		let mut gateway = a.FirstGatewayAddress;
		while !gateway.is_null() {
			let g = unsafe { &*gateway };
			gateway = g.Next;
			if let Some(ip) = windows_ip(g.Address.lpSockaddr) { gateways.push(ip.to_string()) }
		}
		list.push(Interface {
			name: unsafe { a.FriendlyName.to_string() }.unwrap_or_default(),
			description: unsafe { a.Description.to_string() }.unwrap_or_default(),
			index: match a.Ipv6IfIndex { 0 => unsafe { a.Anonymous1.Anonymous.IfIndex }, v6 => v6 },//the IPv6 index, which a link-local address is scoped by, or the IPv4 one on an adapter without IPv6; each is zero when its family is off
			flags: vec![windows_kind(a.IfType), windows_word(a.OperStatus.0, &["0", "up", "down", "testing", "unknown", "dormant", "not-present", "lower-layer-down"])],//IF_OPER_STATUS, which counts from 1
			addresses,
			gateways,
		});
	}
	Ok(list)
}

/// A windows enumeration's value as the word for it, counting from 0, or as its number when the list has no word for it
#[cfg(windows)]
fn windows_word(value: i32, words: &[&str]) -> String {
	usize::try_from(value).ok().and_then(|i| words.get(i)).map(|word| word.to_string()).unwrap_or_else(|| value.to_string())
}

/// An adapter's IANA interface type as a word, for the common ones
#[cfg(windows)]
fn windows_kind(kind: u32) -> String {
	match kind {
		6 => "ethernet".into(),
		23 => "ppp".into(),
		24 => "loopback".into(),
		71 => "wifi".into(),
		131 => "tunnel".into(),
		243 | 244 => "mobile-broadband".into(),
		other => format!("type {other}"),
	}
}

/// The IPv4 or IPv6 address a windows socket address holds, or None for another kind
#[cfg(windows)]
fn windows_ip(address: *const windows::Win32::Networking::WinSock::SOCKADDR) -> Option<IpAddr> {
	use windows::Win32::Networking::WinSock::{AF_INET, AF_INET6, SOCKADDR_IN, SOCKADDR_IN6};
	if address.is_null() { return None }
	let family = unsafe { (*address).sa_family };
	if family == AF_INET { return Some(IpAddr::from(unsafe { (*(address as *const SOCKADDR_IN)).sin_addr.S_un.S_addr }.to_ne_bytes())) }//network order, as it lies in memory
	if family == AF_INET6 { return Some(IpAddr::from(unsafe { (*(address as *const SOCKADDR_IN6)).sin6_addr.u.Byte })) }
	None
}

/// The address and port in a STUN answer to this request, None for a packet that isn't one, or the server's refusal as an error
fn stun_read(answer: &[u8], request: &[u8; 20]) -> Result<Option<(IpAddr, u16)>, String> {
	if answer.len() < 20 || answer[4..20] != request[4..20] { return Ok(None) }//too short, or another cookie or transaction
	let kind = u16::from_be_bytes([answer[0], answer[1]]);//0x0101 for a binding success, 0x0111 for a binding error
	let end = (20 + u16::from_be_bytes([answer[2], answer[3]]) as usize).min(answer.len());//the header says how long the attributes are, believed no further than the bytes that arrived
	let mut at = 20;
	while at + 4 <= end {
		let attribute = u16::from_be_bytes([answer[at], answer[at + 1]]);
		let size = u16::from_be_bytes([answer[at + 2], answer[at + 3]]) as usize;
		let value = answer.get(at + 4..at + 4 + size).ok_or("the answer's attributes run past its end")?;
		if kind == 0x0111 && attribute == 0x0009 && size >= 4 { return Err(format!("the server answered error {}", (value[2] & 7) as u16 * 100 + value[3] as u16)) }//ERROR-CODE: the hundreds in the low three bits of one byte and the rest in the next
		if kind == 0x0101 && attribute == 0x0020 && size >= 8 {//XOR-MAPPED-ADDRESS: a zero byte, the family, the port, the address
			let port = u16::from_be_bytes([value[2], value[3]]) ^ (STUN_COOKIE >> 16) as u16;//the port is masked by the cookie's first two bytes
			let length = match value[1] {//family 1 is IPv4 and 2 is IPv6, and STUN defines no other
				1 => 4,
				2 => 16,
				other => return Err(format!("the mapped address has family {other}, which isn't IPv4 or IPv6")),
			};
			if size < 4 + length { return Err("the mapped address is cut short".into()) }
			let mut bytes = [0u8; 16];
			for i in 0..length { bytes[i] = value[4 + i] ^ request[4 + i] }//the address is masked by the cookie, and an IPv6 one by the transaction id after it, which are bytes 4 through 20 of the request
			let address = if length == 4 { IpAddr::from([bytes[0], bytes[1], bytes[2], bytes[3]]) } else { IpAddr::from(bytes) };
			return Ok(Some((address, port)))
		}
		at += 4 + size.div_ceil(4) * 4;//each value is padded to a multiple of four bytes
	}
	if kind == 0x0101 { return Err("the answer has no mapped address".into()) }
	if kind == 0x0111 { return Err("the server answered an error".into()) }
	Ok(None)//a STUN message of some other kind
}

/// Stream body into path through a .part file beside it, hashing as it goes, and answer the SHA-256 once the file is whole and in place; the body is never held whole in memory or carried to the page, and the path never holds a partial download
fn save_body(body: &mut impl Read, path: &str, limit: u64) -> Result<String, String> {
	let part = format!("{path}.part");
	let written = (|| -> Result<String, String> {
		let mut file = File::create(&part).map_err(|e| e.to_string())?;
		let mut hasher = Sha256::new();
		let mut buffer = vec![0u8; 64 * size::KB as usize];
		let mut total: u64 = 0;
		loop {
			let count = body.read(&mut buffer).map_err(|e| e.to_string())?;
			if count == 0 { break }
			total += count as u64;
			if total > limit { return Err(format!("the answer ran past {limit} bytes")) }//take let one byte past limit through so this can tell
			hasher.write_all(&buffer[..count]).map_err(|e| e.to_string())?;//ergo's hasher is a writer too, so the piece is hashed where it sits
			file.write_all(&buffer[..count]).map_err(|e| e.to_string())?;
		}
		file.sync_all().map_err(|e| e.to_string())?;//on the disk before the rename makes it the real file
		Ok(hasher.finalize().base16())
	})();
	match written {
		Ok(hash) => fs::rename(&part, path).map(|_| hash).map_err(|e| { let _ = fs::remove_file(&part); e.to_string() }),
		Err(e) => { let _ = fs::remove_file(&part); Err(e) }//a timeout, a refusal past limit, or a full disk takes the partial file back
	}
}

/*
The STUN answers in RFC 5769, the test vectors STUN's authors published, with the parts stun_read passes over, the integrity and fingerprint values, filled with zeros. XOR-MAPPED-ADDRESS can be read wrong and still yield a plausible address, so these pin the masking for both families against the addresses the RFC says they hold.
*/
#[cfg(test)]
mod test {
	use super::*;

	//hex with spaces between groups, as the RFC prints it, into bytes
	fn bytes(hex: &str) -> Vec<u8> {
		let hex: String = hex.split_whitespace().collect();
		(0..hex.len()).step_by(2).map(|i| u8::from_str_radix(&hex[i..i + 2], 16).unwrap()).collect()
	}

	//the request these answers belong to: a binding request carrying the cookie and the vectors' transaction id
	fn request() -> [u8; 20] {
		bytes("0001 0000 2112a442 b7e7a701 bc34d686 fa87dfae").try_into().unwrap()
	}

	#[test]
	fn stun_read_ipv4() {//section 2.2: 192.0.2.1 port 32853
		let answer = bytes("0101 003c 2112a442 b7e7a701 bc34d686 fa87dfae
			8022 000b 74657374 20766563 746f7200
			0020 0008 0001 a147 e112a643
			0008 0014 00000000 00000000 00000000 00000000 00000000
			8028 0004 00000000");
		assert_eq!(stun_read(&answer, &request()), Ok(Some(("192.0.2.1".parse().unwrap(), 32853))));
	}

	#[test]
	fn stun_read_ipv6() {//section 2.3: 2001:db8:1234:5678:11:2233:4455:6677 port 32853
		let answer = bytes("0101 0048 2112a442 b7e7a701 bc34d686 fa87dfae
			8022 000b 74657374 20766563 746f7200
			0020 0014 0002 a147 0113a9fa a5d3f179 bc25f4b5 bed2b9d9
			0008 0014 00000000 00000000 00000000 00000000 00000000
			8028 0004 00000000");
		assert_eq!(stun_read(&answer, &request()), Ok(Some(("2001:db8:1234:5678:11:2233:4455:6677".parse().unwrap(), 32853))));
	}

	#[test]
	fn stun_read_another_transaction() {//an answer to some other request is passed over rather than read
		let mut answer = bytes("0101 000c 2112a442 b7e7a701 bc34d686 fa87dfae 0020 0008 0001 a147 e112a643");
		answer[19] ^= 1;
		assert_eq!(stun_read(&answer, &request()), Ok(None));
	}
}
