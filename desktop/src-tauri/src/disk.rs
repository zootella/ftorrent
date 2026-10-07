use serde::Serialize;
use std::fs;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::UNIX_EPOCH;
use tauri::command;
use crate::run_blocking;

/*
The design contract of this module: these commands hand the page the full, standard power a desktop application has over the disk, the same power a native Mac or Windows app wields through its file APIs. Each one is a single atomic operation with POSIX semantics, followed faithfully, sharp edges included: disk_copy overwrites an existing destination, just like cp and std::fs::copy do, and disk_write replaces the whole file, as fs::write does, though by renaming a finished copy over it rather than truncating it in place, so nothing ever meets a half-written file; its comment says why. Code that calls these commands must be careful and correct, exactly as native application code must.

They take any path and hold no guard, on purpose: the page alone knows what a path means and whether writing it is right, and a guard here would be a second copy of that knowledge. lib.rs has the long version of why, and of what keeps this power in the right hands.

Commands here are named for the POSIX call they stand on, disk_readdir, disk_stat, disk_read, so the next ones write themselves: disk_rename over fs::rename, disk_unlink over fs::remove_file, disk_rmdir over fs::remove_dir. Each is a line of std::fs and a map_err, which is why none of them is sitting here waiting. disk_rmtree is the one with no POSIX call beneath it, and takes its name from the tool everyone knows for it.
*/

#[derive(Serialize)]
pub struct DirEntry {
	pub name:       String,//base name of the entry, without the parent path
	pub is_file:    bool,//true if this entry is a regular file
	pub is_dir:     bool,//true if this entry is a directory
	pub is_symlink: bool,//true if this entry is a symbolic link
	pub size:       u64,//size in bytes; typically 0 for directories and symlinks
}

#[derive(Serialize)]
pub struct FileStat {
	pub is_file:    bool,//true if this path is a regular file
	pub is_dir:     bool,//true if this path is a directory
	pub is_symlink: bool,//true if this path is a symbolic link
	pub size:       u64,//size in bytes
	pub atime:      u128,//last access time, in milliseconds since the unix epoch; 0 when the filesystem has no answer
	pub mtime:      u128,//last modification time, in milliseconds since the unix epoch; 0 when the filesystem has no answer
	pub ctime:      u128,//creation time, in milliseconds since the unix epoch; 0 when the filesystem has no answer, which is common on linux
}

/*
Every command here is an async fn whose body goes to run_blocking in lib.rs, and so is every other command that waits, on the disk, on a system daemon, or on another program. The body is ordinary blocking code; the wrapper decides which thread runs it.

Tauri runs a plain command on the thread that runs the window, inside the web view's own callback, and an async command's body on tokio's worker pool, one thread per core. Neither is a place to wait on a disk. The paths that reach this file are the download folders the user chose and the session folder inside each, an external drive that may be asleep or a network share that may be gone, where a stat or an open hangs for the share's own timeout, and the settings file, which a portable copy keeps on a stick; the lock inside each session folder reaches locks.rs the same way. On the window's thread that wait is a frozen window, no repaint, no tray click, no menu, until the disk answers. On a worker it is a worker gone for the duration, and a few of those on a machine with a few cores stall every async command in the app, the ones that never touch that disk among them. A panic is the other half: Tauri replies to the page only after the body returns, so a panic on a worker leaves the page's promise unsettled for good, and a panic on the window's thread unwinds into the web view's callback, a boundary rust will not unwind across, which aborts the whole process.

run_blocking hands the body to spawn_blocking, tokio's blocking pool: a thread for each body that waits, up to 512, each retired after ten idle seconds. Awaiting it reports a panic as an error, which becomes the string every command already answers with, so the page gets a rejected promise rather than silence or an abort. What it costs: it is not faster; it cancels nothing, so a read stuck in the kernel still holds a thread, one that nothing else was waiting for; and a panic arrives as its message without where it happened, which the panic hook in log.rs writes to the log before the unwind. tokio::fs is the other road and goes nowhere new: it runs these same blocking calls on this same pool.
*/

/// POSIX-like `readdir`, shallow only
#[command]
pub async fn disk_readdir(path: String) -> Result<Vec<DirEntry>, String> {
	run_blocking(move || {
		let mut results = Vec::new();
		for entry in fs::read_dir(&path).map_err(|e| e.to_string())? {
			let entry = match entry { Ok(entry) => entry, Err(_) => continue };//skip an entry the listing can't produce, rather than failing the whole folder over it
			let meta  = match fs::symlink_metadata(entry.path()) { Ok(meta) => meta, Err(_) => continue };//same for one we can't stat, like a locked file
			let ft    = meta.file_type();
			results.push(DirEntry {
				name:       entry.file_name().to_string_lossy().into_owned(),
				is_file:    ft.is_file(),
				is_dir:     ft.is_dir(),
				is_symlink: ft.is_symlink(),
				size:       meta.len(),
			});
		}
		Ok(results)
	}).await
}

