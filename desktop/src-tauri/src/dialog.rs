use tauri::{command, AppHandle, WebviewWindow};
use crate::run_blocking;

/*
The operating system's own boxes for choosing what to open and where to save, which only Rust can put up. There are three, and every desktop platform has all three: an open box for files, the same box choosing a folder instead, and a save box. On Windows they're the common item dialog: IFileOpenDialog, the same with FOS_PICKFOLDERS for a folder, which Microsoft has recommended since Vista in place of the old Browse For Folder box that shows only a tree, and IFileSaveDialog. On a mac they're AppKit's NSOpenPanel and NSSavePanel. On Linux they're GTK's file chooser, which the dialog plugin uses by default, or the desktop portal's, which a sandbox like a Flatpak's needs to reach past the folders it was granted. A mac's open panel can also choose a file or a folder in one box, which Windows and Linux can't, so a box asked for either is a box for files off the mac: the asker writes the same call on every platform, and only the Mac's panel makes more of it.

On a mac the panels are called directly. Off the mac it's the dialog plugin's Rust half, which hands each box to the rfd crate, and rfd calls the system's own. None of the plugin is granted to the page, so these commands are the only way the page reaches a dialog, and they reach only these: the plugin's message, ask, and confirm boxes stay out of reach, since a native-looking box an application can fill with any text is a way to fool people.

A box meets one of three windows, and the decision is made once, before the platforms split. A window on the screen holds the box: on a mac the panel slides down from it as a sheet, and off the mac the box is owned by it, kept in front of it and the window waiting until the box closes. A window that's hidden or minimized can't hold one: a sheet on a window nobody can see goes unseen, and the asker waits until someone brings the window back, and a box owned by a hidden window has no taskbar button of its own and is hard to find once something covers it; so the box stands on its own, as it would with no window. That much is reasoned from how AppKit attaches a sheet and how Windows treats an owned window, since neither program here asks from a hidden window yet. And with no window at all there's no page to ask, so only Rust can, which happens on a mac alone, where an app keeps running with every window closed: dialog_alone is that way in, putting the panel up on its own.

The asker says what it wants, and nothing here knows what the answer is for. start is the folder the box opens in, written with either slash on Windows, blank to leave it to the system, which on Windows means the last folder used by a program of the same file name, and before there is one, the folder the process was started in, which for an installed copy is the program's own folder, where a box should never open. kind and extensions are a filter: what the files are called and the extensions that make one, without their dots, like Text documents and txt, or no extensions to offer every file. A filter is for a box that chooses only files, so a box that can choose a folder takes none, on every platform; on a mac a folder is of no file's type, and filtering would show every folder as one that can't be chosen. The extensions are a list, which a command here takes only on purpose: they are one setting of the one box, handed to the system whole.
*/

/// Put the system's open box up over the window asking, or on its own while that window is hidden or minimized, for files, or with folder for a folder, or with either for a file or a folder on a mac and files elsewhere, one or with multiple several, opened in start and offering the files extensions names, every file when there are none; answer the paths chosen, none when the user cancels. On the blocking pool, because the answer waits on the user
#[command]
pub async fn dialog_open(app: AppHandle, window: WebviewWindow, folder: bool, either: bool, multiple: bool, start: String, kind: String, extensions: Vec<String>) -> Result<Vec<String>, String> {
	run_blocking(move || show(&app, Some(&window), Asked { folder, either, multiple, start, kind, extensions, ..Default::default() })).await
}

/// Put the system's save box up over the window asking, or on its own while that window is hidden or minimized, opened in start, suggesting the file name name, and offering the files extensions names; answer the path chosen, blank when the user cancels. The box asks before replacing a file that's there, on every platform, so a path that comes back is one the user agreed to write. On the blocking pool, because the answer waits on the user
#[command]
pub async fn dialog_save(app: AppHandle, window: WebviewWindow, start: String, name: String, kind: String, extensions: Vec<String>) -> Result<String, String> {
	let chosen = run_blocking(move || show(&app, Some(&window), Asked { save: true, start, name, kind, extensions, ..Default::default() })).await?;
	Ok(chosen.into_iter().next().unwrap_or_default())
}

/// Put the open or save box up on its own, with no window behind it, for Rust to ask when no window is open to; call on the main thread, where answer gets the paths chosen, none for a cancel. On a mac alone, the one platform where an app runs with no window
#[cfg(target_os = "macos")]
#[allow(dead_code)]//for a caller with no window to ask from, which a program whose window only ever hides doesn't have
pub fn dialog_alone(asked: Asked, answer: impl FnOnce(Vec<String>) + 'static) {
	mac::panel(None, &asked, answer)
}

