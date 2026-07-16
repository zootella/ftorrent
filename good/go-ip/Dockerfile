# The ip reflector image. Multi-stage: the golang image compiles a fully static binary, and the runtime stage is scratch — no OS, no shell, no libc, nothing in the shipped image but the one executable. CGO_ENABLED=0 keeps the binary free of C library dependencies so it runs on scratch; -trimpath and -ldflags="-s -w" drop build paths and symbol tables, shrinking what ships.
FROM golang:1.24-alpine AS build
WORKDIR /src
COPY go.mod main.go ./
RUN CGO_ENABLED=0 GOOS=linux go build -trimpath -ldflags="-s -w" -o /ftorrent-ip .

FROM scratch
COPY --from=build /ftorrent-ip /ftorrent-ip
EXPOSE 8100
ENTRYPOINT ["/ftorrent-ip"]
