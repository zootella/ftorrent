<script setup>
const downtime = '0 minutes'
const udp4 = 1234567, udp6 = 1234, udpMem = 56
const http4 = 123456, http6 = 123, httpMem = 12
const ws4 = 1234, ws6 = 12, wsMem = 5

function fmt(n) {
	return n.toLocaleString('en-US').replace(/,/g, ' ')
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

			<div class="lcd-right">{{ fmt(udp4) }}</div>
			<div></div>
			<div class="lcd-right">{{ fmt(udp6) }}</div>
			<div></div>
			<div class="lcd-label">UDP announce</div>
			<div></div>
			<div class="lcd-right">{{ udpMem }} MB</div>

			<div class="lcd-right">{{ fmt(http4) }}</div>
			<div></div>
			<div class="lcd-right">{{ fmt(http6) }}</div>
			<div></div>
			<div class="lcd-label">HTTP announce</div>
			<div></div>
			<div class="lcd-right">{{ httpMem }} MB</div>

			<div class="lcd-right">{{ fmt(ws4) }}</div>
			<div></div>
			<div class="lcd-right">{{ fmt(ws6) }}</div>
			<div></div>
			<div class="lcd-label">WebRTC offer</div>
			<div></div>
			<div class="lcd-right">{{ wsMem }} MB</div>

			<div class="lcd-right lcd-downtime-value">{{ fmt(downtime) }}</div>
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
