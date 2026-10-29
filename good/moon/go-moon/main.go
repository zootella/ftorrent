/*
The Go service on moon, the second vantage of good.ftorrent.com, answering behind Caddy at moon.ftorrent.com, ip4.moon.ftorrent.com, and ip6.moon.ftorrent.com: the reflector, the UDP poke, the TCP dial, the ping, and the punch and the offer, which it hands to a peer behind one of moon's two NATs, each an endpoint on this one small server. The root of moon.ftorrent.com is the one path that never reaches it: Caddy answers there itself, with a page linking to moon's guide.

	GET /                                   -> the caller's IP, as text, at ip4.moon and ip6.moon
	GET /?format=json                       -> {"ip":"..."}
	GET /poke?port=N[&address=A][&token=T]  -> three small UDP packets carrying T to address A, or the caller's, at port N, and a JSON account of them
	GET /dial?port=N[&infohash=H]           -> a TCP connection to the caller's address at port N with a BitTorrent handshake, and a JSON account of what came back
	GET /ping?port=N&kind=K                 -> one UDP packet a BitTorrent client answers, a DHT ping for kind=dht or a uTP connection request for kind=utp, to the caller's address at port N, and a JSON account of the answer
	GET /punch?port=N&nat=K[&address=A]     -> a hole punch to address A, or the caller's, at port N from the peer in the namespace named, cone or symmetric over IPv4 and firewall over IPv6, and the peer's public address and port as JSON
	POST /offer?nat=K                       -> a WebRTC offer in, the peer in the namespace named answering it, and its answer out as JSON, for a page on any origin
	GET /health                             -> ok

Where each call sends follows what it could do aimed at someone else. The dial and the ping send only to the address a request came from, never to one it names: the caller names a port and nothing more, and the HTTPS request that asks is a TCP connection, whose source can't be forged the way a UDP packet's can. The poke and the punch send a few small packets, and they go to the address the request names, where the caller's STUN saw its socket, since a machine's UDP can leave from another address than its HTTPS, on a carrier that sends the two out apart or over IPv6 from another of its temporary addresses; a named address has to be public, and with none named they go to the address the request came from. The offer's peer runs ICE with the browser's public candidates in the peer's family wherever they are, and ICE's checks are STUN requests signed with the browser's own ICE password, which nothing but that browser's WebRTC answers, and the peer gives up after eight seconds. Each is rate-limited per source address, and the reflector, which sends nothing anywhere, is not.

The reflector is the same contract as ip.ftorrent.com on the main server, and DNS does the same work here: ip4.moon.ftorrent.com carries only an A record and ip6.moon.ftorrent.com only a AAAA, so a request to one of them travels over that family alone, and moon.ftorrent.com carries both. The same names choose the family of the poke, the dial, and the ping, since moon sends to the address the request arrived from. The service runs as a systemd unit on the box itself, listening on the loopback address alone; Caddy terminates TLS for the three names and proxies to it. Nothing here logs a request or an address.

A few things about Go run through the whole file, and a reader coming from JavaScript meets them on every screen. A function hands back its error as a second value rather than throwing one, so a call reads value, err := f(), and the line after it checks err != nil. := declares a variable and assigns it at once, and _ throws away a value a call returns. Go's net/http runs each request's handler on a goroutine of its own, a lightweight thread, so two requests can be inside the same handler at the same moment, and anything they share, like the rate limiter's map, needs a lock. defer schedules a call for when the surrounding function returns, by whichever return, which is how a socket or a lock gets let go. Text is a string and bytes are a []byte, a slice, Go's growable array, and append answers the slice it grew.

And time, which reads differently than it does in JavaScript: Go's time.Duration counts nanoseconds, not milliseconds. time.Second is 1,000,000,000 and time.Millisecond is 1,000,000, where setTimeout and Date.now() count in milliseconds, so 3 * time.Second is a Duration of three seconds, and that's the type every timeout and sleep in Go's standard library takes. A deadline is the other type, a time.Time, which is a moment rather than a span: time.Now().Add(d) is the moment d from now, and subtracting two moments answers a Duration.

The program uses Go's standard library alone, so go.mod lists no dependencies, and deploy.py builds it on moon with Debian's Go.
*/
package main

import (
	"bytes"//finding a byte in bencode
	"crypto/rand"//random bytes from the operating system, for a token, an info-hash, a node id, or a transaction id
	"encoding/binary"//numbers written into a uTP header, most significant byte first
	"encoding/hex"//bytes to and from hex text
	"encoding/json"//values to and from JSON, for the reports, and to read a peer's
	"errors"//errors.Is, to tell which error a read ended with
	"fmt"//formatted text, written to the answer or into bytes
	"io"//io.EOF, the error a read gives when the other side has closed, and reading a peer's answer whole
	"net"//sockets: UDP for the poke, TCP for the dial, and parsing addresses
	"net/http"//the web server, its router, and the request and answer types, and the client that asks a peer
	"net/netip"//an address as a value, to check the punch's caller is IPv4
	"net/url"//writing the query a peer is asked with
	"os"//the environment and the exit code
	"strconv"//numbers to and from text
	"strings"//splitting and trimming text
	"sync"//the lock the rate limiter takes
	"syscall"//ECONNREFUSED, the error a connected UDP socket reads when the other end refused the port
	"time"//moments, and Durations in nanoseconds
)

