use std::collections::HashMap;
use std::net::{IpAddr, Ipv4Addr, Ipv6Addr, SocketAddr, UdpSocket};
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};
use serde::Serialize;
use tauri::{command, AppHandle, Manager};
use zootella_ergo::Data;
use crate::net::{stun_ask, Bindings};
use crate::run_blocking;

/*
UDP sockets held open for the page between commands. udp_open binds one and answers a number for it; udp_stun asks STUN servers from it, the way net_stun does from a socket of its own; udp_send sends bytes from it to an address; udp_receive waits on it for whatever arrives; and udp_close lets it go. What to send, where, and what an arrival means, is the page's.

Some questions about a network take a socket that outlasts one command. A router keys what it does on the socket: the public port it maps the socket to, and which outside addresses it lets reach that port, which turns on where the socket has already sent. So a question like whether a packet from a new address can reach a socket that has spoken to one server is several steps on the same socket, with something else between them, such as a request over HTTPS asking another machine to send to it. net_stun opens a socket, asks, and closes it, which suits a question that is one step.

The sockets are kept here by number, as locks.rs keeps its files, until udp_close or the end of the process, so a socket the page never closes stays open until then. No lock stands around a socket, since one sends and receives at the same moment without trouble, and a hole punch needs both at once. Two reads at once are the page's to avoid: udp_stun and udp_receive on the same socket would split its arrivals between them and reset each other's time limit. udp_stun reads only answers to its own requests and passes over anything else that arrives while it asks, so the page receives before it or after it, not during.

A socket is bound to no address in particular, in the family asked for, as net_stun's is, so the system picks the source of each packet by its route, and a packet arriving at that port on any of the machine's addresses in that family reaches it. On Windows the firewall's alert can come up at the bind, as it does at net_stun's.

Every command but udp_close waits on the network, so each runs its body through run_blocking in lib.rs and takes the app handle rather than the sockets themselves, as locks.rs does; udp_close only takes a socket out of the map, which answers from memory, and stays plain.
*/

/// The UDP sockets this process holds open for the page, by the number udp_open answered for each, and the last number given out
#[derive(Default)]
pub struct Sockets {
	open: Mutex<HashMap<u32, Arc<UdpSocket>>>,//each socket shared, so a command can hold it while udp_close takes it out of the map
	last: AtomicU32,//counted up, so a number is never given out twice in a process
}

/// What udp_open answers: the number the commands below take, and the local port the socket is bound to
#[derive(Serialize)]
pub struct Opened {
	pub socket: u32,//the number, 1 for the first socket and counting up
	pub port:   u16,//the one asked for, or the system's pick when that was 0
}

/// One datagram that reached a held socket, as udp_receive answers each
#[derive(Serialize)]
pub struct Datagram {
	pub from:         String,//the address and port it came from, like 198.51.100.7:40312
	pub milliseconds: u64,//from the start of the receive until it was read, near 0 for one already waiting
	pub bytes:        usize,
	pub data:         String,//the whole datagram in lowercase hex
}

/// Open a UDP socket on this local port, or one the system picks with 0, over IPv4 or IPv6 as family says, 4 or 6, and hold it open until udp_close; answers the number the commands below take, and the port
#[command]
pub async fn udp_open(app: AppHandle, family: u8, port: u16) -> Result<Opened, String> {
	run_blocking(move || {
		let any: IpAddr = if family == 6 { Ipv6Addr::UNSPECIFIED.into() } else { Ipv4Addr::UNSPECIFIED.into() };//no interface in particular, in the family asked for
		let socket = UdpSocket::bind((any, port)).map_err(|e| e.to_string())?;
		let port = socket.local_addr().map_err(|e| e.to_string())?.port();
		let sockets = app.state::<Sockets>();
		let number = sockets.last.fetch_add(1, Ordering::Relaxed) + 1;
		sockets.open.lock().unwrap_or_else(|poisoned| poisoned.into_inner()).insert(number, Arc::new(socket));
		Ok(Opened {socket: number, port})
	}).await
}

/// Send a STUN binding request to each of servers, names and ports like stun.ftorrent.com:3478, all at once from this held socket, sending each again at STUN's intervals until it answers or seconds pass, and answer what each server saw, as net_stun does from a socket of its own
#[command]
pub async fn udp_stun(app: AppHandle, socket: u32, servers: Vec<String>, seconds: u64) -> Result<Bindings, String> {
	run_blocking(move || { let socket = held(&app, socket)?; stun_ask(&socket, &servers, seconds) }).await
}

