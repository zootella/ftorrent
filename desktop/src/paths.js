//./src/paths.js

import {invoke} from '@tauri-apps/api/core'

//where everything is: paths.rs works it out once at startup, before the page exists, and is the long version

export function pathsStatus() { return invoke('paths_status') }//installed, portable, or translocated; the program's location, the data folder and the files in it, each download folder as the settings write it and as it resolves here, and any trouble
