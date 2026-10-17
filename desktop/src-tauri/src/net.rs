use std::fs::{self, File};
use std::io::{Read, Write};
use std::net::{IpAddr, Ipv4Addr, Ipv6Addr, SocketAddr, ToSocketAddrs, UdpSocket};
use std::time::{Duration, Instant};
use serde::Serialize;
use sha2::{Digest, Sha256};
use tauri::command;
use crate::run_blocking;

/*
Requests to the web for the page: net_get fetches one https address, and either answers its body as text or, given a path, saves the body to that file and answers its SHA-256. What to fetch, where to save it, and what the answer means, is the page's. net_local answers the address this machine would send from to reach another, which takes no request at all. And net_stun asks STUN servers what address and port requests from one socket on this machine arrived from.

ureq makes the request, a blocking call on run_blocking's pool like the disk commands, with its default TLS: rustls, checking certificates against Mozilla's roots compiled into the program. The answer is treated as coming from anywhere: https only, no redirect followed, the whole request bounded by the page's seconds, and the body by its limit, counted as it arrives and again after gzip is undone. A redirect comes back as its own body, which the page's check turns away; a 4xx or 5xx is an error.

STUN is written out by hand, since a binding request is small enough to read whole, RFC 8489. The request is a twenty-byte header and nothing more: its type, the length of what follows, which is zero, the magic cookie, and twelve random bytes the answer has to repeat. The answer carries the same header and then attributes, each a type, a length, and a value padded to four bytes, and the one that matters is XOR-MAPPED-ADDRESS, the address and port the server saw, masked by the cookie, and for IPv6 by the transaction id as well, so a router rewriting addresses it finds inside packets can't mistake it for one. UDP keeps no promises, so the request goes out again at STUN's intervals, half a second and then doubling, until an answer comes or the page's seconds pass.

net_stun asks every server it's given at once, from one socket, which is how Tailscale's netcheck asks its servers too, and it's the one command here that takes a list. A router keys the mapping it makes on the inside address and port, so the question of whether it keeps one public port for every destination can only be asked from one socket, and asking every server from that socket at the same moment answers it in one wait rather than one wait per server. Each request carries a transaction id of its own, which is how an answer finds the server it came from, and the socket stays unconnected, since it speaks to more than one address.
*/

const STUN_COOKIE: u32 = 0x2112A442;//the magic cookie every STUN message since RFC 5389 carries in bytes 4 through 8

/// What one STUN server saw, or why it didn't answer, as net_stun answers it for each server it asked
#[derive(Serialize)]
pub struct Binding {
	pub server:       String,//the server, as the page named it
	pub address:      String,//the address the server saw the request come from, like 203.0.113.42, or blank with no answer
	pub port:         u16,//and the port, which a router may have changed from the local one
	pub milliseconds: u64,//from the first send until this server's answer, or until giving up on it, including any wait before sending again
	pub sends:        u32,//how many times the request went to this server, 1 when nothing was lost
	pub bytes:        usize,//the size of the answer, against the request's 20
	pub error:        String,//why there's no answer, blank when there is one
}

/// What net_stun answers: the local port every request went from, and each server's binding, in the order asked
#[derive(Serialize)]
pub struct Bindings {
	pub local:   u16,//the one asked for, or the system's pick when that was 0
	pub servers: Vec<Binding>,
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
			let mut binding = Binding {server: server.clone(), address: String::new(), port: 0, milliseconds: 0, sends: 0, bytes: 0, error: String::new()};
			let to = match server.to_socket_addrs() {
				Ok(mut addresses) => addresses.find(|address| address.is_ipv6() == (family == 6)),//the name's first address in the family asked for, so a name with records in both can be asked over either
				Err(e) => { binding.error = e.to_string(); None }
			};
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
			let count = match socket.recv_from(&mut answer) {
				Ok((count, _)) => count,
				Err(e) if matches!(e.kind(), std::io::ErrorKind::WouldBlock | std::io::ErrorKind::TimedOut) => continue,//the read timed out, which unix says as would block and windows as timed out
				Err(e) if matches!(e.kind(), std::io::ErrorKind::ConnectionReset | std::io::ErrorKind::ConnectionRefused) => continue,//windows reports an icmp refusal from any one server as an error on the whole socket, naming no server, so the asking goes on and that server runs out of time
				Err(e) => return Err(e.to_string()),
			};
			for ask in asks.iter_mut().filter(|ask| !ask.done) {//the server whose transaction id the answer repeats; anything else, a stray packet or an answer to an earlier send already counted, matches none and is passed over
				match stun_read(&answer[..count], &ask.request) {
					Ok(None) => continue,
					Ok(Some((address, mapped))) => { ask.binding.address = address.to_string(); ask.binding.port = mapped; ask.binding.bytes = count }
					Err(e) => ask.binding.error = e,
				}
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
		let mut buffer = vec![0u8; 64 * 1024];
		let mut total: u64 = 0;
		loop {
			let count = body.read(&mut buffer).map_err(|e| e.to_string())?;
			if count == 0 { break }
			total += count as u64;
			if total > limit { return Err(format!("the answer ran past {limit} bytes")) }//take let one byte past limit through so this can tell
			hasher.update(&buffer[..count]);
			file.write_all(&buffer[..count]).map_err(|e| e.to_string())?;
		}
		file.sync_all().map_err(|e| e.to_string())?;//on the disk before the rename makes it the real file
		Ok(hasher.finalize().iter().map(|byte| format!("{byte:02x}")).collect())
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
