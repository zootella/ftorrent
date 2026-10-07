import {invoke} from '@tauri-apps/api/core'

export function diskRead(path)                { return invoke('disk_read',    {path})                }
export function diskWrite(path, data)         { return invoke('disk_write',   {path, data})          }//data is an array of byte values; the whole file is replaced with them
export function diskMkdir(path)               { return invoke('disk_mkdir',   {path})                }//one folder, whose parent has to exist already
export function diskHide(path)                { return invoke('disk_hide',    {path})                }//hide a file or folder on windows; elsewhere a leading dot already does
export function diskReadDir(path)             { return invoke('disk_readdir', {path})                }
export function diskStat(path)                { return invoke('disk_stat',    {path})                }
export function diskCopy(source, destination) { return invoke('disk_copy',    {source, destination}) }
export function diskRmtree(path)              { return invoke('disk_rmtree',  {path})                }//a folder and everything in it
export function diskSpace(path)               { return invoke('disk_space',   {path})                }//bytes free for this user on the volume holding this path
export function diskAccess(path)              { return invoke('disk_access',  {path})                }//whether this user may read, write, and execute it, as the permissions stand
