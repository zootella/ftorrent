<script setup>
import { inject, computed, reactive, ref, onMounted, onUnmounted } from 'vue'

const page = inject('page')

const SEP = '\u00A0' // digit group separator: non-breaking space so numbers never wrap

const SECTION_MARKER = '┐' // visual indicator before each section header in narrow tally

const BURST = 1_000     // counters with rate at or below this animate in discrete bursts
const BEAT  = 2_000_000 // sweet-spot rate: ones blur, tens still pop visibly
const DRUM  = 500       // ms — colon-blink half-period; also our beat tempo
const RESET = 600_000   // ms — 10 minutes; how often counters snap back to the recorded total

function fmt(n) {
	return n.toLocaleString('en-US').replace(/,/g, SEP)
}

function mb(bytes) {
	return fmt(Math.ceil(bytes / 1048576)) + SEP + 'MB'
}

// Parse the history string into an array of downtime minutes
const history = computed(() => {
	if (!page.history) return []
	return page.history.split(',').map(Number)
})

const BAR_FULL = 100  // 0 minutes downtime
const BAR_ONE  = 87   // 1 minute downtime
const BAR_MIN  = 3    // full day downtime

function barHeight(downMinutes) {
	if (downMinutes <= 0) return BAR_FULL
	if (downMinutes >= 1440) return BAR_MIN
	return BAR_ONE - (downMinutes - 1) / (1440 - 1) * (BAR_ONE - BAR_MIN)
}

// Format a day number as "2026 Apr 07"
const MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

function dayToDate(dayNumber) {
	const d = new Date(dayNumber * 86_400_000)
	const year = d.getUTCFullYear()
	const month = MONTHS[d.getUTCMonth()]
	return `${year} ${month} ${String(d.getUTCDate()).padStart(2, '0')}`
}

// Launch day — days before this show "Before launch" instead of downtime.
// Remove this after the 90-day window passes launch day entirely.
const LAUNCH_DAY = 20551 // 2026 Apr 08

// Index in the history array where launch day falls
function launchIndex() {
	const firstDay = page.day - (history.value.length - 1)
	return Math.max(0, LAUNCH_DAY - firstDay)
}

// 90-day uptime percentage — only counts from launch day onward
const uptimePercent = computed(() => {
	const bars = history.value
	const li = launchIndex()
	const postLaunch = bars.slice(li + 1) // exclude launch day (partial data)
	if (postLaunch.length === 0) return (0).toFixed(3)
	// All but the last are complete days; the last is today's partial day
	const totalMinutes = (postLaunch.length - 1) * 1440 + (page.minute + 1)
	const totalDown = postLaunch.reduce((sum, d) => sum + d, 0)
	const pct = ((totalMinutes - totalDown) / totalMinutes) * 100
	return (Math.floor(pct * 1000) / 1000).toFixed(3)
})

// Hover state: index of hovered bar, or null
const hoveredBar = ref(null)

// Text shown below the bars on hover
const barDate = computed(() => {
	if (hoveredBar.value === null) return ''
	return dayToDate(page.day - (history.value.length - 1 - hoveredBar.value))
})

const barDown = computed(() => {
	if (hoveredBar.value === null) return ''
	const i = hoveredBar.value
	const li = launchIndex()
	if (i < li) return 'Before launch'
	if (i === li) return 'Launch day'
	const down = history.value[i]
	return `${down} minutes downtime`
})

// Simulated counts — recorded 24h totals from page.json grow upward in
// real time so the dashboard feels alive. page.json is fetched in main.js
// before mount and never changes during the navigation, so we snapshot the
// six served counts once here and use them as both the initial display value
// and the reset target.
const COUNT_KEYS = ['udp4', 'udp6', 'http4', 'http6', 'ws4', 'ws6']
const RECORDED = Object.fromEntries(COUNT_KEYS.map(k => [k, page.served[k]]))
const RATES = Object.fromEntries(COUNT_KEYS.map(k => [k, RECORDED[k] / 86_400_000])) // events per ms
const served = reactive({ ...RECORDED })

// Poisson sample: Knuth's algorithm for small λ, normal approximation
// (Box-Muller) for λ ≥ 20. Returns a non-negative integer.
function poissonSample(lambda) {
	if (lambda === 0) return 0
	if (lambda < 20) {
		const L = Math.exp(-lambda)
		let k = 0
		let p = 1
		do {
			k++
			p *= Math.random()
		} while (p > L)
		return k - 1
	}
	const u1 = Math.random()
	const u2 = Math.random()
	const gaussian = Math.sqrt(-2 * Math.log(u1)) * Math.cos(2 * Math.PI * u2)
	return Math.max(0, Math.round(lambda + Math.sqrt(lambda) * gaussian))
}

// Animation frame loop — idempotent start/stop, not refcounted (one feature at a time).
// rAF is auto-paused by the browser when the tab is hidden, so we do zero work then.
let frameHandle = null

