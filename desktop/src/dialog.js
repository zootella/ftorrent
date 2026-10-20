import {invoke} from '@tauri-apps/api/core'

//the operating system's own dialog boxes, which only rust can put up; dialog.rs is the long version

export function dialogOpen({files, folders}) { return invoke('dialog_open', {files, folders}) }//the system's open box over this window, for a file, a folder, or on a mac either: the path chosen, or blank when the user cancels
