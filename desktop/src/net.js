import {invoke} from '@tauri-apps/api/core'

//requests to the web; net.rs is the long version

export function netGet(url, limit, seconds) { return invoke('net_get', {url, limit, seconds}) }//the body at this https address as text, or a rejection past seconds or limit bytes, or for a 4xx or 5xx; a redirect isn't followed, and comes back as its own body
