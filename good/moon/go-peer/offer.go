/*
The WebRTC half of the peer: the same hole punch as main.go's, done by a browser's ICE and pion's rather than by hand.

	POST /offer?caller=A  -> the page's offer in, as its SDP, and the peer's answer out, as JSON with the answer's SDP inside

The page makes a peer connection with a data channel, gathers its candidates through our STUN server, and sends the offer, candidates and all, through ftorrent-moon, which adds the caller's address. The peer makes a peer connection of its own, gathers through the same STUN server, and answers with its candidates. Both sides then run ICE, Interactive Connectivity Establishment: each sends STUN binding requests from its candidates to the other's, and the first pair to get an answer both ways carries the connection, which DTLS encrypts and SCTP carries the data channel over. That is one HTTPS exchange, every candidate gathered before it, the shape WHIP standardized for signaling over HTTP, and it needs no room to meet in, since the peer is always here.

Two edits to the descriptions keep the punch to the point. The offer keeps its public IPv4 candidates, at most keepLimit of them: a browser's host candidates are made-up .local names or private addresses, and an IPv6 one has no path to a peer behind an IPv4 NAT. Those candidates are the browser's public addresses as STUN saw them, which need not be the address its HTTPS came from, since a phone's carrier can send its TCP and its UDP out from different addresses, and the peer checks them all. ICE's checks are STUN requests signed with the browser's own ICE password, so nothing but that browser's WebRTC answers them, and the peer gives up after eight seconds. And the answer drops the peer's host candidate, its private address inside the namespace, which a browser would otherwise try on its own local network.

When the data channel opens, the peer sends one message over it, JSON naming its NAT and the candidate pair ICE chose, and the page reads it. Every peer connection is closed sessionLength after the answer, connected or not.
*/
package main

import (
	"encoding/json"//the answer, and the message over the data channel
	"fmt"//a candidate as text
	"io"//reading the offer
	"net/http"
	"net/netip"//the caller's address, and telling an IPv4 candidate from the rest
	"strings"//going through a description line by line
	"sync"//sync.Once, so a session gives its place back once
	"time"

	"github.com/pion/ice/v4"//the ICE agent beneath pion's WebRTC, for its multicast DNS setting
	"github.com/pion/logging"//pion's logger, to turn it off
	"github.com/pion/webrtc/v4"//WebRTC in Go
)

const (
	offerLimit    = 16 * 1024//bytes; a browser's offer is a few kilobytes
	keepLimit     = 16//IPv4 candidates kept from an offer, more than a browser gathers in one family, so ICE checks a bounded number of addresses
	sessionLength = 20 * time.Second//how long a peer connection lives after the answer, which ICE's own timeouts below settle well inside
)

//api makes every peer connection, with the settings main gives it
var api *webrtc.API

//webrtcAPI builds the settings every peer connection shares: ICE over UDP and IPv4 alone, since the namespace translates nothing else; no multicast DNS, the way browsers hide a host address behind a .local name, since the offer's .local candidates are dropped and the peer's own host candidate never leaves; timeouts short enough that a pair that can't connect is given up in seconds; and no logging, since pion's errors can name a caller's address and moon logs none
func webrtcAPI() *webrtc.API {
	settings := webrtc.SettingEngine{}
	quiet := logging.NewDefaultLoggerFactory()
	quiet.DefaultLogLevel = logging.LogLevelDisabled
	settings.LoggerFactory = quiet
	settings.SetNetworkTypes([]webrtc.NetworkType{webrtc.NetworkTypeUDP4})
	settings.SetICEMulticastDNSMode(ice.MulticastDNSModeDisabled)
	settings.SetICETimeouts(4*time.Second, 8*time.Second, 2*time.Second)//disconnected after four seconds without an answer, failed after eight, and a keepalive every two
	return webrtc.NewAPI(webrtc.WithSettingEngine(settings))
}

