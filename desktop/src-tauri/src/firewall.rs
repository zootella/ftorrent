use serde::Serialize;
use tauri::command;
#[cfg(windows)]
use crate::run_blocking;
#[cfg(windows)]
use windows::Win32::NetworkManagement::WindowsFirewall::INetFwPolicy2;

/*
The Windows firewall, read for the page through its COM interface, INetFwPolicy2, which answers without elevation and in numbers, where netsh prints its words in the system's language. Windows sorts every network it joins into one of three profiles, Domain, Private, and Public, keeps a firewall state for each, and tags every rule with the profiles it applies in, so what the firewall does right now is the current network's profile read against the rules that name it. The two commands here read and change nothing: the profiles, and the rules naming one program by its path. Adding a rule is another thing, an elevated run the user approves, and lives elsewhere when it comes.

COM wants each thread that uses it initialized, and the window's thread already is, in a mode this code shouldn't assume, so both commands run on the blocking pool and initialize an apartment of their own there, apartment-threaded, which the firewall's objects accept, and uninitialize it before the thread goes back to the pool. The rules come as a COM collection whose one way to be walked is the enumerator every automation collection carries, IEnumVARIANT, which hands back each rule as a VARIANT holding an IDispatch, cast to the rule's own interface; the dispatch is taken out of the VARIANT by reading its union, which is what a VARIANT is for, and the one unsafe read here beyond the calls themselves. A machine holds hundreds of rules, most of them Windows' own, so the walk reads each one's application name and keeps the few naming the path asked about, compared without regard to case, since Windows paths have none.

On a Mac and Linux both commands answer an error, since no Windows firewall is there; the page asks only on Windows.
*/

/// One of the firewall's three profiles, as firewall_profiles answers each
#[derive(Serialize)]
pub struct Profile {
	pub name:      String,//Domain, Private, or Public
	pub current:   bool,//whether the network the machine is on now is in this profile; usually one is, and a machine on two networks can be in two
	pub enabled:   bool,//whether the firewall is on for it
	pub block_all: bool,//whether every incoming connection is refused in it, whatever the rules say
	pub inbound:   String,//what happens to an incoming connection no rule names, block or allow; block is the default in every profile
}

/// One rule naming the program asked about, as firewall_rules answers each
#[derive(Serialize)]
pub struct Rule {
	pub name:      String,//the rule's name, like ftorrent.exe, which the security alert takes from the file for an installed copy and from the program's description for a development build
	pub direction: String,//in or out
	pub action:    String,//allow or block
	pub enabled:   bool,
	pub profiles:  Vec<String>,//the profiles it applies in, from Domain, Private, and Public
	pub protocol:  String,//TCP, UDP, any, or the protocol's number
	pub ports:     String,//the local ports it covers, like 51413 or 1024-65535, blank for any
}

/// Each of the firewall's three profiles: whether the machine's current network is in it, whether the firewall is on for it, whether it refuses every incoming connection, and what it does to one no rule names
#[cfg(windows)]
#[command]
pub async fn firewall_profiles() -> Result<Vec<Profile>, String> {
	run_blocking(|| with_policy(profiles)).await
}

/// Every rule naming the program at this path, compared without regard to case, each with its direction, action, profiles, protocol, and ports, the rules Windows wrote on the security alert included; an empty list for a program the firewall has never been asked about
#[cfg(windows)]
#[command]
pub async fn firewall_rules(path: String) -> Result<Vec<Rule>, String> {
	run_blocking(move || with_policy(|policy| rules(policy, &path))).await
}

#[cfg(not(windows))]
#[command]
pub async fn firewall_profiles() -> Result<Vec<Profile>, String> {
	Err("the Windows firewall isn't on this platform".into())
}

#[cfg(not(windows))]
#[command]
pub async fn firewall_rules(path: String) -> Result<Vec<Rule>, String> {
	let _ = path;
	Err("the Windows firewall isn't on this platform".into())
}

/// Run one read against the firewall's policy object, on a COM apartment this thread initializes for itself and leaves when the read is done, however it ends
#[cfg(windows)]
fn with_policy<T>(read: impl FnOnce(&INetFwPolicy2) -> windows::core::Result<T>) -> Result<T, String> {
	use windows::Win32::NetworkManagement::WindowsFirewall::NetFwPolicy2;
	use windows::Win32::System::Com::{CoCreateInstance, CoInitializeEx, CoUninitialize, CLSCTX_INPROC_SERVER, COINIT_APARTMENTTHREADED};
	unsafe { CoInitializeEx(None, COINIT_APARTMENTTHREADED) }.ok().map_err(|e| e.to_string())?;//this thread's own apartment; a thread the pool has used for this before answers that it already is, which is fine
	let answer = (|| {
		let policy: INetFwPolicy2 = unsafe { CoCreateInstance(&NetFwPolicy2, None, CLSCTX_INPROC_SERVER) }?;//the firewall's policy object, HNetCfg.FwPolicy2
		read(&policy)
	})();
	unsafe { CoUninitialize() };
	answer.map_err(|e| e.to_string())
}

