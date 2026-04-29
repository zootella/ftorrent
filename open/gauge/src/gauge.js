import { readFile, readdir, writeFile, rename, stat } from 'node:fs/promises'
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
const daysPath = join(gaugeDir, 'days.json')

// The internet reachability probe file. A cron job on the host pings an
// external IP every minute and touches this file on success. The gauge
// checks its mtime — if it's stale, the server can't reach the internet
// and this minute doesn't count as up. See probe.sh and the README.
const probePath = join(gaugeDir, 'probe')
const PROBE_MAX_AGE_MS = 90_000 // 90 seconds — absorbs jitter between cron and gauge

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
// To get 24-hour served totals: walk backward from the current slot N,
// extending a candidate oldest slot P back one step at a time as long as
// each older slot's counters are all ≤ the current P's counters. A
// counter that's higher at an older slot than at the current P is the
// reset signature of a container restart between them — stop there, so
// P stays in the same era as N. Report N − P per metric. If no older
// slot qualified (first tick after a restart, or a freshly wiped ring),
// P stays equal to N and N − P is zero. Every reported number is proven
// growth between two different ring slots.

const MINUTES_PER_DAY = 1440
const MS_PER_DAY = 86_400_000
const MS_PER_MINUTE = 60_000

// The six counter keys stored in each ring slot
const SERVED_KEYS = ['udp4', 'udp6', 'http4', 'http6', 'ws4', 'ws6']

function currentDay(now) {
	return Math.floor(now / MS_PER_DAY)
}

