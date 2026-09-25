//./src-tauri/src/paths.rs

use std::fs;
use std::path::{Path, PathBuf};
use serde::Serialize;
use tauri::{command, AppHandle, Manager, State};

/*
Where everything is. This runs once, in setup, before the engine starts and before the page exists, and everything that reads or writes a file of ftorrent's own takes its path from what this works out. Nothing else in the app asks Tauri or the operating system where to put things.

Three anchors, and every path comes from one of them. The program's location is the folder the program sits in: beside the executable on Windows and Linux, and the folder holding ftorrent.app on macOS, not a folder inside the bundle. The data folder is ftorrent's own, and holds ftorrent.json and, as the sprint goes on, the lock, the libtorrent state, and the crash log. And the user's home folder.

One decision picks the data folder. If a folder named portable with a ftorrent.json inside sits at the program's location, this copy is portable, and that folder is its data folder, so everything it keeps travels with it. Otherwise it's an installed copy, and the data folder is the one the platform gives the signed-in user: AppData\Local\com.ftorrent.ftorrent on Windows, where Tauri already keeps the web view's profile, and Application Support on macOS. An installed copy keeps nothing of its own in Roaming.

A download folder in ftorrent.json is written one of three ways. ./downloads is relative to the program's location, so it follows a portable copy onto whatever drive letter or mount point it lands on. ~/Downloads/ftorrent is relative to whoever is signed in. And an absolute path like D:/torrents means exactly that place, on the machines that have it. Paths are written with forward slashes on every platform.

Two things this deliberately does not do. It never checks whether a download folder exists: on macOS the default sits in Downloads, which the system guards behind a permission prompt, and the first look inside is what raises it, so that look waits for the step that can warn the user first. And it never writes over a ftorrent.json it can't read: a file with a typo from a hand edit is left exactly as it is, ftorrent runs on its built-in settings, and the page says so.
*/

const SETTINGS_NAME: &str = "ftorrent.json";//the settings file, in the data folder
const PORTABLE_NAME: &str = "portable";//the folder whose ftorrent.json makes a copy portable
const STATE_NAME: &str = "state";//libtorrent's session state, the DHT routing table among it
const DEFAULT_DOWNLOAD_FOLDER: &str = "~/Downloads/ftorrent";//where torrents go when nobody has said otherwise

/// One entry from download_folders: what the settings say, and where that is on this machine
#[derive(Serialize, Clone, Default)]
pub struct DownloadFolder {
	pub setting: String,//as written in ftorrent.json
	pub path: String,//resolved against the program's location or the home folder; whether it exists is not asked yet
}

/// Everything startup worked out about where things are, which the page shows and the engine is told
#[derive(Serialize, Clone, Default)]
pub struct Paths {
	pub mode: String,//installed, portable, or translocated
	pub location: String,//the program's location, the anchor for ./
	pub data: String,//the data folder
	pub settings: String,//ftorrent.json in the data folder
	pub state: String,//the libtorrent state file in the data folder
	pub download_folders: Vec<DownloadFolder>,
	pub trouble: String,//what went wrong, blank when nothing did; startup carries on with the built-in settings rather than stopping
}

