<script setup>
import { inject, computed, ref } from 'vue'

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
</script>

<template>
	<div class="lcd-panel">
		<div class="lcd-grid">
			<div class="lcd-label lcd-right">IPv4</div>
			<div class="lcd-label lcd-right">IPv6</div>
			<div class="lcd-label">Past 24 hours</div>
			<div class="lcd-label lcd-right">Memory in use</div>

			<div class="lcd-right">{{ fmt(page.served.udp4) }}</div>
			<div class="lcd-right">{{ fmt(page.served.udp6) }}</div>
			<div class="lcd-label">UDP announce</div>
			<div class="lcd-right">{{ mb(page.memory.udp) }}</div>

			<div class="lcd-right">{{ fmt(page.served.http4) }}</div>
			<div class="lcd-right">{{ fmt(page.served.http6) }}</div>
			<div class="lcd-label">HTTP announce</div>
			<div class="lcd-right">{{ mb(page.memory.http) }}</div>

			<div class="lcd-right">{{ fmt(page.served.ws4) }}</div>
			<div class="lcd-right">{{ fmt(page.served.ws6) }}</div>
			<div class="lcd-label">WebRTC offer</div>
			<div class="lcd-right">{{ mb(page.memory.ws) }}</div>

			<div></div>
			<div class="lcd-right">{{ fmt(page.downtime) }}{{ page.downtime ? ' minutes' : '' }}</div>
			<div class="lcd-label">Downtime</div>
			<div></div>

		</div>

		<div class="history-row">
			<div class="history-bars" @mouseleave="hoveredBar = null">
				<div
					v-for="(down, i) in history"
					:key="i"
					class="history-bar-slot"
					@mouseenter="hoveredBar = i"
				>
					<div class="history-bar" :style="{ height: barHeight(down) + '%' }"></div>
				</div>
			</div>
			<div class="history-label">
				<div v-if="hoveredBar === null" class="history-label-default">
					<span>{{ history.length }} days ago</span>
					<span>{{ uptimePercent }}% uptime</span>
					<span>Today</span>
				</div>
				<div v-else>{{ barDate }} - {{ barDown }}</div>
			</div>
		</div>

		<div class="lcd-narrow">
			<div class="lcd-label lcd-right">Past 24 hours</div>
			<div></div>

			<div class="lcd-right">{{ fmt(page.downtime) }}{{ page.downtime ? ' minutes' : '' }}</div>
			<div class="lcd-label">Downtime</div>

			<div class="lcd-label lcd-right">UDP announce</div>
			<div></div>

			<div class="lcd-right">{{ fmt(page.served.udp4) }}</div>
			<div class="lcd-label">IPv4</div>
			<div class="lcd-right">{{ fmt(page.served.udp6) }}</div>
			<div class="lcd-label">IPv6</div>

			<div class="lcd-label lcd-right">HTTP announce</div>
			<div></div>

			<div class="lcd-right">{{ fmt(page.served.http4) }}</div>
			<div class="lcd-label">IPv4</div>
			<div class="lcd-right">{{ fmt(page.served.http6) }}</div>
			<div class="lcd-label">IPv6</div>

			<div class="lcd-label lcd-right">WebRTC offer</div>
			<div></div>

			<div class="lcd-right">{{ fmt(page.served.ws4) }}</div>
			<div class="lcd-label">IPv4</div>
			<div class="lcd-right">{{ fmt(page.served.ws6) }}</div>
			<div class="lcd-label">IPv6</div>

			<div class="lcd-label lcd-right">Memory in use</div>
			<div></div>

			<div class="lcd-right">{{ mb(page.memory.udp) }}</div>
			<div class="lcd-label">UDP</div>
			<div class="lcd-right">{{ mb(page.memory.http) }}</div>
			<div class="lcd-label">HTTP</div>
			<div class="lcd-right">{{ mb(page.memory.ws) }}</div>
			<div class="lcd-label">WS</div>
		</div>
	</div>
</template>

<style scoped>
.lcd-panel {
	background: #D5D6C8;
	border: 1px solid #BCBDAF;
	border-radius: 6px;
	box-shadow: inset 2px 2px 4px #A8A99C;
	color: #353531;
	text-shadow: 2px 2px 0 #C8C9BC;
	font-family: 'Monaspace Krypton', monospace;
	font-size: 1.6rem;
	font-weight: 400;
	padding: 1rem;
	text-align: center;
}

.lcd-grid {
	display: inline-grid;
	align-items: baseline;
	margin: 0 auto;
	grid-template-columns: auto auto auto auto;
	gap: 0.25rem 2rem;
}

.lcd-label {
	color: #9B9C90;
	font-family: 'Jura', sans-serif;
	font-size: 1.4rem;
	font-weight: 700;
	text-shadow: none;
	text-align: left;
	white-space: nowrap;
}

.lcd-right {
	text-align: right;
}


.lcd-narrow {
	display: none;
	grid-template-columns: auto auto;
	align-items: baseline;
	gap: 0.25rem 1rem;
}


.history-row {
	margin-top: 1.5rem;
	max-width: fit-content;
	margin-left: auto;
	margin-right: auto;
}

.history-bars {
	display: flex;
	align-items: flex-end;
	height: 3.5rem;
	gap: 2px;
}

.history-bar-slot {
	width: 6px;
	flex-shrink: 0;
	height: 100%;
	display: flex;
	align-items: flex-end;
}

.history-bar {
	width: 100%;
	background: #353531;
	box-shadow: 2px 2px 0 #C8C9BC;
	border-radius: 1px;
}

.history-label {
	font-size: 1.6rem;
	text-align: left;
	margin-top: 0.25rem;
}

.history-label-default {
	display: flex;
}

.history-label-default > span {
	flex: 1;
}

.history-label-default > span:nth-child(2) {
	text-align: center;
}

.history-label-default > span:nth-child(3) {
	text-align: right;
}

@media (max-width: 1024px) {
	.lcd-grid {
		display: none;
	}

	.history-row {
		display: none; /* 90-day bars are desktop only */
	}

	.lcd-narrow {
		display: inline-grid;
	}
}
</style>
