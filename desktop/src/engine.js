//./src/engine.js

import {invoke} from '@tauri-apps/api/core'

//the engine is the second process, a frozen python holding libtorrent, that rust starts beside the app; engine.rs is the long version, and says why only rust can start it and how it is stopped

export function engineStatus()         { return invoke('engine_status') }//how the engine is doing right now: whether it is running, its pid, the ready and folders events it sent, how its last run ended, and the last lines it wrote to stderr
export function engineFolders(folders) { return invoke('engine_folders', {folders}) }//tell the engine which download folders to use, as absolute paths; once after the settings are read, and again whenever they change
