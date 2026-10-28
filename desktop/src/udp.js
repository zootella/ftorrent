import {invoke} from '@tauri-apps/api/core'

//udp sockets held open between calls, for a question that takes several steps on one socket; one read at a time on a socket, and sends whenever; udp.rs is the long version

export function udpOpen(family, port)              { return invoke('udp_open',    {family, port}) }           //open a udp socket on local port, or the system's pick with 0, over family 4 or 6, and hold it until udpClose; answers {socket, port}, the number the calls below take, and the port it's bound to
export function udpStun(socket, servers, seconds)  { return invoke('udp_stun',    {socket, servers, seconds}) }//a stun binding request to each of servers, names and ports like stun.ftorrent.com:3478, all at once from this socket, each sent again until it answers or seconds pass; answers what netStun does, {local, servers: [{server, to, address, port, milliseconds, sends, bytes, from, answer, error}]}
export function udpSend(socket, to, data)          { return invoke('udp_send',    {socket, to, data}) }       //send data, bytes in lowercase hex, from this socket to an address and port like '203.0.113.42:6881' or '[2001:db8::1]:6881'; answers how many bytes went
export function udpReceive(socket, count, seconds) { return invoke('udp_receive', {socket, count, seconds}) } //wait on this socket until count datagrams arrive or seconds pass; answers each one that did, in order, as [{from, milliseconds, bytes, data}]: where it came from, the milliseconds from the start of the wait, its size, and its bytes in lowercase hex; an empty list when nothing came
export function udpClose(socket)                   { return invoke('udp_close',   {socket}) }                 //close this socket; nothing happens for a number that isn't open
