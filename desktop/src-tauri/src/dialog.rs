use tauri::{command, WebviewWindow};
use crate::run_blocking;

/*
The operating system's own open box, which only Rust can put up, told whether it may choose files, folders, or both. On a mac it's AppKit's NSOpenPanel, called directly and shown as a sheet that slides down from the window, because the Mac's box takes those two switches together and the dialog plugin offers one or the other. Off the mac it's the dialog plugin's Rust half, since the open boxes of Windows and of Linux's desktops choose a file or a folder, never either in one. None of the plugin is granted to the page, so this command is the only way the page reaches a dialog, and it reaches only this one: the plugin's message, ask, and confirm boxes stay out of reach, since a native-looking box an application can fill with any text is a way to fool people.
*/

/// Put the system's open box up over the window asking, for one file, one folder, or on a mac either, and answer the path chosen, or blank when the user cancels. Off the mac, asked for both, it offers a file. On the blocking pool, because the answer waits on the user
#[command]
pub async fn dialog_open(window: WebviewWindow, files: bool, folders: bool) -> Result<String, String> {
	run_blocking(move || dialog_open_over(&window, files, folders)).await
}

#[cfg(target_os = "macos")]
fn dialog_open_over(window: &WebviewWindow, files: bool, folders: bool) -> Result<String, String> {//the panel as a sheet on this window, put up on the main thread and waited for here
	let (sender, receiver) = std::sync::mpsc::channel();
	let asking = window.clone();
	window.run_on_main_thread(move || {
		let Ok(address) = asking.ns_window() else { return };//the NSWindow tauri built, read here on the main thread, where windows close, so it can't close before the sheet is on it; a window already gone drops the answer
		let parent = unsafe { &*(address as *const objc2_app_kit::NSWindow) };
		mac::dialog_panel(parent, files, folders, move |path| { let _ = sender.send(path); })
	}).map_err(|e| e.to_string())?;
	receiver.recv().map_err(|_| "the open box never answered".to_string())//a sender dropped unanswered, which only a panel that never went up leaves
}

#[cfg(not(target_os = "macos"))]
fn dialog_open_over(window: &WebviewWindow, files: bool, folders: bool) -> Result<String, String> {//the dialog plugin's box, which blocks this pool thread while the plugin runs it on the main thread
	use tauri_plugin_dialog::DialogExt;
	let dialog = window.dialog().file().set_parent(window);
	let chosen = if folders && !files { dialog.blocking_pick_folder() } else { dialog.blocking_pick_file() };
	let Some(chosen) = chosen else { return Ok(String::new()) };//cancelled, which is an ordinary answer
	let path = chosen.into_path().map_err(|e| e.to_string())?;
	path.into_os_string().into_string().map_err(|path| format!("the path chosen is not text: {path:?}"))//a path that isn't utf-8 can't cross to the page as a string
}

#[cfg(target_os = "macos")]
mod mac {
	use std::cell::Cell;
	use block2::RcBlock;
	use objc2::MainThreadMarker;
	use objc2_app_kit::{NSModalResponse, NSModalResponseOK, NSOpenPanel, NSWindow};

	pub fn dialog_panel(parent: &NSWindow, files: bool, folders: bool, answer: impl FnOnce(String) + 'static) {//one item, as a sheet on the parent, answered once when the user closes it
		let Some(marker) = MainThreadMarker::new() else { return };//the caller runs on the main thread; were it not to, the dropped answer reaches the page as an error rather than as a cancel
		let panel = NSOpenPanel::openPanel(marker);
		panel.setCanChooseFiles(files);
		panel.setCanChooseDirectories(folders);
		panel.setAllowsMultipleSelection(false);
		let answer = Cell::new(Some(answer));//appkit takes a block it may call any number of times, and this one answers the first time only
		let shown = panel.clone();//the block reads the choice off the panel, and holds it until appkit lets the block go
		let done = RcBlock::new(move |response: NSModalResponse| {
			let mut path = String::new();//blank for a cancel
			if response == NSModalResponseOK { path = shown.URL().and_then(|url| url.path()).map(|p| p.to_string()).unwrap_or_default() }
			if let Some(answer) = answer.take() { answer(path) }
		});
		panel.beginSheetModalForWindow_completionHandler(parent, &done);//slides down from the window's title bar, and holds that window alone
	}
}
