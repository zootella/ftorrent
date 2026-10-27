<script setup>
import {inject, onMounted, onUnmounted} from 'vue'
import Space from './Space.vue'
import Url from './Url.vue'
import Panel from './Panel.vue'
import Quote from './Quote.vue'

const page = inject('page')//null when page.json couldn't be read, which leaves out the panel

//the root's color fills the gap when a browser bounces past either end of the page, black at the top and the page color at the bottom, and style.css says why it has to follow the reader. It switches at the halfway point of the scroll, well before either end
function setRootColor() {
	let root = document.documentElement
	root.classList.toggle('lower', window.scrollY > (root.scrollHeight - window.innerHeight) / 2)//a toggle that matches the class already there changes nothing, so the root repaints only when the reader crosses the middle
}

onMounted(() => {
	setRootColor()
	window.addEventListener('scroll', setRootColor, {passive: true})//passive tells the browser this listener never cancels a scroll, so scrolling never waits on it
	window.addEventListener('resize', setRootColor)//a resize moves the middle
})

onUnmounted(() => {
	window.removeEventListener('scroll', setRootColor)
	window.removeEventListener('resize', setRootColor)
})
</script>

<template>
	<Space class="banner" />

	<h1>open.ftorrent.com</h1>

	<div class="announce-urls">
		<Url href="udp://open.ftorrent.com:443/announce" color="#00A89D" />
		<Url href="https://open.ftorrent.com/announce" color="#F55D00" />
		<Url href="wss://open.ftorrent.com" color="#D4006A" />
		<Url href="dht.ftorrent.com:51420" color="#1E6FF5" />
	</div>

	<div class="stack">
		<Panel v-if="page" />
		<div class="sticker">
			<img src="/images/inside.png" alt="ftorrent sticker" />
		</div>
	</div>

	<Quote />

	<div class="footer">
		<a href="https://ftorrent.com">ftorrent.com</a>
		<a href="https://github.com/zootella/ftorrent/tree/master/open">Fork me on GitHub</a>
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
