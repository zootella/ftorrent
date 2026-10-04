use std::sync::atomic::{AtomicBool, Ordering};
use serde::Serialize;
use tauri::{command, AppHandle, Manager, State};
use crate::run_blocking;

/*
Starting at login, offered to the page the way launch.rs offers Launch Services: general commands any Mac application could use as they are, and one fact about this process that every platform has. Whether ftorrent should start at login, and what to do when the system disagrees, is the page's; stores/login.js is the one that uses these.

On the Mac, login_register makes this app a login item and login_unregister takes it off, both through SMAppService's main app service, the way macOS 13 and later expects: the item belongs to the app's bundle, it's listed by name under Open at Login in System Settings, where the user can remove it, and it stops existing when the app is deleted, so nothing is left behind to launch a program that's gone. The older ways, a property list dropped in ~/Library/LaunchAgents, a helper app inside the bundle, or asking System Events through AppleScript, are all ones macOS has moved past, and the first two outlive the app. login_status answers what macOS says about the item: enabled; notRegistered, which is also what removing it from Open at Login leaves, as we measured; requiresApproval, an item registered but waiting on the user's approval in System Settings; or notFound, which macOS answers for an app it has never registered. login_settings opens that pane of System Settings. Each of the four waits on the system's own daemon for its answer, so each runs its body through run_blocking in lib.rs. On Windows the page writes and reads the registry itself, through registry.rs, so each of these answers that there's no SMAppService there.

The fact is login_launch: whether the system started this process at login, which the page reads to start with the window hidden, leaving only the icon near the clock. On the Mac a login item is opened like any app, with an Apple Event, and the event says the launch was a login item's; it's read once, in setup, while that event is still the one being handled. On Windows a program started at sign-in is started with whatever command line its registration carries, so the registration carries one more argument, --login, and this process recognizes it. login_launch answers the argument too, so the page writes the very word this checks for. instance.rs keeps it out of what reaches the page, since it's no file or link, and a second launch carrying only it, a sign-in while ftorrent somehow already runs, changes nothing.
*/

/// The argument a Windows registration adds to the command, so the process it starts knows the system started it at sign-in
pub const ARGUMENT: &str = "--login";

/// Whether the system started this process at login, worked out once in setup; managed by lib.rs
#[derive(Default)]
pub struct Login(AtomicBool);

/// What the page learns about how this process started
#[derive(Serialize)]
pub struct Launch {
	login: bool,//the system started this process at login
	argument: String,//the argument a Windows registration carries, which is how a launch there says so
}

/// Work out whether the system started this process at login, and on the Mac keep the Dock icon away if so; called once from setup, before the window exists
pub fn start(app: &AppHandle) {
	let login = launched_at_login();
	app.state::<Login>().0.store(login, Ordering::SeqCst);
	if !login { return }
	crate::log::log("started at login");
	#[cfg(target_os = "macos")]
	if let Err(e) = app.set_activation_policy(tauri::ActivationPolicy::Accessory) { crate::log::log(&format!("the activation policy did not change: {e}")) }//the policy of an app that lives in the menu bar, from the start, so a login doesn't put ftorrent in the Dock with no window behind it; bring_forward sets Regular when the window comes back
}

#[cfg(target_os = "windows")]
fn launched_at_login() -> bool {
	std::env::args().skip(1).any(|arg| arg == ARGUMENT)
}

