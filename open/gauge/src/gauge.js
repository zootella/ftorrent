import { writeFile, rename } from 'node:fs/promises'
import { join } from 'node:path'

// The gauge's working directory. In Docker, this is /gauge (a bind mount
// to /opt/open.ftorrent.com/data on the host). Locally, it defaults to
// the page workspace's public/ parent — so page.json lands in the right
// place for the Vite dev server.
//
// Inside this directory:
//   public/page.json  — the public output, served by nginx
//   ring.json         — private working state, never served
//
// nginx only serves files inside public/. ring.json sits alongside
// public/, not inside it, so it's never visible to visitors.
const gaugeDir = process.env.GAUGE_DIR || '../page'
const publicDir = join(gaugeDir, 'public')

const INTERVAL_MS = 60_000

function buildPage() {
	return {
		schema_version: 1,
		generated_at: new Date().toISOString(),
	}
}

async function writeAtomic(path, data) {
	const tmp = path + '.tmp'
	// Atomic write: write to tmp file, then rename. A reader never
	// sees a half-written file — they get the old version or the new
	// version, never a mix.
	await writeFile(tmp, data)
	await rename(tmp, path)
}

async function tick() {
	const page = buildPage()
	await writeAtomic(
		join(publicDir, 'page.json'),
		JSON.stringify(page, null, '\t') + '\n'
	)

	const time = page.generated_at.replace('T', ' ').replace('Z', '')
	console.log(`page.json written at ${time}`)
}

// Run immediately on start, then every minute
await tick()
setInterval(tick, INTERVAL_MS)
