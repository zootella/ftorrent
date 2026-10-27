<script setup>

import {inject, computed, reactive, ref, onMounted, onUnmounted} from 'vue'
import {Time, saySize4, sayTick} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go

const page = inject('page')

const nbsp = '\u00A0'//a non-breaking space, the same as &nbsp;, which group() puts between digit groups so a number never wraps

const burstQuantity = 1_000            //counters with rate at or below this animate in discrete bursts
const beatQuantity  = 2_000_000        //sweet-spot rate: ones blur, tens still pop visibly
const drumDuration  = Time.second / 2  //colon-blink half-period; also our beat tempo
const resetDuration = 10 * Time.minute //how often counters snap back to the recorded total

//per-service cool-down, true for a service whose breaker has tripped: it shows coolDownText instead of an animated counter, and the per-frame and per-beat Poisson updates skip it
let coolDown = page.coolDown
const coolDownText = 'cool down'

//a count with its digits grouped by non-breaking spaces, so it never line-breaks
function group(n) {
	return n.toLocaleString('en-US').replace(/,/g, nbsp)//en-US groups digits in threes with commas, which become non-breaking spaces
}

let history = page.downtimeDays.split(',').map(Number)//the 90-day downtime string as an array of per-day downtime minutes

const barAll  = 100//0 minutes downtime
const barOne  = 97 //1 minute downtime
const barNone = 3  //full day downtime

function barHeight(downMinutes) {
	let minutesPerDay = Time.day / Time.minute
	if (downMinutes <= 0) return barAll
	if (downMinutes >= minutesPerDay) return barNone
	return barOne - (downMinutes - 1) / (minutesPerDay - 1) * (barOne - barNone)
}

//a day number as a date, like 2026 Apr 07
function dayToDate(dayNumber) {
	return sayTick(dayNumber * Time.day, 'YYYY Mmm DD')
}

//90-day uptime percentage; all but the last are complete days, and the last is today's partial day
let totalMinutes = (history.length - 1) * (Time.day / Time.minute) + (page.minute + 1)
let totalDown = history.reduce((sum, d) => sum + d, 0)
let uptimePercent = (Math.floor(((totalMinutes - totalDown) / totalMinutes) * 100 * 1000) / 1000).toFixed(3)

const hoveredBar = ref(null)//index of the hovered bar, or null

//text shown below the bars on hover
const barDate = computed(() => {
	if (hoveredBar.value == null) return ''
	return dayToDate(page.day - (history.length - 1 - hoveredBar.value))
})

const barDown = computed(() => {
	if (hoveredBar.value == null) return ''
	return `${history[hoveredBar.value]} minutes downtime`
})

//page.json is read once, before mount, and never changes during the navigation. recorded is the initial display value and the reset target; served is the live, animated copy
const countKeys = ['udp4', 'udp6', 'http4', 'http6', 'ws4', 'ws6']
let recorded = Object.fromEntries(countKeys.map(k => [k, page.servedDay[k]]))
let rates = Object.fromEntries(countKeys.map(k => [k, recorded[k] / Time.day]))//events per millisecond
const served = reactive({...recorded})

//one of the six counter services, as the cool-down message or the live count
function service(key) {
	if (coolDown[key]) return coolDownText//a cooled service shows a static string, no number
	return group(served[key])//a live service shows the animated count, its digits grouped
}

//a Poisson-distributed count with mean lambda, a non-negative integer: Knuth's method below 20, and Box-Muller's normal approximation from 20 up
function poissonSample(lambda) {
	if (lambda == 0) return 0
	if (lambda < 20) {
		let limit = Math.exp(-lambda)
		let k = 0//arrivals so far
		let p = 1//the running product of uniform draws
		do {
			k++
			p *= Math.random()
		} while (p > limit)
		return k - 1
	}
	let u1 = Math.random()
	let u2 = Math.random()
	let gaussian = Math.sqrt(-2 * Math.log(u1)) * Math.cos(2 * Math.PI * u2)
	return Math.max(0, Math.round(lambda + Math.sqrt(lambda) * gaussian))
}

//start and stop are idempotent, and requestAnimationFrame pauses on its own while the tab is hidden
let frameHandle = null

function frameTick() {
	updateClock()
	frameHandle = requestAnimationFrame(frameTick)
}

function startFrames() {
	if (frameHandle != null) return
	frameHandle = requestAnimationFrame(frameTick)
}

function stopFrames() {
	if (frameHandle == null) return
	cancelAnimationFrame(frameHandle)
	frameHandle = null
}

//UTC HH:MM, with a colon that blinks once a second, derived from Date.now() each frame so the visible state snaps to the wall clock and no drift accumulates
const clockHour = ref('00')
const clockMinute = ref('00')
const colonOn = ref(true)

