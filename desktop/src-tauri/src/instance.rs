use std::fs::{self, File, TryLockError};
use std::path::Path;
use std::sync::Mutex;
use serde::Serialize;
use tauri::{command, AppHandle, Manager, State};
use crate::paths::Paths;
use crate::queue::{Drained, Queue};

/*
ftorrent runs once per copy, and a second launch of a copy that's already running hands over what it carried and leaves. That second launch is ordinary: clicking a magnet link or double-clicking a .torrent file while ftorrent is open starts one, on Windows, and a person who selects ten torrents and presses Enter starts ten. Whatever they carried has to reach the copy that's running, every time, and no second window may appear.

Two halves do that. The lock says whether this copy is already running. It's an exclusive lock on ftorrent.lock, an empty file in the data folder, taken with the standard library's File::try_lock, which is flock on macOS and Linux and LockFileEx on Windows. The operating system releases it when the process ends, however it ends, so a crash never leaves a stale lock behind. It's keyed on the data folder, not on the program, so an installed copy and a portable copy each have their own, and so do two people signed in to one machine. A file lock also holds across accounts, which a named mutex doesn't, so two people launching the same portable copy from one stick can't both run it.

The handoff carries the request across. On Windows every launch is a new process, so the copy that holds the lock serves a named pipe, and a launch that finds the lock held writes its command-line arguments into the pipe as one line of JSON and exits. The pipe's name comes from the lock file's path, so each copy only hears from launches of itself. It's created right after the lock and before anything slow, and a launch that finds the lock held but the pipe not there yet keeps trying for a few seconds, because that gap is exactly when a second click during a cold start lands. On macOS, Launch Services brings a running app forward instead of starting a second process, and delivers files and links to it as Apple Events, which Tauri hands over as its Opened event, at a cold start and while running alike, so the lock there is a backstop for a launch that goes around Launch Services, and a second process simply leaves.

What arrives, this copy's own command line, a second launch's, or a Mac's open, goes on a drained queue, each marked with where it came from, and the page takes it and decides what it means. Rust has to hold it, since a magnet can land during a cold start before the page is up, and a handoff arrives on the pipe's thread rather than in the page; what it doesn't do is keep a history or know what a magnet is. That's the page's, which is where adding a torrent will live. One request never reaches the page: a handoff whose only argument is --exit quits this copy the way File, Exit does. The setup program sends it before replacing the files, so an upgrade closes the running copy cleanly rather than ending its process. And one argument never does: --login, which a Windows login registration adds so login.rs knows the system started this copy, is taken out of this copy's own command line, and a handoff carrying only it is dropped, leaving the window as it was. --update, which install.rs puts on the command line of the copy it opens once an update is in place, is taken out too, after it has changed one thing: a launch carrying it that finds the lock held waits for it, since the holder is the installer letting go, rather than handing over to it and leaving.
*/

pub const UPDATE_ARGUMENT: &str = "--update";//on the command line of the copy install.rs opens once an update is in place: the lock is held by the installer, which lets go of it by exiting a moment later, so wait
const UPDATE_WAITS: u32 = 150;//tenths of a second a copy started by an update waits for the lock: fifteen seconds, where the installer holds it for milliseconds past open
const ARRIVALS_WAITING: usize = 100;//how many arrivals wait for the page to take them before the oldest are dropped and counted; the page takes several times a second, so this is room for a burst of launches

/// One launch's arguments, as they reached this copy
#[derive(Serialize, Clone)]
pub struct Request {
	pub from: String,//launch, for this copy's own command line, handoff, for one carried in from a second launch, or open, for files and links macOS opened with this copy
	pub args: Vec<String>,
}

/// What the page sees about this copy's lock and handoff
#[derive(Serialize, Clone, Default)]
pub struct InstanceStatus {
	pub lock: String,//the lock file's path
	pub held: bool,//true once this copy holds it
	pub handoff: String,//how a second launch reaches this copy
	pub trouble: String,//why this copy runs without its lock or its handoff, blank when it has both
}

