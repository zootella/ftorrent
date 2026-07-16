# The pion WebRTC ICE peer — Go, built static, run on scratch.
#
# Multi-stage: the golang image compiles, the runtime is scratch (nothing at all). pion is pure Go, so CGO_ENABLED=0 gives a fully static binary; ICE is raw UDP — no TLS to a CA-signed peer, no DNS — so the runtime needs no certificates, no resolver, no shell, and scratch is the smallest, most hardened base there is. The container runs as a numeric uid (set in the compose file), which needs no /etc/passwd, so scratch works there too.
#
# The peer itself is deferred: this build context waits for the Go source (go.mod, go.sum, *.go), and the compose file never builds it until that lands here. The golang build-stage tag is the one bumpable line — throwaway, it never ships; scratch is the durable choice.

FROM golang:1.25-bookworm AS build
WORKDIR /src
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o /pion .

FROM scratch
COPY --from=build /pion /pion
EXPOSE 49152/udp
ENTRYPOINT ["/pion"]
