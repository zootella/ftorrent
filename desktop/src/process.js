import {invoke} from '@tauri-apps/api/core'

//other programs; process.rs is the long version

export function processRun(program, args)   { return invoke('process_run',   {program, args}) }//run the program at this path and wait, answering its exit code
export function processStart(program, args) { return invoke('process_start', {program, args}) }//start it and let it go, outliving this copy
