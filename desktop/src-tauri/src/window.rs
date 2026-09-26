use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use tauri::{command, AppHandle, Manager, State, WebviewUrl, WebviewWindowBuilder};
use crate::paths::Paths;

/*
ftorrent's one window, made here in code rather than declared in tauri.conf.json, for two reasons. A window declared in the config is built, with its web view inside, before setup runs, and setup is where a second launch finds the lock held and leaves; two launches a few tens of milliseconds apart once both built a WebView2 on the same profile, and the one that left took the shared browser process down with it, leaving the winner with no window it could ever show. Made from the Ready event, a launch that leaves never has a window at all. And the builder is the only place a web view's data folder can be named, which is how a portable copy keeps WebView2's profile in its portable folder rather than on the host.

The window is made hidden, at any size. Where it goes and how big it is are the page's to decide: window.js reads the place the settings file remembers, or picks a fresh one, moves and sizes the hidden window there, and shows it. Then it says so, through window_revealed, and that one flag is the only state here. It's for bring_forward in lifecycle.rs: a second launch, the tray, or the Dock can ask for the window in the first moment of startup, before the page has placed it, and showing it then would put it on screen at the builder's size and spot for an instant before the page moved it. Until the page has revealed it, bringing it forward waits, since the page is about to show it anyway.
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
	if let Err(e) = builder.build() { eprintln!("could not make the window: {e}") }//a window that never appears is worth a line, though in a release build nobody reads it; the tray still offers exit
}

/// The page has placed the window and shown it, so bringing it forward is safe from here on
#[command]
pub fn window_revealed(revealed: State<'_, Revealed>) {
	revealed.0.store(true, Ordering::SeqCst);
}
