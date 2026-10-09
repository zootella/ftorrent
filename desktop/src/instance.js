import {invoke} from '@tauri-apps/api/core'

//one running ftorrent per copy: instance.rs holds the lock, serves the handoff, and queues what arrives; it's the long version

export function instanceTake() { return invoke('instance_take') }//everything that has reached this copy since the last take, oldest first, each marked launch, handoff, or open, as {items, dropped}
