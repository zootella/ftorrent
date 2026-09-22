//./src-tauri/src/lib.rs

mod disk;//compile disk.rs as a module named disk
mod engine;//and engine.rs: the process that holds libtorrent, started here and stopped from the run events below

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
		.invoke_handler(//register all the commands JS can invoke
			tauri::generate_handler![
				disk::disk_readdir,//functions we've written in disk.rs
				disk::disk_stat,
				disk::disk_read,
				disk::disk_copy,
				engine::engine_status,//and in engine.rs
				greet,//the scaffold's demonstration command
			]
		)
		.setup(|app| {//before any page exists
			engine::engine_start(app.handle());//so the engine is already up, or already known to have failed, by the time the page asks
			Ok(())
		})
		.build(tauri::generate_context!())//build rather than run, so the closure below sees the application's events
		.expect("error while building tauri application")
		.run(|app, event| match event {//every event the application loop produces, for the life of the process
			tauri::RunEvent::ExitRequested { .. } => engine::engine_stop(app),//the last window closed; stop the engine now rather than at a quit that may be later
			tauri::RunEvent::Exit => engine::engine_stop(app),//the one event every way of quitting reaches, and a second call finds nothing left to stop
			_ => {}//RunEvent is non-exhaustive, and everything else is somebody else's business
		});
}