/// Work out where everything is; called once from setup
pub fn resolve(app: &AppHandle) -> Paths {
	let mut paths = Paths::default();

	let location = match program_location() {
		Ok(location) => location,
		Err(trouble) => { paths.trouble = trouble; return paths }
	};
	paths.location = display(&location);

	//a quarantined app opened from where it was unpacked runs from a random read-only copy, so the location is wrong and a portable folder beside the real app can't be seen from here; apple offers no supported way to find the real one, so ftorrent stops and says how to fix it
	if cfg!(target_os = "macos") && paths.location.contains("/AppTranslocation/") {
		paths.mode = "translocated".to_string();
		return paths;
	}

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
	paths.data = display(&data);
	let settings = data.join(SETTINGS_NAME);
	paths.settings = display(&settings);
	paths.state = display(&data.join(STATE_NAME));

	let folders = match read_settings(&data, &settings) {
		Ok(folders) => folders,
		Err(trouble) => { paths.trouble = trouble; vec![DEFAULT_DOWNLOAD_FOLDER.to_string()] }//run on the built-in default, and leave the file alone
	};
	let home = app.path().home_dir().ok();
	paths.download_folders = folders.into_iter().map(|setting| {
		let path = display(&resolve_folder(&setting, &location, home.as_deref()));
		DownloadFolder { setting, path }
	}).collect();
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

/// The download folders from ftorrent.json, writing the file with the default if it isn't there yet
fn read_settings(data: &Path, settings: &Path) -> Result<Vec<String>, String> {
	if !settings.exists() {//a first launch; only an installed copy gets here, since a portable one is portable because its file exists
		let first = serde_json::json!({"download_folders": [DEFAULT_DOWNLOAD_FOLDER]});
		fs::create_dir_all(data).map_err(|e| format!("could not create the data folder: {e}"))?;
		fs::write(settings, pretty(&first)).map_err(|e| format!("could not write {SETTINGS_NAME}: {e}"))?;
	}
	let text = fs::read_to_string(settings).map_err(|e| format!("could not read {SETTINGS_NAME}, so ftorrent is running on its built-in settings: {e}"))?;
	let value: serde_json::Value = serde_json::from_str(&text).map_err(|e| format!("{SETTINGS_NAME} isn't valid JSON, so ftorrent is running on its built-in settings and won't save over it: {e}"))?;
	let Some(list) = value.get("download_folders") else { return Ok(vec![DEFAULT_DOWNLOAD_FOLDER.to_string()]) };//a key the user never set takes its default
	let list = list.as_array().ok_or_else(|| format!("download_folders in {SETTINGS_NAME} isn't a list, so ftorrent is running on its built-in settings"))?;
	Ok(list.iter().filter_map(|v| v.as_str().map(str::to_string)).collect())//an entry that isn't text is passed over, the way an unreachable folder is
}

/// Where a download folder setting points on this machine
fn resolve_folder(setting: &str, location: &Path, home: Option<&Path>) -> PathBuf {
	let setting = setting.replace('\\', "/");//a path pasted from Windows works the same as one written the documented way
	let (mut path, rest) = if setting == "~" || setting.starts_with("~/") {
		(home.map(Path::to_path_buf).unwrap_or_default(), &setting[1..])
	} else if setting == "." || setting.starts_with("./") {
		(location.to_path_buf(), &setting[1..])
	} else if is_absolute(&setting) {
		return PathBuf::from(if cfg!(target_os = "windows") { setting.replace('/', "\\") } else { setting });//exactly the place it names, which on a machine without it is simply a folder that isn't there
	} else {
		(location.to_path_buf(), &setting[..])//a bare relative path, like downloads, is read as ./downloads rather than relative to whatever folder the program happened to be started from
	};
	for part in rest.split('/').filter(|part| !part.is_empty()) { path.push(part) }//one part at a time, so the separators come out native
	path
}

/// Whether a setting names one exact place, on any platform: C:/Games is absolute on a Mac too, where it names a drive that isn't there, rather than a folder called C: beside the program
fn is_absolute(setting: &str) -> bool {
	let b = setting.as_bytes();
	setting.starts_with('/') || (b.len() >= 2 && b[0].is_ascii_alphabetic() && b[1] == b':')
}

/// A path as text, for the page and the engine
fn display(path: &Path) -> String {
	path.to_string_lossy().into_owned()
}

/// JSON the way a person would write it, indented with tabs
fn pretty(value: &serde_json::Value) -> Vec<u8> {
	let mut out = Vec::new();
	let mut serializer = serde_json::Serializer::with_formatter(&mut out, serde_json::ser::PrettyFormatter::with_indent(b"\t"));
	serde::Serialize::serialize(value, &mut serializer).expect("a json value always serializes");
	out.push(b'\n');
	out
}

/// Where everything is, as startup worked it out; the page asks once, since none of it changes while the app runs
#[command]
pub fn paths_status(paths: State<'_, Paths>) -> Paths {
	paths.inner().clone()
}