/// One box, as the asker described it; a caller in Rust writes the fields it wants and takes the rest from Default, like Asked { folder: true, ..Default::default() }
#[derive(Default)]
pub(crate) struct Asked {
	pub save: bool,//a save box, rather than an open box
	pub folder: bool,//an open box choosing a folder, rather than files
	pub either: bool,//on a mac, an open box choosing a file or a folder in one, which only the Mac's panel can; off the mac, a box for files
	pub multiple: bool,//an open box choosing several
	pub start: String,//the folder it opens in, blank for the system's choice
	pub name: String,//the file name a save box suggests, blank for none
	#[cfg_attr(target_os = "macos", allow(dead_code))]//read off the mac alone, since the Mac's panels name no filter
	pub kind: String,//what the files the filter offers are called
	pub extensions: Vec<String>,//the extensions that make one, without their dots, none for every file
}

impl Asked {
	fn filtered(&self) -> bool {//whether the box takes the filter: it chooses only files, an open box for them or a save box, and has extensions to filter by
		!self.folder && !self.either && !self.extensions.is_empty()
	}
}

//the window the box goes over, or none for a box on its own: a window on the screen holds it, and one hidden or minimized can't, since a box on a window nobody sees is lost; Tauri's answers here wait on the main thread, as everything on this pool thread may
fn show(app: &AppHandle, window: Option<&WebviewWindow>, asked: Asked) -> Result<Vec<String>, String> {
	let parent = window.filter(|window| window.is_visible().unwrap_or(false) && !window.is_minimized().unwrap_or(true));//a window that can't say is treated as one that can't hold a box
	put_up(app, parent, asked)
}

#[cfg(target_os = "macos")]
fn put_up(app: &AppHandle, parent: Option<&WebviewWindow>, asked: Asked) -> Result<Vec<String>, String> {//the panel, as a sheet on the window or on its own, put up on the main thread and waited for here; through the app rather than the window, since there may be no window; the Mac's panels name no filter, so kind goes unused
	let (sender, receiver) = std::sync::mpsc::channel();
	let asking = parent.cloned();
	app.run_on_main_thread(move || {
		let address = asking.and_then(|window| window.ns_window().ok());//the NSWindow tauri built, read here on the main thread, where windows close, so it can't close before the sheet is on it; a window already gone leaves the panel on its own
		let parent = address.map(|address| unsafe { &*(address as *const objc2_app_kit::NSWindow) });
		mac::panel(parent, &asked, move |paths| { let _ = sender.send(paths); })
	}).map_err(|e| e.to_string())?;
	receiver.recv().map_err(|_| "the box never answered".to_string())//a sender dropped unanswered, which only a panel that never went up leaves
}

#[cfg(not(target_os = "macos"))]
fn put_up(app: &AppHandle, parent: Option<&WebviewWindow>, asked: Asked) -> Result<Vec<String>, String> {//the dialog plugin's box, owned by the window or on its own, which blocks this pool thread while the plugin runs it on the main thread
	use tauri_plugin_dialog::{DialogExt, FilePath};
	let mut dialog = app.dialog().file();
	if let Some(window) = parent { dialog = dialog.set_parent(window) }//owned by the window: in front of it, the window waiting, and no taskbar button of its own; without one the box gets its own button, which is how it's found
	if !asked.start.is_empty() {
		let start = if cfg!(windows) { asked.start.replace('/', "\\") } else { asked.start.clone() };//windows' shell refuses a folder written with forward slashes, which rfd then skips without a word, so on windows the folder goes down in windows' own spelling, whichever way the asker wrote it; a backslash is a letter in a linux name, so linux's paths go as they are
		dialog = dialog.set_directory(&start)//on windows rfd hands it to the box as SetFolder, which outranks the folder windows remembers, so the box opens there every time; a folder that isn't there is skipped, and the system chooses as if none were named
	}
	if !asked.name.is_empty() { dialog = dialog.set_file_name(&asked.name) }
	if asked.filtered() {
		#[cfg(windows)]
		let kind = format!("{} ({})", asked.kind, asked.extensions.iter().map(|extension| format!("*.{extension}")).collect::<Vec<_>>().join(";"));//windows names a type with its patterns after it, like Text Documents (*.txt), and its box shows the name as given; linux's boxes show the name alone
		#[cfg(not(windows))]
		let kind = asked.kind.clone();
		dialog = dialog.add_filter(kind, &asked.extensions.iter().map(String::as_str).collect::<Vec<_>>());//on windows rfd also makes the first extension the one a save box adds to a name typed without one
	}
	let folder = asked.folder && !asked.either;//a box for either is a box for files here, where no box can choose both
	let chosen: Vec<FilePath> = match (asked.save, folder, asked.multiple) {
		(true, _, _)          => dialog.blocking_save_file().into_iter().collect(),
		(false, true, false)  => dialog.blocking_pick_folder().into_iter().collect(),
		(false, true, true)   => dialog.blocking_pick_folders().unwrap_or_default(),
		(false, false, false) => dialog.blocking_pick_file().into_iter().collect(),
		(false, false, true)  => dialog.blocking_pick_files().unwrap_or_default(),
	};//nothing, for a cancel, which is an ordinary answer
	chosen.into_iter().map(|chosen| {
		let path = chosen.into_path().map_err(|e| e.to_string())?;
		path.into_os_string().into_string().map_err(|path| format!("the path chosen is not text: {path:?}"))//a path that isn't utf-8 can't cross to the page as a string
	}).collect()
}

