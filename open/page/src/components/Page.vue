<script setup>
import Space from './Space.vue'
import Url from './Url.vue'
import Panel from './Panel.vue'
</script>

<template>
	<Space class="banner" />

	<h1>open.ftorrent.com</h1>

	<div class="announce-urls">
		<Url href="udp://open.ftorrent.com:443/announce" />
		<Url href="https://open.ftorrent.com/announce" />
		<Url href="wss://open.ftorrent.com" />
	</div>

	<Panel style="margin: 2rem" />
</template>

<style scoped>
.banner {
	height: 300px;
}

h1 {
	text-align: center;
	margin: 2rem;
}

.announce-urls {
	margin: 2rem;
}
</style>
