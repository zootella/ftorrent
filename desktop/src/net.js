import {invoke} from '@tauri-apps/api/core'

//requests to the web, and the address this machine sends from; net.rs is the long version

export function netGet(url, limit, seconds, save = '') { return invoke('net_get', {url, limit, seconds, save: save || null}) }//the body at this https address as text, or with a save path, the body written to that file and its sha-256 in lowercase hex; a rejection past seconds or limit bytes, or for a 4xx or 5xx; a redirect isn't followed, and comes back as its own body
export function netLocal(toward) { return invoke('net_local', {toward}) }//the address this machine would send from to reach the address toward, ipv4 or ipv6, as text: like 192.168.1.23 toward an ipv4 address beyond a home network, and the machine's own global address toward an ipv6 one; nothing is sent, and a rejection when there's no route, as with no network or no ipv6
