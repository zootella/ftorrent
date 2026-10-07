import {invoke} from '@tauri-apps/api/core'

//the app's own life; lifecycle.rs is the long version

export function lifecycleExit() { return invoke('lifecycle_exit') }//quit, the way the menu's Exit and Quit do, which writes the settings and stops the engine
