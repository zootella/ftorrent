import {invoke} from '@tauri-apps/api/core'

//exclusive locks on files, which hold against every other process and which the system releases however this one ends; locks.rs is the long version

export function lockTake(path)    { return invoke('lock_take',    {path}) }//lock the file at this path, making it empty if it isn't there but never its folder; answers held, busy, or trouble with the reason after a colon
export function lockRelease(path) { return invoke('lock_release', {path}) }//let go of the lock on the file at this path
