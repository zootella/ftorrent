import {invoke} from '@tauri-apps/api/core'

//the engine is the second process, a frozen python holding libtorrent, that rust starts beside the app; engine.rs is the long version, with the road between the page and libtorrent that these three commands are the page's end of

export function engineStatus()   { return invoke('engine_status') }       //how the engine process is doing: whether it is running, its pid, how its last run ended, any trouble starting it, and the last lines it wrote to stderr
export function engineSend(line) { return invoke('engine_send', {line}) } //one command down the road, a line of json the page built; rust passes it on without reading it
export function engineTake()     { return invoke('engine_take') }         //every line the engine has written since the last take, oldest first, as {items, dropped}
