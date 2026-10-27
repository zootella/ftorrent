// The Go service on moon, the second vantage of good.ftorrent.com, at moon.ftorrent.com, ip4.moon.ftorrent.com, and ip6.moon.ftorrent.com: the reflector, the UDP poke, and the TCP dial, and later the signaling room for peers behind NATs of our own, each an endpoint on this same small server.
//
//	GET /                           -> the caller's IP, text/plain
//	GET /?format=json               -> {"ip":"..."}
//	GET /poke?port=N[&token=T]      -> three small UDP packets carrying T to the caller's address at port N, and a JSON account of them
//	GET /dial?port=N[&infohash=H]   -> a TCP connection to the caller's address at port N with a BitTorrent handshake, and a JSON account of what came back
//	GET /health                     -> ok
//
// One rule governs the poke and the dial: moon sends only to the address a request came from, never to an address the request names, so it can never be pointed at anyone else. The caller names a port and nothing more, and the HTTPS request that asks is a TCP connection, whose source can't be forged the way a UDP packet's can. Both are rate-limited per source address, and the reflector, which sends nothing anywhere, is not.
//
// The reflector is the same contract as ip.ftorrent.com on the main server, and DNS does the same work here: ip4.moon.ftorrent.com carries only an A record and ip6.moon.ftorrent.com only a AAAA, so a request to one of them travels over that family alone, and moon.ftorrent.com carries both. The same names choose the poke's and the dial's family, since moon sends to the address the request arrived from. The service runs as a systemd unit on the box itself, listening on the loopback address alone; Caddy terminates TLS for the three names and proxies to it. Nothing here logs a request or an address.
package main

import (
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"strconv"
	"strings"
	"sync"
	"time"
)

const (
	pokePackets = 3                         // how many packets a poke sends, since UDP keeps no promises and one lost packet shouldn't read as a closed door
	pokeGap     = 50 * time.Millisecond     // between them, so they don't leave in one burst
	dialTimeout = 3 * time.Second           // to connect, and then to hear back; a peer that answers at all answers within this
	limitBurst  = 10                        // pokes and dials one source may make at once
	limitRate   = 10.0 / 60                 // and may make steadily, per second: ten a minute
	handshake   = "\x13BitTorrent protocol" // the first twenty bytes of every BitTorrent connection, a length byte and the protocol's name
	peerPrefix  = "-FF0000-"                // the eight characters at the front of the peer id moon dials with: FF is ftorrent's client code, and 0000 a version no client has, since this is a prober
)

// clientIP is the address the request came from. Caddy, the only thing that reaches this server, sets X-Forwarded-For to the address it accepted the connection from and ignores any X-Forwarded-For the client sent, so the last entry is the client and an earlier one could only be Caddy's own. With no header at all the request came straight to the socket, which only happens in development, and the socket's peer is the answer; net.SplitHostPort strips the port for both families, "203.0.113.9:56789" and "[2001:db8::9]:56789" alike.
func clientIP(r *http.Request) string {
	if xff := r.Header.Get("X-Forwarded-For"); xff != "" {
		parts := strings.Split(xff, ",")
		return strings.TrimSpace(parts[len(parts)-1])
	}
	host, _, err := net.SplitHostPort(r.RemoteAddr)
	if err != nil {
		return r.RemoteAddr
	}
	return host
}

// A limiter holds a token bucket per source address: each poke or dial spends a token, the bucket refills at limitRate up to limitBurst, and a source with an empty bucket is answered 429 and sent nothing. The map is pruned of full buckets when it grows large, so a day of distinct sources doesn't hold memory.
type limiter struct {
	mu      sync.Mutex
	buckets map[string]*bucket
}

type bucket struct {
	tokens float64
	last   time.Time
}