function frameTick() {
	updateClock()
	frameHandle = requestAnimationFrame(frameTick)
}

function startFrames() {
	if (frameHandle !== null) return
	frameHandle = requestAnimationFrame(frameTick)
}

function stopFrames() {
	if (frameHandle === null) return
	cancelAnimationFrame(frameHandle)
	frameHandle = null
}

// Clock — UTC HH:MM with a 1Hz blinking colon. Re-derived from Date.now() each frame
// so the visible state always snaps to wall clock and no drift can accumulate. Vue skips
// DOM updates when ref values don't change, so the hour/minute spans only repaint when
// they actually change, and the colon only twice a second.
const clockHour = ref('00')
const clockMinute = ref('00')
const colonOn = ref(true)

// Drumbeat — every DRUM ms, the colon flips and the high-rate counters
// (recorded > BEAT) take a Poisson-distributed step. Mid-rate counters
// (BURST < recorded ≤ BEAT) tick every frame instead, because at those
// rates a drumbeat-cadence step would arrive as a visible chunk; per-frame
// steps let each event pop up as it conceptually arrives. Low-rate counters
// (recorded ≤ BURST) don't animate at all — at that scale, a single
// simulated event would be a visible jump on a counter the viewer might
// otherwise read as a precise figure, so we just show the recorded value.
// lastBeat is the most recent drumbeat number we've acted on; lastFrameNow
// is the previous frame's wall time; mountedAt anchors the RESET window.
let lastBeat = null
let lastFrameNow = null
let mountedAt = null

function updateClock() {
	const now = Date.now()
	const d = new Date(now)
	clockHour.value = String(d.getUTCHours()).padStart(2, '0')
	clockMinute.value = String(d.getUTCMinutes()).padStart(2, '0')

	const beat = Math.floor(now / DRUM)
	colonOn.value = beat % 2 === 0

	if (lastBeat !== null && beat !== lastBeat && mountedAt !== null) {
		if (now - mountedAt >= RESET) {
			// Snap back to the recorded total and re-anchor both growth clocks
			// so the per-frame block below adds nothing on this frame and
			// counters resume cleanly from RECORDED.
			mountedAt = now
			lastFrameNow = now
			for (const k of COUNT_KEYS) served[k] = RECORDED[k]
		} else {
			// Multiply by beats elapsed so a thawed tab adds the events that
			// would have arrived during the hide, not just one beat's worth.
			// Sum of N independent Poisson(λ) is Poisson(N·λ), so this is
			// statistically identical to having ticked N separate beats.
			const beatsSince = beat - lastBeat
			for (const k of COUNT_KEYS) {
				if (RECORDED[k] > BEAT) {
					const arrived = poissonSample(RATES[k] * DRUM * beatsSince)
					if (arrived > 0) served[k] += arrived
				}
			}
		}
	}
	lastBeat = beat

	if (lastFrameNow !== null) {
		const dt = now - lastFrameNow
		for (const k of COUNT_KEYS) {
			if (RECORDED[k] > BURST && RECORDED[k] <= BEAT) {
				const arrived = poissonSample(RATES[k] * dt)
				if (arrived > 0) served[k] += arrived
			}
		}
	}
	lastFrameNow = now
}

updateClock()

onMounted(() => {
	mountedAt = Date.now()
	startFrames()
})

onUnmounted(() => {
	stopFrames()
})
</script>

