//./src/desktop.js

import {invoke} from '@tauri-apps/api/core'

//the actions the user takes on the desktop rather than inside the window, which for now is one: quitting. desktop.rs is the long version, and says why only rust can see a quit coming

export function desktopExitHold(path, text) { return invoke('desktop_exit_hold', {path, text}) }//hold this text to be written to this path when ftorrent exits, replacing what was held for it; blank text forgets the path
