
_ftorrent/site/README.md [ftorrent.com](https://ftorrent.com)_

# Home Page

> Prepared by [Claude Code](https://claude.ai/code) using Opus 4.6
> <br>Created: 2026-Apr
> <br>Last reviewed: 2026-Apr

The apex domain [ftorrent.com](https://ftorrent.com/) serves the static files in `public/`.

## Deployment

The contents of `public/` are rsynced to the server. An `upload.hide.sh` script handles the rsync. This script is gitignored (via the `*.hide.*` pattern) because it contains your server's SSH details. Create your own:

```bash
#!/bin/bash
rsync -avz --delete public/ youruser@yourserver:/opt/ftorrent.com/static/ -e "ssh -p 22"
```

Replace `youruser`, `yourserver`, the path, and the SSH port with your own. Make it executable with `chmod +x upload.hide.sh`.

Then deploy with:

```bash
pnpm upload
```

The trailing slash on `public/` copies the contents of the directory rather than the directory itself, and `--delete` keeps the deployed directory matching `public/` exactly.
