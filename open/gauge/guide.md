# The open.ftorrent.com gauge

*Prepared by [Claude Code](https://claude.ai/code) using Opus 4.6*
*Created: 2026-Apr-07 | Last reviewed: 2026-Apr-07*

| Component | Version |
|---|---|
| Node | 22+ |

The gauge is a long-running Node.js process that produces the data behind the [open.ftorrent.com](https://open.ftorrent.com/) dashboard. It runs a cycle every minute: scrapes Prometheus metrics from the three Aquatic tracker containers, reads container memory statistics, computes dashboard data, and writes `page.json`. nginx serves `page.json` as a static file, and the Vue frontend reads it to display the dashboard.

The gauge has no HTTP server and no listening ports. It only writes files. In the Docker deployment, it runs as a hardened container on the `ftorrent-open` network (cafe 2), which gives it access to the Aquatic containers' Prometheus endpoints over internal Docker DNS.

## How it works

Every 60 seconds, the gauge:

1. Builds a data object with the current timestamp (and later, Prometheus metrics and memory stats)
2. Writes the object as JSON to `page.json.tmp`
3. Renames `page.json.tmp` to `page.json` (atomic — readers never see a half-written file)
4. Logs the timestamp to stdout

The atomic write pattern (write to temp file, then rename) is important. Without it, nginx could serve a partially-written `page.json` to a visitor during the brief moment the file is being overwritten. POSIX `rename()` is atomic — the file switches from old to new in a single operation.

## Project structure

```
open/gauge/
├── Dockerfile             Container image definition
├── compose-service.yml    Compose service entry (add to your docker-compose.yml)
├── package.json           Workspace metadata and start script
├── guide.md               This document
└── src/
    └── gauge.js           The gauge script
```

## Development

For local development, the gauge writes `page.json` directly to the page workspace's `public/` directory. This means you can run the gauge and the Vite dev server side by side — the gauge updates the data, and the page shows it.

From the monorepo root:

```bash
pnpm install
```

In one terminal:

```bash
cd open/gauge
pnpm start            # runs the gauge, writes page.json every 60 seconds
```

In another terminal:

```bash
cd open/page
pnpm dev              # serves the page, reads page.json
```

The gauge writes to `open/page/public/page.json` by default. This path is controlled by the `GAUGE_DIR` environment variable — locally it defaults to `../page`, so `public/page.json` lands in the right place for the Vite dev server. In the Docker deployment, `GAUGE_DIR` is set to `/gauge` in the compose file.

## Deployment

In production, the gauge runs as a Docker container on the `ftorrent-open` network (cafe 2) alongside the Aquatic tracker containers.

### Copying the source to the server

The gauge source needs to be on the server in the build context directory next to `docker-compose.yml`. Copy the contents of `open/gauge/` (Dockerfile, package.json, and src/) to the server:

```
/opt/open.ftorrent.com/
├── docker-compose.yml
├── node-gauge/
│   ├── Dockerfile
│   ├── package.json
│   └── src/
│       └── gauge.js
```

The compose service entry references `build: ./node-gauge`, which matches this layout.

### Building the image

From the `/opt/open.ftorrent.com/` directory on the server:

```bash
docker compose build node-gauge
```

Or build it directly from the `node-gauge/` directory:

```bash
cd node-gauge
docker build -t ftorrent-open-node-gauge .
```

The Dockerfile uses `node:22-slim` as the base image, copies the source code, installs production dependencies, and runs as nobody (UID 65534). No build step — the JavaScript runs directly.

### Adding the service to docker-compose.yml

Add the service entry from `compose-service.yml` to the `services:` section of your existing `docker-compose.yml`, alongside the Aquatic tracker services. The key settings:

```yaml
node-gauge:
  container_name: ftorrent-open-gauge-1
  build: ./node-gauge
  user: "65534:65534"
  read_only: true
  cap_drop: [ALL]
  security_opt: [no-new-privileges:true]
  environment:
    - GAUGE_DIR=/gauge
  volumes:
    - /opt/open.ftorrent.com/static:/static:ro
    - /opt/open.ftorrent.com/data:/gauge
    - /sys/fs/cgroup:/host-cgroup:ro
  deploy:
    resources:
      limits:
        memory: 128m
        pids: 32
  restart: unless-stopped
```

The service doesn't need a `ports:` entry — it has no HTTP server and no listening ports. It doesn't need `networks:` either — with the compose network key set to `default`, it joins the `ftorrent-open` network automatically.

### Bind mounts

The gauge container has three bind mounts:

| Host path | Container path | Mode | Purpose |
|---|---|---|---|
| `/opt/open.ftorrent.com/static` | `/static` | read-only | Can't modify the frontend |
| `/opt/open.ftorrent.com/data` | `/gauge` | read-write | Writes `page.json` and `ring.json` |
| `/sys/fs/cgroup` | `/host-cgroup` | read-only | Reads container memory stats |

**The `/gauge` mount has a specific internal layout:**

```
/gauge/                          → /opt/open.ftorrent.com/data/
├── public/
│   └── page.json                → served by nginx at /page.json
│   └── page.json.tmp            → atomic write temp file, briefly exists during writes
└── ring.json                    → private gauge state, never served
```

nginx is configured to serve files from `/opt/open.ftorrent.com/data/public/` at `https://open.ftorrent.com/page.json`. Files outside `public/` (like `ring.json`) are never served — they're the gauge's private working state.

### Verifying the deployment

After `docker compose up -d`:

```bash
# Check the container is running
docker ps | grep gauge

# Check the logs — should show a timestamp every 60 seconds
docker logs -f ftorrent-open-gauge-1

# Check page.json is being written
cat /opt/open.ftorrent.com/data/public/page.json

# Check page.json is served by nginx
curl https://open.ftorrent.com/page.json
```

### Creating the data directory

Before the first run, the data directory needs to exist on the host with the right ownership. The gauge runs as nobody (UID 65534), so the `data/` directory must be writable by that user:

```bash
sudo mkdir -p /opt/open.ftorrent.com/data/public
sudo chown -R 65534:65534 /opt/open.ftorrent.com/data
```

The `static/` directory is already created and owned by the deploy user (rsynced from the Mac). The gauge can read it but not write to it.

## Security separation

The read-only mount of `/static` means the gauge process cannot modify the frontend files (HTML, JS, CSS) that the browser executes. If the gauge is compromised, the worst outcome is wrong numbers on the dashboard — not injected scripts. The frontend code can only be changed by rsyncing from the development machine.

The same container hardening applies as for the Aquatic containers: nobody user, read-only root filesystem, all capabilities dropped, no-new-privileges, memory and PID limits, and no outbound network access (except intra-subnet for Prometheus scraping via the DOCKER-USER rules).