<template>
	<div class="well">
		<div class="clock">UTC {{ clockHour }}<span :class="{ off: !colonOn }">:</span>{{ clockMinute }}</div>

		<div class="tally-wide">
			<div class="label right">IPv4</div>
			<div class="label right">IPv6</div>
			<div class="label">Past 24 hours</div>
			<div class="label right">Memory in use</div>

			<div class="right">{{ fmt(served.udp4) }}</div>
			<div class="right">{{ fmt(served.udp6) }}</div>
			<div class="label">UDP announce</div>
			<div class="right">{{ mb(page.memory.udp) }}</div>

			<div class="right">{{ fmt(served.http4) }}</div>
			<div class="right">{{ fmt(served.http6) }}</div>
			<div class="label">HTTP announce</div>
			<div class="right">{{ mb(page.memory.http) }}</div>

			<div class="right">{{ fmt(served.ws4) }}</div>
			<div class="right">{{ fmt(served.ws6) }}</div>
			<div class="label">WebRTC offer</div>
			<div class="right">{{ mb(page.memory.ws) }}</div>

			<div></div>
			<div class="right">{{ fmt(page.downtime) }}{{ page.downtime ? ' minutes' : '' }}</div>
			<div class="label">Downtime</div>
			<div></div>

		</div>

		<div class="calendar-wide">
			<div class="bars" @mouseleave="hoveredBar = null">
				<div
					v-for="(down, i) in history"
					:key="i"
					class="bar-slot"
					@mouseenter="hoveredBar = i"
				>
					<div class="bar" :style="{ height: barHeight(down) + '%' }"></div>
				</div>
			</div>
			<div class="legend">
				<div v-if="hoveredBar === null" class="legend-default">
					<span>{{ history.length }} days ago</span>
					<span>{{ uptimePercent }}% uptime</span>
					<span>Today</span>
				</div>
				<div v-else>{{ barDate }} - {{ barDown }}</div>
			</div>
		</div>

		<div class="tally-narrow">
			<div class="label right">Past 24 hours</div>
			<div class="label">{{ SECTION_MARKER }}</div>

			<div class="right">{{ fmt(page.downtime) }}{{ page.downtime ? ' minutes' : '' }}</div>
			<div class="label">Downtime</div>

			<div class="label right">UDP announce</div>
			<div class="label">{{ SECTION_MARKER }}</div>

			<div class="right">{{ fmt(served.udp4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ fmt(served.udp6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">HTTP announce</div>
			<div class="label">{{ SECTION_MARKER }}</div>

			<div class="right">{{ fmt(served.http4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ fmt(served.http6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">WebRTC offer</div>
			<div class="label">{{ SECTION_MARKER }}</div>

			<div class="right">{{ fmt(served.ws4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ fmt(served.ws6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">Memory in use</div>
			<div class="label">{{ SECTION_MARKER }}</div>

			<div class="right">{{ mb(page.memory.udp) }}</div>
			<div class="label">UDP</div>
			<div class="right">{{ mb(page.memory.http) }}</div>
			<div class="label">HTTP</div>
			<div class="right">{{ mb(page.memory.ws) }}</div>
			<div class="label">WS</div>
		</div>

		<div class="calendar-narrow">
			<div class="legend">{{ hoveredBar === null ? '90 days' : barDate }}</div>
			<div class="cells" @mouseleave="hoveredBar = null">
				<div
					v-for="(down, i) in history"
					:key="i"
					class="cell"
					:class="{ imperfect: down > 0 }"
					@mouseenter="hoveredBar = i"
				></div>
			</div>
			<div class="legend">{{ hoveredBar === null ? uptimePercent + '% uptime' : barDown }}</div>
		</div>
	</div>
</template>

<style scoped>
.well {
	display: flex;
	flex-direction: column;
	align-items: center;
	gap: 1.5rem;
	width: max-content;
	background: #D5D6C8;
	border: 1px solid #BCBDAF;
	border-radius: 6px;
	box-shadow: inset 2px 2px 4px #A8A99C;
	color: #353531;
	text-shadow: 2px 2px 0 #C8C9BC;
	font-family: 'Monaspace Krypton', monospace;
	font-size: 1.6rem;
	font-weight: 400;
	padding: 1rem 3rem;
	text-align: center;
}

.clock {
	align-self: flex-end;
}

.clock .off {
	opacity: 0;
}

.tally-wide {
	display: grid;
	align-items: baseline;
	grid-template-columns: auto auto auto auto;
	gap: 0.25rem 2rem;
}

.label {
	color: #9B9C90;
	font-family: 'Jura', sans-serif;
	font-size: 1.4rem;
	font-weight: 700;
	text-shadow: none;
	text-align: left;
	white-space: nowrap;
}

.right {
	text-align: right;
}


.tally-narrow {
	display: none;
	align-items: baseline;
	grid-template-columns: auto auto;
	gap: 0.25rem 1rem;
}


.bars {
	display: flex;
	align-items: flex-end;
	height: 3.5rem;
	gap: 2px;
}

.bar-slot {
	width: 6px;
	flex-shrink: 0;
	height: 100%;
	display: flex;
	align-items: flex-end;
}

.bar {
	width: 100%;
	background: #353531;
	box-shadow: 2px 2px 0 #C8C9BC;
	border-radius: 1px;
}

.legend {
	font-size: 1.6rem;
	text-align: left;
	margin-top: 0.25rem;
}

.legend-default {
	display: flex;
}

.legend-default > span {
	flex: 1;
}

.legend-default > span:nth-child(2) {
	text-align: center;
}

.legend-default > span:nth-child(3) {
	text-align: right;
}

.calendar-narrow {
	display: none;
	flex-direction: column;
	align-items: flex-start;
	gap: 0.25rem;
}

.cells {
	display: grid;
	grid-template-columns: repeat(15, 20px);
	grid-auto-rows: 20px;
	gap: 2px;
}

.cell {
	background: #353531;
	box-shadow: 2px 2px 0 #C8C9BC;
	border-radius: 1px;
}

.cell.imperfect {
	background: transparent;
	box-shadow: none;
}

@media (max-width: 1024px) {
	.tally-wide {
		display: none;
	}

	.calendar-wide {
		display: none; /* 90-day bars are desktop only */
	}

	.tally-narrow {
		display: grid;
	}

	.calendar-narrow {
		display: flex;
	}

	.well {
		padding: 1rem;
	}
}
</style>
