// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
	// Frontend only: no runtime SSR, no Nitro server, no API routes.
	// `nuxt generate` emits a static SPA into .output/public/ that Caddy serves.
	ssr: false,

	compatibilityDate: '2026-06-18',

	// Pure single-shell SPA. With link-crawling off, `nuxt generate` stops auto-prerendering
	// a static HTML file per page and emits only index.html (the shell) plus the 200.html /
	// 404.html fallbacks. Every route — /about included — still works: it's served the shell
	// (via Caddy's `try_files {path} /index.html`) and rendered client-side by Vue Router, not
	// from a pre-written file. Keeps the output and the Caddy no-cache rule to a single index.html.
	nitro: {
		prerender: {
			crawlLinks: false,
		},
	},

	app: {
		head: {
			title: 'ftorrent.com',
		},
	},
})
