//the module's name, which go build names the binary after unless -o says otherwise, as deploy.py's does
module ftorrent-moon

//the Go version the code needs, Debian 13's own; there's no require line, since the code uses the standard library alone
go 1.24
