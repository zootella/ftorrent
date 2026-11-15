<script setup>
import {inject} from 'vue'
import {tip} from '../tooltip.js'

let report = inject('report')//the hour's trackers.json, or null when it couldn't be read
</script>

<template>
	<div class="flex min-h-dvh flex-col">
		<header class="border-b border-neutral-200 dark:border-neutral-800">
			<div class="flex h-14 items-center justify-between gap-4 px-4 sm:px-6 lg:px-8">
				<RouterLink to="/" class="flex items-center gap-2 font-semibold tracking-tight">
					<svg class="h-4 w-8" viewBox="0 4 16 8" aria-hidden="true"><rect x="0" y="4" width="16" height="8" rx="4" fill="#FF7900"/><path d="M6.828 7H9.172A3 3 0 1 1 9.172 9H6.828A3 3 0 1 1 6.828 7Z" fill="white"/></svg>
					<span>list<span class="text-neutral-400 dark:text-neutral-500">.ftorrent.com</span></span>
				</RouterLink>
				<nav class="flex gap-4 text-sm text-neutral-500 dark:text-neutral-400">
					<a href="/trackers.txt" class="hover:text-neutral-900 dark:hover:text-neutral-100">trackers.txt</a>
					<a href="/trackers.json" class="hover:text-neutral-900 dark:hover:text-neutral-100">trackers.json</a>
					<a href="https://github.com/zootella/ftorrent/tree/master/list" class="hidden hover:text-neutral-900 sm:inline dark:hover:text-neutral-100">GitHub</a>
				</nav>
			</div>
		</header>

		<main class="w-full flex-1 px-4 py-10 sm:px-6 lg:px-8"><!-- the whole width of the window, so a tracker's URL and its numbers share one line wherever they fit -->
			<p v-if="!report" class="text-neutral-500">The latest report, trackers.json, couldn't be read. Try again in a minute.</p>
			<RouterView v-else />
		</main>

		<!-- the one tooltip, which tooltip.js fills and places -->
		<div
			v-if="tip.text"
			class="pointer-events-none fixed z-50 max-w-80 -translate-x-1/2 whitespace-pre-line rounded-md bg-neutral-900 px-2 py-1 text-xs text-white shadow-lg dark:bg-white dark:text-neutral-900"
			:class="tip.below ? '' : '-translate-y-full'"
			:style="{left: `${tip.x}px`, top: `${tip.y}px`}">{{ tip.text }}</div>

		<footer class="border-t border-neutral-200 dark:border-neutral-800">
			<div class="flex flex-wrap gap-x-6 gap-y-2 px-4 py-6 text-sm text-neutral-500 sm:px-6 lg:px-8 dark:text-neutral-400">
				<a href="https://github.com/zootella/ftorrent/tree/master/list" class="hover:text-neutral-900 dark:hover:text-neutral-100">How it works</a>
				<a href="https://docs.ftorrent.com/tracker-lists" class="hover:text-neutral-900 dark:hover:text-neutral-100">Tracking the Trackers</a>
				<a href="https://ftorrent.com" class="hover:text-neutral-900 dark:hover:text-neutral-100">ftorrent.com</a>
			</div>
		</footer>
	</div>
</template>
