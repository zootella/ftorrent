use tauri::AppHandle;
use crate::paths::Paths;

/*
The newer copy putting itself in place of the old one, on a Mac. The page in the running copy downloads the update, checks it, unzips it into a folder of its own, starts the executable inside that newer bundle with --replace and the path of the bundle to replace, and quits; the essay in update.js has the whole sequence and every path. What runs here is that newer process, from setup, before it has a lock, an engine, or a window, and it never gets any of them: it waits for the old copy to finish quitting, exchanges the two bundles in one step, touches the new one, opens it, and exits. The old copy's bundle stays untouched until the old copy has gone, so every privacy grant it holds works through its last writes; the exchange is a single step the user never sees half of; and the copy that opens afterward is launched by Launch Services from its path, as its own process in the system's eyes for privacy prompts, rather than as a child of this one.

How this fits the rule in lib.rs: the page can't be here, since this process has no page yet and the old copy's page is quitting, and the rule gives the moment of startup to Rust for exactly that. What the page decided arrives on the command line, the bundle to replace and the folder to log into, and the facts this process reads are only its own: where its bundle is, and the lock file every copy of this app shares. Nothing here knows what ftorrent is, beyond the crate's name on the lock, and the same code would carry any app that updates itself this way.

The lock is the signal that the old copy has gone. It's the data folder's lock from instance.rs, which the kernel releases only when the process holding it ends, after its exit event has written the settings and stopped the engine, so taking it here means the old copy is finished, however long its quit took. This process holds it through the exchange, so a launch from the Dock in that moment finds it held and leaves rather than starting the old bundle, and lets it go by exiting, which is when the copy it opened, started with --update, takes it in turn.

Whatever goes wrong here, something comes back. renamex_np with RENAME_SWAP either exchanges the two bundles or changes nothing, so a failed exchange leaves the installed app exactly where it was, and this process opens the bundle at the target path either way, the newer one or the old one, and says in the log which it was. A wait that outlasts the ceiling means the old copy never let go, so it's still there, and this process leaves it alone and goes.
*/

#[cfg(target_os = "macos")]
pub const ARGUMENT: &str = "--replace";//on this process's command line, from the page's update.js, followed by the path of the bundle to replace
#[cfg(target_os = "macos")]
pub const LOG_ARGUMENT: &str = "--log";//followed by the folder to log into, there only when the user has logging on; the page decides that, and this process has no page to ask
#[cfg(target_os = "macos")]
const WAIT_EVERY_MILLIS: u64 = 50;//how often to try the lock while the old copy quits; a normal quit takes a few hundred milliseconds, and this is the most the wait adds past it
#[cfg(target_os = "macos")]
const WAIT_CEILING_MILLIS: u128 = 10 * 60 * 1000;//ten minutes, far past any quit, after which the old copy is taken to be stuck and left alone

/// If this launch carries --replace, do the whole job and answer true, for setup to exit on; otherwise false, and startup goes on. Called from setup right after paths::locate, before the lock, the engine, or the window
#[cfg(target_os = "macos")]
pub fn start(app: &AppHandle, paths: &Paths) -> bool {
	let args: Vec<String> = std::env::args().skip(1).collect();
	let Some(target) = argument(&args, ARGUMENT) else { return false };
	if let Some(folder) = argument(&args, LOG_ARGUMENT) { let _ = crate::log::start(&folder); }//the lines so far and every line below go to the folder the page logs into, so the install's story sits beside the old copy's and the new one's
	replace(app, paths, std::path::Path::new(&target));
	true
}

/// Nothing to do on other platforms: on Windows the setup program is the whole installer, asking the running copy to exit through its pipe and writing the files over the install folder once it has gone, which the essay in update.js lays out, so no copy of the app ever runs in an installer mode there
#[cfg(not(target_os = "macos"))]
pub fn start(_app: &AppHandle, _paths: &Paths) -> bool { false }

//the value after a flag on the command line, like /Applications/ftorrent.app after --replace
#[cfg(target_os = "macos")]
fn argument(args: &[String], flag: &str) -> Option<String> {
	args.iter().position(|a| a == flag).and_then(|i| args.get(i + 1)).cloned()
}