/// On the Mac, whether the Apple Event this process is handling, the one that opened it, says it was opened as a login item. Read through msg_send because the bindings put these two methods behind a crate this app doesn't otherwise need; both take and give four-character codes
#[cfg(target_os = "macos")]
fn launched_at_login() -> bool {
	use objc2::msg_send;
	use objc2::rc::Retained;
	use objc2_foundation::{NSAppleEventDescriptor, NSAppleEventManager};
	let code = |text: &[u8; 4]| u32::from_be_bytes(*text);//how the Apple Event headers spell their constants
	let Some(event) = NSAppleEventManager::sharedAppleEventManager().currentAppleEvent() else { crate::log::log("no apple event at setup"); return false };
	let id: u32 = unsafe { msg_send![&event, eventID] };
	if id != code(b"oapp") { crate::log::log(&format!("apple event at setup is {}, not oapp", String::from_utf8_lossy(&id.to_be_bytes()))); return false }//kAEOpenApplication, the event an app opened with no files gets
	let property: Option<Retained<NSAppleEventDescriptor>> = unsafe { msg_send![&event, paramDescriptorForKeyword: code(b"prdt")] };//keyAEPropData, where the open event says what kind of open it was
	property.is_some_and(|property| property.enumCodeValue() == code(b"lgit"))//keyAELaunchedAsLogInItem
}

#[cfg(not(any(target_os = "windows", target_os = "macos")))]
fn launched_at_login() -> bool { false }

/// Whether the system started this process at login, and the argument a Windows registration carries to say so
#[command]
pub fn login_launch(login: State<'_, Login>) -> Launch {
	Launch { login: login.0.load(Ordering::SeqCst), argument: ARGUMENT.to_string() }
}

/// What macOS says about this app as a login item: enabled, notRegistered, requiresApproval, or notFound
#[command]
pub async fn login_status() -> Result<String, String> {//on the blocking pool, as are the three below, since each waits on the system's own service to answer; lib.rs has the rule
	run_blocking(platform::status).await
}

/// Make this app a login item
#[command]
pub async fn login_register() -> Result<(), String> {
	run_blocking(platform::register).await
}

/// Take this app off the login items
#[command]
pub async fn login_unregister() -> Result<(), String> {
	run_blocking(platform::unregister).await
}

/// Open the pane of System Settings where the user switches login items on and off
#[command]
pub async fn login_settings() -> Result<(), String> {
	run_blocking(platform::settings).await
}

#[cfg(target_os = "macos")]
mod platform {
	use objc2_service_management::{SMAppService, SMAppServiceStatus};

	fn main_app() -> objc2::rc::Retained<SMAppService> {
		unsafe { SMAppService::mainAppService() }//the app itself, as a login item; made fresh for each call and let go of at its end, so nothing of macOS's is held across threads
	}

	pub fn status() -> Result<String, String> {
		objc2::rc::autoreleasepool(|_| {//a pool for this thread, as the three below have too: tokio's threads have none, and every thread that calls into Cocoa needs one, as launch.rs explains
			let status = unsafe { main_app().status() };
			Ok(match status {
				SMAppServiceStatus::Enabled => "enabled",
				SMAppServiceStatus::NotRegistered => "notRegistered",
				SMAppServiceStatus::RequiresApproval => "requiresApproval",
				SMAppServiceStatus::NotFound => "notFound",
				_ => return Err(format!("login: macOS answered a status this doesn't know, {}", status.0)),
			}.to_string())
		})
	}

	pub fn register() -> Result<(), String> {
		objc2::rc::autoreleasepool(|_| unsafe { main_app().registerAndReturnError() }.map_err(|e| format!("login: {}", e.localizedDescription())))
	}

	pub fn unregister() -> Result<(), String> {
		objc2::rc::autoreleasepool(|_| unsafe { main_app().unregisterAndReturnError() }.map_err(|e| format!("login: {}", e.localizedDescription())))
	}

	pub fn settings() -> Result<(), String> {
		objc2::rc::autoreleasepool(|_| unsafe { SMAppService::openSystemSettingsLoginItems() });
		Ok(())
	}
}

#[cfg(not(target_os = "macos"))]
mod platform {
	const NONE: &str = "login: there's no SMAppService on this platform";
	pub fn status() -> Result<String, String> { Err(NONE.to_string()) }
	pub fn register() -> Result<(), String> { Err(NONE.to_string()) }
	pub fn unregister() -> Result<(), String> { Err(NONE.to_string()) }
	pub fn settings() -> Result<(), String> { Err(NONE.to_string()) }
}
