//./src/folders.js

import {invoke} from '@tauri-apps/api/core'

//the lock inside each download folder, which keeps two copies of ftorrent from using the same folder at once; folders.rs is the long version

export function folderLock(path)   { return invoke('folder_lock',   {path}) }//lock one download folder, never making it; answers held, busy, missing, or trouble with the reason after a colon
export function folderUnlock(path) { return invoke('folder_unlock', {path}) }//let go of one folder's lock
