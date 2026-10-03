use std::collections::VecDeque;
use std::io::{BufRead, BufReader, Write};
use std::path::PathBuf;
use std::process::{Child, Command, Stdio};
use std::sync::mpsc::{channel, Sender};
use std::sync::Mutex;
use std::time::{Duration, Instant};
use serde::Serialize;
use tauri::{command, AppHandle, Manager, State};
use crate::paths::Paths;
use crate::queue::{Drained, Queue};

/*
The engine is a second process: a Python program, frozen with its interpreter and libtorrent into a folder the app carries as a resource, that will hold the torrent session. This module starts it, talks to it, and stops it. Nothing else in the app touches the process, and the page cannot start one at all: there is no shell plugin registered, so the only way a process is spawned is the Rust in this file, from a path this file computes.

The conversation is the one road between the page and libtorrent, and this module is the wire in the middle of it. In a plain Python program, libtorrent is one process and function calls: calls on the session and on a torrent's handle post requests to libtorrent's own network thread and return at once, and libtorrent reports everything back as alerts, which the program collects in batches. ftorrent puts a process boundary between the app and the engine, and that boundary is the only wire protocol ftorrent owns: newline-delimited JSON on the engine's own pipes. Pipes rather than a local socket, because a pipe between a parent and its child is reachable by no other process on the machine.

One road down, one road up, and Rust reads neither. Going down, the page builds a command as a line of JSON and hands it to engine_send, which passes it on to the engine's stdin; the only thing checked is that it's one line, since a newline would make it two. Coming up, every line the engine writes to stdout goes on a drained queue as text, and engine_take hands the page everything waiting. The engine is where JSON meets libtorrent, with one entry in its dispatch table per command, and the page is where the answers mean something. So using a libtorrent call ftorrent hasn't used before is two edits, the page that sends the command and the engine's entry that makes the call, and nothing here changes. Take pausing a torrent. With a Rust command per engine message it would be six edits across four layers: a JavaScript wrapper, a Rust command, its registration in lib.rs, the engine's branch, a Rust status field for the answer, and the page. On this road it's two: the page sends {"command":"pause",…}, and one entry in engine.py calls handle.pause(); libtorrent's answer comes back as an alert the engine already turns into a line. That's on purpose: a Rust command per engine message would rebuild libtorrent's whole surface at every layer, one feature at a time.

Down the road there's a thread of its own. engine_send puts the line on a channel and returns at once, and a writer thread that owns the engine's stdin does the writing. The reason is a cycle. The engine handles one message at a time, and blocks writing its answer until someone reads it; so a command that wrote into a full stdin pipe while holding the lock the stdout reader needs would stop all three for good, the command waiting on the engine, the engine on the reader, the reader on the lock. And a command that isn't async runs on Tauri's main thread, so even a write that only waited a while would freeze the window. With the writing on its own thread, the page is never held up by a pipe, and the lock below is only ever held for a moment, never across a read or a write.

Two things stay special here, because they're about the process rather than the conversation. init goes down the moment the engine starts, before the page exists, carrying the app's version, which is how the engine learns the one number it can't read for itself, and the data folder and state file startup worked out; its answer, ready, comes up the road like any other line. And the engine's stderr is kept in a ring of the last hundred lines, uninterpreted, so that a failure to start or a crash has something to show.

Stopping is the part that has to be right. A torrent client that leaves an engine running after its window closes is broken, so engine_stop runs from the two run events every quit path reaches. It puts a quit on the road down and lets go of the channel, so the writer sends what's queued, the quit last, and closes the engine's stdin; then it gives the engine a moment, and kills it if it is still there. Closing stdin is a second, independent signal: the engine exits when its input ends, so even an engine that never sees the quit line goes when the app does.

Where the engine lives is the one platform question, and Tauri's resource directory answers it: inside the bundle on macOS, beside the executable on Windows, under the application's lib directory on Linux, and in the target directory during development. The folder name is the same everywhere, so the path is built once below and never branches.
*/

