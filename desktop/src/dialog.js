import {invoke} from '@tauri-apps/api/core'

//the operating system's own boxes for choosing what to open and where to save, which only rust can put up; dialog.rs is the long version. Each opens in the folder start names, or where the system chooses when it's blank, which on Windows can be the program's own folder, so every box ftorrent shows names its folder; and a box for files offers the ones extensions names, without their dots, like ['torrent'] with the kind 'Torrent files', or every file when there are none

export function dialogOpen({folder = false, either = false, multiple = false, start = '', kind = '', extensions = []} = {}) { return invoke('dialog_open', {folder, either, multiple, start, kind, extensions}) }//the open box over this window, for files or, with folder, a folder, or with either a file or a folder on a mac and files elsewhere, one or with multiple several: the paths chosen, none when the user cancels
export function dialogSave({start = '', name = '', kind = '', extensions = []} = {}) { return invoke('dialog_save', {start, name, kind, extensions}) }//the save box over this window, suggesting the file name name: the path chosen, blank when the user cancels
