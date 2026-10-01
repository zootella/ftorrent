import {invoke} from '@tauri-apps/api/core'

//launch services on the mac, which decides what opens a file or a link; launch.rs is the long version, and on windows and linux each of these answers that there's no launch services

export function launchOpens(name)      { return invoke('launch_opens', {name})      }//the path of the app the mac would open a file type like .torrent or a scheme like magnet with right now; nothing when no app would
export function launchClaim(name, app) { return invoke('launch_claim', {name, app}) }//make the app at this path the default for that type; macOS asks the user nothing, and answers once it's done
