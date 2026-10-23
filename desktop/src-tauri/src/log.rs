use std::fs::{self, File};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::sync::Mutex;
use tauri::command;
use zootella_ergo::{has_tick, say_tick, tick_to_iso};
use crate::run_blocking;

/*
A log: lines of text from anywhere in the program, each stamped with the time, this process's id, and the side that wrote it, appended to a file as they happen. Two functions write a line, log here for Rust, called from anywhere as a plain function, and log in log.js for the page, which hands its lines down through log_line; the stamp is put on here for both, so every line in every file has one shape:

	01:14:05.123 32554 rust  took the lock
	01:14:05.871 32554 page  opens .torrent with Transmission.app

The time is UTC, to the millisecond, and the file's name carries the date. The process id tells copies apart, so the files of several processes, an installed copy and a portable one side by side, say, can sit in one folder and sort into a single timeline, every line already saying who wrote it. The side says which layer spoke, since a line from Rust and one from the page can land a few milliseconds out of order, and that tells whether two lines could have raced.

Lines are written as they arrive, not saved for the end, so a crash keeps everything up to the crash, and the file can be read while the program runs. Each process writes a file of its own, named for when it started and its process id, so two processes never append to one file. A file stops at a ceiling of lines, saying so, so a log left on can't fill a disk.

Whether to log, and where, is the page's to decide, since it's a setting, and the page reads settings. Rust starts logging before any page exists, so until the page says, its lines wait in memory: log_start with a folder writes them to the new file and keeps going, and log_start with a blank folder drops them and every line after. A process that leaves before it has a page, a second launch handing over what it carried, never logs.

Beside the log, a stick: stick.txt in the log folder, which holds one piece of text and is replaced whole each time, where a log only grows. A log answers what happened, in order; a stick answers how things stand right now, a panel's readings as a session runs, say, so someone outside the program can read the current state at any moment without scrolling a log for it. stick here and stick in log.js write it, each replacing whatever was there, with a first line stamped like a log's but with the date, since the file's name has none, saying when, which process, and which side. It's written as a finished copy renamed over the old one, the way disk_write does, so a reader never meets half of one. Every process sticks to the one file, so with several running it shows whichever wrote last, and the stamp says which.

Both are scaffolding for the section of ftorrent being built: code calls log and stick while we work on it, and what ships to people calls neither. While they're called, they work the same in every build, development or release, installed or portable, because some things behave differently in just one of those, file associations in an installed copy, say, and these are the only way to see them there. So a stick has no setting, unlike the log: it's always on, and the way to turn it off is to stop calling it. Setup tells it where the log folder is, and a stick before then waits, only the latest, since one replaced before anyone could read it was never worth keeping.

Panics land here too. log_panics installs a hook that writes every panic's location and message as a line before rust unwinds, from whichever thread it's on, so a panic that goes on to abort the process, or to reach the page as an error without its location, has already said where it was.

The state is a static rather than tauri's managed state, so that log(text) needs no handle to call. A Mutex, because log_start runs on the blocking pool, log_line on the window's thread, and Rust logs from threads of its own.
*/

const CEILING: usize = 100_000;//lines in one file, after which it says it has stopped and takes no more

enum Log {
	Waiting(Vec<String>),//lines from before the page has said whether to log
	On(File, usize),//the open file, and how many lines it holds
	Off,//not logging, and never will in this process
}

static LOG: Mutex<Log> = Mutex::new(Log::Waiting(Vec::new()));

enum Stick {
	Waiting(Option<String>),//the latest stick, from before setup said where the file goes
	At(PathBuf),//the file's path
}

static STICK: Mutex<Stick> = Mutex::new(Stick::Waiting(None));//its own lock, so a stick never waits on a log line or the other way around

/// Start logging into a new file in this folder, named for now and this process, making the folder if it's missing, and write the lines that waited; a blank folder means don't log, and drops them. Answers the file's path, blank when not logging
#[command]
pub async fn log_start(folder: String) -> Result<String, String> {//on the blocking pool, since it makes a folder and opens a file; lib.rs has the rule
	run_blocking(move || start(&folder)).await
}

/// The same as a plain function, for install.rs, which has no page to ask and takes the folder from its command line. The log's mutex is held across making the folder and opening the file, so a line from any thread in that moment waits for them, microseconds on the local disk the page names
pub fn start(folder: &str) -> Result<String, String> {
	let mut log = LOG.lock().unwrap_or_else(|poisoned| poisoned.into_inner());//take the log even if a previous holder panicked; losing every line for that would be worse
	let Log::Waiting(waiting) = &mut *log else { return Err("log: already started".to_string()) };
	let waiting = std::mem::take(waiting);
	if folder.is_empty() { *log = Log::Off; return Ok(String::new()) }
	fs::create_dir_all(folder).map_err(|e| format!("log: could not make {folder}, {e}"))?;
	let path = Path::new(folder).join(format!("{}-{}.log", stamp_date(), std::process::id()));
	let mut file = fs::OpenOptions::new().create(true).append(true).open(&path).map_err(|e| format!("log: could not open {}, {e}", path.display()))?;
	for line in &waiting { let _ = file.write_all(line.as_bytes()); }
	*log = Log::On(file, waiting.len());
	Ok(path.to_string_lossy().into_owned())
}