/*
Three brackets, by the recorded count:

	recorded ≤ burstQuantity         → not animated
	burstQuantity < recorded ≤ beat  → a Poisson step each frame
	recorded > beatQuantity          → a Poisson step once per drumbeat

lastBeat is the most recent drumbeat number acted on, lastFrameNow is the previous frame's wall time, for the time each frame covers, and mountedAt anchors the resetDuration window.
*/
let lastBeat = null
let lastFrameNow = null
let mountedAt = null

function updateClock() {
	let now = Date.now()
	clockHour.value = sayTick(now, 'hh24')
	clockMinute.value = sayTick(now, 'mm')

	let beat = Math.floor(now / drumDuration)
	colonOn.value = beat % 2 == 0

	if (lastBeat != null && beat != lastBeat && mountedAt != null) {
		if (now - mountedAt >= resetDuration) {
			//snap back to recorded and re-anchor both clocks, so the per-frame block below adds nothing this frame
			mountedAt = now
			lastFrameNow = now
			for (let k of countKeys) served[k] = recorded[k]
		} else {
			//beatsSince over 1 means the tab was hidden across several beats; the sum of N independent Poisson(λ) draws is Poisson(N·λ), so one draw covers the whole gap
			let beatsSince = beat - lastBeat
			for (let k of countKeys) {
				if (coolDown[k]) continue
				if (recorded[k] > beatQuantity) {
					let arrived = poissonSample(rates[k] * drumDuration * beatsSince)
					if (arrived > 0) served[k] += arrived
				}
			}
		}
	}
	lastBeat = beat

	if (lastFrameNow != null) {
		let elapsed = now - lastFrameNow//milliseconds since the last frame
		for (let k of countKeys) {
			if (coolDown[k]) continue
			if (recorded[k] > burstQuantity && recorded[k] <= beatQuantity) {
				let arrived = poissonSample(rates[k] * elapsed)
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

			<div class="right">{{ service('udp4') }}</div>
			<div class="right">{{ service('udp6') }}</div>
			<div class="label">UDP announce</div>
			<div class="right">{{ saySize4(page.memory.udp) }}</div>

			<div class="right">{{ service('http4') }}</div>
			<div class="right">{{ service('http6') }}</div>
			<div class="label">HTTP announce</div>
			<div class="right">{{ saySize4(page.memory.http) }}</div>

			<div class="right">{{ service('ws4') }}</div>
			<div class="right">{{ service('ws6') }}</div>
			<div class="label">WebRTC offer</div>
			<div class="right">{{ saySize4(page.memory.ws) }}</div>

			<div class="right">{{ group(page.dht.nodes4) }}</div>
			<div class="right">{{ group(page.dht.nodes6) }}</div>
			<div class="label">DHT nodes</div>
			<div class="right">{{ saySize4(page.memory.dht) }}</div>

			<div></div>
			<div class="right">{{ group(page.downtimeDay) }}{{ page.downtimeDay ? ' minutes' : '' }}</div>
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
				<div v-if="hoveredBar == null" class="legend-default">
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

			<div class="right">{{ group(page.downtimeDay) }}{{ page.downtimeDay ? ' minutes' : '' }}</div>
			<div class="label">Downtime</div>

			<div class="label right">UDP announce</div>
			<div class="label">┐</div>

			<div class="right">{{ service('udp4') }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ service('udp6') }}</div>
			<div class="label">IPv6</div>

			<div class="label right">HTTP announce</div>
			<div class="label">┐</div>

			<div class="right">{{ service('http4') }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ service('http6') }}</div>
			<div class="label">IPv6</div>

			<div class="label right">WebRTC offer</div>
			<div class="label">┐</div>

			<div class="right">{{ service('ws4') }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ service('ws6') }}</div>
			<div class="label">IPv6</div>

			<div class="label right">DHT nodes</div>
			<div class="label">┐</div>

			<div class="right">{{ group(page.dht.nodes4) }}</div>
			<div class="label">IPv4</div>
			<div class="right">{{ group(page.dht.nodes6) }}</div>
			<div class="label">IPv6</div>

			<div class="label right">Memory in use</div>
			<div class="label">┐</div>

			<div class="right">{{ saySize4(page.memory.udp) }}</div>
			<div class="label">UDP</div>
			<div class="right">{{ saySize4(page.memory.http) }}</div>
			<div class="label">HTTP</div>
			<div class="right">{{ saySize4(page.memory.ws) }}</div>
			<div class="label">WS</div>
			<div class="right">{{ saySize4(page.memory.dht) }}</div>
			<div class="label">DHT</div>
		</div>

		<div class="calendar-narrow">
			<div class="legend">{{ hoveredBar == null ? '90 days' : barDate }}</div>
			<div class="cells" @mouseleave="hoveredBar = null">
				<div
					v-for="(down, i) in history"
					:key="i"
					class="cell"
					:class="{ imperfect: down > 0 }"
					@mouseenter="hoveredBar = i"
				></div>
			</div>
			<div class="legend">{{ hoveredBar == null ? uptimePercent + '% uptime' : barDown }}</div>
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
	white-space: nowrap; /* a value stays on one line, even where its space is an ordinary one, like 399 MB */
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
