//./src-tauri/src/lib.rs

mod disk;//compile disk.rs as a module named disk
mod engine;//and engine.rs: the process that holds libtorrent, started here and stopped from the run events below
mod paths;//and paths.rs: where everything is, worked out once at startup
mod instance;//and instance.rs: one running ftorrent per copy, and a second launch handing over what it carried
mod lifecycle;//and lifecycle.rs: closing hides the window, and quitting is explicit

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
		.invoke_handler(//register all the commands JS can invoke
			tauri::generate_handler![
				disk::disk_readdir,//functions we've written in disk.rs
				disk::disk_stat,
				disk::disk_read,
				disk::disk_copy,
				engine::engine_status,//and in engine.rs
				paths::paths_status,//and in paths.rs
				instance::instance_status,//and in instance.rs
				greet,//the scaffold's demonstration command
			]
		)
		.setup(|app| {//before any page exists
			let mut paths = paths::locate(app.handle());//where this copy is, and which data folder is its own
			if let instance::Start::Leave = instance::start(app.handle(), &paths) {//another process is this copy, and now has what this launch carried
				std::process::exit(0);//nothing has started yet, so there's nothing to stop: no engine, no lock, and the window was never shown
			}
			paths::read(app.handle(), &mut paths);//only the copy holding the lock reads and writes ftorrent.json
			app.manage(paths);//before the engine, which is told where everything is
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
			tauri::RunEvent::ExitRequested { .. } => engine::engine_stop(app),//a quit, or on linux the window closing; stop the engine now rather than at an exit that may be later
			#[cfg(target_os = "macos")]
			tauri::RunEvent::Reopen { has_visible_windows, .. } => { if !has_visible_windows { lifecycle::bring_forward(app) } }//the dock icon clicked while the window is hidden, which is how a mac user asks for it back
			tauri::RunEvent::Exit => {//the one event every way of quitting reaches, and a second call finds nothing left to stop
				engine::engine_stop(app);
				#[cfg(target_os = "windows")]
				lifecycle::tray_remove(app);//so the icon goes with the process rather than lingering as a ghost
			}
			_ => {}//RunEvent is non-exhaustive, and everything else is somebody else's business
		});
}
