import {invoke} from '@tauri-apps/api/core'

//requests to the web, and this machine's own address on its network; net.rs is the long version

export function netGet(url, limit, seconds, save = '') { return invoke('net_get', {url, limit, seconds, save: save || null}) }//the body at this https address as text, or with a save path, the body written to that file and its sha-256 in lowercase hex; a rejection past seconds or limit bytes, or for a 4xx or 5xx; a redirect isn't followed, and comes back as its own body
export function netLocal() { return invoke('net_local') }//the ipv4 address this machine sends from on its local network, like 192.168.1.23; a rejection when there's no route out, as with no network
