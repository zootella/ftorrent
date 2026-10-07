import {invoke} from '@tauri-apps/api/core'

//replacing this copy with a newer one; update.rs is the long version, and stores/update.js is when

export function updateReplace(zip) { return invoke('update_replace', {zip}) }//unpack the downloaded update and swap it in for this copy's bundle, answering the bundle's path; on a mac
export function updateRestart()    { return invoke('update_restart') }        //start the copy now in place and quit this one
