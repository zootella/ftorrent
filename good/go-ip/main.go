// The ip reflector — the public what's-my-IP API at ip.ftorrent.com, ip4.ftorrent.com, and ip6.ftorrent.com.
//
//	GET /              -> the caller's IP, text/plain
//	GET /?format=json  -> {"ip":"..."}
//	GET /health        -> ok
//
// All three hostnames route here identically; DNS alone decides which address family gets reflected. ip4.ftorrent.com carries only an A record, so a client can only reach it over IPv4; ip6.ftorrent.com carries only AAAA, forcing IPv6; ip.ftorrent.com carries both and reflects whichever family the caller's stack chose. Calling the ip4/ip6 pair and comparing the two outcomes is therefore a dual-stack test — the same mechanism the good.ftorrent.com page uses.
package main

import (
	"fmt"
	"net"
	"net/http"
	"os"
	"strings"
)

// clientIP finds the visitor's true public address. The reflector runs behind a TLS-terminating front door (HAProxy, then Caddy over a unix socket), so the TCP peer on this socket is always the local proxy, never the visitor. The front door guarantees exactly one trustworthy X-Forwarded-For value: it deletes any X-Forwarded-For the client sent, then appends the source address it accepted the connection from. The visitor is therefore the last entry in the list — reading any earlier entry would trust a header the client could have written.
func clientIP(r *http.Request) string {
	if xff := r.Header.Get("X-Forwarded-For"); xff != "" {
		parts := strings.Split(xff, ",")
		return strings.TrimSpace(parts[len(parts)-1])
	}
	// No X-Forwarded-For means a direct hit on the container port, which only happens in local development — in production the front door always stamps the header. RemoteAddr is "address:port", and net.SplitHostPort strips the port correctly for both families: "203.0.113.9:56789" and "[::1]:56789" alike.
	host, _, err := net.SplitHostPort(r.RemoteAddr)
	if err != nil {
		return r.RemoteAddr
	}
	return host
}

func main() {
	// The deployment contract: Caddy reverse-proxies all three hostnames to this server on 8100. The PORT override exists for running the binary directly during development.
	port := os.Getenv("PORT")
	if port == "" {
		port = "8100"
	}

	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/plain; charset=utf-8")
		fmt.Fprintln(w, "ok")
	})

	// Every path other than /health reflects. CORS is open because the reflection is the caller's own public data — browser JavaScript on any origin may read its own address here.
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		ip := clientIP(r)
		w.Header().Set("Access-Control-Allow-Origin", "*")
		if r.URL.Query().Get("format") == "json" {
			// %q quotes and escapes the string, which is all the JSON a one-field object needs — no encoding/json import for this.
			w.Header().Set("Content-Type", "application/json")
			fmt.Fprintf(w, "{\"ip\":%q}\n", ip)
		} else {
			w.Header().Set("Content-Type", "text/plain; charset=utf-8")
			fmt.Fprintf(w, "%s\n", ip)
		}
	})

	if err := http.ListenAndServe(":"+port, nil); err != nil {
		fmt.Fprintf(os.Stderr, "fatal: %v\n", err)
		os.Exit(1)
	}
}
