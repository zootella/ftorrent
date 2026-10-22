use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use tauri::{command, AppHandle, Manager, State, WebviewUrl, WebviewWindow, WebviewWindowBuilder};
use crate::paths::Paths;

/*
ftorrent's one window, made here in code rather than declared in tauri.conf.json, for two reasons. A window declared in the config is built, with its web view inside, before setup runs, and setup is where a second launch finds the lock held and leaves; two launches a few tens of milliseconds apart once both built a WebView2 on the same profile, and the one that left took the shared browser process down with it, leaving the winner with no window it could ever show. Made from the Ready event, a launch that leaves never has a window at all. And the builder is the only place a web view's data folder can be named, which is how a portable copy keeps WebView2's profile in its portable folder rather than on the host.

The window is made hidden, at any size. Where it goes and how big it is are the page's to decide: window.js reads the place the settings file remembers, or picks a fresh one, moves and sizes the hidden window there, and shows it. Then it says so, through window_revealed, and that one flag is the only state here; a launch the system made at login is placed and left hidden, and says so all the same. It's for bring_forward in lifecycle.rs: a second launch, the tray, or the Dock can ask for the window in the first moment of startup, before the page has placed it, and showing it then would put it on screen at the builder's size and spot for an instant before the page moved it. Until the page has revealed it, bringing it forward waits, since the page is about to show it anyway.

And the web view inside is a browser's, and on Windows it takes the keyboard for itself. While the page has focus, WebView2 gets each keystroke before the window's message loop, where Tauri checks every key against the menu, and it raises an event of its own for each key that isn't typing, a function key or a key with Ctrl or Alt held, which neither wry nor Tauri answers. So the menu's shortcuts never reach it: F1, View Help's, Ctrl+O, Open's, and Alt with a letter, the chord that opens a menu by its underlined letter, Alt+F for File; tapping Alt alone gets through. And the browser keeps a browser's keys: F5 and Ctrl+R reload the page, Ctrl+P prints it, Ctrl+F opens a find bar, and Alt+Left goes back. None of that belongs in a desktop application, and a reload would run the page's startup again beside an engine that never stopped. So as the window is made, a handler goes on that event. In every build it hands the menu back its keys: F1 sends View Help's own event, Ctrl+O sends Open's, and Alt with a letter posts the window the message Windows itself sends to open a menu by its letter. In a release build it also tells the browser to skip every other key it might take for its own, and which keys those are stays Microsoft's list rather than one of ours, while the page still receives every key. WebView2 also has a single setting that turns the browser's keys off, and it would be simpler, but it takes effect only at the next navigation: by the time Tauri hands the web view over, the first page is already loading, and ftorrent never navigates again. A development build keeps the browser's keys, for reloading and the developer tools. On macOS the menu hears its keys before the web view does, and the web views on macOS and Linux have no browser keys. The right-click menu is a browser's on every platform, and the page turns it away, in main.js, with the same code everywhere.
*/

/// Whether the page has placed the window yet, and shown it unless a login started this copy; managed by lib.rs
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
		Ok(window) => _keys(&window),
		Err(e) => eprintln!("could not make the window: {e}"),//a window that never appears is worth a line, though in a release build nobody reads it; the tray still offers exit
	}
}

