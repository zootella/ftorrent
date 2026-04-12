<script setup>
import { inject, computed, ref, watch, onMounted, onUnmounted } from 'vue'

const page = inject('page')

const SEP = ' ' // digit group separator: ' ' for space, '\u2009' for thin space

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

// Animation frame loop — idempotent start/stop, not refcounted (one feature at a time).
// frameTime updates reactively while running, stays still otherwise — so no Vue work happens
// when nothing is animating, and the browser auto-pauses rAF when the tab is hidden.
const frameTime = ref(0)
let frameHandle = null

function frameTick(t) {
	frameTime.value = t
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

function updateClock() {
	const now = Date.now()
	const d = new Date(now)
	clockHour.value = String(d.getUTCHours()).padStart(2, '0')
	clockMinute.value = String(d.getUTCMinutes()).padStart(2, '0')
	colonOn.value = now % 1000 < 500
}

updateClock()
watch(frameTime, updateClock)

onMounted(() => {
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

			<div class="right">{{ fmt(page.served.udp4) }}</div>
			<div class="right">{{ fmt(page.served.udp6) }}</div>
			<div class="label">UDP announce</div>
			<div class="right">{{ mb(page.memory.udp) }}</div>

			<div class="right">{{ fmt(page.served.http4) }}</div>
			<div class="right">{{ fmt(page.served.http6) }}</div>
			<div class="label">HTTP announce</div>
			<div class="right">{{ mb(page.memory.http) }}</div>

			<div class="right">{{ fmt(page.served.ws4) }}</div>
			<div class="right">{{ fmt(page.served.ws6) }}</div>
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
			<div></div>

			<div class="right">{{ fmt(page.downtime) }}{{ page.downtime ? ' minutes' : '' }}</div>
			<div class="label">Downtime</div>

			<div class="label right">UDP announce</div>
			<div></div>

			<div class="right">{{ fmt(page.served.udp4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ fmt(page.served.udp6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">HTTP announce</div>
			<div></div>

			<div class="right">{{ fmt(page.served.http4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ fmt(page.served.http6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">WebRTC offer</div>
			<div></div>

			<div class="right">{{ fmt(page.served.ws4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ fmt(page.served.ws6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">Memory in use</div>
			<div></div>

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
	align-items: center;
	gap: 0.25rem;
}

.cells {
	display: grid;
	grid-template-columns: repeat(10, 20px);
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
}
</style>
