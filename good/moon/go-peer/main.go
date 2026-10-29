/*
A peer behind one of moon's NATs. systemd runs a copy inside each of moon's two network namespaces, cone and symmetric, where the masquerade rules in rules.v4 translate everything it sends, and moon's Go service, ftorrent-moon, reaches it across the namespace's veth pair at its private address, 10.88.1.2 or 10.88.2.2, on port 8200. Nothing on the internet can reach that address, since the namespace's one way out is the masquerade, which lets answers back in and nothing else.

	GET /punch?to=A:P&token=T  -> a new UDP socket's public address and port, from STUN, as JSON, and then a punch from that socket to A:P
	POST /offer?caller=A       -> the same punch over WebRTC, for a page: an offer in and an answer out, which offer.go explains

A punch is the hole punch, done by hand. The caller learns its own public address and port from STUN and names the port to ftorrent-moon, which hands the caller's address and that port here. The peer opens a socket, learns its own public address and port from the main server's STUN, answers them, and from then sends the caller a small packet every fifth of a second for five seconds, while the caller sends to the peer. A NAT lets a packet in only once the socket behind it has sent to where the packet comes from, so the two get through only when both send, and only when each NAT shows the other side the port STUN saw. Each packet says whether the caller's have arrived, so the caller reads both directions from its own socket: a packet from the peer at all is the peer getting through, and one saying heard yes is the caller getting through.

The peer sends only to the address and port it was handed, which ftorrent-moon takes from the request itself, so neither can be pointed at anyone else, and only what the NAT lets in from there can reach the socket.

The STUN server is the main server's, stun.ftorrent.com, and not moon's own: a packet from the namespace to an address on moon itself is delivered on the box and never crosses the masquerade, so moon's STUN would see the private address. The peer resolves the name through the provider's resolvers, which its unit binds over /etc/resolv.conf, since moon's own resolver listens on a loopback address the namespace doesn't share.

The punch uses Go's standard library alone, and the offer pion, WebRTC written in Go, which go.mod names and go.sum pins by hash. ftorrent-moon's main.go, beside this folder, opens with the Go idioms both programs lean on: errors as a second value, goroutines, defer, and Durations that count nanoseconds.
*/
package main

import (
	"bytes"//comparing a STUN answer's transaction id
	"crypto/rand"//random bytes from the operating system, for a STUN transaction id
	"encoding/binary"//numbers in STUN's header and attributes, most significant byte first
	"encoding/json"//the punch's report
	"errors"//errors.Is, and an error of our own
	"fmt"//formatted text, written into a packet
	"net"//the UDP socket, and resolving the STUN server's name
	"net/http"//the server ftorrent-moon calls
	"net/netip"//parsing the address and port to send to, strictly
	"os"//the exit code
	"strings"//reading a packet's text
	"time"//moments, and Durations in nanoseconds
)

const (
	stunServer   = "stun.ftorrent.com:3478"//the main server's STUN, which sees the namespace's traffic after the masquerade
	stunTimeout  = 3 * time.Second//to hear STUN's answer, across every send of the request
	magicCookie  = 0x2112A442//the fixed value in every STUN header since RFC 5389, which XOR-MAPPED-ADDRESS masks the address with
	punchGap     = 200 * time.Millisecond//between the packets of a punch
	punchPackets = 25//how many, so a punch lasts five seconds
	atOnce       = 32//punches and WebRTC sessions running at the same moment, each with sockets of its own, past which a new one is answered 503
)

//running holds a value for each punch under way, by hand or over WebRTC: a channel with room for atOnce values, which a punch puts one into as it starts and takes back out as it ends, so a full channel means atOnce are running
var running = make(chan struct{}, atOnce)//struct{} is a type with no fields, which takes no memory, so the channel counts and carries nothing

//nat is the name of the NAT this copy runs behind, cone or symmetric, which its unit passes as the one argument, for the reports
var nat string

