use std::collections::HashMap;
use std::sync::Mutex;
use tauri::{command, AppHandle, Manager, State};

/*
Text to write when the process exits. The page hands down a path and the text that ought to be there, and this writes it from RunEvent::Exit; a second call for the same path replaces the first, and blank text forgets the path. It never looks inside the text.

It exists for the one moment the page can't reach. Exit is the event every way of quitting reaches, and it arrives on the main thread inside an operating system callback: a synchronous file write is fine there, but a round trip to JavaScript needs the run loop to turn again, and it won't. So the page keeps what it wants written current here as it goes, an in-memory copy costing nothing, and Rust writes whatever is held when the process leaves. Today that's the settings file, whose window place changes too often to write on every move.

A write that fails at exit has nowhere to report: the page is unreachable, and a release build has no console. The page writes the same file itself whenever it can report trouble, so this write is the backstop, not the witness.
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
		if let Err(e) = std::fs::write(&path, text) { eprintln!("could not write {path} on the way out: {e}") }//the page can't be told now; in a release build this line goes nowhere, and the page's own write on each change is what gets witnessed
	}
}
