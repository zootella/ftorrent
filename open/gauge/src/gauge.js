import { readFile, readdir, writeFile, rename } from 'node:fs/promises'
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

// Where container cgroup files live. In Docker, /sys/fs/cgroup is mounted
// at /host-cgroup (read-only). Each container has a scope directory:
//   /host-cgroup/system.slice/docker-<64-char-id>.scope/
// containing memory.current (usage in bytes) and memory.max (ceiling).
// Locally this path won't exist, so memory stats are skipped gracefully.
const cgroupDir = process.env.CGROUP_DIR || '/host-cgroup'
const cgroupSlice = join(cgroupDir, 'system.slice')

// The three Aquatic tracker containers we care about. We identify them
// by their memory ceiling (memory.max from cgroup), which is set in the
// compose file and is deliberately unique per container.
//
// This is a concession: the cgroup tree only exposes container IDs, not
// names, and we chose not to mount the Docker socket (too much privilege)
// or pass IDs as environment variables (they change on every restart).
// Matching by ceiling works because the three containers have intentionally
// different memory limits — 2 GB, 1 GB, and 512 MB — documented in the
// guide and unlikely to collide with other containers on the same host.
//
// If a ceiling is changed in the compose file, update the matching value
// here too, or the gauge will silently stop reporting that container.
const targets = [
	{ key: 'udp',  ceiling: 2 * 1024 * 1024 * 1024 },  // 2 GB — aquatic-udp
	{ key: 'http', ceiling: 512 * 1024 * 1024 },        // 512 MB — aquatic-http
	{ key: 'ws',   ceiling: 1 * 1024 * 1024 * 1024 },   // 1 GB — aquatic-ws
]

// Scan the cgroup tree for Docker containers and match the three Aquatic
// trackers by their memory ceiling. Returns an object like:
//   { udp: 5083136, http: 27623424, ws: 33914880 }
// with memory usage in bytes, or an empty object if cgroups aren't available.
async function readTrackerMemory() {
	const result = {}
	try {
		const entries = await readdir(cgroupSlice)
		for (const entry of entries) {
			if (!entry.startsWith('docker-') || !entry.endsWith('.scope')) continue

			const scopeDir = join(cgroupSlice, entry)
			try {
				const current = parseInt(await readFile(join(scopeDir, 'memory.current'), 'utf8'))
				const max = parseInt(await readFile(join(scopeDir, 'memory.max'), 'utf8'))

				// Match this container to a target by its ceiling
				const target = targets.find(t => t.ceiling === max)
				if (target) {
					result[target.key] = current
				}
			} catch {
				// Skip containers whose cgroup files are unreadable
			}
		}
	} catch {
		// cgroup path doesn't exist (local development) — return empty
	}
	return result
}

async function buildPage() {
	const memory = await readTrackerMemory()
	return {
		schema_version: 3,
		generated_at: new Date().toISOString(),
		memory,
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
	const page = await buildPage()
	await writeAtomic(
		join(publicDir, 'page.json'),
		JSON.stringify(page, null, '\t') + '\n'
	)

	const time = page.generated_at.replace('T', ' ').replace('Z', '')
	const keys = Object.keys(page.memory)
	console.log(`page.json written at ${time} (${keys.length ? keys.join(', ') : 'no containers'})`)
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