//managed by lib.rs; the lock file lives here so it stays open, and the lock stays held, for as long as the process runs
#[derive(Default)]
pub struct Instance {
	lock: Mutex<Option<File>>,
	status: Mutex<InstanceStatus>,
	arrivals: Mutex<Queue<Request, ARRIVALS_WAITING>>,//what has reached this copy that the page hasn't taken yet
}

/// How startup should go on
pub enum Start {
	Run,//this copy is the one running; carry on
	Leave,//another process holds this copy's lock and has what this launch carried; exit now
}

/// Take this copy's lock, or hand this launch to the copy that has it; called from setup right after paths::locate, which has already created the data folder
pub fn start(app: &AppHandle, paths: &Paths) -> Start {
	let instance = app.state::<Instance>();
	let args: Vec<String> = std::env::args().skip(1).collect();//what the operating system launched this copy with, like a magnet link or the path of a .torrent file
	if paths.data.is_empty() {
		status(&instance).trouble = "no data folder, so no lock".to_string();//the platform gave no data folder, and paths.rs has already said so
		return Start::Run;
	}
	let brand_stem = app.package_info().crate_name.to_string();//brandStem, the crate's name, which names the lock file and the pipe; brand.js in the page says which name goes where
	let lock_path = lock_path(app, paths);
	status(&instance).lock = lock_path.to_string_lossy().into_owned();

	let mut taken = take(&lock_path);
	if matches!(taken, Err(Taken::Busy)) && args.iter().any(|arg| arg == UPDATE_ARGUMENT) {//the installer that opened this copy still holds the lock, and lets go of it as it exits; wait for it rather than handing over to it
		let mut waited = 0;
		while waited < UPDATE_WAITS && matches!(taken, Err(Taken::Busy)) {
			std::thread::sleep(std::time::Duration::from_millis(100));
			waited += 1;
			taken = take(&lock_path);
		}
		crate::log::log(&format!("waited {waited} tenths of a second for the installer to let go of the lock"));
	}
	match taken {
		Ok(file) => {
			*instance.lock.lock().unwrap_or_else(|p| p.into_inner()) = Some(file);
			status(&instance).held = true;
			crate::log::log(&format!("took the lock, {}", lock_path.display()));
		}
		Err(Taken::Busy) => {
			crate::log::log(&format!("another process holds the lock, {}, so this one hands over and leaves", lock_path.display()));//waits in memory, and goes nowhere, since this process never has a page to start the log
			handoff::send(&brand_stem, &lock_path, &args);//deliver, or give up trying after a few seconds; either way this launch is done
			return Start::Leave;
		}
		Err(Taken::Unsupported(e)) => {
			status(&instance).trouble = format!("running without a lock, because this folder couldn't be locked: {e}");//a volume that can't lock, where refusing to start would help nobody
		}
	}

	match handoff::serve(app, &brand_stem, &lock_path) {
		Ok(how) => status(&instance).handoff = how,
		Err(e) => {
			let mut s = status(&instance);
			if !s.trouble.is_empty() { s.trouble.push_str("; ") }
			s.trouble.push_str(&format!("a second launch can't reach this copy: {e}"));
		}
	}
	let carried: Vec<String> = args.into_iter().filter(|arg| arg != crate::login::ARGUMENT && arg != UPDATE_ARGUMENT).collect();//the arguments a login registration and an update add are no file or link, and have done their work
	if !carried.is_empty() { arrive(app, "launch", carried, false) }
	Start::Run
}

/// The lock file's path, brandStem.lock in the data folder: empty, never written, and there to be locked; install.rs waits on the same file
pub fn lock_path(app: &AppHandle, paths: &Paths) -> std::path::PathBuf {
	Path::new(&paths.data).join(format!("{}.lock", app.package_info().crate_name))
}

/// Why a lock couldn't be taken
pub enum Taken {
	Busy,//another process holds it
	Unsupported(String),//the file system couldn't lock at all
}

