
<!--
PoissonBurst — a standalone demo of animated event counters.

Simulates a dashboard that received a snapshot of event counts by category,
each covering the last 24 hours. Values range from 0 (inactive categories)
to ~20 billion (high-throughput streams).

Each counter animates upward at the implied average rate, using Poisson-
distributed increments so the ticking feels organic (bursty at low rates,
shimmery at high rates) rather than metronomic.

To keep displayed counts close to what a page refresh would show, each
counter resets to its original total every RESET_INTERVAL. The drop is
at most a few minutes' worth of accumulation — imperceptible in a single frame.

Drop this component into any page with no props or wiring needed.
-->

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'

const RESET_INTERVAL = 600_000 // 10 minutes
/*
Maximum delta-time (ms) between animation frames. Normally dt is ~16ms
(60fps), but when the tab is hidden the browser pauses requestAnimationFrame.
On return, dt could be seconds or minutes, which would cause the counter to
jump by a huge amount in a single frame. Clamping to 100ms means we "miss"
those events instead — the counter just resumes ticking normally.
*/
const MAX_DT = 100//maybe rename this AWAY_DELTA

const RECORDED_DURATION = 86_400_000 // 24h — the duration the server's totals cover
const EXAMPLES = [
	{ recorded: 0, label: 'Dead silence' },
	{ recorded: 86, label: '~1 per 1000s' },
	{ recorded: 4_320, label: 'Slow drip' },
	{ recorded: 86_400, label: 'One per second' },
	{ recorded: 1_000_000, label: 'Busy service' },
	{ recorded: 2_000_000, label: 'Rapid squirts' },//Note from the user: At two million our animation is the most satisifying to humans, i believe! The ones rolls up faster than you can see, but you can still tell their growth is not regular, and then tens is clearly in discreet squirts up!
	{ recorded: 500_000_000, label: 'Heavy traffic' },
	{ recorded: 15_000_000_000, label: 'Firehose' },
]

/**
 * poissonSample(λ)
 *
 * Draws a single sample from a Poisson distribution with mean λ.
 *
 * λ < 20  → Knuth's algorithm.
 * λ ≥ 20  → Normal approximation via Box-Muller.
 */
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

function formatNumber(n) {
	return n.toLocaleString('en-US').replace(/,/g, ' ')
}

function rateLabelFor(count) {
	const perSec = (count / RECORDED_DURATION) * 1000
	if (perSec >= 1000) return `~${formatNumber(Math.round(perSec))}/sec`
	if (perSec >= 1) return `~${perSec.toFixed(1)}/sec`
	if (perSec >= 0.01) return `~1 every ${Math.round(1 / perSec)}s`
	if (perSec > 0) return `~1 every ${Math.round(1 / perSec / 60)}min`
	return 'silent'
}

// Per-counter state: each entry gets its own reactive count
const counters = EXAMPLES.map((ex) => ({
	...ex,
	simulated: ref(ex.recorded),
	rate: ex.recorded / RECORDED_DURATION,
	rateLabel: rateLabelFor(ex.recorded),
}))

let lastTime = null//maybe rename to just before
let rafId = null//this is a reference to our registration to get called on animation frames?
let mountedAt = null

function tick(now) {
	if (lastTime !== null) {
		const wallNow = Date.now()
		const elapsed = wallNow - mountedAt
		if (elapsed >= RESET_INTERVAL) {
			mountedAt = wallNow
			for (const c of counters) {
				c.simulated.value = c.recorded
			}
		}

		const dt = Math.min(now - lastTime, MAX_DT)

		for (const c of counters) {
			const lambdaFrame = c.rate * dt
			if (lambdaFrame > 0) {
				const arrived = poissonSample(lambdaFrame)
				if (arrived > 0) {
					c.simulated.value += arrived
				}
			}
		}
	}
	lastTime = now
	rafId = requestAnimationFrame(tick)
}

onMounted(() => {
	mountedAt = Date.now()
	rafId = requestAnimationFrame(tick)
})

onUnmounted(() => {
	if (rafId !== null) {
		cancelAnimationFrame(rafId)
	}
})
</script>

<template>
	<div class="root">
		<div class="header">
			<h1>Poisson Ticking Counters</h1>
			<p>
				Same algorithm, different scales. Watch how burstiness feels at each
				rate — the low ones drip, the high ones shimmer.
			</p>
		</div>
		<div class="grid">
			<div v-for="c in counters" :key="c.label" class="counter">
				<div class="label">{{ c.label }}</div>
				<div class="number">{{ formatNumber(c.simulated.value) }}</div>
				<div class="rate">
					{{ formatNumber(c.recorded) }} / 24h → {{ c.rateLabel }}
				</div>
			</div>
		</div>
	</div>
</template>

<style scoped>
.root {
	min-height: 100vh;
	background: #0a0a0c;
	color: #e0e0e0;
	font-family: 'SF Mono', 'Fira Code', 'Consolas', monospace;
	padding: 2rem;
}
.header {
	margin-bottom: 2rem;
	border-bottom: 1px solid #1a1a2e;
	padding-bottom: 1.5rem;
}
h1 {
	font-size: 1.1rem;
	font-weight: 500;
	color: #999;
	letter-spacing: 0.08em;
	text-transform: uppercase;
	margin: 0;
}
p {
	font-size: 0.8rem;
	color: #999;
	margin-top: 0.5rem;
	line-height: 1.5;
	max-width: 44em;
}
.grid {
	display: grid;
	grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
	gap: 1px;
	background: #1a1a2e;
	border: 1px solid #1a1a2e;
}
.counter {
	background: #0a0a0c;
	padding: 1.5rem;
	display: flex;
	flex-direction: column;
	align-items: flex-end;
	text-align: right;
	gap: 0.5rem;
}
.label {
	font-size: 0.7rem;
	color: #999;
	text-transform: uppercase;
	letter-spacing: 0.1em;
}
.number {
	font-size: 1.75rem;
	font-weight: 300;
	color: #c8f7c5;
	font-variant-numeric: tabular-nums;
	letter-spacing: 0.02em;
}
.rate {
	font-size: 0.65rem;
	color: #999;
	margin-top: 0.25rem;
}
</style>
