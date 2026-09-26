/*
The Rust core, and the one rule every module in it follows: Rust stays general. It reads like a general-purpose API, a set of commands that are atomic, do one thing each, and could belong to any desktop application: read a file, write bytes, make a folder, take a lock, write text when the process exits, start a process and pass it lines, read and write the registry, hide and show a window. Reading the command list below should tell you what the machine can be asked to do, and nothing about what ftorrent is.

The page above holds the application. It decides what a setting is, which folders to lock and when, what to say to the engine and what the engine's answers mean, what a second launch's magnet is for, and which registry keys make ftorrent the thing a .torrent file opens with. Rust receives each request and carries it out. Why the page wants it is above Rust's pay grade: a command here that has to know the reason, or a comment here that has to explain it, is the sign that application logic has leaked down, and names that belong to ftorrent, a registry key, a settings section, a command the engine understands, a folder's purpose, turning up in Rust are the same sign. The engine has its own road through here for the same reason: the page builds each command for libtorrent, Rust passes the line down without reading it, and passes the engine's lines back up the same way, so using a new libtorrent feature never needs new Rust; engine.rs has the long version.

This is safer, not only tidier. Logic that lives in one place stays true. Logic split across the boundary, half in the page and half down here, drifts apart as each half is edited alone, and the gap between the two halves is where bugs live; a rule or a guard in Rust that the page also knows is a second copy of the same knowledge, and second copies go stale. What makes it right for Rust to follow the page's commands without second-guessing them is that the page runs only ftorrent's own code: untrusted text, like file names, torrent metadata, and what peers send, reaches it only through Vue's escaping interpolation and never becomes script, and the Content-Security-Policy in tauri.conf.json keeps foreign script out of the web view even if that wall someday cracks. So the commands here hold no guards of their own.

Rust grows when the operating system is the only one who can do the thing, or when the page can't be there yet. The instance lock and the handoff between launches run in setup, before any page exists, and so does starting the engine. The tray, the menus, and hiding the window instead of closing it are platform plumbing only Rust can reach. Registry calls, file locks, and process pipes need native calls. And some facts belong down here, because they're facts about the machine rather than decisions: where the program is, where the user's home is, whether two paths name the same file, whether a queue overflowed. In each case Rust offers the operation in general terms, the way registry_get and registry_set read and write the registry without knowing a single key ftorrent uses, and the page, or the moment of startup, decides what to do with it.

The test for a new command is to describe it without naming a ftorrent feature. "Lock this folder" passes. "Lock the download folders in the settings, and let go of the ones no longer listed" fails, and the second half of it belongs to the page. A command that fails the test gets split: the general operation stays here, and the decision goes up.
*/

mod disk;//compile disk.rs as a module named disk: file commands the page calls, thin wrappers over std::fs
mod desktop;//and desktop.rs: text the page hands down to be written when ftorrent exits
mod engine;//and engine.rs: the process that holds libtorrent, started here and stopped from the run events below
mod paths;//and paths.rs: where everything is, worked out once at startup
mod queue;//and queue.rs: the drained queue the engine's lines and a copy's arrivals wait in until the page takes them
mod registry;//and registry.rs: the windows registry, read and written for the page
mod instance;//and instance.rs: one running ftorrent per copy, and a second launch handing over what it carried
mod lifecycle;//and lifecycle.rs: closing hides the window, and quitting is explicit
mod locks;//and locks.rs: exclusive locks on files, taken and released for the page
mod window;//and window.rs: the one window, made hidden for the page to place and show

use tauri::Manager;//brings manage into scope, for handing the paths to tauri's shared state in setup

// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
#[tauri::command]
fn greet(name: &str) -> String {
	format!("Hello, {}! You've been greeted from Rust!", name)
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
	tauri::Builder::default()
		.plugin(tauri_plugin_opener::init())
		.plugin(tauri_plugin_dialog::init())
		.manage(engine::Engine::default())//the engine's process and status, shared state any command can reach
		.manage(instance::Instance::default())//this copy's lock, and what has reached it
		.manage(locks::Locks::default())//the file locks this process holds for the page, kept open so they stay held
		.manage(desktop::ExitFiles::default())//text to write on the way out, by path
		.manage(window::Revealed::default())//whether the page has placed and shown the window yet
		.invoke_handler(//register all the commands JS can invoke
			tauri::generate_handler![
				disk::disk_readdir,//functions we've written in disk.rs
				disk::disk_stat,
				disk::disk_read,
				disk::disk_write,
				disk::disk_mkdir,
				disk::disk_hide,
				disk::disk_copy,
				desktop::desktop_exit_hold,//and in desktop.rs
				engine::engine_status,//and in engine.rs
				engine::engine_send,
				engine::engine_take,
				locks::lock_take,//and in locks.rs
				locks::lock_release,
				paths::paths_status,//and in paths.rs
				registry::registry_get,//and in registry.rs
				registry::registry_set,
				registry::registry_notify,
				window::window_revealed,//and in window.rs
				instance::instance_status,//and in instance.rs
				instance::instance_take,
				greet,//the scaffold's demonstration command
			]
		)
		.setup(|app| {//before any page exists
			let paths = paths::locate(app.handle());//where this copy is, and which data folder is its own
			if let instance::Start::Leave = instance::start(app.handle(), &paths) {//another process is this copy, and now has what this launch carried
				std::process::exit(0);//nothing has started yet, so there's nothing to stop: no engine, no lock, and no window, which is only built from the Ready event below
			}
			app.manage(paths);//before the engine, which is told where everything is; the settings file inside the data folder is the page's to read, once it's up, and only the copy holding the lock has a page
			engine::engine_start(app.handle());//so the engine is already up, or already known to have failed, by the time the page asks
			#[cfg(target_os = "windows")]
			lifecycle::tray_install(app.handle())?;//windows has no dock, so the tray is where a hidden ftorrent is brought back and quit
			#[cfg(target_os = "windows")]
			lifecycle::menu_install(app.handle())?;//and the window's own File menu is the other way to quit, the one that needs no tray
			Ok(())
		})
		.on_window_event(lifecycle::window_event)//the close button hides the window rather than closing it
		.build(tauri::generate_context!())//build rather than run, so the closure below sees the application's events
		.expect("error while building tauri application")
		.run(|app, event| match event {//every event the application loop produces, for the life of the process
			tauri::RunEvent::Ready => window::window_build(app),//the window, made here rather than declared in tauri.conf.json, so a launch that leaves in setup never builds one; window.rs has the other reason
			tauri::RunEvent::ExitRequested { .. } => engine::engine_stop(app),//a quit, or on linux the window closing; stop the engine now rather than at an exit that may be later
			#[cfg(target_os = "macos")]
			tauri::RunEvent::Reopen { has_visible_windows, .. } => { if !has_visible_windows { lifecycle::bring_forward(app) } }//the dock icon clicked while the window is hidden, which is how a mac user asks for it back
			tauri::RunEvent::Exit => {//the one event every way of quitting reaches, and a second call finds nothing left to stop
				desktop::desktop_exit_write(app);//whatever text the page handed down for this moment, first, because it's quick and the page can no longer do it
				engine::engine_stop(app);
				#[cfg(target_os = "windows")]
				lifecycle::tray_remove(app);//so the icon goes with the process rather than lingering as a ghost
			}
			_ => {}//RunEvent is non-exhaustive, and everything else is somebody else's business
		});
}
