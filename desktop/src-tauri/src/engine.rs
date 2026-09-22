//./src-tauri/src/engine.rs

use std::collections::VecDeque;
use std::io::{BufRead, BufReader, Write};
use std::path::PathBuf;
use std::process::{Child, ChildStdin, Command, Stdio};
use std::sync::Mutex;
use std::time::{Duration, Instant};
use serde::Serialize;
use tauri::{command, AppHandle, Manager, State};

/*
The engine is a second process: a Python program, frozen with its interpreter and libtorrent into a folder the app carries as a resource, that will hold the torrent session. This module starts it, talks to it, and stops it. Nothing else in the app touches the process, and the page cannot start one at all: there is no shell plugin registered, so the only way a process is spawned is the Rust in this file, from a path this file computes.

The conversation is newline-delimited JSON on the engine's own pipes. Rust writes one command per line to its stdin and reads one event per line from its stdout, and pipes are the reason for that choice over a local socket: a pipe between a parent and its child is reachable by no other process on the machine. Today the whole protocol is one exchange, init in and ready out, which proves both directions work; init carries the app's version, which is how the engine learns the one number it cannot read for itself. The engine's stderr is kept in memory, the last hundred lines, so that a failure to start has something to show.

Stopping is the part that has to be right. A torrent client that leaves an engine running after its window closes is broken, so engine_stop runs from the two run events every quit path reaches, asks the engine to quit and closes its stdin, gives it a moment, and kills it if it is still there. Closing stdin is a second, independent signal: the engine exits when its input ends, so even an engine that never sees the quit line goes when the app does.

Where the engine lives is the one platform question, and Tauri's resource directory answers it: inside the bundle on macOS, beside the executable on Windows, under the application's lib directory on Linux, and in the target directory during development. The folder name is the same everywhere, so the path is built once below and never branches.
*/

const ENGINE_NAME: &str = "ftorrent-engine";//the folder the freeze produced, and the executable inside it
const STDERR_LINES: usize = 100;//how many of the engine's stderr lines to keep; the last ones are the ones that explain a crash
const STOP_WAIT: Duration = Duration::from_secs(2);//how long a quit gets before a kill

/// What the page sees when it asks how the engine is doing
#[derive(Serialize, Clone, Default)]
pub struct EngineStatus {
	pub path: String,//where the app looked for the engine
	pub running: bool,
	pub pid: u32,
	pub ready: Option<serde_json::Value>,//the engine's ready event, exactly as it sent it, once it has arrived
	pub exit: String,//how the last run ended, blank while running or before the first start
	pub trouble: String,//why the engine could not be started, blank when it could
	pub stderr: Vec<String>,//the last lines the engine wrote to stderr
}

#[derive(Default)]
struct EngineInner {
	child: Option<Child>,//the process while it runs; taken by whoever ends up reaping it, so nothing waits on it twice
	stdin: Option<ChildStdin>,//the pipe commands go down; dropped to close it
	status: EngineStatus,
	stderr: VecDeque<String>,//the ring behind status.stderr
}

//managed by lib.rs; the Mutex because commands arrive on tauri's threads and the reader threads below on their own
#[derive(Default)]
pub struct Engine(Mutex<EngineInner>);

fn lock(engine: &Engine) -> std::sync::MutexGuard<'_, EngineInner> {
	engine.0.lock().unwrap_or_else(|poisoned| poisoned.into_inner())//take the state even if a previous holder panicked; losing track of a running engine for that would be worse
}

/// Where the engine's executable is on this platform, whether or not it is there
fn engine_path(app: &AppHandle) -> Result<PathBuf, String> {
	let mut path = app.path().resource_dir().map_err(|e| format!("no resource directory: {e}"))?.join(ENGINE_NAME);
	path.push(if cfg!(target_os = "windows") { "ftorrent-engine.exe" } else { ENGINE_NAME });
	Ok(path)
}