const STDERR_LINES: usize = 100;//how many of the engine's stderr lines to keep; the last ones are the ones that explain a crash
const STDOUT_LINES: usize = 1000;//how many stdout lines wait for the page to take them before the oldest are dropped and counted; the page takes several times a second, so this is room for a burst, not a history
const STOP_WAIT: Duration = Duration::from_secs(2);//how long a quit gets before a kill

/// What the page sees when it asks how the engine is doing
#[derive(Serialize, Clone, Default)]
pub struct EngineStatus {
	pub path: String,//where the app looked for the engine
	pub running: bool,
	pub pid: u32,
	pub exit: String,//how the last run ended, blank while running or before the first start
	pub trouble: String,//why the engine could not be started, blank when it could
	pub stderr: Vec<String>,//the last lines the engine wrote to stderr
}

#[derive(Default)]
struct EngineInner {
	child: Option<Child>,//the process while it runs; taken by whoever ends up reaping it, so nothing waits on it twice
	down: Option<Sender<String>>,//the channel to the writer thread, which owns the engine's stdin; dropped to close it. Unbounded, so sending never waits, and what it holds is only the lines the writer hasn't written yet
	status: EngineStatus,
	stderr: VecDeque<String>,//the ring behind status.stderr
	lines: Queue<String, STDOUT_LINES>,//what the engine has written to stdout that the page hasn't taken yet
}

//managed by lib.rs; the Mutex because commands arrive on tauri's main thread and the reader threads below run on their own. The writer thread never takes it
#[derive(Default)]
pub struct Engine(Mutex<EngineInner>);

fn lock(engine: &Engine) -> std::sync::MutexGuard<'_, EngineInner> {
	engine.0.lock().unwrap_or_else(|poisoned| poisoned.into_inner())//take the state even if a previous holder panicked; losing track of a running engine for that would be worse
}

/// Where the engine's executable is on this platform, whether or not it is there
fn engine_path(app: &AppHandle) -> Result<PathBuf, String> {
	let name = format!("{}-engine", app.package_info().crate_name);//the folder the freeze produced, and the executable inside it, named brandStem-engine, the crate's name; tauri.conf.json's resources and the PyInstaller spec write that name out, and must agree
	let mut path = app.path().resource_dir().map_err(|e| format!("no resource directory: {e}"))?.join(&name);
	path.push(if cfg!(target_os = "windows") { format!("{name}.exe") } else { name });
	Ok(path)
}

