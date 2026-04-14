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

	<div class="stack">
		<Panel />
		<div class="sticker">
			<img src="/images/inside.png" alt="ftorrent sticker" />
		</div>
	</div>

	<Quote />

	<div class="footer">
		<a href="https://ftorrent.com">www.ftorrent.com</a>
		<a href="https://github.com/zootella/ftorrent/tree/master/open">we ♥{{ '\uFE0E' }} open source</a><!-- U+FE0E after the heart is Variation Selector-15: "render the previous glyph as text, not emoji." Without it, mobile browsers upgrade ♥ to a red color emoji. -->
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

.stack {
	display: flex;
	flex-direction: column;
	align-items: flex-end;
	width: max-content;
	margin: 2rem auto;
	gap: 2rem;
}

.sticker img {
	width: 75px;
	height: auto;
}

.announce-urls {
	margin: 2rem;
}

.footer {
	display: flex;
	justify-content: center;
	gap: 1rem;
	margin: 2rem;
	padding-bottom: 2rem;
	font-family: 'Jura', sans-serif;
	font-weight: var(--muted-weight);
	font-size: 1rem;
}

.footer a {
	color: var(--text-muted);
	text-decoration: underline;
}

@media (max-width: 480px) {
	h1 {
		margin: 2rem 0.5rem;
	}
	.announce-urls {
		margin: 2rem 0.5rem;
	}
}
</style>
