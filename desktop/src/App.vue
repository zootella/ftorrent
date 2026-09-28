<script setup>
import {useAssociationsStore} from './stores/associations.js'
import {brandName} from './brand.js'

let associations = useAssociationsStore()//the banner's question and answers; the store decides when it's up
</script>

<template>
	<!-- ./src/App.vue -->
	<!-- the shell every page sits inside: the banner when there's a question to ask, the navigation across the top, and the outlet the router fills with whichever page is current -->
	<div v-if="associations.bannerUp" class="banner">
		<span class="banner-text">Use {{ brandName }} for .torrent files and magnet links? Windows may ask you to confirm in its Settings.</span>
		<button type="button" @click="associations.choose('yes')">Yes</button>
		<button type="button" @click="associations.choose('no')">No</button>
		<button type="button" class="banner-close" aria-label="Close" @click="associations.dismiss()">×</button>
	</div>
	<nav class="nav">
		<router-link to="/">Main</router-link>
		<router-link to="/settings">Settings</router-link>
		<router-link to="/about">About</router-link>
	</nav>
	<router-view />
</template>

<style>
:root {
	font-family: Inter, Avenir, Helvetica, Arial, sans-serif;
	font-size: 16px;
	line-height: 24px;
	font-weight: 400;

	color: #0f0f0f;
	background-color: #f6f6f6;

	font-synthesis: none;
	text-rendering: optimizeLegibility;
	-webkit-font-smoothing: antialiased;
	-moz-osx-font-smoothing: grayscale;
	-webkit-text-size-adjust: 100%;
}

.banner {
	display: flex;
	align-items: center;
	gap: 0.6em;
	padding: 0.5em 0.5em 0.5em 1em;
	background-color: #e3e9fb;
	border-bottom: 1px solid #c3cdea;
}

.banner-text {
	flex: 1;/* the sentence takes the room, and the buttons sit together at the right */
}

.banner button {
	padding: 0.3em 1em;
}

.banner .banner-close {/* a bare ×, so it reads as closing the bar rather than as a third answer */
	padding: 0.3em 0.6em;
	background-color: transparent;
	box-shadow: none;
	font-size: 1.2em;
	line-height: 1;
}

.nav {
	display: flex;
	justify-content: center;
	gap: 1.5em;
	padding: 1em;
}

.nav a.router-link-active {
	font-weight: 700;
}

.container {
	margin: 0;
	padding-top: 6vh;
	display: flex;
	flex-direction: column;
	justify-content: center;
	text-align: center;
}

.logo {
	height: 6em;
	padding: 1.5em;
	will-change: filter;
	transition: 0.75s;
}

.row {
	display: flex;
	justify-content: center;
}

a {
	font-weight: 500;
	color: #646cff;
	text-decoration: inherit;
}

a:hover {
	color: #535bf2;
}

h1 {
	text-align: center;
}

input,
button {
	border-radius: 8px;
	border: 1px solid transparent;
	padding: 0.6em 1.2em;
	font-size: 1em;
	font-weight: 500;
	font-family: inherit;
	color: #0f0f0f;
	background-color: #ffffff;
	transition: border-color 0.25s;
	box-shadow: 0 2px 2px rgba(0, 0, 0, 0.2);
}

button {
	cursor: pointer;
}

button:hover {
	border-color: #396cd8;
}
button:active {
	border-color: #396cd8;
	background-color: #e8e8e8;
}

input,
button {
	outline: none;
}

#greet-input {
	margin-right: 5px;
}

@media (prefers-color-scheme: dark) {
	:root {
		color: #f6f6f6;
		background-color: #2f2f2f;
	}

	a:hover {
		color: #24c8db;
	}

	input,
	button {
		color: #ffffff;
		background-color: #0f0f0f98;
	}
	button:active {
		background-color: #0f0f0f69;
	}

	.banner {
		background-color: #26304a;
		border-bottom-color: #3a4666;
	}
}

</style>