//offer answers POST /offer: the page's offer, kept to its IPv4 candidates, then a peer connection answering it, and the answer once the peer's candidates are gathered; the caller's address, where ftorrent-moon saw the request come from, goes in the report beside what was kept
func offer(w http.ResponseWriter, r *http.Request) {
	caller, err := netip.ParseAddr(r.URL.Query().Get("caller"))
	if err != nil || !caller.Is4() {
		http.Error(w, "caller must be an IPv4 address", http.StatusBadRequest)
		return
	}
	body, err := io.ReadAll(io.LimitReader(r.Body, offerLimit+1))//one byte past the limit, so an offer that's too long shows as one
	if err != nil || len(body) > offerLimit {
		http.Error(w, "the offer must be an SDP description under 16 KB", http.StatusBadRequest)
		return
	}
	trimmed, dropped := keepCandidates(string(body))
	select {
	case running <- struct{}{}:
	default:
		http.Error(w, "too many punches at once", http.StatusServiceUnavailable)
		return
	}
	var once sync.Once
	release := func() { once.Do(func() { <-running }) }//the place this session holds, given back once, by whichever of the closes below comes first
	connection, err := api.NewPeerConnection(webrtc.Configuration{ICEServers: []webrtc.ICEServer{{URLs: []string{"stun:" + stunServer}}}})
	if err != nil {
		release()
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	finish := func() { connection.Close(); release() }//close the session and give its place back, without counting on pion to report the close
	connection.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {//a pair that can't connect is given up in seconds, by the timeouts webrtcAPI sets
		if state == webrtc.PeerConnectionStateFailed {
			finish()
		}
	})
	time.AfterFunc(sessionLength, finish)//AfterFunc runs the function on a goroutine of its own once the time has passed, whether the session connected or not
	connection.OnDataChannel(func(channel *webrtc.DataChannel) {//the page's channel, which the peer meets rather than makes
		channel.OnOpen(func() {//the hole punch done: ICE found a pair, and DTLS and SCTP ran over it
			report := map[string]any{"nat": nat}
			if pair, err := connection.SCTP().Transport().ICETransport().GetSelectedCandidatePair(); err == nil && pair != nil {
				report["local"] = sayCandidate(pair.Local)
				report["remote"] = sayCandidate(pair.Remote)
			}
			message, _ := json.Marshal(report)
			channel.SendText(string(message))
		})
	})
	if err := connection.SetRemoteDescription(webrtc.SessionDescription{Type: webrtc.SDPTypeOffer, SDP: trimmed}); err != nil {
		finish()
		http.Error(w, "the offer: "+err.Error(), http.StatusBadRequest)
		return
	}
	answer, err := connection.CreateAnswer(nil)
	if err != nil {
		finish()
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	gathered := webrtc.GatheringCompletePromise(connection)//a channel that closes once every candidate is gathered, made before gathering starts so the end can't be missed
	if err := connection.SetLocalDescription(answer); err != nil {//which starts gathering
		finish()
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	select {//a select waits for whichever of its cases is ready first
	case <-gathered:
	case <-time.After(stunTimeout)://STUN has three seconds to answer, here as in the punch
	}
	writeJSON(w, map[string]any{"nat": nat, "caller": caller.String(), "kept": candidateLines(trimmed), "dropped": dropped, "answer": dropHost(connection.LocalDescription().SDP)})
}

//keepCandidates answers the offer with only its public IPv4 candidates, the first keepLimit of them, and how many it dropped. A candidate line reads a=candidate:, then the foundation, the component, the transport, the priority, the address, the port, typ, and the type, so the address is the fifth field, and a .local name or an IPv6 address doesn't parse as IPv4
func keepCandidates(sdp string) (string, int) {
	var lines []string
	kept, dropped := 0, 0
	for _, line := range strings.SplitAfter(sdp, "\n") {//SplitAfter keeps each line's ending, \r\n in SDP, so the lines join back as they were
		if strings.HasPrefix(line, "a=candidate:") {
			fields := strings.Fields(line)
			ipv4 := false
			if len(fields) >= 5 {
				address, err := netip.ParseAddr(fields[4])
				ipv4 = err == nil && address.Is4() && address.IsGlobalUnicast() && !address.IsPrivate()//a public address, since a private one, like a host address a browser shows unmasked, is nowhere moon can reach
			}
			if !ipv4 || kept == keepLimit {
				dropped++
				continue
			}
			kept++
		}
		lines = append(lines, line)
	}
	return strings.Join(lines, ""), dropped
}

//dropHost answers the peer's own description without its host candidates, which name its private address inside the namespace
func dropHost(sdp string) string {
	var lines []string
	for _, line := range strings.SplitAfter(sdp, "\n") {
		if strings.HasPrefix(line, "a=candidate:") && strings.Contains(line, " typ host") {
			continue
		}
		lines = append(lines, line)
	}
	return strings.Join(lines, "")
}

//candidateLines answers the candidate lines in a description, without their a= and line endings, for the report
func candidateLines(sdp string) []string {
	candidates := []string{}//empty rather than nil, so the JSON says [] rather than null
	for _, line := range strings.Split(sdp, "\n") {
		if strings.HasPrefix(line, "a=candidate:") {
			candidates = append(candidates, strings.TrimSpace(strings.TrimPrefix(line, "a=")))
		}
	}
	return candidates
}

//sayCandidate writes a candidate as its type and its address and port, like srflx 203.0.113.42:52814
func sayCandidate(c *webrtc.ICECandidate) string {
	return fmt.Sprintf("%s %s:%d", c.Typ, c.Address, c.Port)
}
