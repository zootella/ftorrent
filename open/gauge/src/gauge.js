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

// Prometheus endpoints for the three Aquatic containers. These hostnames
// resolve via Docker's internal DNS on the ftorrent-open network.
// Locally these won't resolve, so scraping is skipped gracefully.
const prometheusEndpoints = [
	{ key: 'udp',  url: 'http://ftorrent-open-udp-1:9000/metrics', type: 'announce' },
	{ key: 'http', url: 'http://ftorrent-open-http-1:9000/metrics', type: 'announce' },
	{ key: 'ws',   url: 'http://ftorrent-open-ws-1:9000/metrics',  type: 'offer' },
]

// ---------------------------------------------------------------------------
// Ring buffer — 1,440 slots, one per minute of the day
// ---------------------------------------------------------------------------

// ring.json is an object with a "minutes" array of 1,440 slots. Each slot
// is either null (the gauge didn't run that minute) or an object with:
//   day     — day number (Math.floor(Date.now() / 86_400_000))
//   served  — cumulative counter snapshot: { udp4, udp6, http4, http6, ws4, ws6 }
//
// Slot 0 is 00:00 UTC, slot 1439 is 23:59 UTC.
//
// To count downtime: slots at or before the current minute should have
// today's day number. Slots after the current minute should have
// yesterday's day number. Anything else (null or older) is a missed minute.
//
// To get 24-hour served totals: subtract the oldest valid slot's counters
// from the current slot's counters. Since Prometheus counters are cumulative,
// one subtraction per metric gives the exact total for the period between
// those two snapshots. If a container restarted (current < oldest), the
// counter reset — report 0 rather than fabricating a number.

const MINUTES_PER_DAY = 1440
const MS_PER_DAY = 86_400_000
const MS_PER_MINUTE = 60_000

// The six counter keys stored in each ring slot
const SERVED_KEYS = ['udp4', 'udp6', 'http4', 'http6', 'ws4', 'ws6']

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

// To get "served in the last 24 hours": find the oldest valid slot in
// the ring (the one closest to 24 hours ago) and subtract its counters
// from the current counters. The ring may have gaps — that's fine, we
// just need any two valid points to subtract.
//
// A slot is "valid" (within the last 24 hours) if its day number matches
// what we'd expect for its position relative to the current minute:
// slots at or before the current minute should be today, slots after
// should be yesterday. Anything older is stale.
//
// If no valid oldest slot exists (fresh start, or long outage), we can't
// compute a delta — return 0 for everything.
function computeServed24h(currentServed, minutes, day, minute) {
	// Walk from the farthest point back toward the current minute,
	// return the first (oldest) valid slot we find
	let oldest = null
	for (let offset = MINUTES_PER_DAY - 1; offset >= 1; offset--) {
		const i = (minute - offset + MINUTES_PER_DAY) % MINUTES_PER_DAY
		const slot = minutes[i]
		if (!slot) continue
		const expectedDay = (i <= minute) ? day : day - 1
		if (slot.day === expectedDay) { oldest = slot; break }
	}

	// Every number in page.json must be the growth between two ring
	// slots — we never report a raw Prometheus counter. If there's no
	// oldest slot, or the counter dropped (container restarted), report 0.
	const result = {}
	for (const key of SERVED_KEYS) {
		const current = currentServed[key] || 0
		const old = oldest?.served?.[key] || 0
		result[key] = (oldest && current >= old) ? current - old : 0
	}
	return result
}

// ---------------------------------------------------------------------------
// Prometheus scraping
// ---------------------------------------------------------------------------

// Parse Prometheus text format into a flat object of metric values.
function parsePrometheus(text) {
	const metrics = {}
	for (const line of text.split('\n')) {
		if (line.startsWith('#') || line.trim() === '') continue
		const match = line.match(/^(\S+)\s+(\S+)$/)
		if (match) {
			metrics[match[1]] = parseFloat(match[2])
		}
	}
	return metrics
}

// Extract counts by ip_version for a given response type.
// Returns { v4: number, v6: number }, always both keys present.
function extractByType(metrics, type) {
	let v4 = 0, v6 = 0
	for (const [key, value] of Object.entries(metrics)) {
		if (!key.includes('aquatic_responses_total')) continue
		if (!key.includes(`type="${type}"`)) continue
		if (key.includes('ip_version="4"')) v4 = value
		if (key.includes('ip_version="6"')) v6 = value
	}
	return { v4, v6 }
}

// Scrape all three Prometheus endpoints. Returns cumulative counters as
// a flat object: { udp4, udp6, http4, http6, ws4, ws6 }
// All six keys are always present, defaulting to 0.
async function scrapePrometheus() {
	const result = {}
	for (const key of SERVED_KEYS) result[key] = 0

	for (const { key, url, type } of prometheusEndpoints) {
		try {
			const response = await fetch(url, { signal: AbortSignal.timeout(5000) })
			const text = await response.text()
			const metrics = parsePrometheus(text)
			const counts = extractByType(metrics, type)
			result[`${key}4`] = counts.v4
			result[`${key}6`] = counts.v6
		} catch {
			// Endpoint unreachable (local development or container down)
		}
	}
	return result
}

// ---------------------------------------------------------------------------
// Memory reading
// ---------------------------------------------------------------------------

// Returns { udp, http, ws } with memory in bytes, all keys always present.
async function readTrackerMemory() {
	const result = { udp: 0, http: 0, ws: 0 }
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
	const served = await scrapePrometheus()
	const day = currentDay()
	const minute = currentMinute()

	// Update the ring with current cumulative counters
	const ring = await loadRing()
	ring.minutes[minute] = { day, served }
	await writeAtomic(ringPath, JSON.stringify(ring) + '\n')

	// Build page.json — all fields always present
	const downtime = countDowntime(ring.minutes, day, minute)
	const served24h = computeServed24h(served, ring.minutes, day, minute)
	const page = { minute, memory, served: served24h, downtime }
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
