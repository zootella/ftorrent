use std::fs::{self, File};
use std::io::Write;
use std::path::Path;
use std::sync::Mutex;
use tauri::command;
use zootella_ergo::{has_tick, tick_to_iso};
use crate::run_blocking;

/*
A log: lines of text from anywhere in the program, each stamped with the time, this process's id, and the side that wrote it, appended to a file as they happen. Two functions write a line, log here for Rust, called from anywhere as a plain function, and log in log.js for the page, which hands its lines down through log_line; the stamp is put on here for both, so every line in every file has one shape:

	01:14:05.123 32554 rust  took the lock
	01:14:05.871 32554 page  opens .torrent with Transmission.app

The time is UTC, to the millisecond, and the file's name carries the date. The process id tells copies apart, so the files of several processes, an installed copy and a portable one side by side, say, can sit in one folder and sort into a single timeline, every line already saying who wrote it. The side says which layer spoke, since a line from Rust and one from the page can land a few milliseconds out of order, and that tells whether two lines could have raced.

Lines are written as they arrive, not saved for the end, so a crash keeps everything up to the crash, and the file can be read while the program runs. Each process writes a file of its own, named for when it started and its process id, so two processes never append to one file. A file stops at a ceiling of lines, saying so, so a log left on can't fill a disk.

Whether to log, and where, is the page's to decide, since it's a setting, and the page reads settings. Rust starts logging before any page exists, so until the page says, its lines wait in memory: log_start with a folder writes them to the new file and keeps going, and log_start with a blank folder drops them and every line after. A process that leaves before it has a page, a second launch handing over what it carried, never logs.

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

//now in UTC, in the iso form like 2026-10-01T01:14:05.123Z, which both stamps below are cut from
fn stamp_iso() -> String {
	let now = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map_or(0, |since| since.as_millis() as i64);//milliseconds since 1970, which is ergo's tick
	tick_to_iso(if has_tick(now) { now } else { 0 })//a clock set before 1970 or past 9999 reads as the start of 1970 rather than panicking inside a log line
}

//the time of day in UTC to the millisecond, like 01:14:05.123
fn stamp_clock() -> String {
	stamp_iso()[11..23].to_string()
}

//the date and time in UTC to the second, like 2026-10-01-011405, for a file name that sorts by when its process started
fn stamp_date() -> String {
	let iso = stamp_iso();
	format!("{}-{}{}{}", &iso[..10], &iso[11..13], &iso[14..16], &iso[17..19])//the date, then the hour, minute, and second without their colons
}