#[cfg(target_os = "macos")]
fn replace(app: &AppHandle, paths: &Paths, target: &std::path::Path) {
	use std::path::Path;
	let Some(bundle) = std::env::current_exe().ok().and_then(|exe| exe.ancestors().find(|p| p.extension().is_some_and(|x| x == "app")).map(Path::to_path_buf)) else {//the .app this process runs from, found from the executable inside it
		crate::log::log("replace: this copy isn't running from an app bundle, so there's nothing to put in place");
		return
	};
	crate::log::log(&format!("replace: {} with this copy at {}", target.display(), bundle.display()));

	//wait for the old copy to finish quitting, which is when the kernel lets go of its lock
	let lock_path = crate::instance::lock_path(app, paths);
	let started = std::time::Instant::now();
	let _lock = loop {//held until this process exits, so nothing starts the old bundle while the two are exchanged
		match crate::instance::take(&lock_path) {
			Ok(file) => break file,
			Err(crate::instance::Taken::Busy) if started.elapsed().as_millis() < WAIT_CEILING_MILLIS => std::thread::sleep(std::time::Duration::from_millis(WAIT_EVERY_MILLIS)),
			Err(crate::instance::Taken::Busy) => { crate::log::log("replace: the old copy never let go of the lock, so it's left as it is"); return }
			Err(crate::instance::Taken::Unsupported(e)) => { crate::log::log(&format!("replace: couldn't take the lock, so can't tell when the old copy has gone: {e}")); return }//a volume that can't lock, which the boot volume isn't
		}
	};
	crate::log::log(&format!("replace: took the lock after {} ms", started.elapsed().as_millis()));

	//exchange the two bundles, in one step that happens whole or not at all
	match swap(&bundle, target) {
		Ok(()) => {
			crate::log::log(&format!("replace: swapped, so the newer version is at {}", target.display()));
			if let Err(e) = touch(target) { crate::log::log(&format!("replace: couldn't touch the new bundle, so the system may notice it late: {e}")) }
		}
		Err(e) => crate::log::log(&format!("replace: couldn't swap, so both are where they were, and the old copy opens again: {e}")),
	}

	//open whatever is at the target now, through launch services and by path, with --update so it waits for the lock this process is about to let go of
	match std::process::Command::new("/usr/bin/open").arg("-n").arg(target).args(["--args", crate::instance::UPDATE_ARGUMENT]).status() {//-n starts a new process even though launch services may still count this one, running from the bundle that was just moved, under the same identifier
		Ok(status) if status.success() => crate::log::log("replace: opened it, and leaving"),
		Ok(status) => crate::log::log(&format!("replace: open exited {status}")),
		Err(e) => crate::log::log(&format!("replace: couldn't run open: {e}")),
	}
}

/// Exchange what's at two paths in one step, renamex_np with RENAME_SWAP: afterward each holds what the other did, and if it fails neither has changed. Both have to be on one volume that supports it, which APFS, the boot volume's format, does
#[cfg(target_os = "macos")]
fn swap(a: &std::path::Path, b: &std::path::Path) -> Result<(), String> {
	use std::os::unix::ffi::OsStrExt;
	let a = std::ffi::CString::new(a.as_os_str().as_bytes()).map_err(|e| e.to_string())?;//the paths as the c library takes them, ending in a zero
	let b = std::ffi::CString::new(b.as_os_str().as_bytes()).map_err(|e| e.to_string())?;
	if unsafe { libc::renamex_np(a.as_ptr(), b.as_ptr(), libc::RENAME_SWAP) } != 0 { return Err(std::io::Error::last_os_error().to_string()) }
	Ok(())
}

/// Set a bundle's modification time to now, what touch does, so Launch Services and Finder read it again: the system registers an app anew when its modification time is newer than its record, and Sparkle found this works where LSRegisterURL didn't
#[cfg(target_os = "macos")]
fn touch(path: &std::path::Path) -> Result<(), String> {
	std::fs::File::open(path).and_then(|file| file.set_modified(std::time::SystemTime::now())).map_err(|e| e.to_string())//a folder opens read-only like a file on a mac, and its times are set through the descriptor
}
