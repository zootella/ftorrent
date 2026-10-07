use tauri::command;
use crate::run_blocking;

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
			command.creation_flags(0x00000008 | 0x08000000);//DETACHED_PROCESS and CREATE_NO_WINDOW, from winbase.h: a program started from a windowed app would otherwise get a console of its own or share this one's; on a mac and linux a child simply goes on when its parent ends
		}
		command.spawn().map(|_| ()).map_err(|e| format!("could not start {program}: {e}"))//dropping the child lets it run on; this process never waits on it
	}).await
}