func (l *limiter) allow(source string) bool {
	l.mu.Lock()
	defer l.mu.Unlock()
	now := time.Now()
	b, ok := l.buckets[source]
	if !ok {
		if len(l.buckets) >= 4096 { // room to make: drop every bucket that has refilled, which is every source not asking right now
			for key, old := range l.buckets {
				if old.tokens+now.Sub(old.last).Seconds()*limitRate >= limitBurst {
					delete(l.buckets, key)
				}
			}
		}
		b = &bucket{tokens: limitBurst, last: now}
		l.buckets[source] = b
	}
	b.tokens = min(limitBurst, b.tokens+now.Sub(b.last).Seconds()*limitRate)
	b.last = now
	if b.tokens < 1 {
		return false
	}
	b.tokens--
	return true
}

// portParam reads the port the caller named, 1 through 65535, or answers 400 and returns false.
func portParam(w http.ResponseWriter, r *http.Request) (int, bool) {
	port, err := strconv.Atoi(r.URL.Query().Get("port"))
	if err != nil || port < 1 || port > 65535 {
		http.Error(w, "port must be a number from 1 to 65535", http.StatusBadRequest)
		return 0, false
	}
	return port, true
}

// randomHex is n random bytes as lowercase hex, for a token or an info-hash the caller didn't supply.
func randomHex(n int) string {
	b := make([]byte, n)
	rand.Read(b)
	return hex.EncodeToString(b)
}

// writeJSON answers one object, which is every poke's and dial's whole report.
func writeJSON(w http.ResponseWriter, v any) {
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(v)
}

// isHex says whether s is exactly n hex digits.
func isHex(s string, n int) bool {
	if len(s) != n {
		return false
	}
	_, err := hex.DecodeString(s)
	return err == nil
}

// isToken says whether a caller's token is up to 32 letters and digits, which is all a token needs to be and keeps it out of any other interpretation.
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