const (
	pokePackets = 3//how many packets a poke sends, since UDP keeps no promises and one lost packet shouldn't read as a closed door
	pokeGap     = 50 * time.Millisecond//the pause between them, so they don't leave in one burst; a Duration, which counts nanoseconds, so this is 50,000,000 of them
	dialTimeout = 3 * time.Second//to connect, and then to hear back; a peer that answers at all answers within this
	pingTimeout = 3 * time.Second//to hear an answer to a ping, across every send of it
	limitBurst  = 200//calls that send one source address may make at once: a carrier's translator puts many phones and computers behind one address, the panel's Check makes up to seven of these calls in a family, and the page two
	limitRate   = 2.0//and may make steadily, in tokens a second: two a second, a hundred and twenty a minute; written 2.0 so Go keeps it a float, as the bucket's arithmetic is
	handshake   = "\x13BitTorrent protocol"//the first twenty bytes of every BitTorrent connection: a length byte, hex 13 for 19, then the protocol's 19-character name
	peerPrefix  = "-FF0000-"//the eight characters at the front of the peer id moon dials with: FF is ftorrent's client code, and 0000 a version no client has, since this is a prober
	tooMany     = "too many calls that send from this address; two hundred at once, then two a second"//the answer to a source whose bucket is empty, for any of the calls that send
)

//peers holds where each peer listens, behind moon's two NATs and its IPv6 firewall: the inside end of its namespace's veth pair, which namespaces.py makes, at the port the peer serves on, ftorrent-peer in go-peer/
var peers = map[string]string{"cone": "10.88.1.2:8200", "symmetric": "10.88.2.2:8200", "firewall": "10.88.3.2:8200"}

//peerClient asks a peer, giving it five seconds, past its three to hear from STUN
var peerClient = &http.Client{Timeout: 5 * time.Second}

//clientIP answers the address the request came from, as text without a port. Caddy, the only thing that reaches this server, sets X-Forwarded-For to the address it accepted the connection from and ignores any X-Forwarded-For the client sent, so the last entry is the client and an earlier one could only be Caddy's own. With no header at all the request came straight to the socket, which only happens in development, and the socket's peer is the answer
func clientIP(r *http.Request) string {//r points at the request net/http parsed; the * is a pointer, so the function reads the one request rather than a copy
	if xff := r.Header.Get("X-Forwarded-For"); xff != "" {//an if with a statement first: get the header, then test it, with xff alive only inside the if; Get answers "" for a header that isn't there
		parts := strings.Split(xff, ",")//the list a chain of proxies writes, like "198.51.100.7, 203.0.113.9"
		return strings.TrimSpace(parts[len(parts)-1])//the last entry, the one Caddy wrote
	}
	host, _, err := net.SplitHostPort(r.RemoteAddr)//RemoteAddr is the socket's peer, like "203.0.113.9:56789" or "[2001:db8::9]:56789"; this splits off the port for both families, and _ drops it
	if err != nil {
		return r.RemoteAddr//not an address and a port, which net/http never hands a real connection; answer it as it is
	}
	return host
}

//limiter holds a token bucket for each source address: each poke, dial, ping, punch, or offer spends a token, the bucket refills at limitRate up to limitBurst, and a source with an empty bucket is answered 429 and sent nothing. The map is pruned of full buckets when it grows large, so a day of distinct sources doesn't hold memory
type limiter struct {
	mu      sync.Mutex//the lock allow takes before touching buckets, since two requests can be in allow at once on two goroutines
	buckets map[string]*bucket//each source's bucket, by its address as text; a map is Go's dictionary, and *bucket a pointer, so changing a bucket changes the one in the map
}

//bucket is one source's tokens and when they were last brought up to date
type bucket struct {
	tokens float64//how many calls the source may make right now, a fraction while it refills
	last   time.Time//the moment tokens was last counted
}

