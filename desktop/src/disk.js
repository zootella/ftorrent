//./src/disk.js

import {invoke} from '@tauri-apps/api/core'

export function diskRead(path)                { return invoke('disk_read',    {path})                }
export function diskReadDir(path)             { return invoke('disk_readdir', {path})                }
export function diskStat(path)                { return invoke('disk_stat',    {path})                }
export function diskCopy(source, destination) { return invoke('disk_copy',    {source, destination}) }
