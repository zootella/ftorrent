import {invoke} from '@tauri-apps/api/core'
import {brandStem} from './brand.js'

/*
The page's half of the log; log.rs is the long version. log(text) hands one line down, from any file, and Rust stamps it with the time, this process's id, and page, and appends it to the file, beside Rust's own lines. Off at the factory: [log] record in ftorrent.toml turns it on, and then every process writes a file of its own into one folder, ftorrent-logs under the home folder, so a session with several copies running leaves one place to look. Turning it on is the permission to write there, which is why the folder is outside every copy's own, and a portable copy, which otherwise leaves nothing on the host, writes there too.

A log carries paths and magnet links, which say what someone downloads, so ftorrent keeps none unless the user asks for it.
*/

export const logFolder = `${brandStem}-logs`//under the home folder, on every platform

export function logPlace(record, home) {//the folder this process logs into, from the setting and the home folder, or blank when not logging
	return record && home ? `${home}/${logFolder}` : ''
}

export async function logStart(record, home) {//say whether this process logs, once, after the settings are read; until then rust holds every line, the page's and its own, and this writes them out or drops them. Answers the file's path, blank when not logging
	return await invoke('log_start', {folder: logPlace(record, home)})
}

export function log(text) {//one line, from anywhere in the page; dropped when not logging, and never a reason to stop what the caller is doing
	invoke('log_line', {text}).catch(() => {})
}
