use std::fs::{self, File};
use std::io::Write;
use std::path::Path;
use std::sync::Mutex;
use tauri::command;
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
pub async fn log_start(folder: String) -> Result<String, String> {//on the blocking pool, since it makes a folder and opens a file; lib.rs has the rule. The log's mutex is held across both, so a line from any thread in that moment waits for them, microseconds on the local disk the page names
	run_blocking(move || {
		let mut log = LOG.lock().unwrap_or_else(|poisoned| poisoned.into_inner());//take the log even if a previous holder panicked; losing every line for that would be worse
		let Log::Waiting(waiting) = &mut *log else { return Err("log: already started".to_string()) };
		let waiting = std::mem::take(waiting);
		if folder.is_empty() { *log = Log::Off; return Ok(String::new()) }
		fs::create_dir_all(&folder).map_err(|e| format!("log: could not make {folder}, {e}"))?;
		let path = Path::new(&folder).join(format!("{}-{}.log", stamp_date(), std::process::id()));
		let mut file = fs::OpenOptions::new().create(true).append(true).open(&path).map_err(|e| format!("log: could not open {}, {e}", path.display()))?;
		for line in &waiting { let _ = file.write_all(line.as_bytes()); }
		*log = Log::On(file, waiting.len());
		Ok(path.to_string_lossy().into_owned())
	}).await
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

//the time of day in UTC to the millisecond, like 01:14:05.123
fn stamp_clock() -> String {
	let since = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap_or_default();//a clock set before 1970 reads as midnight rather than panicking inside a log line
	let seconds = since.as_secs() % 86_400;//seconds into today, since a unix day is exactly that long
	format!("{:02}:{:02}:{:02}.{:03}", seconds / 3600, seconds / 60 % 60, seconds % 60, since.subsec_millis())
}

//the date and time in UTC to the second, like 2026-10-01-011405, for a file name that sorts by when its process started
fn stamp_date() -> String {
	let seconds = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap_or_default().as_secs();
	let days = (seconds / 86_400) as i64;
	let (year, month, day) = civil(days);
	let time = seconds % 86_400;
	format!("{year:04}-{month:02}-{day:02}-{:02}{:02}{:02}", time / 3600, time / 60 % 60, time % 60)
}

//days since 1970 as a year, month, and day, by Howard Hinnant's civil-from-days, which the standard library doesn't offer
fn civil(days: i64) -> (i64, i64, i64) {
	let z = days + 719_468;//counted from 0000-03-01, so a leap day ends the year rather than falling inside it
	let era = z.div_euclid(146_097);//400-year eras
	let day_of_era = z - era * 146_097;
	let year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36_524 - day_of_era / 146_096) / 365;
	let day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
	let month_from_march = (5 * day_of_year + 2) / 153;
	let day = day_of_year - (153 * month_from_march + 2) / 5 + 1;
	let month = if month_from_march < 10 { month_from_march + 3 } else { month_from_march - 9 };
	(year_of_era + era * 400 + if month <= 2 { 1 } else { 0 }, month, day)
}