function currentMinute(now) {
	return Math.floor((now % MS_PER_DAY) / MS_PER_MINUTE)
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
// Days — 90-day uptime history
// ---------------------------------------------------------------------------

// days.json is a sparse object keyed by day number. Values are up-minutes
// (0–1440). Only days when the gauge actually ran have entries. Missing
// keys mean the server was fully down that day — no process was running
// to write a record, and absence of evidence is treated as downtime.

const HISTORY_DAYS = 90

// Launch day — days before this are excluded from uptime stats. The page
// shows them as "Before launch" and "Launch day" in the bar chart, and
// excludes the launch day itself from the percentage as partial data.
// Remove this once the 90-day window has fully passed launch day.
const LAUNCH_DAY = 20551 // 2026 Apr 08

async function loadDays() {
	try {
		return JSON.parse(await readFile(daysPath, 'utf8'))
	} catch {
		return {}
	}
}

// Finalize completed days: scan the ring for day numbers that are in the
// past and not yet recorded in days.json. Count up-minutes for each and
// write the entry. This runs before the ring slot is written, so stale
// slots from past days haven't been overwritten yet.
async function finalizeCompletedDays(minutes, day, days) {
	// Collect distinct past day numbers from the ring
	const pastDays = new Set()
	for (const slot of minutes) {
		if (slot && slot.day < day) pastDays.add(slot.day)
	}

	let changed = false
	for (const d of pastDays) {
		if (d in days) continue // already finalized
		let up = 0
		for (const slot of minutes) {
			if (slot && slot.day === d) up++
		}
		days[d] = up
		changed = true
	}

	if (changed) {
		// Trim days older than 90 days ago
		const cutoff = day - HISTORY_DAYS
		for (const key of Object.keys(days)) {
			if (parseInt(key) < cutoff) delete days[key]
		}
		await writeAtomic(daysPath, JSON.stringify(days, null, '\t') + '\n')
	}
}

// Build the 90-day history of per-day downtime minutes. Index 0 is 89 days
// ago, last entry is today. Completed days come from days.json (missing =
// 1440 downtime). Today is live: (minutes elapsed) - (up-minutes in ring).
// Returns an array; the caller joins to a string for page.json and reuses
// the array to compute uptime.
function buildHistory(minutes, day, minute, days) {
	const values = []

	// 89 completed days, oldest first
	for (let i = HISTORY_DAYS - 1; i >= 1; i--) {
		const d = day - i
		const up = days[d] ?? 0
		values.push(MINUTES_PER_DAY - up)
	}

	// Today: live count from the ring
	let upToday = 0
	for (const slot of minutes) {
		if (slot && slot.day === day) upToday++
	}
	values.push((minute + 1) - upToday)

	return values
}

// Mirrors Panel.vue's uptimePercent: percentage of post-launch minutes
// that were up. The launch day itself is excluded as partial data; today
// contributes only the elapsed minutes so far. Returns null when the
// window has no post-launch days yet (launch is today or in the future).
function computeUptime(historyValues, day, minute) {
	const firstDay = day - (historyValues.length - 1)
	const launchIndex = LAUNCH_DAY < firstDay ? -1 : LAUNCH_DAY - firstDay
	const postLaunch = historyValues.slice(launchIndex + 1)
	if (postLaunch.length === 0) return null
	const totalMinutes = (postLaunch.length - 1) * MINUTES_PER_DAY + (minute + 1)
	const totalDown = postLaunch.reduce((sum, d) => sum + d, 0)
	return ((totalMinutes - totalDown) / totalMinutes) * 100
}

// Diff the immediately prior ring slot against the current scrape to get
// per-minute served counts. Returns zeros when the prior slot is missing,
// stale (wrong day for its index — e.g. server was down through that
// minute), or shows a reset signature (any key higher in the prior slot
// than now, meaning Aquatic restarted between them). Same conservative
// stance as computeServed24h: never report more than a real same-era diff.
function computeServed1m(currentServed, minutes, day, minute) {
	const result = {}
	for (const key of SERVED_KEYS) result[key] = 0

	const i = (minute - 1 + MINUTES_PER_DAY) % MINUTES_PER_DAY
	const slot = minutes[i]
	if (!slot) return result
	const expectedDay = (i <= minute) ? day : day - 1
	if (slot.day !== expectedDay) return result

	const isReset = SERVED_KEYS.some(k =>
		(slot.served[k] || 0) > (currentServed[k] || 0)
	)
	if (isReset) return result

	for (const key of SERVED_KEYS) {
		result[key] = (currentServed[key] || 0) - (slot.served[key] || 0)
	}
	return result
}

// Walk backward from the current minute, extending a candidate oldest
// slot P back one step at a time as long as each older slot's counters
// are all ≤ the current P's counters. A counter higher at an older
// slot than at the current P is the reset signature of a container
// restart between them — we stop there, keeping P in the same era as
// the current scrape. A slot is only considered if its stored day
// number matches what we'd expect at that index: slots at or before
// the current minute should hold today, slots after should hold
// yesterday; anything else is stale.
function computeServed24h(currentServed, minutes, day, minute) {
	// P starts with the current scrape so the loop can uniformly compare
	// older slots against it. If no older slot extends P, the final
	// subtraction yields zero for every key.
	let P = { served: currentServed }
	for (let offset = 1; offset < MINUTES_PER_DAY; offset++) {
		const i = (minute - offset + MINUTES_PER_DAY) % MINUTES_PER_DAY
		const slot = minutes[i]
		if (!slot) continue
		const expectedDay = (i <= minute) ? day : day - 1
		if (slot.day !== expectedDay) continue

		const isReset = SERVED_KEYS.some(k =>
			(slot.served[k] || 0) > (P.served[k] || 0)
		)
		if (isReset) break
		P = slot
	}

	const result = {}
	for (const key of SERVED_KEYS) {
		result[key] = (currentServed[key] || 0) - (P.served[key] || 0)
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
// Probe check
// ---------------------------------------------------------------------------

// Returns true if the probe file was touched within the last 90 seconds.
// Returns false if the file doesn't exist or is stale (local dev, or ISP down).
async function probeIsFresh(now) {
	try {
		const { mtimeMs } = await stat(probePath)
		return (now - mtimeMs) < PROBE_MAX_AGE_MS
	} catch {
		return false
	}
}

// ---------------------------------------------------------------------------
// Tick — runs once per minute
// ---------------------------------------------------------------------------

async function tick() {
	// Capture one clock reading and pass it everywhere downstream so
	// every derived value (probe freshness, day, minute) sees the same
	// instant. This rules out a sub-millisecond straddle of the day or
	// minute boundary producing inconsistent numbers within a tick.
	const now = Date.now()

	// If the server can't reach the internet, skip this tick entirely.
	// No ring slot is written, so countDowntime counts this minute as down.
	if (!await probeIsFresh(now)) return

	const memory = await readTrackerMemory()
	const served = await scrapePrometheus()
	const day = currentDay(now)
	const minute = currentMinute(now)

	// Finalize any completed days before writing to the ring,
	// so stale slots from past days haven't been overwritten yet
	const ring = await loadRing()
	const days = await loadDays()
	await finalizeCompletedDays(ring.minutes, day, days)

	// Update the ring with current cumulative counters
	ring.minutes[minute] = { day, served }
	await writeAtomic(ringPath, JSON.stringify(ring) + '\n')

	// Build page.json — all fields always present
	const downtime = countDowntime(ring.minutes, day, minute)
	const historyValues = buildHistory(ring.minutes, day, minute, days)
	const history = historyValues.join(',')
	const uptime = computeUptime(historyValues, day, minute)
	const served24h = computeServed24h(served, ring.minutes, day, minute)
	const served1m = computeServed1m(served, ring.minutes, day, minute)
	const page = {
		title: 'open.ftorrent.com live tracker performance record, fresh each minute',
		image: 'https://open.ftorrent.com/images/open.ftorrent.com.jpg',
		epoch: now,
		day, minute, memory, served: served24h, served1minute: served1m, downtime, uptime90days: uptime, history,
	}
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
