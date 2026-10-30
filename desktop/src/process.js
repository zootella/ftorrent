import {invoke} from '@tauri-apps/api/core'

//other programs, and this one; process.rs is the long version

export function processId()                  { return invoke('process_id') }                    //this copy's own process id, the number the task manager shows for it
export function processRun(program, args)    { return invoke('process_run',    {program, args}) }//run the program at this path and wait, answering its exit code
export function processOutput(program, args) { return invoke('process_output', {program, args}) }//run it and wait, answering {code, out, err}, its exit code and what it printed
export function processStart(program, args)  { return invoke('process_start',  {program, args}) }//start it and let it go, outliving this copy
export function processOpen(target)          { return invoke('process_open',   {target}) }        //open this file or address with the program the system has for it, the way a double-click would, and let it go
export function processElevate(program, args) { return invoke('process_elevate', {program, args}) }//run it with an administrator's rights, which the system asks the user to approve, and wait, answering its exit code; rejects when the user declines, and off windows
