//./src/instance.js

import {invoke} from '@tauri-apps/api/core'

//one running ftorrent per copy: instance.rs holds the lock, serves the handoff, and keeps what arrives; it's the long version

export function instanceStatus() { return invoke('instance_status') }//the lock file and whether this copy holds it, how a second launch reaches this copy, any trouble, and every request that has arrived, oldest first
