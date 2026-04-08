<script setup>
import { inject } from 'vue'

const page = inject('page')

function fmt(n) {
	return n.toLocaleString('en-US').replace(/,/g, ' ')
}

function mb(bytes) {
	return Math.round(bytes / 1048576)
}
</script>

<template>
	<div class="lcd-panel">
		<div class="lcd-grid">
			<div class="lcd-label lcd-right">IPv4</div>
			<div></div>
			<div class="lcd-label lcd-right">IPv6</div>
			<div></div>
			<div class="lcd-label lcd-right lcd-mem-header">Memory in use</div>

			<div class="lcd-right">{{ fmt(page.served.udp4) }}</div>
			<div></div>
			<div class="lcd-right">{{ fmt(page.served.udp6) }}</div>
			<div></div>
			<div class="lcd-label">UDP announce</div>
			<div></div>
			<div class="lcd-right">{{ mb(page.memory.udp) }} MB</div>

			<div class="lcd-right">{{ fmt(page.served.http4) }}</div>
			<div></div>
			<div class="lcd-right">{{ fmt(page.served.http6) }}</div>
			<div></div>
			<div class="lcd-label">HTTP announce</div>
			<div></div>
			<div class="lcd-right">{{ mb(page.memory.http) }} MB</div>

			<div class="lcd-right">{{ fmt(page.served.ws4) }}</div>
			<div></div>
			<div class="lcd-right">{{ fmt(page.served.ws6) }}</div>
			<div></div>
			<div class="lcd-label">WebRTC offer</div>
			<div></div>
			<div class="lcd-right">{{ mb(page.memory.ws) }} MB</div>

			<div class="lcd-right lcd-downtime-value">{{ fmt(page.downtime) }}</div>
			<div class="lcd-label lcd-downtime-label">Downtime past 24 hours</div>
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
	grid-template-columns: auto 2rem auto 0.8rem auto 7rem auto;
	row-gap: 0.25rem;
}

.lcd-label {
	color: #9B9C90;
	font-family: 'Jura', sans-serif;
	font-size: 1.4rem;
	font-weight: 700;
	text-shadow: none;
	text-align: left;
}

.lcd-right {
	text-align: right;
}

.lcd-mem-header {
	grid-column: 5 / 8;
}

.lcd-downtime-value {
	grid-column: 1 / 4;
}

.lcd-downtime-label {
	grid-column: 5 / 8;
}
</style>
