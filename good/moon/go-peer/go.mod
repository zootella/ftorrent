//the module's name, which deploy.py's go build names the binary after
module ftorrent-peer

//the Go version the code needs, Debian 13's own, which pion's own go.mod asks for too, so the box builds with the Go it has
go 1.24.0

//pion's WebRTC, with its ICE agent and its logger for one setting each; go.sum pins each module below by hash, and go build on the box fetches them through Go's module proxy
require (
	github.com/pion/ice/v4 v4.4.8
	github.com/pion/logging v0.2.4
	github.com/pion/webrtc/v4 v4.2.26
)

//what those two need in turn, which go mod tidy lists: the rest of pion, the Go project's own extended libraries under golang.org/x, a UUID library, and a small one pion uses to list interfaces on Android
require (
	github.com/google/uuid v1.6.0 // indirect
	github.com/pion/datachannel v1.6.3 // indirect
	github.com/pion/dtls/v3 v3.1.15 // indirect
	github.com/pion/interceptor v0.1.51 // indirect
	github.com/pion/mdns/v2 v2.2.2 // indirect
	github.com/pion/randutil v0.1.0 // indirect
	github.com/pion/rtcp v1.2.20 // indirect
	github.com/pion/rtp v1.10.6 // indirect
	github.com/pion/sctp v1.12.0 // indirect
	github.com/pion/sdp/v3 v3.0.20 // indirect
	github.com/pion/srtp/v3 v3.1.3 // indirect
	github.com/pion/stun/v4 v4.0.1 // indirect
	github.com/pion/transport/v5 v5.1.4 // indirect
	github.com/pion/turn/v5 v5.1.3 // indirect
	github.com/wlynxg/anet v0.0.5 // indirect
	golang.org/x/crypto v0.48.0 // indirect
	golang.org/x/net v0.50.0 // indirect
	golang.org/x/sys v0.41.0 // indirect
	golang.org/x/time v0.14.0 // indirect
)