/// Open a lock file, creating it empty, and take its exclusive lock without waiting; download folders will use this too
pub fn take(path: &Path) -> Result<File, Taken> {
	let file = fs::OpenOptions::new().read(true).write(true).create(true).truncate(false).open(path).map_err(|e| Taken::Unsupported(e.to_string()))?;
	match file.try_lock() {
		Ok(()) => Ok(file),
		Err(TryLockError::WouldBlock) => Err(Taken::Busy),
		Err(TryLockError::Error(e)) => Err(Taken::Unsupported(e.to_string())),
	}
}

/// On macOS, the files and links Launch Services opened with this copy, as one request: a file as its path, the way a launch on Windows carries one, and a link as it was written; called from the Opened run event in lib.rs
#[cfg(target_os = "macos")]
pub fn opened(app: &AppHandle, urls: Vec<tauri::Url>) {
	let args = urls.into_iter().map(|url| match url.to_file_path() {
		Ok(path) if url.scheme() == "file" => path.to_string_lossy().into_owned(),
		_ => url.to_string(),//magnet:, ftorrent:, or anything else a link might be
	}).collect();
	arrive(app, "open", args, true)//the user reached for ftorrent, so the window comes forward, hidden or not; at a cold start bring_forward waits, since the page is about to show it
}

/// Something reached this copy: queue it for the page, and bring the window forward if a second launch sent it
fn arrive(app: &AppHandle, from: &str, args: Vec<String>, forward: bool) {
	crate::log::log(&format!("arrived by {from}: {}", args.join(" ")));
	app.state::<Instance>().arrivals.lock().unwrap_or_else(|poisoned| poisoned.into_inner()).push(Request { from: from.to_string(), args });
	if forward { crate::lifecycle::bring_forward(app) }//closing the window hides it, so it may be hidden, minimized, or behind something else
}

fn status(instance: &Instance) -> std::sync::MutexGuard<'_, InstanceStatus> {
	instance.status.lock().unwrap_or_else(|poisoned| poisoned.into_inner())
}

/// This copy's lock and its handoff; the page asks
#[command]
pub fn instance_status(instance: State<'_, Instance>) -> InstanceStatus {
	status(&instance).clone()
}

/// Everything that has reached this copy since the last take, oldest first, and how many were dropped because the page fell behind
#[command]
pub fn instance_take(instance: State<'_, Instance>) -> Drained<Request> {
	instance.arrivals.lock().unwrap_or_else(|poisoned| poisoned.into_inner()).take()
}

#[cfg(target_os = "windows")]
mod handoff {
	use std::io::Write;
	use std::path::Path;
	use std::time::{Duration, Instant};
	use tauri::AppHandle;
	use tokio::io::AsyncReadExt;
	use tokio::net::windows::named_pipe::ServerOptions;

	const PATIENCE: Duration = Duration::from_secs(5);//how long a second launch keeps trying a copy that's still starting
	const MOST: u64 = 64 * 1024;//the longest request read from the pipe; a launch's arguments are far shorter
	const ERROR_PIPE_BUSY: i32 = 231;//every instance of the pipe is taken for the moment, as when many launches arrive together

	/// The pipe's name: brandStem and a hash of the lock file's path, so each copy has its own and a launch finds the right one; the windows installer computes the same name to ask a running copy to exit
	fn name(brand_stem: &str, lock: &Path) -> String {
		let path = lock.to_string_lossy().to_lowercase();//windows paths ignore case, so C:\Users and c:\users are one copy
		let mut hash: u64 = 0xcbf29ce484222325;//FNV-1a, the same answer in every process and every build; a name needs no more than that, and first_pipe_instance below stops another process from taking it
		for byte in path.bytes() { hash ^= byte as u64; hash = hash.wrapping_mul(0x100000001b3); }
		format!(r"\\.\pipe\{brand_stem}-{hash:016x}")
	}

