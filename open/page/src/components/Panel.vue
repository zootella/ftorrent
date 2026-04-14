<script setup>

// Time unit constants — milliseconds. Multiply for readable durations: 10 * time_minute, etc.
const time_second = 1000
const time_minute = 60 * time_second
const time_hour   = 60 * time_minute
const time_day    = 24 * time_hour

// Byte size constants — bytes. Divide for readable units: bytes / size_mb, etc.
const size_kb = 1024
const size_mb = size_kb * size_kb

const month_names = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

import { inject, computed, reactive, ref, onMounted, onUnmounted } from 'vue'

const page = inject('page')

const nbsp = '\u00A0' // non-breaking space (same as &nbsp;); used as digit group separator so numbers never wrap

const burst_quantity = 1_000     // counters with rate at or below this animate in discrete bursts
const beat_quantity  = 2_000_000 // sweet-spot rate: ones blur, tens still pop visibly
const drum_duration  = time_second / 2  // colon-blink half-period; also our beat tempo
const reset_duration = 10 * time_minute // how often counters snap back to the recorded total

function group(n) {
	return n.toLocaleString('en-US').replace(/,/g, nbsp) // en-US gives commas every 3 digits; swap to nbsp so it never line-breaks
}

function mb(bytes) {
	return group(Math.ceil(bytes / size_mb)) + nbsp + 'MB'
}

// Parse the history string into an array of downtime minutes
const history = computed(() => {
	if (!page.history) return []
	return page.history.split(',').map(Number)
})

const bar_all  = 100  // 0 minutes downtime
const bar_one  = 87   // 1 minute downtime
const bar_none = 3    // full day downtime

function barHeight(downMinutes) {
	const minutesPerDay = time_day / time_minute
	if (downMinutes <= 0) return bar_all
	if (downMinutes >= minutesPerDay) return bar_none
	return bar_one - (downMinutes - 1) / (minutesPerDay - 1) * (bar_one - bar_none)
}

// Format a day number as "2026 Apr 07"
function dayToDate(dayNumber) {
	const d = new Date(dayNumber * time_day)
	const year = d.getUTCFullYear()
	const month = month_names[d.getUTCMonth()]
	return `${year} ${month} ${String(d.getUTCDate()).padStart(2, '0')}`
}

// Launch day — days before this show "Before launch" instead of downtime.
// Remove this after the 90-day window passes launch day entirely.
const launch_day = 20551 // 2026 Apr 08

// Index in the history array where launch day falls
function launchIndex() {
	const firstDay = page.day - (history.value.length - 1)
	return Math.max(0, launch_day - firstDay)
}

// 90-day uptime percentage — only counts from launch day onward
const uptimePercent = computed(() => {
	const bars = history.value
	const li = launchIndex()
	const postLaunch = bars.slice(li + 1) // exclude launch day (partial data)
	if (postLaunch.length === 0) return (0).toFixed(3)
	// All but the last are complete days; the last is today's partial day
	const totalMinutes = (postLaunch.length - 1) * (time_day / time_minute) + (page.minute + 1)
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

// page.json is snapshotted once, before mount, and never changes during the
// navigation. recorded is the initial display value and the reset target;
// served is the live, animated copy.
const count_keys = ['udp4', 'udp6', 'http4', 'http6', 'ws4', 'ws6']
const recorded = Object.fromEntries(count_keys.map(k => [k, page.served[k]]))
const rates = Object.fromEntries(count_keys.map(k => [k, recorded[k] / time_day])) // events per ms
const served = reactive({ ...recorded })

// Knuth for λ < 20, Box-Muller normal approximation for λ ≥ 20.
// Returns a non-negative integer.
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

// Idempotent start/stop. rAF auto-pauses when the tab is hidden.
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

// UTC HH:MM, 1Hz blinking colon. Re-derived from Date.now() each frame so the
// visible state snaps to wall clock and no drift accumulates.
const clockHour = ref('00')
const clockMinute = ref('00')
const colonOn = ref(true)

// Three brackets by recorded:
//   recorded ≤ burst_quantity         → not animated
//   burst_quantity < recorded ≤ beat  → per-frame Poisson step
//   recorded > beat_quantity          → Poisson step once per drumbeat
// lastBeat: most recent drumbeat number acted on.
// lastFrameNow: previous frame's wall time, for per-frame dt.
// mountedAt: anchor for the reset_duration window.
let lastBeat = null
let lastFrameNow = null
let mountedAt = null

function updateClock() {
	const now = Date.now()
	const d = new Date(now)
	clockHour.value = String(d.getUTCHours()).padStart(2, '0')
	clockMinute.value = String(d.getUTCMinutes()).padStart(2, '0')

	const beat = Math.floor(now / drum_duration)
	colonOn.value = beat % 2 === 0

	if (lastBeat !== null && beat !== lastBeat && mountedAt !== null) {
		if (now - mountedAt >= reset_duration) {
			// Snap back to recorded and re-anchor both clocks so the per-frame
			// block below adds nothing this frame.
			mountedAt = now
			lastFrameNow = now
			for (const k of count_keys) served[k] = recorded[k]
		} else {
			// beatsSince > 1 means the tab was hidden across several beats.
			// Sum of N independent Poisson(λ) is Poisson(N·λ), so one draw
			// covers the whole gap.
			const beatsSince = beat - lastBeat
			for (const k of count_keys) {
				if (recorded[k] > beat_quantity) {
					const arrived = poissonSample(rates[k] * drum_duration * beatsSince)
					if (arrived > 0) served[k] += arrived
				}
			}
		}
	}
	lastBeat = beat

	if (lastFrameNow !== null) {
		const dt = now - lastFrameNow
		for (const k of count_keys) {
			if (recorded[k] > burst_quantity && recorded[k] <= beat_quantity) {
				const arrived = poissonSample(rates[k] * dt)
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

			<div class="right">{{ group(served.udp4) }}</div>
			<div class="right">{{ group(served.udp6) }}</div>
			<div class="label">UDP announce</div>
			<div class="right">{{ mb(page.memory.udp) }}</div>

			<div class="right">{{ group(served.http4) }}</div>
			<div class="right">{{ group(served.http6) }}</div>
			<div class="label">HTTP announce</div>
			<div class="right">{{ mb(page.memory.http) }}</div>

			<div class="right">{{ group(served.ws4) }}</div>
			<div class="right">{{ group(served.ws6) }}</div>
			<div class="label">WebRTC offer</div>
			<div class="right">{{ mb(page.memory.ws) }}</div>

			<div></div>
			<div class="right">{{ group(page.downtime) }}{{ page.downtime ? ' minutes' : '' }}</div>
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
			<div class="label">┐</div>

			<div class="right">{{ group(page.downtime) }}{{ page.downtime ? ' minutes' : '' }}</div>
			<div class="label">Downtime</div>

			<div class="label right">UDP announce</div>
			<div class="label">┐</div>

			<div class="right">{{ group(served.udp4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ group(served.udp6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">HTTP announce</div>
			<div class="label">┐</div>

			<div class="right">{{ group(served.http4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ group(served.http6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">WebRTC offer</div>
			<div class="label">┐</div>

			<div class="right">{{ group(served.ws4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ group(served.ws6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">Memory in use</div>
			<div class="label">┐</div>

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
