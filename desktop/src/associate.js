//./src/associate.js

import {invoke} from '@tauri-apps/api/core'

//what ftorrent has told the operating system it can open: associate.rs registers the file types and link schemes at startup on windows, taking nothing another program holds, and is the long version

export function associateStatus() { return invoke('associate_status') }//one line about what registration did this launch, or blank where there was nothing to do: a mac, a debug build, a portable copy
