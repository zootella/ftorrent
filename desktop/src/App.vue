<template>
	<!-- ./src/App.vue -->
	<!-- the shell every page sits inside: the navigation across the top, and the outlet the router fills with whichever page is current -->
	<nav class="nav">
		<router-link to="/">Main</router-link>
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
}

</style>