/// Start the engine and send it init; called once from setup, before the page exists. Trouble is recorded rather than returned, because the page will ask
pub fn engine_start(app: &AppHandle) {
	let engine = app.state::<Engine>();
	let path = match engine_path(app) {
		Ok(path) => path,
		Err(trouble) => { lock(&engine).status.trouble = trouble; return }
	};
	lock(&engine).status.path = path.to_string_lossy().into_owned();
	if !path.is_file() { lock(&engine).status.trouble = "the engine is not built; run pnpm engine in the desktop workspace".to_string(); return }

	let mut command = Command::new(&path);
	command.stdin(Stdio::piped()).stdout(Stdio::piped()).stderr(Stdio::piped());
	#[cfg(target_os = "windows")]
	{
		use std::os::windows::process::CommandExt;
		command.creation_flags(0x0800_0000);//CREATE_NO_WINDOW: the engine is a console program, and without this windows would open a console for it behind the app
	}
	let mut child = match command.spawn() {
		Ok(child) => child,
		Err(e) => { lock(&engine).status.trouble = format!("could not start the engine: {e}"); return }
	};

	let mut stdin = child.stdin.take().expect("stdin was piped");
	let stdout = child.stdout.take().expect("stdout was piped");
	let stderr = child.stderr.take().expect("stderr was piped");
	let pid = child.id();
	let init = format!("{{\"command\":\"init\",\"version\":{}}}\n", serde_json::to_string(&app.package_info().version.to_string()).unwrap_or_else(|_| "\"\"".to_string()));//the app's version, read from tauri.conf.json at compile time, so the engine can name the client on the wire without a second place the number is written
	if let Err(e) = stdin.write_all(init.as_bytes()) { lock(&engine).status.trouble = format!("could not write to the engine: {e}") }//recorded and carried on: the reader below will see the engine's side of whatever went wrong
	{
		let mut inner = lock(&engine);
		inner.child = Some(child);
		inner.stdin = Some(stdin);
		inner.status.running = true;
		inner.status.pid = pid;
		inner.status.ready = None;
		inner.status.exit = String::new();
		inner.status.trouble = String::new();
	}

	//stdout: one json event per line, of which only ready means anything yet. The end of the stream is the end of the engine, so this thread is also what notices a crash
	let app_out = app.clone();
	std::thread::spawn(move || {
		for line in BufReader::new(stdout).lines().filter_map(Result::ok) {//a line that is not utf-8 is dropped rather than ending the reader, because a reader that stops while the engine keeps writing leaves the engine blocked on a full pipe; the stream ends only when the engine does
			if let Ok(value) = serde_json::from_str::<serde_json::Value>(&line) {
				if value.get("event").and_then(|v| v.as_str()) == Some("ready") { lock(&app_out.state::<Engine>()).status.ready = Some(value) }
			}
		}
		let child = lock(&app_out.state::<Engine>()).child.take();//outside the lock below, because wait blocks, and engine_stop must be able to get in meanwhile
		let exit = match child { Some(mut child) => child.wait().map(|s| s.to_string()).unwrap_or_else(|e| e.to_string()), None => String::new() };//none means engine_stop already has it and will record how it ended
		let engine = app_out.state::<Engine>();
		let mut inner = lock(&engine);
		inner.status.running = false;
		inner.stdin = None;
		if !exit.is_empty() { inner.status.exit = exit }
	});

	//stderr: kept, not shown, until somebody asks
	let app_err = app.clone();
	std::thread::spawn(move || {
		for line in BufReader::new(stderr).lines().filter_map(Result::ok) {//the same rule as stdout
			let engine = app_err.state::<Engine>();
			let mut inner = lock(&engine);
			if inner.stderr.len() >= STDERR_LINES { inner.stderr.pop_front(); }
			inner.stderr.push_back(line);
		}
	});
}

/// Stop the engine: ask, wait a moment, then kill. Called from the run events in lib.rs, and safe to call when nothing is running
pub fn engine_stop(app: &AppHandle) {
	let engine = app.state::<Engine>();
	let (child, stdin) = { let mut inner = lock(&engine); (inner.child.take(), inner.stdin.take()) };//taken, so the reader thread finds nothing to wait on and only records that the engine stopped
	let Some(mut child) = child else { return };//not running, or already being reaped by the reader thread
	if let Some(mut stdin) = stdin {
		let _ = stdin.write_all(b"{\"command\":\"quit\"}\n");//best effort; a dead engine cannot read it, and dropping stdin next is the signal that always lands
	}
	let started = Instant::now();
	let exit = loop {
		match child.try_wait() {
			Ok(Some(status)) => break status.to_string(),
			Ok(None) if started.elapsed() < STOP_WAIT => std::thread::sleep(Duration::from_millis(50)),
			Ok(None) => { let _ = child.kill(); break child.wait().map(|s| format!("killed, {s}")).unwrap_or_else(|e| e.to_string()) }//it had its chance
			Err(e) => break e.to_string(),
		}
	};
	let mut inner = lock(&engine);
	inner.status.running = false;
	inner.status.exit = exit;
}

/// How the engine is doing right now; the page asks, rather than being told, so nothing here has to know whether a page exists
#[command]
pub fn engine_status(engine: State<'_, Engine>) -> EngineStatus {
	let inner = lock(&engine);
	let mut status = inner.status.clone();
	status.stderr = inner.stderr.iter().cloned().collect();
	status
}
