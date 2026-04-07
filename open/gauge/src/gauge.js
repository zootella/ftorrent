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
const gaugeDir = process.env.GAUGE_DIR || '../page'
const publicDir = join(gaugeDir, 'public')
const ringPath = join(gaugeDir, 'ring.json')

// Where container cgroup files live. In Docker, /sys/fs/cgroup is mounted
// at /host-cgroup (read-only). Locally this path won't exist, so memory
// stats are skipped gracefully.
const cgroupDir = process.env.CGROUP_DIR || '/host-cgroup'
const cgroupSlice = join(cgroupDir, 'system.slice')

// ---------------------------------------------------------------------------
// Container identification
// ---------------------------------------------------------------------------

// We identify the three Aquatic containers by their memory ceiling
// (memory.max from cgroup), which is deliberately unique per container.
//
// This is a concession: the cgroup tree only exposes container IDs, not
// names. We chose not to mount the Docker socket (too much privilege) or
// pass IDs as environment variables (they change on every restart).
// Matching by ceiling works because the limits are intentionally different
// and documented in the guide.
const targets = [
	{ key: 'udp',  ceiling: 2 * 1024 * 1024 * 1024 },  // 2 GB
	{ key: 'http', ceiling: 512 * 1024 * 1024 },        // 512 MB
	{ key: 'ws',   ceiling: 1 * 1024 * 1024 * 1024 },   // 1 GB
]

// ---------------------------------------------------------------------------
// Ring buffer — 1,440 slots, one per minute of the day
// ---------------------------------------------------------------------------

// ring.json is an object with a "minutes" array of 1,440 slots. Each slot
// is either null (the gauge didn't run that minute) or { day } where day
// is the day number (Math.floor(Date.now() / 86_400_000)). Slot 0 is
// 00:00 UTC, slot 1439 is 23:59 UTC. The outer object leaves room for
// future fields alongside the ring.
//
// To count downtime: slots at or before the current minute should have
// today's day number. Slots after the current minute should have
// yesterday's day number. Anything else (null or older) is a missed minute.

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
		if (!Array.isArray(data.minutes) || data.minutes.length !== MINUTES_PER_DAY) {
			data.minutes = new Array(MINUTES_PER_DAY).fill(null)
		}
		return data
	} catch {
		return { minutes: new Array(MINUTES_PER_DAY).fill(null) }
	}
}

function countDowntime(minutes, day, minute) {
	let downtime = 0
	for (let i = 0; i < MINUTES_PER_DAY; i++) {
		const expectedDay = (i <= minute) ? day : day - 1
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

				const target = targets.find(t => t.ceiling === max)
				if (target) {
					result[target.key] = current
				}
			} catch {
				// Skip unreadable cgroup files
			}
		}
	} catch {
		// cgroup path doesn't exist (local development)
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
	ring.minutes[minute] = { day }
	await writeAtomic(ringPath, JSON.stringify(ring) + '\n')

	// Build page.json
	const downtime = countDowntime(ring.minutes, day, minute)
	const page = { minute, memory, downtime }
	await writeAtomic(
		join(publicDir, 'page.json'),
		JSON.stringify(page, null, '\t') + '\n'
	)
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

await tick()
scheduleNextTick()
