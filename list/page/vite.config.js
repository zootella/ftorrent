import { fileURLToPath, URL } from 'node:url'
import { copyFileSync } from 'node:fs'
import { join } from 'node:path'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import tailwindcss from '@tailwindcss/vite'
import vueDevTools from 'vite-plugin-vue-devtools'

/*
The report, trackers.json, on this machine. On the server, the IPv4 container writes it every hour and the web server answers /trackers.json from that container's folder, apart from the static files this workspace builds. Here, both the dev server and pnpm preview forward that one path to the live site, which serves the report to anyone, so the page reads real data with the same fetch it makes on the server, and no copy of the report ever lands in a build.
*/
let report = {'/trackers.json': {target: 'https://list.ftorrent.com', changeOrigin: true}}//changeOrigin names list.ftorrent.com as the host, so the server answers as it would a browser on the site

/*
The page routes by path, like /udp/tracker.example.com:6969/announce for one tracker, and a reload or a shared link asks the server for that path, where no file is. The web server answers any path that isn't a file with the folder's 200.html, the name Nuxt and other static hosts use for this, so the build writes one: a copy of index.html, which loads the page, and the router draws the tracker the path names.
*/
let fallback = {
	name: 'list-200-html',
	apply: 'build',
	writeBundle(options) { copyFileSync(join(options.dir, 'index.html'), join(options.dir, '200.html')) },//once the build has written index.html
}

// https://vite.dev/config/
export default defineConfig({
	server: {proxy: report},
	preview: {proxy: report},
	plugins: [
		vue(),
		tailwindcss(),
		vueDevTools(),
		fallback,
	],
	resolve: {
		alias: {
			'@': fileURLToPath(new URL('./src', import.meta.url))
		},
	},
})