/// Send these bytes, given in lowercase hex, from this held socket to an address and port like 203.0.113.42:6881 or [2001:db8::1]:6881; answers how many bytes went
#[command]
pub async fn udp_send(app: AppHandle, socket: u32, to: SocketAddr, data: String) -> Result<usize, String> {
	run_blocking(move || {
		let data = Data::from_base16(&data).map_err(|e| e.to_string())?;
		held(&app, socket)?.send_to(data.array(), to).map_err(|e| e.to_string())
	}).await
}

/// Wait on this held socket until count datagrams have arrived or seconds pass, and answer each one that did, in order, with where it came from; an empty list when nothing came
#[command]
pub async fn udp_receive(app: AppHandle, socket: u32, count: usize, seconds: u64) -> Result<Vec<Datagram>, String> {
	run_blocking(move || { let socket = held(&app, socket)?; receive(&socket, count, seconds) }).await
}

/// Close this held socket; nothing happens for a number that isn't open, and a command already waiting on the socket finishes before it closes
#[command]
pub fn udp_close(app: AppHandle, socket: u32) {
	app.state::<Sockets>().open.lock().unwrap_or_else(|poisoned| poisoned.into_inner()).remove(&socket);//the socket closes when the last command holding it lets go
}

/// The socket this number names, shared with any other command using it, or an error for a number that isn't open
fn held(app: &AppHandle, socket: u32) -> Result<Arc<UdpSocket>, String> {
	app.state::<Sockets>().open.lock().unwrap_or_else(|poisoned| poisoned.into_inner()).get(&socket).cloned().ok_or_else(|| format!("no socket {socket} is open"))
}

/// The datagrams that reach socket until count have arrived or seconds pass, the body of udp_receive
fn receive(socket: &UdpSocket, count: usize, seconds: u64) -> Result<Vec<Datagram>, String> {
	let start = Instant::now();
	let deadline = start + Duration::from_secs(seconds);
	let mut datagrams = Vec::new();
	let mut buffer = vec![0u8; 65536];//room for the largest datagram UDP can carry, so none is cut short
	while datagrams.len() < count {
		let left = deadline.saturating_duration_since(Instant::now());
		if left.is_zero() { break }
		socket.set_read_timeout(Some(left)).map_err(|e| e.to_string())?;
		let (size, from) = match socket.recv_from(&mut buffer) {
			Ok(received) => received,
			Err(e) if matches!(e.kind(), std::io::ErrorKind::WouldBlock | std::io::ErrorKind::TimedOut) => continue,//the read timed out, which unix says as would block and windows as timed out, and the clock above decides whether that was the end
			Err(e) if matches!(e.kind(), std::io::ErrorKind::ConnectionReset | std::io::ErrorKind::ConnectionRefused) => continue,//windows reports an icmp refusal of something this socket sent earlier as an error on the next read, naming no address, so the waiting goes on
			Err(e) => return Err(e.to_string()),
		};
		datagrams.push(Datagram {from: from.to_string(), milliseconds: start.elapsed().as_millis() as u64, bytes: size, data: Data::from_array(&buffer[..size]).base16()});
	}
	Ok(datagrams)
}

/*
receive, over the loopback address, which needs no network: datagrams that arrive are answered in order with their source, the count stops the wait as soon as it's met, and a wait that hears nothing ends on the clock with an empty list.
*/
#[cfg(test)]
mod test {
	use super::*;

	#[test]
	fn receive_count() {//three waiting, two asked for, then the third and the clock
		let here = UdpSocket::bind((Ipv4Addr::LOCALHOST, 0)).unwrap();
		let there = UdpSocket::bind((Ipv4Addr::LOCALHOST, 0)).unwrap();
		for text in ["one", "two", "three"] { there.send_to(text.as_bytes(), here.local_addr().unwrap()).unwrap(); }
		let start = Instant::now();
		let datagrams = receive(&here, 2, 4).unwrap();
		assert!(start.elapsed() < Duration::from_secs(1));//the count ended the wait, not the clock
		assert_eq!(datagrams.len(), 2);
		assert_eq!(datagrams[0].data, Data::from_text("one").base16());
		assert_eq!(datagrams[1].data, Data::from_text("two").base16());
		assert_eq!(datagrams[0].from, there.local_addr().unwrap().to_string());
		assert_eq!(receive(&here, 5, 1).unwrap().len(), 1);//the third, still waiting, and then the clock
	}

	#[test]
	fn receive_nothing() {//nothing waiting, so the wait ends on the clock
		let here = UdpSocket::bind((Ipv4Addr::LOCALHOST, 0)).unwrap();
		let start = Instant::now();
		assert!(receive(&here, 3, 1).unwrap().is_empty());
		assert!(start.elapsed() >= Duration::from_secs(1));
	}
}
