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

// Schedule ticks aligned to the clock — fire once just after the top
// of each minute. This ensures exactly one tick per calendar minute
// with no drift, no missed minutes, and no double-fires.
function scheduleNextTick() {
	const now = Date.now()
	const nextMinute = Math.ceil(now / 60_000) * 60_000
	const delay = nextMinute - now + 100 // 100ms past the boundary
	setTimeout(async () => {
		await tick()
		scheduleNextTick()
	}, delay)
}

// Run immediately on start, then aligned to each minute boundary
await tick()
scheduleNextTick()
