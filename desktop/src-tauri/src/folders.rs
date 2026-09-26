//./src-tauri/src/folders.rs

use std::collections::HashMap;
use std::fs::{self, File};
use std::path::{Path, PathBuf};
use std::sync::Mutex;
use tauri::{command, State};
use crate::instance::{take, Taken};

/*
A download folder is shared ground. Each one carries a .ftorrent folder with the session data of the torrents in it, and while ftorrent uses a download folder, it holds an exclusive lock on .ftorrent/ftorrent.lock inside it. The settings lock in instance.rs keeps one copy of ftorrent from running twice; this keeps two different copies from using the same download folder at once. An installed copy and a portable one that both list D:/torrents, two people on one Mac who both point at /Users/Shared/torrents, two machines sharing a folder on the network: without the lock, each would load the same torrents, write pieces into the same files, and overwrite each other's resume data, and libtorrent, which assumes it owns what it's given, wouldn't notice. With it, the second copy to arrive leaves that folder alone and says it's in use.

The lock is the same one the settings lock uses, File::try_lock through instance::take, so it has the same guarantees: it belongs to an open file, the operating system releases it however the process ends, and a crash never leaves a folder locked. On a network share that release waits for the file server to notice the dead connection, which takes its own timeout.

Two commands, one folder at a time; which folders, and when, is the page's to decide. folder_lock never makes the folder it's given: a folder that isn't on this machine is answered as missing, which is how a drive that isn't plugged in, or a fresh install's default folder that nothing has needed yet, stays out of the way. A folder that is there gets its .ftorrent, if it hasn't one yet, so there's somewhere to put the lock. The leading dot hides it on macOS and Linux; Windows reads no meaning into a dot, so there it gets the hidden attribute too, the way the planning document has always said, and a user browsing their downloads in Explorer sees their files and not ftorrent's bookkeeping. The one thing decided here rather than on the page is whether two paths are the same folder, like ./downloads and the absolute path to it: that's a fact of the file system, and asking for a second lock on a folder this copy already holds would find this copy's own lock and answer busy.
*/

const SESSION_FOLDER: &str = ".ftorrent";//inside each download folder, beside the downloads, holding one subfolder per torrent
const LOCK_NAME: &str = "ftorrent.lock";//inside that, empty, and never written; it exists to be locked

/// The locks this copy holds, by folder: the open lock files themselves, kept open so the locks stay held
#[derive(Default)]
pub struct Folders(Mutex<HashMap<PathBuf, File>>);

/// Lock one download folder, making its .ftorrent if it hasn't one but never the folder itself; answers held, busy (another copy of ftorrent has it), missing (not on this machine), or trouble with the reason after a colon
#[command]
pub fn folder_lock(folders: State<'_, Folders>, path: String) -> String {
	let Ok(real) = fs::canonicalize(&path) else { return "missing".to_string() };//the folder's one true name, so two spellings of one place are one folder
	if !real.is_dir() { return "trouble: this is a file, not a folder".to_string() }
	let mut held = folders.0.lock().unwrap_or_else(|poisoned| poisoned.into_inner());
	if held.contains_key(&real) { return "held".to_string() }//already this copy's, under this name or another
	let session = real.join(SESSION_FOLDER);
	if let Err(e) = fs::create_dir_all(&session) { return format!("trouble: could not make {SESSION_FOLDER}: {e}") }
	hide(&session);
	match take(&session.join(LOCK_NAME)) {
		Ok(file) => { held.insert(real, file); "held".to_string() }
		Err(Taken::Busy) => "busy".to_string(),
		Err(Taken::Unsupported(e)) => format!("trouble: {e}"),//a permission, a read-only volume, or a file system that can't lock
	}
}

/// Let go of one download folder's lock; nothing happens if this copy doesn't hold it
#[command]
pub fn folder_unlock(folders: State<'_, Folders>, path: String) {
	let Ok(real) = fs::canonicalize(&path) else { return };
	folders.0.lock().unwrap_or_else(|poisoned| poisoned.into_inner()).remove(&real);//dropping the file closes it, which releases the lock
}

/// On Windows, give a folder the hidden attribute, keeping the attributes it already has; elsewhere the dot at the front of .ftorrent already hides it. A failure here costs a visible folder and nothing else, so it isn't reported
#[cfg(target_os = "windows")]
fn hide(folder: &Path) {
	use std::os::windows::ffi::OsStrExt;
	use windows::core::PCWSTR;
	use windows::Win32::Storage::FileSystem::{GetFileAttributesW, SetFileAttributesW, FILE_ATTRIBUTE_HIDDEN, FILE_FLAGS_AND_ATTRIBUTES, INVALID_FILE_ATTRIBUTES};
	let wide: Vec<u16> = folder.as_os_str().encode_wide().chain(std::iter::once(0)).collect();//the path as windows takes it, utf-16 ending in a zero, bound to a variable so the pointer below points into something that lives
	let current = unsafe { GetFileAttributesW(PCWSTR(wide.as_ptr())) };
	if current == INVALID_FILE_ATTRIBUTES || current & FILE_ATTRIBUTE_HIDDEN.0 != 0 { return }//couldn't read them, or it's hidden already, so there's nothing to write
	let _ = unsafe { SetFileAttributesW(PCWSTR(wide.as_ptr()), FILE_FLAGS_AND_ATTRIBUTES(current | FILE_ATTRIBUTE_HIDDEN.0)) };
}

#[cfg(not(target_os = "windows"))]
fn hide(_folder: &Path) {}//the dot does it
