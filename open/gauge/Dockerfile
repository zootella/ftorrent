# Dockerfile for the open.ftorrent.com gauge
#
# A long-running Python process that scrapes Prometheus metrics from the Aquatic tracker containers and writes page.json every minute. It wakes at the top of each minute, does its work, and sleeps until the next — resident, but holding almost nothing between ticks. No HTTP server, no listening ports.
#
# Build:
#   docker build -t ftorrent-open-python-gauge .
#
# The container expects GAUGE_DIR to be set to the path of a writable volume, where it writes public/page.json and its private files beside it, and reads the probe's file.

# python:3.12-slim-bookworm pins one Debian release and Python minor across the whole deployment: bookworm matches the Aquatic tracker containers (debian:bookworm-slim), and 3.12 matches the host's bare-metal scripts. One Debian, one libc, one Python to reason about. slim (not alpine) keeps us on glibc and the Debian security team; not digest-pinned, so upstream security rebuilds reach us on a periodic `docker build --pull`.
FROM python:3.12-slim-bookworm

WORKDIR /app

# One dependency beyond the standard library: ergo, for its size and time constants and its tick. requirements.txt pins the release and the hash of its wheel, so pip installs that file and nothing else. It's copied and installed before the source, so a change to gauge.py rebuilds only the COPY layer below rather than fetching ergo again, and --no-cache-dir keeps pip's download cache out of the image.
COPY requirements.txt ./
RUN pip install --no-cache-dir --require-hashes -r requirements.txt

COPY src/ ./src/

# Run as nobody — matching the container hardening model
USER 65534:65534

ENV GAUGE_DIR=/gauge

CMD ["python", "src/gauge.py"]
