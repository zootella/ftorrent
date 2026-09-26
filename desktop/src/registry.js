import {invoke} from '@tauri-apps/api/core'

//the windows registry, read and written for the page; registry.rs is the long version, and on macOS and linux each of these answers that there's no registry

export function registryGet(root, key, name)  { return invoke('registry_get',    {root, key, name})  }//read a string value through classes or user; a blank name is the key's default value, and nothing comes back when it isn't there
export function registrySet(key, name, value) { return invoke('registry_set',    {key, name, value}) }//write a string value under the current user, only if it would change; answers whether it did
export function registryNotify()              { return invoke('registry_notify')                     }//tell the shell that file associations changed
