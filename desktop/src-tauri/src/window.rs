use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use tauri::{command, AppHandle, Manager, State, WebviewUrl, WebviewWindow, WebviewWindowBuilder};
use crate::paths::Paths;

/*
ftorrent's one window, made here in code rather than declared in tauri.conf.json, for two reasons. A window declared in the config is built, with its web view inside, before setup runs, and setup is where a second launch finds the lock held and leaves; two launches a few tens of milliseconds apart once both built a WebView2 on the same profile, and the one that left took the shared browser process down with it, leaving the winner with no window it could ever show. Made from the Ready event, a launch that leaves never has a window at all. And the builder is the only place a web view's data folder can be named, which is how a portable copy keeps WebView2's profile in its portable folder rather than on the host.

The window is made hidden, at any size. Where it goes and how big it is are the page's to decide: window.js reads the place the settings file remembers, or picks a fresh one, moves and sizes the hidden window there, and shows it. Then it says so, through window_revealed, and that one flag is the only state here. It's for bring_forward in lifecycle.rs: a second launch, the tray, or the Dock can ask for the window in the first moment of startup, before the page has placed it, and showing it then would put it on screen at the builder's size and spot for an instant before the page moved it. Until the page has revealed it, bringing it forward waits, since the page is about to show it anyway.

And the web view inside is a browser's, which comes with a browser's keys: on Windows, WebView2 answers F5 and Ctrl+R by reloading the page, Ctrl+P by printing it, Ctrl+F by opening a find bar, and Alt+Left by going back, and neither wry nor Tauri turns that off. None of it belongs in a desktop application, and a reload would run the page's startup again beside an engine that never stopped. So as the window is made, in a release build, a handler goes on WebView2's key event, which every key the browser might take for its own passes through first, and tells the browser to skip each one; which keys those are stays Microsoft's list rather than one of ours, and the page still receives every key. WebView2 also has a single setting that turns them all off, and it would be simpler, but it takes effect only at the next navigation: by the time Tauri hands the web view over, the first page is already loading, and ftorrent never navigates again. A development build keeps the keys, for reloading and the developer tools. The web views on macOS and Linux have no such keys. The right-click menu is a browser's on every platform, and the page turns it away, in main.js, with the same code everywhere.
*/

/// Whether the page has placed and shown the window yet; managed by lib.rs
#[derive(Default)]
pub struct Revealed(pub AtomicBool);

/// Make ftorrent's window, hidden, with its web view's data in the data folder; called once, from RunEvent::Ready
pub fn window_build(app: &AppHandle) {
	let paths = app.state::<Paths>().inner().clone();
	let mut builder = WebviewWindowBuilder::new(app, "main", WebviewUrl::default())//main, because capabilities/default.json grants to that label and lifecycle.rs finds the window by it
		.title(&app.package_info().name)//the product name from tauri.conf.json
		.visible(false);//window.js places it and then shows it
	if !paths.data.is_empty() { builder = builder.data_directory(PathBuf::from(&paths.data)) }//where webview2 keeps its profile, in a subfolder of its own inside the data folder: the host's local application data for an installed copy, exactly where tauri would have put it anyway, and portable/ for a portable one, which is the point
	match builder.build() {
		Ok(window) => _browser_keys_off(&window),
		Err(e) => eprintln!("could not make the window: {e}"),//a window that never appears is worth a line, though in a release build nobody reads it; the tray still offers exit
	}
}

/// Turn off the web view's browser shortcut keys, reload, print, find, back, and the rest, in a release build; on Windows only, where WebView2 has them
#[cfg(target_os = "windows")]
fn _browser_keys_off(window: &WebviewWindow) {
	use webview2_com::AcceleratorKeyPressedEventHandler;
	use webview2_com::Microsoft::Web::WebView2::Win32::ICoreWebView2AcceleratorKeyPressedEventArgs2;
	use windows::core::Interface;//for cast, which asks the event's arguments for the newer interface that holds the switch
	if cfg!(debug_assertions) { return }//a development build keeps them, for reloading the page and opening the developer tools
	let _ = window.with_webview(|webview| unsafe {//runs on the main thread, where the web view lives
		let handler = AcceleratorKeyPressedEventHandler::create(Box::new(|_, args| {//every key the browser might take for its own comes through here first
			if let Some(args) = args.and_then(|args| args.cast::<ICoreWebView2AcceleratorKeyPressedEventArgs2>().ok()) { args.SetIsBrowserAcceleratorKeyEnabled(false)?; }//skip the browser's handling of this key; the page still gets it, and editing keys like ctrl+c were never the browser's. The newer interface arrived in webview2 1.0.2210, and on a runtime older than that the cast fails and the key stays the browser's
			Ok(())
		}));
		let mut token = 0;//what removing the handler would take; it stays for the life of the window
		let _ = webview.controller().add_AcceleratorKeyPressed(&handler, &mut token);//a failure leaves the keys on, the way they started
	});
}

#[cfg(not(target_os = "windows"))]
fn _browser_keys_off(_window: &WebviewWindow) {}//the web views on macOS and Linux have no browser shortcut keys to turn off

/// The page has placed the window and shown it, so bringing it forward is safe from here on
#[command]
pub fn window_revealed(revealed: State<'_, Revealed>) {
	revealed.0.store(true, Ordering::SeqCst);
}

/// The version of the web view the page runs in, as the platform reports it: WebView2's on Windows, and WebKit's on macOS and Linux
#[command]
pub fn window_webview_version() -> Result<String, String> {
	tauri::webview_version().map_err(|e| e.to_string())
}
