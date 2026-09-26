use std::collections::HashMap;
use std::fs::{self, File};
use std::path::PathBuf;
use std::sync::Mutex;
use tauri::{command, State};
use crate::instance::{take, Taken};

/*
Exclusive locks on files, for the page to take and release as it sees fit. lock_take opens the file at a path, creating it empty if it isn't there, and takes an exclusive lock on it without waiting; lock_release lets go. The lock is the one instance.rs takes for itself, File::try_lock through instance::take, flock on macOS and Linux and LockFileEx on Windows, so it has the same guarantees: it belongs to an open file, it holds against every other process whatever account it runs under, and the operating system releases it however the process ends, so a crash never leaves a file locked. On a network share the release waits for the file server to notice the dead connection, which takes its own timeout.

The locks this process holds are kept here, their files open, until released or until the process ends. One thing is decided here rather than by the caller, because it's a fact of the file system: whether two paths name the same file. A path is reduced to its one true name before anything else, so asking twice for the same file under two spellings answers held both times, rather than the second request finding this process's own lock and answering busy.

lock_take never makes the folder the file goes in; a missing folder is trouble, like any other reason the file can't be opened. What a lock is for, and which files to lock when, is the page's.
*/

/// The locks this process holds, by the file's one true name, each file kept open so its lock stays held
#[derive(Default)]
pub struct Locks(Mutex<HashMap<PathBuf, File>>);

/// Take an exclusive lock on the file at this path, creating the file empty if it isn't there; answers held, busy (another process holds it), or trouble with the reason after a colon
#[command]
pub fn lock_take(locks: State<'_, Locks>, path: String) -> String {
	let mut held = locks.0.lock().unwrap_or_else(|poisoned| poisoned.into_inner());
	if let Ok(real) = fs::canonicalize(&path) { if held.contains_key(&real) { return "held".to_string() } }//already this process's, under this name or another
	let file = match take(std::path::Path::new(&path)) {
		Ok(file) => file,
		Err(Taken::Busy) => return "busy".to_string(),
		Err(Taken::Unsupported(e)) => return format!("trouble: {e}"),//a missing folder, a permission, a read-only volume, or a file system that can't lock
	};
	match fs::canonicalize(&path) {//the file exists now, so it has a true name
		Ok(real) => { held.insert(real, file); "held".to_string() }
		Err(e) => format!("trouble: {e}"),//dropping the file here releases the lock just taken
	}
}

/// Let go of the lock on the file at this path; nothing happens if this process doesn't hold it
#[command]
pub fn lock_release(locks: State<'_, Locks>, path: String) {
	let Ok(real) = fs::canonicalize(&path) else { return };
	locks.0.lock().unwrap_or_else(|poisoned| poisoned.into_inner()).remove(&real);//dropping the file closes it, which releases the lock
}