	/// Serve the pipe for as long as this copy runs; each connection brings one launch's arguments
	pub fn serve(app: &AppHandle, brand_stem: &str, lock: &Path) -> Result<String, String> {
		let name = name(brand_stem, lock);
		let first = tauri::async_runtime::block_on(async {//created here, before setup goes on, so the gap a cold start leaves is as short as it can be
			ServerOptions::new()
				.first_pipe_instance(true)//fails if something already has this name, rather than joining it
				.access_outbound(false)//inbound only: requests come in, and nothing goes back for anyone to read
				.create(&name)//tokio refuses clients from other machines by default
		}).map_err(|e| e.to_string())?;
		let app = app.clone();
		let pipe = name.clone();
		tauri::async_runtime::spawn(async move {
			let mut server = first;
			loop {
				if server.connect().await.is_err() {//a launch that gave up mid-connect; this instance is spent, so make a fresh one and wait again
					server = match ServerOptions::new().access_outbound(false).create(&pipe) { Ok(next) => next, Err(_) => break };
					continue;
				}
				let connected = server;
				server = match ServerOptions::new().access_outbound(false).create(&pipe) {//the next instance, ready before this one is read, so a burst of launches finds one waiting
					Ok(next) => next,
					Err(_) => break,
				};
				let app = app.clone();
				tauri::async_runtime::spawn(async move {
					let mut bytes = Vec::new();
					if connected.take(MOST).read_to_end(&mut bytes).await.is_err() { return }//the launch writes one line and closes, which ends the read
					let Ok(value) = serde_json::from_slice::<serde_json::Value>(&bytes) else { return };
					let args: Vec<String> = value.get("args").and_then(|a| a.as_array()).map(|a| a.iter().filter_map(|v| v.as_str().map(str::to_string)).collect()).unwrap_or_default();
					if args.len() == 1 && args[0] == "--exit" { crate::log::log("asked to exit by handoff"); app.exit(0); return }//the one request that isn't for the page: the setup program sends it before it replaces the files, and it reaches the same Exit run event as File, Exit, so the engine stops and the session is saved
					if args.len() == 1 && args[0] == crate::login::ARGUMENT { crate::log::log("a login launch handed over, and changes nothing"); return }//windows started ftorrent at sign-in while this copy already runs, which is no request: the window stays as it is
					super::arrive(&app, "handoff", args, true);
				});
			}
		});
		Ok(format!("named pipe {name}"))
	}

	/// Hand this launch's arguments to the copy that holds the lock
	pub fn send(brand_stem: &str, lock: &Path, args: &[String]) {
		let name = name(brand_stem, lock);
		let line = serde_json::json!({ "args": args }).to_string() + "\n";
		let started = Instant::now();
		loop {
			match std::fs::OpenOptions::new().write(true).open(&name) {//opening a pipe by name is an ordinary file open on windows
				Ok(mut pipe) => { let _ = pipe.write_all(line.as_bytes()); return }
				Err(e) if e.kind() == std::io::ErrorKind::PermissionDenied => return,//the copy is running under another account, whose pipe this one can't write to
				Err(e) if (e.kind() == std::io::ErrorKind::NotFound || e.raw_os_error() == Some(ERROR_PIPE_BUSY)) && started.elapsed() < PATIENCE => std::thread::sleep(Duration::from_millis(50)),//still starting, or busy with other launches
				Err(_) => return,
			}
		}
	}
}

#[cfg(not(target_os = "windows"))]
mod handoff {
	use std::path::Path;
	use tauri::AppHandle;

	/// Nothing to serve: Launch Services delivers to a running app on macOS
	pub fn serve(_app: &AppHandle, _brand_stem: &str, _lock: &Path) -> Result<String, String> {
		Ok(if cfg!(target_os = "macos") { "Launch Services".to_string() } else { "none yet on this platform".to_string() })
	}

	/// A second process here got around Launch Services; the copy that's running is already on screen, so this one leaves
	pub fn send(_brand_stem: &str, _lock: &Path, _args: &[String]) {}
}
