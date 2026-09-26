import {invoke} from '@tauri-apps/api/core'

//one running ftorrent per copy: instance.rs holds the lock, serves the handoff, and queues what arrives; it's the long version

export function instanceStatus() { return invoke('instance_status') }//the lock file and whether this copy holds it, how a second launch reaches this copy, and any trouble
export function instanceTake()   { return invoke('instance_take') }  //everything that has reached this copy since the last take, oldest first, each marked launch or handoff, as {items, dropped}