/// POSIX-like `stat(2)` metadata
#[command]
pub async fn disk_stat(path: String) -> Result<FileStat, String> {
	run_blocking(move || {
		let meta  = fs::symlink_metadata(&path).map_err(|e| e.to_string())?;
		let ft    = meta.file_type();
		Ok(FileStat {
			is_file:    ft.is_file(),
			is_dir:     ft.is_dir(),
			is_symlink: ft.is_symlink(),
			size:       meta.len(),
			atime:      millis(meta.accessed()),
			mtime:      millis(meta.modified()),
			ctime:      millis(meta.created()),
		})
	}).await
}
fn millis(time: std::io::Result<std::time::SystemTime>) -> u128 {//a timestamp as milliseconds since the unix epoch, or 0 when the filesystem can't say, so one missing date never fails the whole stat
	time.ok().and_then(|t| t.duration_since(UNIX_EPOCH).ok()).map(|d| d.as_millis()).unwrap_or(0)
}

/// POSIX-like `open` + `read` + `close`
#[command]
pub async fn disk_read(path: String) -> Result<tauri::ipc::Response, String> {
	run_blocking(move || fs::read(&path).map(tauri::ipc::Response::new).map_err(|e| e.to_string())).await//Response carries the bytes raw, and the page gets an ArrayBuffer; the note below has why
}
/*
Returning Response rather than Vec<u8> is the difference between a copy and a translation. A Vec<u8> crosses as a JSON array, one decimal number per byte, written here and parsed by the page on its main thread, so a megabyte becomes a million numbers, and the time that takes grows with the file. Response hands the same bytes over as an ArrayBuffer instead. The page wraps the result in new Uint8Array(...), which takes either, so nothing above had to change.

This still reads the whole file into memory, and holds it more than once: Rust's buffer, the transfer, and the JS heap. That is fine for a settings file or a .torrent, and would not be for a big one. Tauri's plugin-fs streams instead, reading on the Rust side in 64 KB chunks and presenting them to the page as a ReadableStream, which is the shape a read of something large would take, here or through that plugin.
*/

/// POSIX `cp`, shallow, files only
#[command]
pub async fn disk_copy(source: String, destination: String) -> Result<(), String> {
	run_blocking(move || fs::copy(&source, &destination).map(|_| ()).map_err(|e| e.to_string())).await
}
/*
Bytes in a Tauri application live in three places: the kernel's page cache, the Rust process, and the web view's JS heap. Reading a file drags them through all three, disk, page cache, Rust buffer, the transfer, JS heap, and every one of those is a copy. That is the cost disk_read above pays, and the Response change was about removing the worst step from it rather than the steps themselves.

disk_copy never pays it at all. std::fs::copy reaches CopyFileEx on Windows and fclonefileat or fcopyfile on macOS, and both do the whole thing inside the kernel: no user-mode buffer is allocated in this process, and no byte of the file enters the Rust or JS heaps. It is as fast as the hardware allows for a file of any size.

What it cannot do is report. It is fire and forget: nothing above can learn that it is halfway done, or pause it, or cancel it. Tauri's plugin-fs answers that with streaming file handles, reading and writing in 64 KB chunks that become a ReadableStream on the JS side, which buys progress and cancellation at the price of hauling every chunk up through all three layers. There is a third shape, if a progress bar on a large copy is ever needed: keep the copy in the kernel and lift only the counter. CopyFileExW takes a progress callback on Windows, copyfile takes a status callback on macOS, and everywhere else a 64 KB loop checking an AtomicBool would do. That is roughly two screenfuls of Rust, and it would let the page watch a copy it never touches.
*/

static WRITES: AtomicU64 = AtomicU64::new(0);//how many writes this process has begun, so each gets a temporary file of its own