func main() {
	// Caddy proxies the three names to 127.0.0.1:8100; PORT exists for running the binary by hand during development. The address stays the loopback one whatever the port, so nothing reaches this server but Caddy on the same box.
	port := os.Getenv("PORT")
	if port == "" {
		port = "8100"
	}
	limits := &limiter{buckets: map[string]*bucket{}}
	mux := http.NewServeMux()

	mux.HandleFunc("GET /health", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/plain; charset=utf-8")
		fmt.Fprintln(w, "ok")
	})

	// The reflector answers at the root path alone, so the other endpoints' paths are their own and anything else is a plain 404. CORS is open because the reflection is the caller's own public data: a page on any origin may read its own address here, as it may at ip.ftorrent.com.
	mux.HandleFunc("GET /{$}", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		w.Header().Set("Access-Control-Allow-Origin", "*")
		if r.URL.Query().Get("format") == "json" {
			// %q quotes and escapes the string, which is all the JSON a one-field object needs.
			w.Header().Set("Content-Type", "application/json")
			fmt.Fprintf(w, "{\"ip\":%q}\n", ip)
		} else {
			w.Header().Set("Content-Type", "text/plain; charset=utf-8")
			fmt.Fprintf(w, "%s\n", ip)
		}
	})

	// The poke: a UDP socket on an ephemeral port, in the caller's family, with no address of its own, so the kernel sends from the address it would use toward the caller, moon's own; then pokePackets packets to the caller's address at the port they named, each "poke <token> <n>/3", a few dozen bytes. The caller, listening on that port, reports whether they arrived; the answer here says what left and from which port, which is the port their filtering test needs to know.
	mux.HandleFunc("GET /poke", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		if !limits.allow(ip) {
			http.Error(w, "too many pokes and dials from this address; ten a minute", http.StatusTooManyRequests)
			return
		}
		port, ok := portParam(w, r)
		if !ok {
			return
		}
		token := r.URL.Query().Get("token")
		if token == "" {
			token = randomHex(8)
		} else if !isToken(token) {
			http.Error(w, "token must be up to 32 letters and digits", http.StatusBadRequest)
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
		report := map[string]any{"to": to.String(), "token": token, "packets": pokePackets, "sent": 0}
		socket, err := net.ListenUDP(network, &net.UDPAddr{})
		if err != nil {
			report["error"] = err.Error()
			writeJSON(w, report)
			return
		}
		defer socket.Close()
		report["from_port"] = socket.LocalAddr().(*net.UDPAddr).Port
		for i := 1; i <= pokePackets; i++ {
			if i > 1 {
				time.Sleep(pokeGap)
			}
			payload := fmt.Appendf(nil, "poke %s %d/%d", token, i, pokePackets)
			if _, err := socket.WriteToUDP(payload, to); err != nil {
				report["error"] = err.Error()
				break
			}
			report["sent"] = i
			report["bytes"] = len(payload)
		}
		writeJSON(w, report)
	})

	// The dial: a TCP connection to the caller's address at the port they named, then a BitTorrent handshake, 68 bytes, with the info-hash the caller gave or a random one, and whatever comes back within the timeout. A BitTorrent client that holds the torrent answers with its own handshake, which names it; one that doesn't closes the connection without a word, which still proves the connection opened; and a port nobody listens on refuses or stays silent, which the error says.
	mux.HandleFunc("GET /dial", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		if !limits.allow(ip) {
			http.Error(w, "too many pokes and dials from this address; ten a minute", http.StatusTooManyRequests)
			return
		}
		port, ok := portParam(w, r)
		if !ok {
			return
		}
		infohash := r.URL.Query().Get("infohash")
		if infohash == "" {
			infohash = randomHex(20)
		} else if !isHex(infohash, 40) {
			http.Error(w, "infohash must be 40 hex digits", http.StatusBadRequest)
			return
		}
		to := net.JoinHostPort(ip, strconv.Itoa(port))
		report := map[string]any{"to": to, "infohash": infohash, "connected": false}
		start := time.Now()
		conn, err := net.DialTimeout("tcp", to, dialTimeout)
		report["milliseconds"] = time.Since(start).Milliseconds()
		if err != nil {
			report["error"] = err.Error()
			writeJSON(w, report)
			return
		}
		defer conn.Close()
		report["connected"] = true
		report["from_port"] = conn.LocalAddr().(*net.TCPAddr).Port
		conn.SetDeadline(time.Now().Add(dialTimeout))
		hash, _ := hex.DecodeString(infohash)
		id := []byte(peerPrefix + randomHex(6))                                                  // 8 characters and 12 of hex: a 20-byte peer id in the usual shape
		message := append(append(append([]byte(handshake), make([]byte, 8)...), hash...), id...) // the name, eight reserved bytes, the info-hash, the peer id
		if _, err := conn.Write(message); err != nil {
			report["error"] = err.Error()
			writeJSON(w, report)
			return
		}
		report["sent"] = len(message)
		// Read until a whole handshake is in hand, the peer closes, or the deadline passes; a partial answer is reported as it is.
		answer := make([]byte, 0, len(message))
		buffer := make([]byte, 256)
		for len(answer) < len(message) {
			n, err := conn.Read(buffer)
			answer = append(answer, buffer[:n]...)
			if err != nil {
				if errors.Is(err, io.EOF) {
					report["closed"] = true // the peer hung up, which a client without this torrent does
				} else {
					report["error"] = err.Error()
				}
				break
			}
		}
		report["received"] = len(answer)
		if len(answer) >= len(message) && string(answer[:len(handshake)]) == handshake {
			report["handshake"] = map[string]any{"infohash": hex.EncodeToString(answer[28:48]), "peer_id": fmt.Sprintf("%q", answer[48:68])}
		} else if len(answer) > 0 {
			report["received_hex"] = hex.EncodeToString(answer)
		}
		writeJSON(w, report)
	})

	// Each request gets a few seconds and each idle connection a minute, so a stalled client holds nothing for long; a dial's own timeout fits inside the write timeout. Caddy in front has its own limits, and these are the service's own.
	server := &http.Server{
		Addr:              "127.0.0.1:" + port,
		Handler:           mux,
		ReadHeaderTimeout: 5 * time.Second,
		ReadTimeout:       10 * time.Second,
		WriteTimeout:      10 * time.Second,
		IdleTimeout:       60 * time.Second,
	}
	if err := server.ListenAndServe(); err != nil {
		fmt.Fprintf(os.Stderr, "fatal: %v\n", err)
		os.Exit(1)
	}
}