/// One line from the page; plain on purpose, though it touches the disk: it's one append to a local file, and lines sent through the pool could land out of order
#[command]
pub fn log_line(text: String) {
	write("page", &text);
}

/// The page's stick, replacing stick.txt; plain like log_line, for the same reasons: it's one small write to a local file, and sticks sent through the pool could land out of order, leaving an older one as the last word
#[command]
pub fn log_stick(text: String) {
	write_stick("page", &text);
}

/// Write every panic to the log, with where it happened, before rust unwinds; called once from run, before anything else. A panic in a plain command goes on to abort the process, since it unwinds into the web view's callback, and a panic in a body on the blocking pool reaches the page as an error without its location, so this line is what says where either was
pub fn log_panics() {
	let default_hook = std::panic::take_hook();//what rust does on its own, printing to stderr, which a development build still shows
	std::panic::set_hook(Box::new(move |info| {
		let location = info.location().map(|at| format!("{}:{}", at.file(), at.line())).unwrap_or_default();
		log(&format!("panic at {location}: {}", info.payload_as_str().unwrap_or("")));//this takes the log's own mutex, so nothing in this file may panic while holding it, and nothing here does
		default_hook(info);
	}));
}

/// One line from Rust; callable from anywhere, and dropped when not logging
pub fn log(text: &str) {
	write("rust", text);
}

fn write(side: &str, text: &str) {
	let line = format!("{} {} {side:<5} {text}\n", stamp_clock(), std::process::id());//the side padded to five, so the text starts in one column whoever wrote it
	let mut log = LOG.lock().unwrap_or_else(|poisoned| poisoned.into_inner());
	match &mut *log {
		Log::Waiting(waiting) => waiting.push(line),
		Log::On(file, count) => {
			if *count > CEILING { return }
			*count += 1;
			let _ = file.write_all(if *count > CEILING { b"# stopped here, at the ceiling of lines in one file\n" } else { line.as_bytes() });//a write that fails has nowhere to report; the log is the thing that would have taken it
		}
		Log::Off => {}
	}
}

/// Say where stick.txt goes, once, from setup: the log folder under the home folder, named for the brand the way log.js names it, made if it's missing when the first stick lands; a blank home leaves it nowhere, and sticks wait unwritten
pub fn stick_at(home: &str, stem: &str) {
	if home.is_empty() { return }
	let path = Path::new(home).join(format!("{stem}-logs")).join("stick.txt");
	let mut stick = STICK.lock().unwrap_or_else(|poisoned| poisoned.into_inner());
	if let Stick::Waiting(Some(text)) = &*stick { flash(&path, text) }
	*stick = Stick::At(path);
}

/// Replace the stick with this text, from Rust; callable from anywhere
#[allow(dead_code)]//ready for Rust's first caller, the way log is; the page is the only one so far
pub fn stick(text: &str) {
	write_stick("rust", text);
}

fn write_stick(side: &str, text: &str) {
	let stuck = format!("{} {} {side}\n{text}\n", stamp_iso(), std::process::id());
	let mut stick = STICK.lock().unwrap_or_else(|poisoned| poisoned.into_inner());
	match &mut *stick {
		Stick::Waiting(waiting) => *waiting = Some(stuck),
		Stick::At(path) => flash(path, &stuck),//under the lock, so two sticks from two threads land in the order they took it
	}
}

//write text to path as a finished copy renamed over what was there, so a reader never meets half of it, making the folder first if it's missing; a failure has nowhere to report, as with a log line
fn flash(path: &Path, text: &str) {
	if let Some(folder) = path.parent() { let _ = fs::create_dir_all(folder); }
	let temporary = path.with_extension(format!("{}.tmp", std::process::id()));//beside it, named for this process, so two processes sticking at once never share one
	if fs::write(&temporary, text).is_ok() && fs::rename(&temporary, path).is_err() { let _ = fs::remove_file(&temporary); }
}

//now, as ergo's tick, the milliseconds since 1970 each stamp below writes in UTC
fn now() -> i64 {
	let tick = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map_or(0, |since| since.as_millis() as i64);
	if has_tick(tick) { tick } else { 0 }//a clock set before 1970 or past 9999 reads as the start of 1970 rather than panicking inside a log line
}

//the date and time to the millisecond in the iso form, like 2026-10-01T01:14:05.123Z, for the stick, whose file name carries no date
fn stamp_iso() -> String {
	tick_to_iso(now())
}

//the time of day to the millisecond, like 01:14:05.123, for a line in a file whose name carries the date
fn stamp_clock() -> String {
	say_tick(now(), "hh24:mm:ss.ttt")
}

//the date and time to the second, like 2026-10-01-011405, for a file name that sorts by when its process started
fn stamp_date() -> String {
	say_tick(now(), "YYYY-MM-DD-hh24mmss")
}
