//./src-tauri/src/desktop.rs

use std::collections::HashMap;
use std::sync::Mutex;
use tauri::{command, AppHandle, Manager, State};

/*
Text the page wants written when ftorrent exits. The page hands down a path and the text that ought to be there, and this writes it from RunEvent::Exit; a second call for the same path replaces the first. It never looks inside the text and doesn't know it's the settings file, which is the point: the page owns the settings, and Rust owns the one moment the page can't reach.

That moment is why this exists. The page writes settings itself, through disk_write, whenever a setting changes, and that covers nearly everything. What it can't cover is the window's position for a user who never presses X: the page records the position in memory on every move and writes it only when the window is closed with its X, so quitting from the tray or the File menu would lose it. Exit is the one event every quit path reaches, and it arrives on the main thread inside an operating system callback: a synchronous file write is fine there, but a round trip to JavaScript needs the run loop to turn again, and it won't. So the page keeps this map current as it goes, an in-memory copy costing nothing, and Rust writes whatever is held when the process leaves.

A write that fails at exit has nowhere to report; the page is unreachable and a release build has no console. That's an honest trade for a small file the page rewrites on every change anyway, and the page's own write, which can report trouble, is the one that gets witnessed.
*/

/// The text to write to each path when the application exits; a tuple struct reached as .0, behind a Mutex because tauri shares managed state by reference, and holding new text means changing it
#[derive(Default)]
pub struct ExitFiles(pub Mutex<HashMap<String, String>>);

/// Hold this text for this path until exit, replacing whatever was held for it; blank text forgets the path
#[command]
pub fn desktop_exit_hold(files: State<'_, ExitFiles>, path: String, text: String) {
	let mut files = files.0.lock().unwrap_or_else(|poisoned| poisoned.into_inner());//take the map even if a previous holder panicked; losing the settings for that would be worse
	if text.is_empty() { files.remove(&path); } else { files.insert(path, text); }
}

/// Write everything held, draining as it goes so a second call finds nothing; called from RunEvent::Exit in lib.rs
pub fn desktop_exit_write(app: &AppHandle) {
	let state = app.state::<ExitFiles>();
	let mut files = state.0.lock().unwrap_or_else(|poisoned| poisoned.into_inner());
	for (path, text) in files.drain() {
		if let Err(e) = std::fs::write(&path, text) { eprintln!("ftorrent could not write {path} on the way out: {e}") }//the page can't be told now; in a release build this line goes nowhere, and the page's own write on each change is what gets witnessed
	}
}