/// Hand the menu back the keys WebView2 keeps from it, F1, Ctrl+O, and Alt with a letter, and in a release build turn off the browser's own shortcut keys, reload, print, find, back, and the rest; on Windows only, where WebView2 does both
#[cfg(target_os = "windows")]
fn _keys(window: &WebviewWindow) {
	use tauri::Emitter;
	use webview2_com::AcceleratorKeyPressedEventHandler;
	use webview2_com::Microsoft::Web::WebView2::Win32::{ICoreWebView2AcceleratorKeyPressedEventArgs2, COREWEBVIEW2_KEY_EVENT_KIND, COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN, COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN};
	use windows::core::Interface;//for cast, which asks the event's arguments for the newer interface that holds the switch
	use windows::Win32::Foundation::{HWND, LPARAM, WPARAM};
	use windows::Win32::UI::Input::KeyboardAndMouse::{GetKeyState, VIRTUAL_KEY, VK_CONTROL, VK_MENU, VK_SHIFT};
	use windows::Win32::UI::WindowsAndMessaging::{PostMessageW, SC_KEYMENU, WM_SYSCOMMAND};
	let Ok(hwnd) = window.hwnd() else { return };
	let hwnd = hwnd.0 as isize;//the window's handle as a number, since a handle as it is can't cross to the main thread
	let app = window.app_handle().clone();
	let _ = window.with_webview(move |webview| unsafe {//runs on the main thread, where the web view lives
		let handler = AcceleratorKeyPressedEventHandler::create(Box::new(move |_, args| {//every key that isn't typing comes through here first: a function key, or a key with ctrl or alt held
			let Some(args) = args else { return Ok(()) };
			let mut kind = COREWEBVIEW2_KEY_EVENT_KIND::default();
			args.KeyEventKind(&mut kind)?;//a system key is one pressed with alt held and ctrl not, or F10
			let mut key = 0;
			args.VirtualKey(&mut key)?;//windows' code for the key, which for a letter is its capital's, 0x41 through 0x5a
			let mut lparam = 0;
			args.KeyEventLParam(&mut lparam)?;
			let repeat = lparam & (1 << 30) != 0;//the key was already down, so this is a held key repeating
			if kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN && key == 0x70 {//F1
				args.SetHandled(true)?;
				if !repeat { let _ = app.emit("menu", "help"); }//View Help's own event, as lifecycle.rs's menu handler sends it, once per press
				return Ok(())
			}
			let held = |key: VIRTUAL_KEY| GetKeyState(key.0 as i32) < 0;//whether a key was down as this one went down; the high bit, which makes the number negative, says so
			if kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN && key == 0x4f && held(VK_CONTROL) && !held(VK_MENU) && !held(VK_SHIFT) {//ctrl and O alone; ctrl with alt is AltGr, which types letters like the ó of a Polish keyboard, and is left to type
				args.SetHandled(true)?;
				if !repeat { let _ = app.emit("menu", "open"); }//File, Open's own event, as lifecycle.rs's menu handler sends it
				return Ok(())
			}
			if kind == COREWEBVIEW2_KEY_EVENT_KIND_SYSTEM_KEY_DOWN && (0x41..=0x5a).contains(&key) {//a letter with alt held
				args.SetHandled(true)?;
				if !repeat { let _ = PostMessageW(Some(HWND(hwnd as _)), WM_SYSCOMMAND, WPARAM(SC_KEYMENU as usize), LPARAM(key as isize + 0x20)); }//the message Windows itself sends a window for alt and a letter, with the letter in lowercase as it sends it: the menu with that underlined letter opens, F for File, and with none there's the system's beep
				return Ok(())
			}
			if !cfg!(debug_assertions) {//a development build keeps the browser's keys, for reloading the page and opening the developer tools
				if let Ok(args) = args.cast::<ICoreWebView2AcceleratorKeyPressedEventArgs2>() { args.SetIsBrowserAcceleratorKeyEnabled(false)?; }//skip the browser's handling of this key; the page still gets it, and editing keys like ctrl+c were never the browser's. The newer interface arrived in webview2 1.0.2210, and on a runtime older than that the cast fails and the key stays the browser's
			}
			Ok(())
		}));
		let mut token = 0;//what removing the handler would take; it stays for the life of the window
		let _ = webview.controller().add_AcceleratorKeyPressed(&handler, &mut token);//a failure leaves the keys as WebView2 has them, the way they started
	});
}

#[cfg(not(target_os = "windows"))]
fn _keys(_window: &WebviewWindow) {}//on macOS the menu hears its keys before the web view does, and the web views on macOS and Linux have no browser keys to turn off

/// The page has placed the window, and shown it unless a login started this copy, so bringing it forward is safe from here on
#[command]
pub fn window_revealed(revealed: State<'_, Revealed>) {
	revealed.0.store(true, Ordering::SeqCst);
}

/// The version of the web view the page runs in, as the platform reports it: WebView2's on Windows, and WebKit's on macOS and Linux
#[command]
pub fn window_webview_version() -> Result<String, String> {
	tauri::webview_version().map_err(|e| e.to_string())
}
