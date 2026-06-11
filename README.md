
_ftorrent/README.md [ftorrent.com](https://ftorrent.com)_

# ftorrent

[open.ftorrent.com](https://open.ftorrent.com)

Workspaces and folders:
- ䷱ **site** Home page and website _[ftorrent.com](https://ftorrent.com/)_
- ䷴ **docs** (VitePress) Documentation site _[docs.ftorrent.com](https://docs.ftorrent.com/)_
- ䷯ **open** BitTorrent, WebTorrent, and WebRTC trackers, plus a DHT node _[open.ftorrent.com](https://open.ftorrent.com/)_
  - **open/docker** [Aquatic](https://github.com/greatest-ape/aquatic) Dockerfiles, configs, seccomp profile, and compose file
  - **open/gauge** (Python) Scrapes tracker and DHT stats, writes page.json and breaker.json
  - **open/breaker** (Python) Per-service circuit breaker; sheds load at the proxy and firewall
  - **open/page** (Vue+Vite) Frontend dashboard
  - **open/dht** [qBittorrent](https://www.qbittorrent.org/) Mainline DHT bootstrap node _dht.ftorrent.com_
- ䷼ **good** (Node, Docker) IPv4/IPv6 connectivity diagnostic tool _[good.ftorrent.com](https://good.ftorrent.com/)_
- ䷺ **desktop** ([Tauri](https://tauri.app/), [libtorrent-rasterbar](https://www.libtorrent.org/)) Desktop BitTorrent client
- ䷣ **classic** ([Win32](https://learn.microsoft.com/en-us/windows/win32/api/)) Rasterbar libtorrent in a Petzold-style Windows native EXE
- **notes** Notes and scraps (deprecating)
