//./src/engine.js

import {invoke} from '@tauri-apps/api/core'

//the engine is the second process, a frozen python holding libtorrent, that rust starts beside the app; engine.rs is the long version, and says why only rust can start it and how it is stopped

export function engineStatus() { return invoke('engine_status') }//how the engine is doing right now: whether it is running, its pid, the ready event it sent, how its last run ended, and the last lines it wrote to stderr
