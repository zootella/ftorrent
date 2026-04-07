import { readFile, readdir, writeFile, rename } from 'node:fs/promises'
import { join } from 'node:path'

// ---------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------

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
const ringPath = join(gaugeDir, 'ring.json')

// Where container cgroup files live. In Docker, /sys/fs/cgroup is mounted
// at /host-cgroup (read-only). Each container has a scope directory:
//   /host-cgroup/system.slice/docker-<64-char-id>.scope/
// containing memory.current (usage in bytes) and memory.max (ceiling).
// Locally this path won't exist, so memory stats are skipped gracefully.
const cgroupDir = process.env.CGROUP_DIR || '/host-cgroup'
const cgroupSlice = join(cgroupDir, 'system.slice')

// ---------------------------------------------------------------------------
// Container identification
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Ring buffer — 1,440 slots, one per minute of the day
// ---------------------------------------------------------------------------

// ring.json structure:
// {
//   "memory": { "udp": 5083136, "http": 27623424, "ws": 33914880 },
//   "minutes": [ { "day": 19820 }, { "day": 19820 }, null, ... ]
// }
//
// "memory" is the latest snapshot, overwritten every tick.
// "minutes" is a fixed-size array of 1,440 slots (minute 0 = 00:00 UTC,
// minute 1439 = 23:59 UTC). Each slot holds { day } where day is
// Math.floor(Date.now() / 86_400_000). A null or missing slot means the
// gauge didn't run during that minute.
//
// To count downtime over the last 24 hours: iterate all 1,440 slots and
// count how many should have been tended but weren't. A slot is "expected"
// if it's either today's day number, or yesterday's day number at a minute
// index after the current minute (those haven't been reached yet today).
// A slot represents downtime if it's expected but null or has an older day.

const MINUTES_PER_DAY = 1440
const MS_PER_DAY = 86_400_000
const MS_PER_MINUTE = 60_000

function currentDay() {
	return Math.floor(Date.now() / MS_PER_DAY)
}

function currentMinute() {
	return Math.floor((Date.now() % MS_PER_DAY) / MS_PER_MINUTE)
}

async function loadRing() {
	try {
		const data = JSON.parse(await readFile(ringPath, 'utf8'))
		// Ensure minutes array exists and is the right size
		if (!Array.isArray(data.minutes) || data.minutes.length !== MINUTES_PER_DAY) {
			data.minutes = new Array(MINUTES_PER_DAY).fill(null)
		}
		return data
	} catch {
		// File doesn't exist or is corrupt — start fresh
		return {
			memory: {},
			minutes: new Array(MINUTES_PER_DAY).fill(null),
		}
	}
}

function countDowntime(minutes) {
	const day = currentDay()
	const minute = currentMinute()
	const yesterday = day - 1

	let downtime = 0
	for (let i = 0; i < MINUTES_PER_DAY; i++) {
		// Determine if this slot is within the last 24 hours.
		// Slots at index > current minute are "yesterday's side" of the ring.
		// Slots at index <= current minute are "today's side."
		const expectedDay = (i <= minute) ? day : yesterday

		const slot = minutes[i]
		if (!slot || slot.day !== expectedDay) {
			downtime++
		}
	}
	return downtime
}

// ---------------------------------------------------------------------------
// Memory reading
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Atomic file writing
// ---------------------------------------------------------------------------

async function writeAtomic(path, data) {
	const tmp = path + '.tmp'
	await writeFile(tmp, data)
	await rename(tmp, path)
}

// ---------------------------------------------------------------------------
// Tick — runs once per minute
// ---------------------------------------------------------------------------

async function tick() {
	const memory = await readTrackerMemory()
	const day = currentDay()
	const minute = currentMinute()

	// Update the ring
	const ring = await loadRing()
	ring.memory = memory
	ring.minutes[minute] = { day }
	await writeAtomic(ringPath, JSON.stringify(ring) + '\n')

	// Count downtime from the ring and build page.json
	const downtime = countDowntime(ring.minutes)
	const page = {
		schema_version: 4,
		generated_at: new Date().toISOString(),
		memory,
		downtime,
	}
	await writeAtomic(
		join(publicDir, 'page.json'),
		JSON.stringify(page, null, '\t') + '\n'
	)

	const time = page.generated_at.replace('T', ' ').replace('Z', '')
	const keys = Object.keys(memory)
	console.log(`page.json written at ${time} — downtime: ${downtime} min, containers: ${keys.length ? keys.join(', ') : 'none'}`)
}

// ---------------------------------------------------------------------------
// Scheduling — aligned to the top of each minute
// ---------------------------------------------------------------------------

function scheduleNextTick() {
	const now = Date.now()
	const nextMinute = Math.ceil(now / MS_PER_MINUTE) * MS_PER_MINUTE
	const delay = nextMinute - now + 100 // 100ms past the boundary
	setTimeout(async () => {
		await tick()
		scheduleNextTick()
	}, delay)
}

// Run immediately on start, then aligned to each minute boundary
await tick()
scheduleNextTick()
