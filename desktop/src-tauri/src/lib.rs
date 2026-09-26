//./src-tauri/src/lib.rs

mod associate;//compile associate.rs as a module named associate: what ftorrent tells windows it can open, taking nothing another program holds
mod disk;//and disk.rs: file commands the page calls, thin wrappers over std::fs
mod desktop;//and desktop.rs: text the page hands down to be written when ftorrent exits
mod engine;//and engine.rs: the process that holds libtorrent, started here and stopped from the run events below
mod folders;//and folders.rs: the lock inside each download folder, so two copies of ftorrent never share one
mod paths;//and paths.rs: where everything is, worked out once at startup
mod instance;//and instance.rs: one running ftorrent per copy, and a second launch handing over what it carried
mod lifecycle;//and lifecycle.rs: closing hides the window, and quitting is explicit
mod settings;//and settings.rs: the window's size, place, and maximized flag, which rust reads out of ftorrent.toml before the page exists; the page owns the rest of that file
mod window;//and window.rs: the one window, made at the size and place the user left it

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
		.manage(folders::Folders::default())//the download folders this copy holds, kept open so their locks stay held
		.manage(desktop::ExitFiles::default())//text to write on the way out, by path
		.manage(associate::Associate::default())//what registration did this launch, for the page
		.invoke_handler(//register all the commands JS can invoke
			tauri::generate_handler![
				associate::associate_status,//in associate.rs
				disk::disk_readdir,//functions we've written in disk.rs
				disk::disk_stat,
				disk::disk_read,
				disk::disk_write,
				disk::disk_mkdir,
				disk::disk_copy,
				desktop::desktop_exit_hold,//and in desktop.rs
				engine::engine_status,//and in engine.rs
				engine::engine_folders,
				folders::folder_lock,//and in folders.rs
				folders::folder_unlock,
				paths::paths_status,//and in paths.rs
				instance::instance_status,//and in instance.rs
				greet,//the scaffold's demonstration command
			]
		)
		.setup(|app| {//before any page exists
			let paths = paths::locate(app.handle());//where this copy is, and which data folder is its own
			if let instance::Start::Leave = instance::start(app.handle(), &paths) {//another process is this copy, and now has what this launch carried
				std::process::exit(0);//nothing has started yet, so there's nothing to stop: no engine, no lock, and no window, which is only built from the Ready event below
			}
			app.manage(paths);//before the engine, which is told where everything is; the settings file inside the data folder is the page's to read, once it's up, and only the copy holding the lock has a page
			associate::associate_register(app.handle());//tell windows what an installed copy can open; here, after the lock, so ten launches at once make one set of registry writes rather than ten racing each other
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
			tauri::RunEvent::Ready => window::window_build(app),//the window, made here rather than declared in tauri.conf.json, so a launch that leaves in setup never builds one; window.rs has the other two reasons
			tauri::RunEvent::ExitRequested { .. } => engine::engine_stop(app),//a quit, or on linux the window closing; stop the engine now rather than at an exit that may be later
			#[cfg(target_os = "macos")]
			tauri::RunEvent::Reopen { has_visible_windows, .. } => { if !has_visible_windows { lifecycle::bring_forward(app) } }//the dock icon clicked while the window is hidden, which is how a mac user asks for it back
			tauri::RunEvent::Exit => {//the one event every way of quitting reaches, and a second call finds nothing left to stop
				desktop::desktop_exit_write(app);//the settings text the page held for this moment, first, because it's quick and the page can no longer do it
				engine::engine_stop(app);
				#[cfg(target_os = "windows")]
				lifecycle::tray_remove(app);//so the icon goes with the process rather than lingering as a ghost
			}
			_ => {}//RunEvent is non-exhaustive, and everything else is somebody else's business
		});
}