/// Start the engine and send it init; called once from setup, before the page exists. Trouble is recorded rather than returned, because the page will ask
pub fn engine_start(app: &AppHandle) {
	let engine = app.state::<Engine>();
	let paths = app.state::<Paths>().inner().clone();
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

	let mut stdin = child.stdin.take().expect("stdin was piped");//moved into the writer thread below
	let stdout = child.stdout.take().expect("stdout was piped");
	let stderr = child.stderr.take().expect("stderr was piped");
	let pid = child.id();
	let init = serde_json::json!({
		"command": "init",
		"name": app.package_info().name,//brandName, the product name, which the engine puts at the front of the client name peers and trackers see
		"version": app.package_info().version.to_string(),//the app's version, read from tauri.conf.json at compile time, so the engine can name the client on the wire without a second place the number is written
		"paths": {//where the engine will keep what it keeps, as paths.rs resolved it; the engine never works these out for itself. The download folders are not here: they're a setting, so the page sends them down the road once it has read the settings file
			"data": paths.data,
			"state": paths.state,
		},
	}).to_string() + "\n";
	let (down, queued) = channel::<String>();
	let _ = down.send(init);//first on the road, so it's the first line the engine reads; this can't fail, since the writer's end is still right here
	{
		let mut inner = lock(&engine);
		inner.child = Some(child);
		inner.down = Some(down);
		inner.status.running = true;
		inner.status.pid = pid;
		inner.status.exit = String::new();
		inner.status.trouble = String::new();
	}

	//stdin: every line going down, in order, written here and nowhere else, so a write that waits on a full pipe waits on this thread alone
	std::thread::spawn(move || {
		for line in queued {//ends once every sender is gone, which is engine_stop letting go or the stdout reader seeing the engine end
			if stdin.write_all(line.as_bytes()).is_err() { break }//the pipe is gone, which means the engine is; the stdout reader is what notices that and records how it ended
		}
	});//stdin drops here, closing the pipe, which the engine reads as the end of its input

	//stdout: every line onto the queue for the page, unread. The end of the stream is the end of the engine, so this thread is also what notices a crash
	let app_out = app.clone();
	std::thread::spawn(move || {
		for line in BufReader::new(stdout).split(b'\n').map_while(Result::ok) {//raw bytes up to each newline; only a failed read ends the loop, and that means the pipe is gone, because a reader that stopped while the engine kept writing would leave the engine blocked on a full pipe
			let line = String::from_utf8_lossy(&line).trim_end_matches('\r').to_string();//text, with anything not utf-8 shown as a replacement character rather than losing the line, and a windows line ending trimmed
			lock(&app_out.state::<Engine>()).lines.push(line);
		}
		let child = lock(&app_out.state::<Engine>()).child.take();//outside the lock below, because wait blocks, and engine_stop must be able to get in meanwhile
		let exit = match child { Some(mut child) => child.wait().map(|s| s.to_string()).unwrap_or_else(|e| e.to_string()), None => String::new() };//none means engine_stop already has it and will record how it ended
		let engine = app_out.state::<Engine>();
		let mut inner = lock(&engine);
		inner.status.running = false;
		inner.down = None;//so the writer thread, with no sender left, finishes and lets go of the pipe
		if !exit.is_empty() { inner.status.exit = exit }
	});

	//stderr: kept, not shown, until somebody asks
	let app_err = app.clone();
	std::thread::spawn(move || {
		for line in BufReader::new(stderr).split(b'\n').map_while(Result::ok) {//the same rule as stdout: only a failed read ends the loop
			let line = String::from_utf8_lossy(&line).trim_end_matches('\r').to_string();//kept as text, with anything not utf-8 shown as a replacement character rather than losing the line
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
	let (child, down) = { let mut inner = lock(&engine); (inner.child.take(), inner.down.take()) };//taken, so the reader thread finds nothing to wait on and only records that the engine stopped
	let Some(mut child) = child else { return };//not running, or already being reaped by the reader thread
	if let Some(down) = down {
		let _ = down.send("{\"command\":\"quit\"}\n".to_string());//best effort; a dead engine cannot read it
	}//and down drops here, so the writer sends what's queued, the quit last, and closes stdin, the signal that always lands
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

/// Send the engine one line, a command the page built; it isn't read here, only checked to be a single line
#[command]
pub fn engine_send(engine: State<'_, Engine>, line: String) -> Result<(), String> {
	if line.contains('\n') || line.contains('\r') { return Err("a line for the engine can't contain a line break".to_string()) }//it would arrive as two messages, the second a fragment
	let inner = lock(&engine);
	let Some(down) = inner.down.as_ref() else { return Err("the engine is not running".to_string()) };//not started, or already gone; the page shows the engine's own status beside this
	down.send(line + "\n").map_err(|_| "the engine is not running".to_string())//onto the road down and back at once, without waiting on the pipe; this fails only once the writer thread has stopped, because the engine's pipe broke
}

/// Everything the engine has written to stdout since the last take, oldest first, and how many lines were dropped because the page fell behind
#[command]
pub fn engine_take(engine: State<'_, Engine>) -> Drained<String> {
	lock(&engine).lines.take()
}

/// How the engine is doing right now; the page asks, rather than being told, so nothing here has to know whether a page exists
#[command]
pub fn engine_status(engine: State<'_, Engine>) -> EngineStatus {
	let inner = lock(&engine);
	let mut status = inner.status.clone();
	status.stderr = inner.stderr.iter().cloned().collect();
	status
}
