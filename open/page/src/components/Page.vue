<script setup>
import Space from './Space.vue'
import Url from './Url.vue'
import Panel from './Panel.vue'
import Quote from './Quote.vue'
</script>

<template>
	<Space class="banner" />

	<h1>open.ftorrent.com</h1>

	<div class="announce-urls">
		<Url href="udp://open.ftorrent.com:443/announce" color="#00A89D" />
		<Url href="https://open.ftorrent.com/announce" color="#F55D00" />
		<Url href="wss://open.ftorrent.com" color="#D4006A" />
	</div>

	<Panel style="margin: 2rem" />

	<div class="sticker">
		<img src="/images/inside.png" alt="ftorrent sticker" />
	</div>

	<Quote />

	<div class="footer">
		<a href="https://ftorrent.com">www.ftorrent.com</a>
	</div>
</template>

<style scoped>
.banner {
	height: 300px;
}

h1 {
	text-align: center;
	margin: 2rem;
}

.sticker {
	display: flex;
	justify-content: flex-end;
	margin: 2rem;
}

.sticker img {
	width: 75px;
	height: auto;
}

.announce-urls {
	margin: 2rem;
}

.footer {
	text-align: center;
	margin: 2rem;
	font-family: 'Jura', sans-serif;
	font-weight: 700;
	font-size: 1rem;
}

.footer a {
	color: var(--text-muted);
	text-decoration: underline;
}
</style>
