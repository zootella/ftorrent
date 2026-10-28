/*
The Go service on moon, the second vantage of good.ftorrent.com, answering behind Caddy at moon.ftorrent.com, ip4.moon.ftorrent.com, and ip6.moon.ftorrent.com: the reflector, the UDP poke, and the TCP dial, and later the signaling room for peers behind NATs of our own, each an endpoint on this one small server. The root of moon.ftorrent.com is the one path that never reaches it: Caddy answers there itself, with a page linking to moon's guide.

	GET /                          -> the caller's IP, as text, at ip4.moon and ip6.moon
	GET /?format=json              -> {"ip":"..."}
	GET /poke?port=N[&token=T]     -> three small UDP packets carrying T to the caller's address at port N, and a JSON account of them
	GET /dial?port=N[&infohash=H]  -> a TCP connection to the caller's address at port N with a BitTorrent handshake, and a JSON account of what came back
	GET /health                    -> ok

One rule governs the poke and the dial: moon sends only to the address a request came from, never to an address the request names, so it can never be pointed at anyone else. The caller names a port and nothing more, and the HTTPS request that asks is a TCP connection, whose source can't be forged the way a UDP packet's can. Both are rate-limited per source address, and the reflector, which sends nothing anywhere, is not.

The reflector is the same contract as ip.ftorrent.com on the main server, and DNS does the same work here: ip4.moon.ftorrent.com carries only an A record and ip6.moon.ftorrent.com only a AAAA, so a request to one of them travels over that family alone, and moon.ftorrent.com carries both. The same names choose the poke's and the dial's family, since moon sends to the address the request arrived from. The service runs as a systemd unit on the box itself, listening on the loopback address alone; Caddy terminates TLS for the three names and proxies to it. Nothing here logs a request or an address.

A few things about Go run through the whole file, and a reader coming from JavaScript meets them on every screen. A function hands back its error as a second value rather than throwing one, so a call reads value, err := f(), and the line after it checks err != nil. := declares a variable and assigns it at once, and _ throws away a value a call returns. Go's net/http runs each request's handler on a goroutine of its own, a lightweight thread, so two requests can be inside the same handler at the same moment, and anything they share, like the rate limiter's map, needs a lock. defer schedules a call for when the surrounding function returns, by whichever return, which is how a socket or a lock gets let go. Text is a string and bytes are a []byte, a slice, Go's growable array, and append answers the slice it grew.

And time, which reads differently than it does in JavaScript: Go's time.Duration counts nanoseconds, not milliseconds. time.Second is 1,000,000,000 and time.Millisecond is 1,000,000, where setTimeout and Date.now() count in milliseconds, so 3 * time.Second is a Duration of three seconds, and that's the type every timeout and sleep in Go's standard library takes. A deadline is the other type, a time.Time, which is a moment rather than a span: time.Now().Add(d) is the moment d from now, and subtracting two moments answers a Duration.

The program uses Go's standard library alone, so go.mod lists no dependencies, and deploy.py builds it on moon with Debian's Go.
*/
package main

import (
	"crypto/rand"//random bytes from the operating system, for a token or an info-hash the caller didn't give
	"encoding/hex"//bytes to and from hex text
	"encoding/json"//values to JSON, for the poke's and the dial's reports
	"errors"//errors.Is, to tell which error a read ended with
	"fmt"//formatted text, written to the answer or into bytes
	"io"//io.EOF, the error a read gives when the other side has closed
	"net"//sockets: UDP for the poke, TCP for the dial, and parsing addresses
	"net/http"//the web server, its router, and the request and answer types
	"os"//the environment and the exit code
	"strconv"//numbers to and from text
	"strings"//splitting and trimming text
	"sync"//the lock the rate limiter takes
	"time"//moments, and Durations in nanoseconds
)

