import {invoke} from '@tauri-apps/api/core'

//other programs, and this one; process.rs is the long version

export function processId()                 { return invoke('process_id') }                   //this copy's own process id, the number the task manager shows for it
export function processRun(program, args)   { return invoke('process_run',   {program, args}) }//run the program at this path and wait, answering its exit code
export function processStart(program, args) { return invoke('process_start', {program, args}) }//start it and let it go, outliving this copy
export function processOpen(target)         { return invoke('process_open',  {target}) }       //open this file or address with the program the system has for it, the way a double-click would, and let it go