/// The three profiles, each asked about in turn
#[cfg(windows)]
fn profiles(policy: &INetFwPolicy2) -> windows::core::Result<Vec<Profile>> {
	use windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_ACTION_ALLOW;
	let current = unsafe { policy.CurrentProfileTypes() }?;//a bitmask of the profiles the machine's networks are in now
	let mut list = Vec::new();
	for (kind, name) in kinds() {
		list.push(Profile {
			name: name.into(),
			current: current & kind.0 != 0,
			enabled: unsafe { policy.get_FirewallEnabled(kind) }?.as_bool(),
			block_all: unsafe { policy.get_BlockAllInboundTraffic(kind) }?.as_bool(),
			inbound: if unsafe { policy.get_DefaultInboundAction(kind) }? == NET_FW_ACTION_ALLOW { "allow".into() } else { "block".into() },
		});
	}
	Ok(list)
}

/// Every rule naming this path, from a walk of the whole collection
#[cfg(windows)]
fn rules(policy: &INetFwPolicy2, path: &str) -> windows::core::Result<Vec<Rule>> {
	use windows::core::Interface;
	use windows::Win32::NetworkManagement::WindowsFirewall::{INetFwRule, NET_FW_ACTION_ALLOW, NET_FW_PROFILE2_ALL, NET_FW_RULE_DIR_IN};
	use windows::Win32::System::Ole::IEnumVARIANT;
	use windows::Win32::System::Variant::{VARIANT, VT_DISPATCH};
	let wanted = path.to_lowercase();
	let enumerator: IEnumVARIANT = unsafe { policy.Rules()?._NewEnum()? }.cast()?;//the collection's enumerator, its one way to be walked
	let mut list = Vec::new();
	loop {
		let mut item = [VARIANT::default()];
		let mut fetched = 0u32;
		unsafe { enumerator.Next(&mut item, &mut fetched) }.ok()?;//S_FALSE at the end, which ok accepts, with nothing fetched
		if fetched == 0 { break }
		let dispatch = unsafe { if item[0].Anonymous.Anonymous.vt == VT_DISPATCH { (*item[0].Anonymous.Anonymous.Anonymous.pdispVal).clone() } else { None } };//the rule, as the dispatch interface the VARIANT holds, read out of its union
		let Some(dispatch) = dispatch else { continue };
		let rule: INetFwRule = dispatch.cast()?;
		let program = unsafe { rule.ApplicationName() }.map(|name| name.to_string()).unwrap_or_default();//blank for a rule naming a service or a port rather than a program
		if program.to_lowercase() != wanted { continue }
		let profiles = unsafe { rule.Profiles() }?;
		list.push(Rule {
			name: unsafe { rule.Name() }?.to_string(),
			direction: if unsafe { rule.Direction() }? == NET_FW_RULE_DIR_IN { "in".into() } else { "out".into() },
			action: if unsafe { rule.Action() }? == NET_FW_ACTION_ALLOW { "allow".into() } else { "block".into() },
			enabled: unsafe { rule.Enabled() }?.as_bool(),
			profiles: kinds().into_iter().filter(|(kind, _)| profiles == NET_FW_PROFILE2_ALL.0 || profiles & kind.0 != 0).map(|(_, name)| name.to_string()).collect(),//the bitmask as names, where all is its own value rather than the three bits
			protocol: match unsafe { rule.Protocol() }? { 6 => "TCP".into(), 17 => "UDP".into(), 256 => "any".into(), other => other.to_string() },
			ports: unsafe { rule.LocalPorts() }.map(|ports| ports.to_string()).unwrap_or_default(),//blank for any, and for a protocol that has no ports
		});
	}
	Ok(list)
}

/// The three profiles' values and names, in the order Windows lists them
#[cfg(windows)]
fn kinds() -> [(windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_PROFILE_TYPE2, &'static str); 3] {
	use windows::Win32::NetworkManagement::WindowsFirewall::{NET_FW_PROFILE2_DOMAIN, NET_FW_PROFILE2_PRIVATE, NET_FW_PROFILE2_PUBLIC};
	[(NET_FW_PROFILE2_DOMAIN, "Domain"), (NET_FW_PROFILE2_PRIVATE, "Private"), (NET_FW_PROFILE2_PUBLIC, "Public")]
}