//main serves the punch and the offer on port 8200 of every address in the namespace, which is the veth pair's inside end and the loopback, until the process stops
func main() {
	if len(os.Args) > 1 {//os.Args is the command line, the program's own path first
		nat = os.Args[1]
	}
	api = webrtcAPI()
	mux := http.NewServeMux()
	mux.HandleFunc("GET /punch", punch)
	mux.HandleFunc("POST /offer", offer)
	server := &http.Server{
		Addr:              ":8200",//every address in the namespace, which is none that the internet can reach
		Handler:           mux,
		ReadHeaderTimeout: 5 * time.Second,
		ReadTimeout:       10 * time.Second,
		WriteTimeout:      10 * time.Second,//the answer leaves once STUN has answered, within its three seconds; the punch goes on after it
		IdleTimeout:       60 * time.Second,
	}
	if err := server.ListenAndServe(); err != nil {
		fmt.Fprintf(os.Stderr, "fatal: %v\n", err)//to standard error, which systemd keeps in the journal
		os.Exit(1)//a failing exit code, so the unit's Restart=on-failure starts the peer again
	}
}

//punch answers GET /punch: it opens a socket, asks STUN its public address and port, answers them with the rest of the report, and then sends the punch on a goroutine of its own, which outlives the request
func punch(w http.ResponseWriter, r *http.Request) {
	to, err := netip.ParseAddrPort(r.URL.Query().Get("to"))//an address and a port as text, like 203.0.113.42:52814; ParseAddrPort takes only a literal address, never a name to look up
	if err != nil || !to.Addr().Is4() || to.Port() == 0 {//IPv4 alone, since the namespace translates IPv4 alone
		http.Error(w, "to must be an IPv4 address and a port", http.StatusBadRequest)
		return
	}
	token := r.URL.Query().Get("token")
	if !isToken(token) {
		http.Error(w, "token must be 1 to 32 letters and digits", http.StatusBadRequest)
		return
	}
	select {//a select with a default never waits: it puts a value in when there's room, and takes the default when there isn't
	case running <- struct{}{}:
	default:
		http.Error(w, "too many punches at once", http.StatusServiceUnavailable)
		return
	}
	socket, err := net.ListenUDP("udp4", &net.UDPAddr{})//no address of its own and port 0, so the system picks a free port, which the cone namespace's NAT then keeps
	if err != nil {
		<-running//take back the place this punch held, since it won't run
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	report := map[string]any{"to": to.String(), "token": token, "local_port": socket.LocalAddr().(*net.UDPAddr).Port, "packets": punchPackets, "gap_milliseconds": punchGap.Milliseconds()}
	start := time.Now()
	mapped, err := askStun(socket)
	report["stun_milliseconds"] = time.Since(start).Milliseconds()
	if err != nil {
		socket.Close()
		<-running
		report["error"] = "STUN: " + err.Error()
		writeJSON(w, report)
		return
	}
	report["mapped"] = mapped.String()//the public address and port the main server's STUN saw, which the caller sends to
	go func() {//the punch, on a goroutine of its own, so the answer below leaves now and the caller can start sending while the peer does
		defer func() { <-running }()//deferred calls run last first, so the socket closes, then the place is given back
		defer socket.Close()
		send(socket, net.UDPAddrFromAddrPort(to), token)
	}()
	writeJSON(w, report)
}

//send is the punch itself: punchPackets packets to the caller, punchGap apart, each "punch <token> <n>/<total> heard <yes or no>", and between them a read of whatever arrives, any packet carrying the token being the caller's. Only what the NAT lets in reaches the socket, which is packets from the address and port the peer has sent to
func send(socket *net.UDPConn, to *net.UDPAddr, token string) {
	heard := "no"
	mark := "punch " + token + " "//how a packet of this punch's starts, from either side
	buffer := make([]byte, 1500)
	for i := 1; i <= punchPackets; i++ {
		socket.WriteToUDP(fmt.Appendf(nil, "punch %s %d/%d heard %s", token, i, punchPackets, heard), to)//a send that fails is the same to the caller as one that's lost, so its error isn't read
		socket.SetReadDeadline(time.Now().Add(punchGap))//read until the moment of the next send
		for {
			n, _, err := socket.ReadFromUDP(buffer)
			if err != nil {
				break//the deadline, or the socket closing
			}
			if strings.HasPrefix(string(buffer[:n]), mark) {
				heard = "yes"
			}
		}
	}
}

//askStun sends a STUN binding request from socket to the main server, again after half a second and then a second, and answers the public address and port in the first answer to it, or an error when none comes within stunTimeout. The request is twenty bytes built by hand: the type, a binding request, 0x0001; the length of the attributes after the header, none; the magic cookie; and a random twelve-byte transaction id, which the answer repeats
func askStun(socket *net.UDPConn) (*net.UDPAddr, error) {
	server, err := net.ResolveUDPAddr("udp4", stunServer)//the name to an IPv4 address, through the resolvers the unit binds in
	if err != nil {
		return nil, err
	}
	request := make([]byte, 20)
	binary.BigEndian.PutUint16(request[0:2], 0x0001)
	binary.BigEndian.PutUint32(request[4:8], magicCookie)//bytes 2 and 3, the length, stay zero
	rand.Read(request[8:20])
	defer socket.SetReadDeadline(time.Time{})//the zero moment, which takes the deadline off again, so the punch's reads start clean
	deadline := time.Now().Add(stunTimeout)
	wait := 500 * time.Millisecond
	buffer := make([]byte, 1500)
	for time.Now().Before(deadline) {
		if _, err := socket.WriteToUDP(request, server); err != nil {
			return nil, err
		}
		next := time.Now().Add(wait)
		if next.After(deadline) {
			next = deadline
		}
		wait *= 2
		socket.SetReadDeadline(next)
		for {
			n, _, err := socket.ReadFromUDP(buffer)
			if errors.Is(err, os.ErrDeadlineExceeded) {
				break//the moment to send again, or the end
			}
			if err != nil {
				return nil, err
			}
			if n < 20 || binary.BigEndian.Uint16(buffer[0:2]) != 0x0101 || !bytes.Equal(buffer[8:20], request[8:20]) {
				continue//not a binding answer to this request, which only a stray packet would be
			}
			if mapped := xorMapped(buffer[:n]); mapped != nil {
				return mapped, nil
			}
			return nil, errors.New("an answer with no IPv4 XOR-MAPPED-ADDRESS")
		}
	}
	return nil, errors.New("no answer in 3 seconds")
}

//xorMapped reads the XOR-MAPPED-ADDRESS from a STUN answer, or answers nil when it holds no IPv4 one. Attributes follow the twenty-byte header one after another, each a type, a length, and a value padded out to four bytes. This one's value is a reserved byte, the family, 1 for IPv4, the port masked with the cookie's top two bytes, and the address masked with the whole cookie, a mask that keeps a NAT rewriting addresses it finds in a packet from rewriting this one
func xorMapped(answer []byte) *net.UDPAddr {
	cookie := answer[4:8]
	for at := 20; at+4 <= len(answer); {
		kind := binary.BigEndian.Uint16(answer[at : at+2])
		size := int(binary.BigEndian.Uint16(answer[at+2 : at+4]))
		value := answer[at+4 : min(at+4+size, len(answer))]
		if kind == 0x0020 && len(value) >= 8 && value[1] == 1 {
			port := binary.BigEndian.Uint16(value[2:4]) ^ binary.BigEndian.Uint16(cookie[0:2])
			ip := make(net.IP, 4)
			for i := range 4 {//range over a number counts from 0 up to it, since Go 1.22
				ip[i] = value[4+i] ^ cookie[i]
			}
			return &net.UDPAddr{IP: ip, Port: int(port)}
		}
		at += 4 + (size+3)/4*4//past the value and its padding
	}
	return nil
}

//isToken says whether a token is 1 to 32 letters and digits, which is all ftorrent-moon passes on
func isToken(s string) bool {
	if len(s) == 0 || len(s) > 32 {
		return false
	}
	for _, c := range s {
		if !(c >= '0' && c <= '9' || c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {
			return false
		}
	}
	return true
}

//writeJSON answers v as JSON
func writeJSON(w http.ResponseWriter, v any) {
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(v)
}
