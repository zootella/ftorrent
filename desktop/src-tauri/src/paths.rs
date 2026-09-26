//./src-tauri/src/paths.rs

use std::path::{Path, PathBuf};
use serde::Serialize;
use tauri::{command, AppHandle, Manager, State};

/*
Where everything is. This runs in setup, before instance.rs takes the lock, before the engine starts, and before the page exists, and everything that reads or writes a file of ftorrent's own takes its path from what this works out. Nothing else in the app asks Tauri or the operating system where to put things.

Three anchors, and every path comes from one of them. The program's location is the folder the program sits in: beside the executable on Windows and Linux, and the folder holding ftorrent.app on macOS, not a folder inside the bundle. The data folder is ftorrent's own, and holds ftorrent.toml and the lock, and, as the sprint goes on, the libtorrent state and the crash log. And the user's home folder. The page gets all three, because the page is what turns a download folder setting like ./downloads or ~/Downloads/ftorrent into a real path: ./ is relative to the program's location, so it follows a portable copy onto whatever drive letter or mount point it lands on, ~ is relative to whoever is signed in, and an absolute path like D:/torrents means exactly that place, on the machines that have it. Settings are written with forward slashes on every platform. Neither side checks whether a download folder exists: on macOS the default sits in Downloads, which the system guards behind a permission prompt, and the first look inside is what raises it, so that look waits for the step that locks download folders and can warn the user first.

One decision picks the data folder. If a folder named portable with a ftorrent.toml inside sits at the program's location, this copy is portable, and that folder is its data folder, so everything it keeps travels with it. Otherwise it's an installed copy, and the data folder is the one the platform gives the signed-in user: AppData\Local\com.ftorrent.ftorrent on Windows, where Tauri already keeps the web view's profile, and Application Support on macOS. An installed copy keeps nothing of its own in Roaming. Anything that isn't portable is installed, whatever the reason: a development build, a copy on the Desktop, or a Mac app that macOS has translocated, running it from a random read-only folder because it still carries a download's quarantine mark and was opened from where it arrived. That last one is usually someone who opened ftorrent inside its disk image without dragging it to Applications, and the installed data folder is exactly what they want, since it doesn't depend on where the app is. A translocated portable copy can't see its portable folder, so it acts installed too, and works; the portable instructions clear the mark first, which is what keeps it portable.

This module never opens ftorrent.toml. The page owns the settings, reads the file once it's up, repairs it, and writes it, and Rust knows the file only as a path to hand over, as text to write when the page asks, and, in settings.rs, as the window's size, place, and maximized flag, read before the page exists. That keeps every setting defined in one place, the page's schema, with nothing duplicated on this side of the boundary.
*/

const SETTINGS_NAME: &str = "ftorrent.toml";//the settings file, in the data folder; the page reads and writes it
const PORTABLE_NAME: &str = "portable";//the folder whose ftorrent.toml makes a copy portable
const STATE_NAME: &str = "state";//libtorrent's session state, the DHT routing table among it

/// Everything startup worked out about where things are, which the page shows and resolves settings against, and the engine is told
#[derive(Serialize, Clone, Default)]
pub struct Paths {
	pub mode: String,//installed or portable
	pub location: String,//the program's location, the anchor for ./
	pub home: String,//the signed-in user's home folder, the anchor for ~
	pub data: String,//the data folder
	pub settings: String,//ftorrent.toml in the data folder
	pub state: String,//the libtorrent state file in the data folder
	pub trouble: String,//what went wrong, blank when nothing did; startup carries on rather than stopping
}

/// Find the program's location and the data folder; instance.rs locks the data folder next
pub fn locate(app: &AppHandle) -> Paths {
	let mut paths = Paths {
		home: app.path().home_dir().map(|home| display(&home)).unwrap_or_default(),//blank on a platform that can't say, and then ~ resolves to nothing rather than to the wrong place
		..Default::default()//the rest filled in below, as each is found
	};

	let location = match program_location() {
		Ok(location) => location,
		Err(trouble) => { paths.trouble = trouble; return paths }
	};
	paths.location = display(&location);

	let portable = location.join(PORTABLE_NAME);
	let data = if portable.join(SETTINGS_NAME).is_file() {
		paths.mode = "portable".to_string();
		portable
	} else {
		paths.mode = "installed".to_string();
		match app.path().app_local_data_dir() {//per user, named by the bundle identifier; Local rather than Roaming on Windows
			Ok(data) => data,
			Err(e) => { paths.trouble = format!("the platform gave no data folder: {e}"); return paths }
		}
	};
	if let Err(e) = std::fs::create_dir_all(&data) { paths.trouble = format!("could not create the data folder: {e}") }//an installed copy's first launch; the lock and the settings file both go inside, so the folder has to exist before either
	paths.data = display(&data);
	paths.settings = display(&data.join(SETTINGS_NAME));
	paths.state = display(&data.join(STATE_NAME));
	paths
}

/// The folder the program sits in: beside the executable, or beside the .app on macOS
fn program_location() -> Result<PathBuf, String> {
	let exe = std::env::current_exe().map_err(|e| format!("could not find the program's own path: {e}"))?;
	let folder = exe.parent().ok_or("the program's path has no folder")?;
	if cfg!(target_os = "macos") {
		//inside a bundle the executable is at ftorrent.app/Contents/MacOS/ftorrent, and the user thinks of the program as ftorrent.app, so the location is the folder holding that; a binary outside a bundle, like the one tauri dev runs, stays where it is
		let contents = folder.parent();
		let bundle = contents.and_then(|c| c.parent());
		if folder.file_name().is_some_and(|n| n == "MacOS") && contents.and_then(|c| c.file_name()).is_some_and(|n| n == "Contents") && bundle.and_then(|b| b.extension()).is_some_and(|e| e == "app") {
			return bundle.and_then(|b| b.parent()).map(Path::to_path_buf).ok_or_else(|| "the app bundle has no folder".to_string());
		}
	}
	Ok(folder.to_path_buf())
}

/// A path as text, for the page and the engine
fn display(path: &Path) -> String {
	path.to_string_lossy().into_owned()
}

/// Where everything is, as startup worked it out; the page asks once, since none of it changes while the app runs
#[command]
pub fn paths_status(paths: State<'_, Paths>) -> Paths {
	paths.inner().clone()
}