//allow spends one of source's tokens and answers true, or answers false when its bucket has none to spend; first it adds back the tokens the time since the last count has earned
func (l *limiter) allow(source string) bool {//a method: the (l *limiter) before the name makes it callable as limits.allow(ip), with l the limiter it's called on
	l.mu.Lock()
	defer l.mu.Unlock()//let go of the lock when allow returns, by whichever return
	now := time.Now()
	b, ok := l.buckets[source]//the comma-ok form: b is the bucket, and ok says whether the map held one, since a key that isn't there answers nil rather than failing
	if !ok {
		if len(l.buckets) >= 4096 {//room to make: drop every bucket that has refilled, which is every source not asking right now
			for key, old := range l.buckets {//range walks a map's keys and values, in no particular order
				if old.tokens+now.Sub(old.last).Seconds()*limitRate >= limitBurst {//its tokens as they'd stand now: now.Sub(old.last) is the Duration since the last count, and .Seconds() turns it into a float of seconds
					delete(l.buckets, key)//Go allows deleting from a map while ranging over it
				}
			}
		}
		b = &bucket{tokens: limitBurst, last: now}//a new source starts full; & makes a pointer to the new bucket
		l.buckets[source] = b
	}
	b.tokens = min(limitBurst, b.tokens+now.Sub(b.last).Seconds()*limitRate)//refill for the seconds since the last count, never past the burst; min is built into Go
	b.last = now
	if b.tokens < 1 {
		return false
	}
	b.tokens--
	return true
}

//sendTo answers where the poke or the punch sends: the address the request names, which is where the caller's STUN saw its socket and can differ from where its HTTPS came from, as on a carrier that sends a machine's TCP and UDP out from different addresses, or over IPv6 from another of its temporary addresses; or, with none named, the address the request came from. A named address has to be public, so nothing can aim moon at a private network, its own namespaces, or its loopback
func sendTo(r *http.Request) (netip.Addr, error) {
	named := r.URL.Query().Get("address")
	if named == "" {
		address, err := netip.ParseAddr(clientIP(r))
		return address.Unmap(), err//Unmap turns an IPv4 address written in IPv6's form, like ::ffff:203.0.113.42, back into plain IPv4
	}
	address, err := netip.ParseAddr(named)//a literal address only, never a name to look up
	if err != nil {
		return netip.Addr{}, errors.New("address must be an IP address")
	}
	address = address.Unmap()
	if !address.IsGlobalUnicast() || address.IsPrivate() || sharedSpace.Contains(address) {//IsPrivate covers 10/8, 172.16/12, 192.168/16, and IPv6's fc00::/7
		return netip.Addr{}, errors.New("address must be a public address")
	}
	return address, nil
}

//sharedSpace is the range carriers number their customers from behind their translators, RFC 6598, which no packet from moon can reach and which netip doesn't count as private
var sharedSpace = netip.MustParsePrefix("100.64.0.0/10")

//portParam reads the port the caller named, 1 through 65535, or answers 400 itself and returns false, so the handler returns at once
func portParam(w http.ResponseWriter, r *http.Request) (int, bool) {//two return values, the port and whether it was good; w is where the answer to the caller is written
	port, err := strconv.Atoi(r.URL.Query().Get("port"))//Atoi, ASCII to integer, reads text as a whole number and fails on anything else, a blank included
	if err != nil || port < 1 || port > 65535 {
		http.Error(w, "port must be a number from 1 to 65535", http.StatusBadRequest)//writes the status and this text as the body
		return 0, false
	}
	return port, true
}

//randomBytes answers n random bytes, for a node id, a transaction id, or a connection id
func randomBytes(n int) []byte {
	b := make([]byte, n)//make builds a slice of n zero bytes
	rand.Read(b)//fills it from the operating system's random source; since Go 1.24 this never fails, so there's no error to read
	return b
}

//randomHex answers n random bytes as lowercase hex, 2n characters, for a token or an info-hash the caller didn't supply
func randomHex(n int) string {
	return hex.EncodeToString(randomBytes(n))
}

//writeJSON answers v as JSON, which is every poke's and dial's whole report
func writeJSON(w http.ResponseWriter, v any) {//any takes a value of any type, here a map
	w.Header().Set("Content-Type", "application/json")//headers first, since the first write sends them
	json.NewEncoder(w).Encode(v)//writes v as JSON straight into the answer, with a newline after; a map's keys come out sorted
}

//isHex says whether s is exactly n hex digits, in either case
func isHex(s string, n int) bool {
	if len(s) != n {//len of a string counts bytes, which for hex digits is the number of characters
		return false
	}
	_, err := hex.DecodeString(s)//decoding fails on anything that isn't pairs of hex digits; the bytes themselves aren't needed
	return err == nil
}