/// Replace the whole file at this path with these bytes, by writing them to a file beside it and renaming that over it, so the file is always whole, the old one or the new one, never a mix. Nothing is synced to the disk itself, so a power cut is not what this guards, and the new file carries the platform's default permissions and attributes rather than the old file's
#[command]
pub async fn disk_write(path: String, data: Vec<u8>) -> Result<(), String> {//data crosses the ipc as one json number per byte, which is fine for a settings file of a few kilobytes and would not be for a big one; disk_read has the same note in the other direction
	run_blocking(move || {
		let path = fs::canonicalize(&path).map(|real| real.to_string_lossy().into_owned()).unwrap_or(path);//the file itself where the path is a link to it, so the rename below replaces what the link points at rather than the link; a file not there yet keeps the path as given
		let temporary = format!("{path}.{}-{}.tmp", std::process::id(), WRITES.fetch_add(1, Ordering::Relaxed));//beside the file, named for this process and this write, so two writes in flight at once never share one
		if let Err(e) = fs::write(&temporary, data) { let _ = fs::remove_file(&temporary); return Err(e.to_string()) }//a disk that fills partway leaves a partial temporary file, taken back here, and the real file untouched
		fs::rename(&temporary, &path).map_err(|e| { let _ = fs::remove_file(&temporary); e.to_string() })//the swap: one atomic step on NTFS, APFS, and ext4, and on a FAT stick on Windows a delete and a rename, a moment with no file but never a torn one. The second writer this guards against is the exit write in desktop.rs, which can land while this one is under way. A crash between the write and the rename leaves the temporary behind, the new text inside it, which is harmless
	}).await
}

/// POSIX `mkdir(2)`, single level: the parent has to exist, and a folder already there is an error, just like mkdir without -p
#[command]
pub async fn disk_mkdir(path: String) -> Result<(), String> {
	run_blocking(move || fs::create_dir(&path).map_err(|e| e.to_string())).await
}

/// `rm -r`: remove a folder and everything in it, which has no single POSIX call, so this stands on fs::remove_dir_all and borrows its name from Python's shutil.rmtree; a link inside is removed rather than followed
#[command]
pub async fn disk_rmtree(path: String) -> Result<(), String> {
	run_blocking(move || fs::remove_dir_all(&path).map_err(|e| e.to_string())).await
}

/// POSIX `statvfs(3)`: the bytes free for this user on the volume that holds this path, which has to exist
#[command]
pub async fn disk_space(path: String) -> Result<u64, String> {
	run_blocking(move || space(&path)).await
}

#[cfg(unix)]
fn space(path: &str) -> Result<u64, String> {
	let c = std::ffi::CString::new(path).map_err(|e| e.to_string())?;//the path as the c library takes it, ending in a zero
	let mut stat: libc::statvfs = unsafe { std::mem::zeroed() };
	if unsafe { libc::statvfs(c.as_ptr(), &mut stat) } != 0 { return Err(std::io::Error::last_os_error().to_string()) }
	Ok(stat.f_bavail as u64 * stat.f_frsize as u64)//blocks free to a user who isn't root, times the size of a block
}

#[cfg(target_os = "windows")]
fn space(path: &str) -> Result<u64, String> {
	use windows::core::PCWSTR;
	use windows::Win32::Storage::FileSystem::GetDiskFreeSpaceExW;
	let wide: Vec<u16> = path.encode_utf16().chain(std::iter::once(0)).collect();//utf-16 ending in a zero, bound to a variable so the pointer below points into something that lives
	let mut available: u64 = 0;//free to this user, which a quota can make less than free on the disk
	unsafe { GetDiskFreeSpaceExW(PCWSTR(wide.as_ptr()), Some(&mut available as *mut u64), None, None) }.map_err(|e| e.to_string())?;
	Ok(available)
}

/// Hide a file or folder from the file browser: on Windows, set its hidden attribute, keeping the ones it already has; on macOS and Linux, do nothing, since there a name that starts with a dot is what hides it
#[command]
pub async fn disk_hide(path: String) -> Result<(), String> {
	run_blocking(move || hide(std::path::Path::new(&path))).await
}

#[cfg(target_os = "windows")]
fn hide(path: &std::path::Path) -> Result<(), String> {
	use std::os::windows::ffi::OsStrExt;
	use windows::core::PCWSTR;
	use windows::Win32::Storage::FileSystem::{GetFileAttributesW, SetFileAttributesW, FILE_ATTRIBUTE_HIDDEN, FILE_FLAGS_AND_ATTRIBUTES, INVALID_FILE_ATTRIBUTES};
	let wide: Vec<u16> = path.as_os_str().encode_wide().chain(std::iter::once(0)).collect();//the path as windows takes it, utf-16 ending in a zero, bound to a variable so the pointers below point into something that lives
	let current = unsafe { GetFileAttributesW(PCWSTR(wide.as_ptr())) };
	if current == INVALID_FILE_ATTRIBUTES { return Err(format!("could not read the attributes of {}", path.display())) }
	if current & FILE_ATTRIBUTE_HIDDEN.0 != 0 { return Ok(()) }//hidden already, so there's nothing to write
	unsafe { SetFileAttributesW(PCWSTR(wide.as_ptr()), FILE_FLAGS_AND_ATTRIBUTES(current | FILE_ATTRIBUTE_HIDDEN.0)) }.map_err(|e| e.to_string())
}

#[cfg(not(target_os = "windows"))]
fn hide(_path: &std::path::Path) -> Result<(), String> { Ok(()) }//the dot does it