const (
	pokePackets = 3//how many packets a poke sends, since UDP keeps no promises and one lost packet shouldn't read as a closed door
	pokeGap     = 50 * time.Millisecond//the pause between them, so they don't leave in one burst; a Duration, which counts nanoseconds, so this is 50,000,000 of them
	dialTimeout = 3 * time.Second//to connect, and then to hear back; a peer that answers at all answers within this
	limitBurst  = 10//pokes and dials one source may make at once
	limitRate   = 10.0 / 60//and may make steadily, in tokens a second: ten a minute; written 10.0 so Go keeps the fraction, since 10 / 60 between two whole numbers is 0
	handshake   = "\x13BitTorrent protocol"//the first twenty bytes of every BitTorrent connection: a length byte, hex 13 for 19, then the protocol's 19-character name
	peerPrefix  = "-FF0000-"//the eight characters at the front of the peer id moon dials with: FF is ftorrent's client code, and 0000 a version no client has, since this is a prober
)

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

//limiter holds a token bucket for each source address: each poke or dial spends a token, the bucket refills at limitRate up to limitBurst, and a source with an empty bucket is answered 429 and sent nothing. The map is pruned of full buckets when it grows large, so a day of distinct sources doesn't hold memory
type limiter struct {
	mu      sync.Mutex//the lock allow takes before touching buckets, since two requests can be in allow at once on two goroutines
	buckets map[string]*bucket//each source's bucket, by its address as text; a map is Go's dictionary, and *bucket a pointer, so changing a bucket changes the one in the map
}

//bucket is one source's tokens and when they were last brought up to date
type bucket struct {
	tokens float64//how many pokes and dials the source may make right now, a fraction while it refills
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

//portParam reads the port the caller named, 1 through 65535, or answers 400 itself and returns false, so the handler returns at once
func portParam(w http.ResponseWriter, r *http.Request) (int, bool) {//two return values, the port and whether it was good; w is where the answer to the caller is written
	port, err := strconv.Atoi(r.URL.Query().Get("port"))//Atoi, ASCII to integer, reads text as a whole number and fails on anything else, a blank included
	if err != nil || port < 1 || port > 65535 {
		http.Error(w, "port must be a number from 1 to 65535", http.StatusBadRequest)//writes the status and this text as the body
		return 0, false
	}
	return port, true
}

//randomHex answers n random bytes as lowercase hex, 2n characters, for a token or an info-hash the caller didn't supply
func randomHex(n int) string {
	b := make([]byte, n)//make builds a slice of n zero bytes
	rand.Read(b)//fills it from the operating system's random source; since Go 1.24 this never fails, so there's no error to read
	return hex.EncodeToString(b)
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

//main builds the router, adds a handler for each endpoint, and serves until the process stops
func main() {
	port := os.Getenv("PORT")//Caddy proxies the three names to 127.0.0.1:8100, and PORT exists only for running the binary by hand during development; Getenv answers "" for a variable that isn't set
	if port == "" {
		port = "8100"
	}
	limits := &limiter{buckets: map[string]*bucket{}}//one limiter for the whole process, shared by the poke and the dial; the map has to be made, since writing to a nil map crashes
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

	//the poke: a UDP socket on a port the system picks, in the caller's family, bound to no address, so the kernel sends from the address it would use toward the caller, moon's own; then pokePackets packets to the caller's address at the port they named, each "poke <token> <n>/3", 17 bytes with an eight-character token and 41 with the longest. The caller, listening on that port, reports whether they arrived; the answer here says what left and from which port, which is the port a filtering test needs
	mux.HandleFunc("GET /poke", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		if !limits.allow(ip) {//the token is spent before the request is even read, so a flood of bad requests costs its sender too
			http.Error(w, "too many pokes and dials from this address; ten a minute", http.StatusTooManyRequests)
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
		to := &net.UDPAddr{IP: net.ParseIP(ip), Port: port}//where the packets go: the caller's own address, at the port they named
		if to.IP == nil {//ParseIP answers nil for text that isn't an address, which only a malformed header could give it
			http.Error(w, "the request's address isn't an IP address", http.StatusBadRequest)
			return
		}
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
		if !limits.allow(ip) {//the poke's bucket too, so ten a minute between them
			http.Error(w, "too many pokes and dials from this address; ten a minute", http.StatusTooManyRequests)
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
