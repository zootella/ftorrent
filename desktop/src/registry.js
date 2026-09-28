import {invoke} from '@tauri-apps/api/core'

//the windows registry, read and written for the page; registry.rs is the long version, and on macOS and linux each of these answers that there's no registry

export function registryGet(root, key, name)  { return invoke('registry_get',        {root, key, name})  }//read a string value through classes or user; a blank name is the key's default value, and nothing comes back when it isn't there
export function registrySet(key, name, value) { return invoke('registry_set',        {key, name, value}) }//write a string value under the current user, only if it would change; answers whether it did
export function registryDelete(key, name)     { return invoke('registry_delete',     {key, name})        }//delete a value under the current user, a blank name being the key's default; answers whether there was one
export function registryDeleteKey(key)        { return invoke('registry_delete_key', {key})              }//delete a key under the current user and everything under it; answers whether there was one
export function registryNotify()              { return invoke('registry_notify')                         }//tell the shell that file associations changed
export function registryOpens(name)           { return invoke('registry_opens',      {name})             }//which program windows opens a file type like .torrent or a scheme like magnet with right now, as {program, executable}, the ProgID and the path it runs; nothing when nothing opens it
