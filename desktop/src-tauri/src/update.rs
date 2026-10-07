#[cfg(target_os = "macos")]
use std::path::{Path, PathBuf};
use tauri::{command, AppHandle};
#[cfg(target_os = "macos")]
use crate::run_blocking;

/*
Replacing this copy with a newer one, on the user's click. The page finds the newer version, downloads the file the update comes in with net_get, and checks its SHA-256 against the sidecar; what's here begins after that, and names no program or place the page chose: only the file the page downloaded crosses over, and everything it becomes, where it goes and what starts, is worked out here from where this copy runs.

On a Mac the update is ftorrent.app zipped. update_replace unpacks it with ditto into a hidden folder beside this copy's bundle, in /Applications, so what follows are renames on one volume rather than copies; renames this running bundle aside and the new one into its place, which macOS allows while the old one runs, its code already mapped; and removes the old bundle and the zip. update_restart then starts the bundle now in place, by its path rather than by its identifier, since Launch Services may otherwise choose another copy with the same one, like a build in the repository's target folder; and quits this copy through the same Exit every quit reaches, which saves the settings and stops the engine. The new copy launches with ARGUMENT, which tells instance.rs to wait a few seconds for the lock this copy is about to release rather than handing over to it and leaving.

The whole update on a Mac, from the click to the new window, with the paths it touches:

	the click     the page reads https://ftorrent.com/ftorrent.app.zip.json again, for the hash beside the file it's about to fetch
	download      https://ftorrent.com/ftorrent.app.zip streams to ~/Library/Application Support/com.ftorrent.ftorrent/ftorrent.app.zip.part
	              and, whole, is renamed to ~/Library/Application Support/com.ftorrent.ftorrent/ftorrent.app.zip; the page compares its SHA-256 with the sidecar's
	unpack        ditto -x -k unpacks the zip to /Applications/.ftorrent.app.update/ftorrent.app, beside the running copy and hidden
	swap          /Applications/ftorrent.app, still running, is renamed to /Applications/.ftorrent.app.old
	              /Applications/.ftorrent.app.update/ftorrent.app is renamed to /Applications/ftorrent.app
	clean up      /Applications/.ftorrent.app.old, /Applications/.ftorrent.app.update, and the zip in the data folder are removed
	restart       open -n /Applications/ftorrent.app --args --update starts the new copy
	quit          this copy's Exit writes ~/Library/Application Support/com.ftorrent.ftorrent/ftorrent.toml, the window's place among it, stops the engine, and ends, which lets go of ftorrent.lock beside it
	new copy      takes ftorrent.lock once it's free, reads ftorrent.toml, and opens its window where the old one was

Every step before the swap leaves the running copy as it was if it fails, and the swap puts the old bundle back if the new one won't move in.

On Windows the update is the setup program itself, which asks a running copy to exit through its pipe, writes over the files, and starts the program when it's done. That half isn't written yet.
*/

pub const ARGUMENT: &str = "--update";//on the new copy's command line, from update_restart: the copy holding the lock is the old one, on its way out, so wait for it

/// Unpack the update at zip, which the page downloaded and checked, swap it in for this copy's bundle, then remove the old bundle and the zip; answers the bundle's path
#[cfg(target_os = "macos")]
#[command]
pub async fn update_replace(zip: String) -> Result<String, String> {
	run_blocking(move || {
		let bundle = bundle()?;
		let folder = bundle.parent().ok_or("the app bundle has no folder")?;
		let name = bundle.file_name().ok_or("the app bundle has no name")?.to_string_lossy().into_owned();
		let staging = folder.join(format!(".{name}.update"));//hidden, beside the bundle, so the new one is on the same volume and moves into place by a rename
		let old = folder.join(format!(".{name}.old"));
		for leftover in [&staging, &old] { if leftover.exists() { std::fs::remove_dir_all(leftover).map_err(|e| format!("could not clear {}: {e}", leftover.display()))? } }//from an update that stopped partway

		std::fs::create_dir(&staging).map_err(|e| format!("could not write beside {}: {e}", bundle.display()))?;//where a user who can't write to /Applications learns it, before anything has moved
		let unpacked = std::process::Command::new("/usr/bin/ditto").args(["-x", "-k", &zip]).arg(&staging).status().map_err(|e| format!("could not run ditto: {e}"))?;//-x -k undoes dmg.js's -c -k, and the --keepParent there means the zip holds the .app folder itself
		let fresh = staging.join(&name);//so this is where the new bundle has to be, or the zip wasn't ours
		if !unpacked.success() || !fresh.is_dir() { let _ = std::fs::remove_dir_all(&staging); return Err(format!("the update didn't unpack to {name}")) }

		std::fs::rename(&bundle, &old).map_err(|e| { let _ = std::fs::remove_dir_all(&staging); format!("could not move this copy aside: {e}") })?;
		if let Err(e) = std::fs::rename(&fresh, &bundle) {
			let _ = std::fs::rename(&old, &bundle);//put this copy back, so a failed update leaves the one that was there
			let _ = std::fs::remove_dir_all(&staging);
			return Err(format!("could not move the update into place: {e}"));
		}
		let _ = std::fs::remove_dir_all(&old);//this process still runs from it, from pages already in memory, until update_restart quits it
		let _ = std::fs::remove_dir_all(&staging);
		let _ = std::fs::remove_file(&zip);
		crate::log::log(&format!("update: replaced {}", bundle.display()));
		Ok(bundle.to_string_lossy().into_owned())
	}).await
}

/// Start the bundle now in place, telling it to wait for the lock, and quit this copy
#[cfg(target_os = "macos")]
#[command]
pub fn update_restart(app: AppHandle) -> Result<(), String> {
	let bundle = bundle()?;
	std::process::Command::new("/usr/bin/open").arg("-n").arg(&bundle).args(["--args", ARGUMENT]).status().map_err(|e| format!("could not start {}: {e}", bundle.display()))?;//-n starts a new process even though launch services sees this one running under the same identifier
	crate::log::log("update: started the new copy, and quitting");
	app.exit(0);//the Exit run event, which writes the settings and stops the engine
	Ok(())
}

#[cfg(not(target_os = "macos"))]
#[command]
pub async fn update_replace(_zip: String) -> Result<String, String> { Err("updating in place isn't written for this platform yet".to_string()) }

#[cfg(not(target_os = "macos"))]
#[command]
pub fn update_restart(_app: AppHandle) -> Result<(), String> { Err("updating in place isn't written for this platform yet".to_string()) }

/// The .app folder this process runs from, found from the executable inside it
#[cfg(target_os = "macos")]
fn bundle() -> Result<PathBuf, String> {
	let exe = std::env::current_exe().map_err(|e| format!("could not find the program's own path: {e}"))?;
	exe.ancestors().find(|p| p.extension().is_some_and(|x| x == "app")).map(Path::to_path_buf).ok_or_else(|| "this copy isn't running from an app bundle".to_string())
}
