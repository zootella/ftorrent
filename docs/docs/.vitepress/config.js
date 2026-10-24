import { defineConfig } from 'vitepress'
import taskLists from 'markdown-it-task-lists'
import { existsSync, readFileSync } from 'node:fs'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'

// Where the desktop client's installers and their sidecars live. Said once here because three things need it: the installing page's download boxes link to it, downloads.js fetches sidecars from it, and the dev proxy below forwards to it. The install commands on that page name it too, as plain text in their fences. This site is docs.ftorrent.com, so in production the page fetches sidecars from that other host, across origins, which works because ftorrent.com sends an Access-Control-Allow-Origin header with its downloads.
const origin = 'https://ftorrent.com'

/*
A sidecar comes from this machine if it is staged here, and from production if it is not.

`pnpm hash` in the desktop workspace stages installers and writes their sidecars into desktop/release and desktop/linux/release, and until they are uploaded the site has no way to show them: the installing page would say "not yet published" about a file sitting finished on your own disk. So in development, downloads.js asks this server rather than ftorrent.com, and this middleware answers from those two folders at request time, calling through to the proxy below only for what this machine has not staged: the exe, say, which is built on Windows. Local work wins, production fills the gaps.

Read at request time and never copied. A sidecar copied into docs/public would be baked into a build and served from docs.ftorrent.com, pinning the page to a stale hash. A dev-only middleware cannot do that, because `vitepress build` never runs it, and neither does it run the proxy.
*/
function stagedSidecars() {
	// from this file's own location rather than the working directory, because vitepress can be started from the workspace or the root
	let staged = ['../../../desktop/release/', '../../../desktop/linux/release/'].map(where => fileURLToPath(new URL(where, import.meta.url)))
	return {
		name: 'ftorrent-staged-sidecars',
		configureServer(server) {
			// added here rather than in a returned function, so it runs before vite's own middlewares and gets first refusal ahead of the proxy
			server.middlewares.use((request, response, next) => {
				let asked = /^\/(ftorrent\.[a-z0-9_.]+\.json)(?:[?#]|$)/.exec(request.url || '')
				if (!asked) return next()
				for (let folder of staged) {
					let file = join(folder, asked[1])
					if (!existsSync(file)) continue
					response.setHeader('content-type', 'application/json')
					response.setHeader('x-ftorrent-sidecar', 'staged on this machine')// so a fetch can be told apart from the proxied one without guessing
					return response.end(readFileSync(file))
				}
				next()// nothing staged under that name, so the proxy asks production
			})
		},
	}
}

// https://vitepress.dev/reference/site-config
export default defineConfig({
	title: "ftorrent",
	description: "docs.ftorrent.com",
	lastUpdated: true,// each page's footer shows the date of its last commit, read from git at build time, so a reader knows how fresh a page is without our saying so
	sitemap: { hostname: 'https://docs.ftorrent.com' },// sitemap.xml at the root of the built site, for a public site that wants to be found
	// the brand mark, desktop/icon-studio/favicon.svg, inline as a data URI so the site carries no favicon file; the # of the color is written %23
	head: [['link', {rel: 'icon', href: "data:image/svg+xml,<svg width='1024' height='1024' viewBox='0 0 16 16' fill='none' xmlns='http://www.w3.org/2000/svg'><rect x='0' y='4' width='16' height='8' rx='4' fill='%23FF7900'/><path d='M6.828 7H9.172A3 3 0 1 1 9.172 9H6.828A3 3 0 1 1 6.828 7Z' fill='white'/></svg>"}]],
	markdown: {
		// Open external links in the same tab so the browser back button works.
		// These docs hold no state (no forms), so leaving and returning is safe.
		externalLinks: { target: '_self' },
		// The planning articles' task boxes, - [ ] and - [x], as real checkboxes, disabled, rather than literal brackets. VitePress has no task lists of its own; this is the small markdown-it plugin that adds them.
		config: (md) => { md.use(taskLists) },
	},
	vite: {
		plugins: [stagedSidecars()],
		// VitePress's "Last updated" line swapped for the theme's own LastUpdated.vue, which writes the date the roadmap's way, 2026-Oct-08 01:04 UTC, in place of Intl's reader-locale form. An alias on the internal component's path is how VitePress says to override one of its own components.
		resolve: { alias: [{ find: /^.*\/VPDocFooterLastUpdated\.vue$/, replacement: fileURLToPath(new URL('./theme/components/LastUpdated.vue', import.meta.url)) }] },
		// The report page inlines its nine figures so they take the site's colors, and VitePress ships them in the page's script as well as in its HTML, which puts that one chunk past Vite's 500 kB warning on purpose. The limit is raised just past it, so a chunk that grows for some other reason still warns.
		build: { chunkSizeWarningLimit: 600 },
		server: {
			proxy: {
				// every sidecar, matched by shape, reached only for what stagedSidecars above did not find on this machine
				'^/ftorrent\\.[a-z0-9_.]+\\.json$': { target: origin, changeOrigin: true },
			},
		},
	},

	// Open Graph tags on every page, so a link to an article pasted into a chat or a feed shows its title and description as a card. VitePress adds none of its own. The title and description are the page's frontmatter, the site's when a page has none, and the address is the page's on docs.ftorrent.com with the .html the live server needs.
	transformPageData(pageData) {
		let path = pageData.relativePath.replace(/index\.md$/, '').replace(/\.md$/, '.html')
		let title = pageData.frontmatter.title || pageData.title || 'ftorrent'
		let description = pageData.frontmatter.description || pageData.description || 'docs.ftorrent.com'
		pageData.frontmatter.head ??= []
		pageData.frontmatter.head.push(
			['meta', { property: 'og:type', content: path ? 'article' : 'website' }],
			['meta', { property: 'og:site_name', content: 'ftorrent' }],
			['meta', { property: 'og:title', content: title }],
			['meta', { property: 'og:description', content: description }],
			['meta', { property: 'og:url', content: 'https://docs.ftorrent.com/' + path }],
			['meta', { name: 'twitter:card', content: 'summary' }],
		)
	},

	themeConfig: {
		origin,// not a theme setting; themeConfig is just how VitePress hands a value of our own to the app, where useData().theme reads it

		// https://vitepress.dev/reference/default-theme-config
		outline: { level: [2, 3] },

		// An index built at build time and searched in the browser, with no outside service.
		search: { provider: 'local' },

		// Under every page, a link to its source on GitHub, so a reader who finds a wrong word can fix it in a pull request.
		editLink: { pattern: 'https://github.com/zootella/ftorrent/edit/master/docs/docs/:path', text: 'Edit this page on GitHub' },

		nav: [
			{ text: 'Home', link: '/' }
		],

		sidebar: [
			{
				text: 'Project',
				items: [
					{ text: 'Roadmap in Reverse', link: '/roadmap-in-reverse' }
				]
			},
			{
				text: 'Essays and Guides',
				items: [
					{ text: 'Names and Numbers', link: '/names-and-numbers' },
					{ text: 'Desktop Architecture', link: '/desktop-architecture' },
					{ text: 'Installing ftorrent', link: '/installing-ftorrent' },
					{ text: 'How Can Two Peers Connect?', link: '/how-peers-connect' },
					{ text: 'libtorrent Provenance', link: '/libtorrent-provenance' },
					{ text: 'Software Selections', link: '/software-selections' },
					{ text: 'Tracker Load', link: '/tracker-load' },
					{ text: 'Tuning for Load', link: '/tuning-for-load' }
				]
			},
			{
				text: 'Research',
				items: [
					{ text: 'Open Address, Closed Device', link: '/open-address-closed-device' },
					{ text: 'Waiting for IPv6', link: '/waiting-for-ipv6' }
				]
			},
			{
				text: 'Planning',
				items: [
					{ text: 'Desktop Client Planning', link: '/desktop-client-planning' },
					{ text: 'Good Service Planning', link: '/good-service-planning' }
				]
			},
			{
				text: 'Drafts',
				items: [
					{ text: 'Port Control', link: '/port-control' }
				]
			},
			{
				text: 'Repository Documentation',
				items: [
					{ text: 'Repository Overview', link: 'https://github.com/zootella/ftorrent/blob/master/README.md' },
					{ text: 'Dockerizing Aquatic', link: 'https://github.com/zootella/ftorrent/blob/master/open/README.md' },
					{ text: 'Tracker Page', link: 'https://github.com/zootella/ftorrent/blob/master/open/page/README.md' },
					{ text: 'Tracker Gauge', link: 'https://github.com/zootella/ftorrent/blob/master/open/gauge/README.md' },
					{ text: 'Tracker Circuit Breaker', link: 'https://github.com/zootella/ftorrent/blob/master/open/breaker/README.md' },
					{ text: 'DHT Bootstrap Node', link: 'https://github.com/zootella/ftorrent/blob/master/open/dht/README.md' },
					{ text: 'Connection Checker', link: 'https://github.com/zootella/ftorrent/blob/master/good/README.md' },
					{ text: 'Home Page', link: 'https://github.com/zootella/ftorrent/blob/master/site/README.md' },
					{ text: 'Desktop Client', link: 'https://github.com/zootella/ftorrent/blob/master/desktop/README.md' },
					{ text: 'Desktop Client on Linux', link: 'https://github.com/zootella/ftorrent/blob/master/desktop/linux/README.md' },
					{ text: 'Documentation Website', link: 'https://github.com/zootella/ftorrent/blob/master/docs/README.md' },
					{ text: 'Open Address, Closed Device Scripts', link: 'https://github.com/zootella/ftorrent/blob/master/docs/research/ipv6/README.md' }
				]
			},
			{
				text: 'Notes and Planning (TODO)',
				items: [
					{ text: 'Decentralized Infrastructure', link: '/decentralized-infrastructure' },
					{ text: 'Magnet Link User Guide', link: '/magnet-link-user-guide' },
					{ text: 'How the DHT Works', link: '/how-the-dht-works' }
				]
			}
		],

		// The home page has no sidebar and so is the one page that shows this footer; it carries the same license badge and line the articles carry under their content, with the markup Creative Commons recommends.
		footer: { message: '<a rel="license" href="https://creativecommons.org/licenses/by/4.0/"><img src="/somerights.gif" alt="Some rights reserved: Creative Commons Attribution 4.0 International" width="88" height="31"></a>ftorrent licenses the text of this site under <a rel="license" href="https://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International</a>.' },

		socialLinks: [
			{ icon: 'github', link: 'https://github.com/zootella/ftorrent' }
		]
	}
})