//isToken says whether a caller's token is 1 to 32 letters and digits, which is all a token needs to be, and keeps it out of any other interpretation
func isToken(s string) bool {
	if len(s) == 0 || len(s) > 32 {
		return false
	}
	for _, c := range s {//range over a string steps through its characters, c each one as a rune, a Unicode code point, with _ dropping its byte position
		if !(c >= '0' && c <= '9' || c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {//&& binds tighter than ||, so this is three ranges; '0' in single quotes is a character, not a string
			return false
		}
	}
	return true
}

//bdecode reads one value from the front of b in bencode, the encoding BitTorrent writes DHT messages and torrent files in: an integer, i42e; a string of bytes, its length, a colon, and the bytes, 4:spam; a list, l, its values, then e; or a dictionary, d, keys and values alternating, then e, every key a string. It answers the value, as an int64, a string, a []any, or a map[string]any, and how many bytes it took, or an error for bytes that aren't bencode; a ping's answer is all it reads, so it holds the whole message in memory and recurses into what it holds
func bdecode(b []byte) (any, int, error) {
	if len(b) == 0 {
		return nil, 0, errors.New("bencode ended early")
	}
	switch {
	case b[0] == 'i'://an integer, the digits between i and e
		end := bytes.IndexByte(b, 'e')//the position of the first e, or -1 for none
		if end < 0 {
			return nil, 0, errors.New("an integer with no end")
		}
		n, err := strconv.ParseInt(string(b[1:end]), 10, 64)//base 10, into 64 bits
		return n, end + 1, err
	case b[0] >= '0' && b[0] <= '9'://a string, its length written out first
		colon := bytes.IndexByte(b, ':')
		if colon < 0 {
			return nil, 0, errors.New("a string with no colon")
		}
		size, err := strconv.Atoi(string(b[:colon]))
		if err != nil || size > len(b)-colon-1 {//compared to what's left rather than added to the colon's position, so a length near the largest int can't wrap the sum negative and slip past
			return nil, 0, errors.New("a string that runs past the end")
		}
		return string(b[colon+1 : colon+1+size]), colon + 1 + size, nil//a Go string holds any bytes, not only text, so a node id fits in one
	case b[0] == 'l' || b[0] == 'd'://a list or a dictionary: values one after another until an e
		var values []any//a nil slice, which append grows
		at := 1//past the l or the d
		for at < len(b) && b[at] != 'e' {
			v, n, err := bdecode(b[at:])
			if err != nil {
				return nil, 0, err
			}
			values = append(values, v)
			at += n
		}
		if at >= len(b) {
			return nil, 0, errors.New("a list with no end")
		}
		if b[0] == 'l' {
			return values, at + 1, nil
		}
		if len(values)%2 != 0 {
			return nil, 0, errors.New("a dictionary with a key and no value")
		}
		dictionary := map[string]any{}
		for i := 0; i < len(values); i += 2 {
			key, ok := values[i].(string)//comma-ok on a type assertion: ok says whether the value really is a string
			if !ok {
				return nil, 0, errors.New("a dictionary key that isn't a string")
			}
			dictionary[key] = values[i+1]
		}
		return dictionary, at + 1, nil
	}
	return nil, 0, errors.New("not bencode")
}

//compactAddress reads an address and port the way the DHT packs them, 4 bytes of IPv4 or 16 of IPv6 and then 2 of port, most significant first, into text like 203.0.113.9:56789, or answers "" for any other length
func compactAddress(b string) string {
	if len(b) != 6 && len(b) != 18 {
		return ""
	}
	address := net.IP(b[:len(b)-2])//an IP is a byte slice, 4 bytes or 16
	port := binary.BigEndian.Uint16([]byte(b[len(b)-2:]))
	return net.JoinHostPort(address.String(), strconv.Itoa(int(port)))
}

//utpTypes names uTP's five packet types by the number in the high four bits of a packet's first byte, BEP 29
var utpTypes = [...]string{"DATA", "FIN", "STATE", "RESET", "SYN"}//[...] makes an array exactly as long as its list

//main builds the router, adds a handler for each endpoint, and serves until the process stops
func main() {
	port := os.Getenv("PORT")//Caddy proxies the three names to 127.0.0.1:8100, and PORT exists only for running the binary by hand during development; Getenv answers "" for a variable that isn't set
	if port == "" {
		port = "8100"
	}
	limits := &limiter{buckets: map[string]*bucket{}}//one limiter for the whole process, shared by every call that sends; the map has to be made, since writing to a nil map crashes
	mux := http.NewServeMux()//the router: it matches each request's method and path to one of the handlers below

	mux.HandleFunc("GET /health", func(w http.ResponseWriter, r *http.Request) {//a pattern of a method and a path, which Go's router reads since 1.22, then the handler, a function written in place
		w.Header().Set("Content-Type", "text/plain; charset=utf-8")
		fmt.Fprintln(w, "ok")//Fprintln writes to w, the answer, with a newline after
	})

	//the reflector, at the root path alone: {$} anchors the pattern at the end, so "GET /{$}" is / and nothing under it, and any path no pattern claims is a plain 404. CORS is open because the reflection is the caller's own public data: a page on any origin may read its own address here, as it may at ip.ftorrent.com
	mux.HandleFunc("GET /{$}", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		w.Header().Set("Access-Control-Allow-Origin", "*")
		if r.URL.Query().Get("format") == "json" {
			w.Header().Set("Content-Type", "application/json")
			fmt.Fprintf(w, "{\"ip\":%q}\n", ip)//%q writes ip quoted and escaped, which is all the JSON a one-field object needs
		} else {
			w.Header().Set("Content-Type", "text/plain; charset=utf-8")
			fmt.Fprintf(w, "%s\n", ip)
		}
	})

	//the poke: a UDP socket on a port the system picks, in the family of the address it sends to, bound to no address, so the kernel sends from the address it would use toward the caller, moon's own; then pokePackets packets to the address sendTo answers, at the port the caller named, each "poke <token> <n>/3", 17 bytes with an eight-character token and 41 with the longest. The caller, listening on that port, reports whether they arrived; the answer here says what left and from which port, which is the port a filtering test needs
	mux.HandleFunc("GET /poke", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		if !limits.allow(ip) {//the token is spent before the request is even read, so a flood of bad requests costs its sender too
			http.Error(w, tooMany, http.StatusTooManyRequests)
			return
		}
		port, ok := portParam(w, r)
		if !ok {
			return//portParam has answered 400 already
		}
		token := r.URL.Query().Get("token")
		if token == "" {
			token = randomHex(8)//no token given: make one, 16 hex characters
		} else if !isToken(token) {
			http.Error(w, "token must be up to 32 letters and digits", http.StatusBadRequest)
			return
		}
		address, err := sendTo(r)
		if err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		to := net.UDPAddrFromAddrPort(netip.AddrPortFrom(address, uint16(port)))//where the packets go: the address the request named or came from, at the port it named
		network := "udp4"
		if to.IP.To4() == nil {//To4 answers the address as four bytes, or nil when it's IPv6, which is how Go tells the two families apart
			network = "udp6"
		}
		report := map[string]any{"to": to.String(), "token": token, "packets": pokePackets, "sent": 0}//the JSON answer, filled in as the poke goes; String writes the address and port, with brackets around an IPv6 address
		socket, err := net.ListenUDP(network, &net.UDPAddr{})//an empty UDPAddr is no address of its own and port 0, so the system picks a free port
		if err != nil {
			report["error"] = err.Error()//Error answers an error's text
			writeJSON(w, report)
			return
		}
		defer socket.Close()//closed when the handler returns
		report["from_port"] = socket.LocalAddr().(*net.UDPAddr).Port//LocalAddr answers a general net.Addr, and .(*net.UDPAddr) asserts it's the UDP kind, to reach its Port
		for i := 1; i <= pokePackets; i++ {
			if i > 1 {
				time.Sleep(pokeGap)//pauses this handler's goroutine alone; other requests carry on
			}
			payload := fmt.Appendf(nil, "poke %s %d/%d", token, i, pokePackets)//formats the text straight into a new byte slice, the form a socket writes
			if _, err := socket.WriteToUDP(payload, to); err != nil {//WriteToUDP answers the bytes written and an error, and only the error matters here
				report["error"] = err.Error()
				break
			}
			report["sent"] = i
			report["bytes"] = len(payload)//each packet's size, the same for all three
		}
		writeJSON(w, report)
	})

	//the dial: a TCP connection to the caller's address at the port they named, then a BitTorrent handshake, 68 bytes, with the info-hash the caller gave or a random one, and whatever comes back within the timeout. A BitTorrent client that holds the torrent answers with its own handshake, which names it; one that doesn't closes the connection without a word, which still proves the connection opened; and a port nobody listens on refuses or stays silent, which the error says
	mux.HandleFunc("GET /dial", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		if !limits.allow(ip) {//the poke's bucket too, shared between them
			http.Error(w, tooMany, http.StatusTooManyRequests)
			return
		}
		port, ok := portParam(w, r)
		if !ok {
			return
		}
		infohash := r.URL.Query().Get("infohash")
		if infohash == "" {
			infohash = randomHex(20)//no info-hash given: a random one, 20 bytes as 40 hex characters, which names no torrent
		} else if !isHex(infohash, 40) {
			http.Error(w, "infohash must be 40 hex digits", http.StatusBadRequest)
			return
		}
		to := net.JoinHostPort(ip, strconv.Itoa(port))//the address and port as one string, with brackets around an IPv6 address; Itoa is Atoi's reverse
		report := map[string]any{"to": to, "infohash": infohash, "connected": false}
		start := time.Now()
		conn, err := net.DialTimeout("tcp", to, dialTimeout)//the family follows the address; a refusal comes back at once, and silence after dialTimeout
		report["milliseconds"] = time.Since(start).Milliseconds()//how long connecting took, the Duration's nanoseconds converted to a whole number of milliseconds
		if err != nil {
			report["error"] = err.Error()
			writeJSON(w, report)
			return
		}
		defer conn.Close()
		report["connected"] = true
		report["from_port"] = conn.LocalAddr().(*net.TCPAddr).Port//the port moon dialed from, reached through the TCP kind as the poke reaches its UDP one
		conn.SetDeadline(time.Now().Add(dialTimeout))//one deadline for the write and every read after it, a moment three seconds out; past it each fails with a timeout
		hash, _ := hex.DecodeString(infohash)//20 bytes; isHex or randomHex has made sure it decodes, so the error is dropped
		id := []byte(peerPrefix + randomHex(6))//8 characters and 12 of hex: a 20-byte peer id in the usual shape
		message := append(append(append([]byte(handshake), make([]byte, 8)...), hash...), id...)//the name, eight reserved bytes of zeros, the info-hash, and the peer id, 20 + 8 + 20 + 20 = 68 bytes; append(a, b...) adds b's bytes onto a
		if _, err := conn.Write(message); err != nil {
			report["error"] = err.Error()
			writeJSON(w, report)
			return
		}
		report["sent"] = len(message)

		//read until a whole handshake is in hand, the peer closes, or the deadline passes; a partial answer is reported as it is
		answer := make([]byte, 0, len(message))//empty, with room for 68 bytes, which append fills
		buffer := make([]byte, 256)//where each read lands first, up to 256 bytes at a time
		for len(answer) < len(message) {//for is Go's only loop, and with just a condition it's a while
			n, err := conn.Read(buffer)//n bytes arrived, perhaps fewer than were sent, since TCP is a stream with no message boundaries
			answer = append(answer, buffer[:n]...)//keep what arrived even when an error came with it; buffer[:n] is its first n bytes
			if err != nil {
				if errors.Is(err, io.EOF) {
					report["closed"] = true//the peer hung up, which a client without this torrent does
				} else {
					report["error"] = err.Error()//most often the deadline, which reads as an i/o timeout
				}
				break
			}
		}
		report["received"] = len(answer)
		if len(answer) >= len(message) && string(answer[:len(handshake)]) == handshake {//a handshake back starts with the same twenty bytes ours did
			report["handshake"] = map[string]any{"infohash": hex.EncodeToString(answer[28:48]), "peer_id": fmt.Sprintf("%q", answer[48:68])}//its info-hash is bytes 28 through 47 and its peer id 48 through 67, since a slice's end is one past its last byte; %q quotes the peer id and escapes any byte that isn't printable
		} else if len(answer) > 0 {
			report["received_hex"] = hex.EncodeToString(answer)//anything else that came back, whole
		}
		writeJSON(w, report)
	})

	//the ping: one UDP packet a BitTorrent client answers, to the caller's address at the port they named, and the answer. kind=dht sends a DHT ping, a query any node on the DHT answers with a pong carrying its node id and, by BEP 42, the address and port it saw the ping come from. kind=utp sends a uTP connection request, a SYN, uTP being BitTorrent's own transport over UDP, which a client answers with a STATE to accept it or a FIN or RESET to turn it away, and moon resets one it accepted so nothing is left half open. Any answer proves a packet from a stranger reached the port and the reply found its way back out. The socket is connected to the caller, so only their answer is read, and an ICMP port unreachable comes back as a refusal rather than silence. The packet goes again after half a second and then a second, as STUN's does, until an answer comes or pingTimeout passes
	mux.HandleFunc("GET /ping", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		if !limits.allow(ip) {//the poke's and the dial's bucket too
			http.Error(w, tooMany, http.StatusTooManyRequests)
			return
		}
		port, ok := portParam(w, r)
		if !ok {
			return
		}
		kind := r.URL.Query().Get("kind")
		if kind != "dht" && kind != "utp" {
			http.Error(w, "kind must be dht or utp", http.StatusBadRequest)
			return
		}
		to := &net.UDPAddr{IP: net.ParseIP(ip), Port: port}
		if to.IP == nil {
			http.Error(w, "the request's address isn't an IP address", http.StatusBadRequest)
			return
		}
		network := "udp4"
		if to.IP.To4() == nil {
			network = "udp6"
		}
		report := map[string]any{"to": to.String(), "kind": kind, "sent": 0, "answered": false}
		socket, err := net.DialUDP(network, nil, to)//a UDP socket connected to the caller: nil lets the system pick moon's address and a free port, sends go to the caller alone, and only packets from their address and port are read
		if err != nil {
			report["error"] = err.Error()
			writeJSON(w, report)
			return
		}
		defer socket.Close()
		report["from_port"] = socket.LocalAddr().(*net.UDPAddr).Port

		//the one packet, and what an answer has to carry back to count as one: the DHT's transaction id, or uTP's connection id
		var request, mark []byte
		if kind == "dht" {
			mark = randomBytes(2)
			request = fmt.Appendf(nil, "d1:ad2:id20:%se1:q4:ping1:t2:%s1:y1:qe", randomBytes(20), mark)//a ping in bencode: arguments with a node id of 20 random bytes, the query's name, the transaction id the pong repeats, and y for a query; %s writes a byte slice's bytes as they are
		} else {
			mark = randomBytes(2)
			request = make([]byte, 20)//a uTP header and nothing after it
			request[0] = 4<<4 | 1//the type in the high four bits, 4 for SYN, and the version in the low four, 1
			copy(request[2:4], mark)//the connection id, which the answer carries back
			binary.BigEndian.PutUint32(request[4:8], uint32(time.Now().UnixMicro()))//the send time in microseconds, which uTP measures delay by; bytes 8 to 11, the delay seen so far, stay zero
			binary.BigEndian.PutUint32(request[12:16], 1<<20)//the window, how many bytes moon could take in flight, a megabyte
			binary.BigEndian.PutUint16(request[16:18], 1)//this packet's sequence number, the first; bytes 18 and 19, the last one acknowledged, stay zero
		}

		//send, and read until a matching answer, the next send, or the end; then send again
		start := time.Now()
		deadline := start.Add(pingTimeout)
		wait := 500 * time.Millisecond
		buffer := make([]byte, 1500)//more than any answer to these, which fits in one packet
		var answer []byte//nil until an answer arrives
		sent := 0
	asking://a label, which lets the break below leave both loops at once
		for answer == nil && time.Now().Before(deadline) {
			if _, err := socket.Write(request); err != nil {
				report["error"] = err.Error()
				break
			}
			sent++
			next := time.Now().Add(wait)
			if next.After(deadline) {
				next = deadline
			}
			wait *= 2
			socket.SetReadDeadline(next)//a read past this moment fails with a timeout
			for {
				n, err := socket.Read(buffer)
				if errors.Is(err, os.ErrDeadlineExceeded) {
					break//the moment for the next send, or the end
				}
				if errors.Is(err, syscall.ECONNREFUSED) {
					report["refused"] = true//the caller's machine answered that nothing listens on the port, which a NAT or firewall that drops would never say
					break asking
				}
				if err != nil {
					report["error"] = err.Error()
					break asking
				}
				got := buffer[:n]
				if kind == "dht" {
					message, _, err := bdecode(got)
					if dictionary, ok := message.(map[string]any); err == nil && ok && dictionary["t"] == string(mark) {//this ping's transaction id; comparing a value of type any to a string is true only when it holds that same string
						answer = append([]byte(nil), got...)//a copy, since the buffer is read into again
					}
				} else if n >= 20 && bytes.Equal(got[2:4], mark) {//this request's connection id
					answer = append([]byte(nil), got...)
				}
				if answer != nil {
					break
				}
			}
		}
		report["sent"] = sent
		report["milliseconds"] = time.Since(start).Milliseconds()
		if answer == nil {
			writeJSON(w, report)
			return
		}
		report["answered"] = true
		report["bytes"] = len(answer)
		report["received_hex"] = hex.EncodeToString(answer)
		if kind == "dht" {
			message, _, _ := bdecode(answer)
			dictionary := message.(map[string]any)//the match above made sure it's a dictionary
			pong := map[string]any{"y": dictionary["y"]}//r for an answer, e for an error, either one an answer
			if reply, ok := dictionary["r"].(map[string]any); ok {
				if id, ok := reply["id"].(string); ok {
					pong["id"] = hex.EncodeToString([]byte(id))//the answering node's id
				}
			}
			if seen, ok := dictionary["ip"].(string); ok {
				pong["saw"] = compactAddress(seen)//where the node saw the ping come from, moon's address and port as it arrived
			}
			report["dht"] = pong
		} else {
			kindOf := answer[0] >> 4//the type, from the high four bits
			name := "unknown"
			if int(kindOf) < len(utpTypes) {
				name = utpTypes[kindOf]
			}
			report["utp"] = name
			if name == "STATE" {//accepted: reset the connection, so the client isn't left waiting for data that never comes
				reset := make([]byte, 20)
				reset[0] = 3<<4 | 1//RESET, version 1
				binary.BigEndian.PutUint16(reset[2:4], binary.BigEndian.Uint16(mark)+1)//after the SYN, the side that asked sends with the connection id one higher, by BEP 29
				binary.BigEndian.PutUint16(reset[16:18], 2)
				copy(reset[18:20], answer[16:18])//acknowledging the answer's sequence number
				socket.Write(reset)
			}
		}
		writeJSON(w, report)
	})

	//the punch: the address sendTo answers and the port the caller names, handed to the peer in the namespace they name, across that namespace's veth pair, and the peer's answer passed back with the namespace's name. The peer opens a socket, learns its public address and port from the main server's STUN, answers them, and then sends to the caller for five seconds while the caller sends to it, so the caller reads from its own socket which way got through. Over IPv4 through cone and symmetric, and over IPv6 through firewall
	mux.HandleFunc("GET /punch", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		if !limits.allow(ip) {//the same bucket as the poke, the dial, and the ping, and the offer below
			http.Error(w, tooMany, http.StatusTooManyRequests)
			return
		}
		port, ok := portParam(w, r)
		if !ok {
			return
		}
		nat := r.URL.Query().Get("nat")
		peer, known := peers[nat]//comma-ok: known says whether the name is one of the two
		if !known {
			http.Error(w, "nat must be cone, symmetric, or firewall", http.StatusBadRequest)
			return
		}
		token := r.URL.Query().Get("token")
		if token == "" {
			token = randomHex(8)
		} else if !isToken(token) {
			http.Error(w, "token must be up to 32 letters and digits", http.StatusBadRequest)
			return
		}
		address, err := sendTo(r)
		if err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		if address.Is4() != (nat != "firewall") {//cone and symmetric translate IPv4, and firewall's peer has a public IPv6 address
			http.Error(w, "the punch through cone or symmetric is IPv4, and through firewall IPv6; ask at ip4.moon.ftorrent.com or ip6.moon.ftorrent.com, or name an address in that family", http.StatusBadRequest)
			return
		}
		to := netip.AddrPortFrom(address, uint16(port))
		report := map[string]any{"nat": nat, "to": to.String(), "token": token}
		answer, err := peerClient.Get("http://" + peer + "/punch?" + url.Values{"to": {to.String()}, "token": {token}}.Encode())//Encode writes the query with each value escaped
		if err != nil {
			report["error"] = err.Error()
			writeJSON(w, report)
			return
		}
		defer answer.Body.Close()
		body, _ := io.ReadAll(io.LimitReader(answer.Body, 4096))//the whole answer, a few hundred bytes, read no further than 4 KB
		if answer.StatusCode != http.StatusOK || json.Unmarshal(body, &report) != nil {//Unmarshal into a map that already holds values adds the peer's fields to them
			report["error"] = fmt.Sprintf("the peer answered %s: %s", answer.Status, strings.TrimSpace(string(body)))
		}
		writeJSON(w, report)
	})

	//the offer: a page's WebRTC offer, its SDP description as the request's body, handed with the caller's address to the peer in the namespace named, and the peer's answer passed back as it came. The peer keeps the offer's public candidates in its own family, the browser's addresses as STUN saw them, which on a phone's carrier can differ from the address the HTTPS came from. CORS is open so a page on any origin, good.ftorrent.com's among them, can read the answer, and the page posts its offer as text/plain, which a browser sends without first asking leave in a preflight request. The caller's address can be in either family, since it goes to the peer only for its report
	mux.HandleFunc("POST /offer", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")//set first, so every answer below carries it, a refusal included, and the page can read why
		ip := clientIP(r)
		if !limits.allow(ip) {
			http.Error(w, tooMany, http.StatusTooManyRequests)
			return
		}
		nat := r.URL.Query().Get("nat")
		peer, known := peers[nat]
		if !known {
			http.Error(w, "nat must be cone, symmetric, or firewall", http.StatusBadRequest)
			return
		}
		address, err := netip.ParseAddr(ip)
		if err != nil {
			http.Error(w, "the request's address isn't an IP address", http.StatusBadRequest)
			return
		}
		answer, err := peerClient.Post("http://"+peer+"/offer?"+url.Values{"caller": {address.Unmap().String()}}.Encode(), "application/sdp", io.LimitReader(r.Body, 16*1024+1))//the offer passed on as it streams in, cut one byte past the peer's limit, so the peer sees an offer too long as one
		if err != nil {
			http.Error(w, "the peer: "+err.Error(), http.StatusBadGateway)
			return
		}
		defer answer.Body.Close()
		w.Header().Set("Content-Type", answer.Header.Get("Content-Type"))
		w.WriteHeader(answer.StatusCode)//the peer's status, an answer or a refusal, as the peer gave it
		io.Copy(w, io.LimitReader(answer.Body, 64*1024))//and its body, a few kilobytes, read no further than 64
	})

	//each request gets a few seconds and each idle connection a minute, so a stalled client holds nothing for long. Caddy in front has limits of its own, and these are the service's
	server := &http.Server{
		Addr:              "127.0.0.1:" + port,//the loopback address whatever the port, so nothing reaches this server but Caddy on the same box
		Handler:           mux,
		ReadHeaderTimeout: 5 * time.Second,//to read a request's headers
		ReadTimeout:       10 * time.Second,//to read the whole request
		WriteTimeout:      10 * time.Second,//from the end of the request's headers to the end of the answer, which a dial's three seconds to connect and three to hear back fit inside
		IdleTimeout:       60 * time.Second,//how long a kept-alive connection waits for its next request
	}
	if err := server.ListenAndServe(); err != nil {//ListenAndServe runs for the life of the process and returns only with an error, like the port already being taken
		fmt.Fprintf(os.Stderr, "fatal: %v\n", err)//to standard error, which systemd keeps in the journal; %v writes the error's text
		os.Exit(1)//a failing exit code, so the unit's Restart=on-failure starts the service again
	}
}
