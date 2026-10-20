<script setup>
// The "Last updated" line under every article, in place of VitePress's own, which config.js swaps for this file through a Vite alias, the way VitePress documents for its internal components. VitePress formats the date with Intl in the reader's locale, "10/9/26, 1:04 AM" for one reader and another form for the next; this site writes a date one way everywhere, the roadmap's way, 2026-Oct-08 01:04 UTC. The timestamp is the one VitePress reads from git at build time, the page's last commit. It is formatted here in UTC, which is the same on the server and in every browser, so the built page already carries the words and nothing changes on hydration; VitePress's own component has to wait for the browser because a locale is only known there.
import { computed } from 'vue'
import { useData } from 'vitepress'
import { tickToIso } from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go

const { theme, page } = useData()
const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']
const date = computed(() => new Date(page.value.lastUpdated))
const iso = computed(() => tickToIso(page.value.lastUpdated))//a whole count of milliseconds, as VitePress reads it from git
const two = (n) => String(n).padStart(2, '0')//two digits, so days and hours line up down a page of them
const said = computed(() => {
	let d = date.value
	return `${d.getUTCFullYear()}-${months[d.getUTCMonth()]}-${two(d.getUTCDate())} ${two(d.getUTCHours())}:${two(d.getUTCMinutes())} UTC`
})
</script>

<template>
	<p class="VPLastUpdated">
		{{ theme.lastUpdated?.text || 'Last updated' }}:
		<time :datetime="iso">{{ said }}</time>
	</p>
</template>

<style scoped>
/* VitePress's own rules for the line, carried over unchanged, so it sits beside the edit link as before: 14px at weight 500 in the secondary ink. */
.VPLastUpdated {
	line-height: 24px;
	font-size: 14px;
	font-weight: 500;
	color: var(--vp-c-text-2);
}
@media (min-width: 640px) {
	.VPLastUpdated {
		line-height: 32px;
	}
}
</style>
