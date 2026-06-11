# Dockerfile for the open.ftorrent.com DHT bootstrap node.
#
# A headless qBittorrent (qbittorrent-nox) configured as a pure mainline-DHT
# participant: no torrents, no seeding, no PeX — it only joins and bootstraps
# the Kademlia DHT and reports its health over an internal Web API.
#
# Built FROM debian:bookworm-slim to match the Aquatic tracker containers'
# runtime base — one Debian, one libc across the deployment — and to run
# cleanly as nobody (65534) under a read-only root filesystem. qbittorrent-nox
# comes from Debian's own repositories, vetted by the distribution's security
# team (which backports fixes), rather than a community image whose s6/PUID
# init model fights user:65534 + read_only.
#
# Build:
#   docker build -t ftorrent-open-dht .
#
# Configuration is NOT baked in: qbittorrent-nox rewrites its own config file,
# so it lives on a writable /config volume, seeded with qBittorrent.conf before
# first start (see README.md). Everything writable is confined to /config (the
# bind mount) and /tmp (a tmpfs); the rest of the filesystem stays read-only.

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    qbittorrent-nox \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Run as nobody (UID 65534), matching the tracker containers' hardening.
USER 65534:65534

# Confine every writable path qBittorrent or Qt might touch to the /config
# volume or the /tmp tmpfs, so the root filesystem can stay read-only:
#   config + DHT state  -> /config  (persistent bind mount)
#   cache + runtime     -> /tmp     (ephemeral tmpfs)
ENV HOME=/config \
    XDG_CONFIG_HOME=/config \
    XDG_DATA_HOME=/config \
    XDG_CACHE_HOME=/tmp \
    XDG_RUNTIME_DIR=/tmp

EXPOSE 51420/udp
EXPOSE 8080/tcp

# No CMD: the seeded qBittorrent.conf's [LegalNotice] Accepted=true records
# acceptance non-interactively. Don't add --confirm-legal-notice — bookworm's
# qbittorrent-nox rejects it as an unknown flag and the container crash-loops.
ENTRYPOINT ["qbittorrent-nox"]
