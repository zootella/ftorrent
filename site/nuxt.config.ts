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
			// the brand mark, desktop/icon-studio/favicon.svg, inline as a data URI so the site carries no favicon file; the # of the color is written %23
			link: [{rel: 'icon', href: "data:image/svg+xml,<svg width='1024' height='1024' viewBox='0 0 16 16' fill='none' xmlns='http://www.w3.org/2000/svg'><rect x='0' y='4' width='16' height='8' rx='4' fill='%23FF7900'/><path d='M6.828 7H9.172A3 3 0 1 1 9.172 9H6.828A3 3 0 1 1 6.828 7Z' fill='white'/></svg>"}],
		},
	},
})
