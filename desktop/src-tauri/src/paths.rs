use std::path::{Path, PathBuf};
use serde::Serialize;
use tauri::{command, AppHandle, Manager, State};

/*
Where everything is: facts about the machine and about this copy, worked out once in setup, before instance.rs takes the lock, before the engine starts, and before the page exists. Everything that reads or writes a file of the app's own takes its path from here, and the page gets all of it through paths_status.

The facts. The program's location, the folder the program sits in: beside the executable on Windows and Linux, and the folder holding the .app on macOS, not a folder inside the bundle. The executable itself. The signed-in user's home folder. On Windows, the folder the per-user installer puts the program in, %LOCALAPPDATA% joined with the product name, which the page compares with the location to know it's running from an installed copy. And the data folder, with the paths of the settings file and the state file inside it. The page turns settings into real paths against these, like a download folder written as ./downloads or ~/Downloads/ftorrent; paths.js has how.

One decision is made here, because it has to be made before the page exists: which data folder is this copy's. If a folder named portable with a settings file inside sits at the program's location, the copy is portable, and that folder is its data folder, so everything it keeps travels with it. Otherwise it's installed, and the data folder is the one the platform gives the signed-in user: local application data on Windows, where the web view's profile already lives, and Application Support on macOS, and never Roaming. Anything that isn't portable is installed, whatever the reason: a development build, a copy on the Desktop, or a Mac app that macOS has translocated, running it from a random read-only folder because it still carries a download's quarantine mark. That last one is usually someone who opened the app inside its disk image without dragging it to Applications, and the installed data folder is exactly what they want, since it doesn't depend on where the app is; a translocated portable copy can't see its portable folder, so it acts installed too, and works.

This module never opens the settings file. It names it, because the portable decision looks for it, and the page reads it, writes it, and knows what's in it.
*/

const PORTABLE_NAME: &str = "portable";//the folder whose ftorrent.toml makes a copy portable
const STATE_NAME: &str = "state";//libtorrent's session state, the DHT routing table among it

/// Everything startup worked out about where things are, which the page shows and resolves settings against, and the engine is told
#[derive(Serialize, Clone, Default)]
pub struct Paths {
	pub mode: String,//installed or portable
	pub location: String,//the program's location, the anchor for ./
	pub executable: String,//the program file itself, which is what a registered command runs
	pub installer: String,//on windows, the folder the per-user installer puts the program in, %LOCALAPPDATA% and the product name; blank elsewhere, where no installer places a program the page needs to recognize
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
		executable: std::env::current_exe().map(|exe| display(&exe)).unwrap_or_default(),
		installer: installer_folder(app),
		..Default::default()//the rest filled in below, as each is found
	};

	let location = match program_location() {
		Ok(location) => location,
		Err(trouble) => { paths.trouble = trouble; return paths }
	};
	paths.location = display(&location);

	let portable = location.join(PORTABLE_NAME);
	let settings_name = format!("{}.toml", app.package_info().name);//the settings file, named for the product in tauri.conf.json, in the data folder; the page reads and writes it
	let data = if portable.join(&settings_name).is_file() {
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
	paths.settings = display(&data.join(&settings_name));
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

/// Where the per-user installer puts the program on windows, %LOCALAPPDATA% joined with the product name from tauri.conf.json; a fact the page compares with the program's location to know it's running from an installed copy
fn installer_folder(app: &AppHandle) -> String {
	if !cfg!(target_os = "windows") { return String::new() }
	std::env::var_os("LOCALAPPDATA").map(|local| display(&PathBuf::from(local).join(&app.package_info().name))).unwrap_or_default()
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
