# Dockerfile for the open.ftorrent.com DHT bootstrap node.
#
# A small Python program, dht.py, holding libtorrent's DHT and nothing else, with each routing table's size answered on an internal port. libtorrent is the maintainers' own wheel, pinned by hash in uv.lock, the same Linux wheels the desktop client's engine is frozen against.
#
# Two stages. The first has uv, which fetches the wheel by the hash the lock records into a virtual environment. The second copies that environment and the program onto a clean copy of the same base, so uv never reaches the image that runs, and the environment's link to its Python resolves the same in both.
#
# python:3.13-slim-trixie because the wheel is built for Python 3.13, and trixie is Debian's current release. It isn't pinned by digest, so `docker compose build --pull` brings in the base's fixes; libtorrent's arrive by moving the pin in pyproject.toml and uv.lock.
#
# Build:
#   docker build -t ftorrent-open-libtorrent-dht .

FROM python:3.13-slim-trixie AS builder

# uv at the version the desktop client's Linux build image pins, copied out of Astral's published image
COPY --from=ghcr.io/astral-sh/uv:0.12.3 /uv /usr/local/bin/uv

WORKDIR /app
COPY pyproject.toml uv.lock ./

# The image's own Python, never one uv would download, and the wheel's files copied into the environment rather than linked to a cache this stage throws away
ENV UV_PYTHON_PREFERENCE=only-system \
    UV_PYTHON_DOWNLOADS=never \
    UV_LINK_MODE=copy
RUN uv sync --frozen --no-dev

FROM python:3.13-slim-trixie

COPY --from=builder /app/.venv /app/.venv
COPY dht.py /app/dht.py

# Run as nobody (UID 65534), matching the tracker containers' hardening
USER 65534:65534

EXPOSE 51420/udp
EXPOSE 8080/tcp

ENTRYPOINT ["/app/.venv/bin/python", "/app/dht.py"]