#[cfg(target_os = "macos")]
mod mac {
	use std::cell::Cell;
	use block2::RcBlock;
	use objc2::MainThreadMarker;
	use objc2_app_kit::{NSModalResponse, NSModalResponseOK, NSOpenPanel, NSSavePanel, NSWindow};
	use objc2_foundation::{NSArray, NSString, NSURL};
	use objc2_uniform_type_identifiers::UTType;
	use super::Asked;

	pub fn panel(parent: Option<&NSWindow>, asked: &Asked, answer: impl FnOnce(Vec<String>) + 'static) {//the box asked for, as a sheet on the parent or on its own, answered once when the user closes it
		let Some(marker) = MainThreadMarker::new() else { return };//every caller runs on the main thread; were one not to, the dropped answer reaches the asker as an error rather than as a cancel
		if asked.save {
			let panel = NSSavePanel::savePanel(marker);
			shared(&panel, asked);
			if !asked.name.is_empty() { panel.setNameFieldStringValue(&NSString::from_str(&asked.name)) }
			let shown = panel.clone();//the block reads the choice off the panel, and holds it until appkit lets the block go
			begin(&panel, parent, move || shown.URL().and_then(|url| url.path()).map(|path| path.to_string()).into_iter().collect(), answer);
		} else {
			let panel = NSOpenPanel::openPanel(marker);
			shared(&panel, asked);
			panel.setCanChooseFiles(!asked.folder || asked.either);
			panel.setCanChooseDirectories(asked.folder || asked.either);
			panel.setAllowsMultipleSelection(asked.multiple);
			let shown = panel.clone();
			begin(&panel, parent, move || shown.URLs().iter().filter_map(|url| url.path()).map(|path| path.to_string()).collect(), answer);
		}
	}

	fn shared(panel: &NSSavePanel, asked: &Asked) {//what an open panel and a save panel take alike, since NSOpenPanel is an NSSavePanel underneath
		panel.setCanCreateDirectories(true);//New Folder in the panel, as every box on Windows has, which matters most when choosing a folder
		if !asked.start.is_empty() { panel.setDirectoryURL(Some(&NSURL::fileURLWithPath_isDirectory(&NSString::from_str(&asked.start), true))) }//where the panel opens, in place of the last folder appkit remembers for the app; blank leaves that choice to appkit
		if !asked.filtered() { return }//a box that can choose a folder takes no filter, and one with no extensions has none to take
		let types: Vec<_> = asked.extensions.iter().filter_map(|extension| UTType::typeWithFilenameExtension(&NSString::from_str(extension))).collect();//each extension as the type the system knows it by, or one made up for it on the spot when the system knows none
		if !types.is_empty() { panel.setAllowedContentTypes(&NSArray::from_retained_slice(&types)) }//the files it offers, and for a save box the extension it adds to the name; folders still show, so the user can walk into them
	}

	fn begin(panel: &NSSavePanel, parent: Option<&NSWindow>, read: impl Fn() -> Vec<String> + 'static, answer: impl FnOnce(Vec<String>) + 'static) {//put the panel up, and answer once, with what read takes off it when the user chooses, or nothing for a cancel
		let answer = Cell::new(Some(answer));//appkit takes a block it may call any number of times, and this one answers the first time only
		let done = RcBlock::new(move |response: NSModalResponse| {
			let paths = if response == NSModalResponseOK { read() } else { Vec::new() };
			if let Some(answer) = answer.take() { answer(paths) }
		});
		match parent {
			Some(window) => panel.beginSheetModalForWindow_completionHandler(window, &done),//slides down from the window's title bar, and holds that window alone
			None => panel.beginWithCompletionHandler(&done),//a window of its own, which is how a mac app asks with no window to ask from
		}
	}
}
