use tauri::command;
use crate::run_blocking;

/// This process's id, the number the task manager and the log's stamps show for it; answered from memory, so plain rather than async. The engine's is in engine_status, since the engine is the one child this process starts and keeps
#[command]
pub fn process_id() -> u32 {
	std::process::id()
}

/// Run the program at this path with these arguments and wait for it to finish, answering its exit code, -1 when a signal ended it and so it has none; started directly rather than through a shell, so no argument is ever read as a command, and with nothing to read and nowhere to print
#[command]
pub async fn process_run(program: String, args: Vec<String>) -> Result<i32, String> {
	run_blocking(move || {
		let status = std::process::Command::new(&program).args(&args)
			.stdin(std::process::Stdio::null()).stdout(std::process::Stdio::null()).stderr(std::process::Stdio::null())
			.status().map_err(|e| format!("could not run {program}: {e}"))?;
		Ok(status.code().unwrap_or(-1))
	}).await
}

/// Start the program at this path with these arguments and answer once it has started, leaving it to outlive this process; directly, like process_run, and with no console
#[command]
pub async fn process_start(program: String, args: Vec<String>) -> Result<(), String> {
	run_blocking(move || {
		let mut command = std::process::Command::new(&program);
		command.args(&args).stdin(std::process::Stdio::null()).stdout(std::process::Stdio::null()).stderr(std::process::Stdio::null());
		#[cfg(target_os = "windows")]
		{
			use std::os::windows::process::CommandExt;
			command.creation_flags(0x00000008);//DETACHED_PROCESS, from winbase.h: a console program started from a windowed app would otherwise open a console window of its own, and a windowed program like the setup program never has one; CREATE_NO_WINDOW would be ignored beside this flag, so it isn't set. On a mac and linux a child simply goes on when its parent ends
		}
		command.spawn().map(|_| ()).map_err(|e| format!("could not start {program}: {e}"))//dropping the child lets it run on; this process never waits on it
	}).await
}

/// Open this file, folder, or address with the program the system has for it, the way a double-click in the file manager would, and let that program go: a .txt file opens in the text editor, an https address in the browser, an ms-settings address in Windows Settings; through ShellExecuteEx on windows, /usr/bin/open and launch services on a mac, and xdg-open on linux, each the system's own table of what opens what. Answered once the system has taken it, with no wait on the program, and on the blocking pool because taking it can mean starting a browser
#[command]
pub async fn process_open(target: String) -> Result<(), String> {
	run_blocking(move || open::that_detached(&target).map_err(|e| format!("could not open {target}: {e}"))).await
}
