# Dockerfile for the open.ftorrent.com gauge
#
# A long-running Node.js process that scrapes Prometheus metrics from
# the Aquatic tracker containers and writes page.json every minute.
# No HTTP server, no listening ports.
#
# Build:
#   docker build -t ftorrent-open-node-gauge .
#
# The container expects GAUGE_DIR to be set to the path of a writable
# volume where it writes page.json and ring.json.

FROM node:22-slim

WORKDIR /app

# Copy package files and install dependencies
COPY package.json ./
RUN npm install --omit=dev

# Copy source
COPY src/ ./src/

# Run as nobody — matching the container hardening model
USER 65534:65534

ENV GAUGE_DIR=/gauge

CMD ["node", "src/gauge.js"]
