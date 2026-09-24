import { defineConfig } from 'vitepress'
import { existsSync, readFileSync } from 'node:fs'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'

// Where the desktop client's installers and their sidecars live. Said once here because three things need it: the installing page's links point at it, curl.data.js writes it into the install commands, and the dev proxy below forwards to it. This site is docs.ftorrent.com, so in production the page fetches sidecars from that other host, across origins, which works because ftorrent.com sends an Access-Control-Allow-Origin header with its downloads.
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
	markdown: {
		// Open external links in the same tab so the browser back button works.
		// These docs hold no state (no forms), so leaving and returning is safe.
		externalLinks: { target: '_self' }
	},
	vite: {
		plugins: [stagedSidecars()],
		server: {
			proxy: {
				// every sidecar, matched by shape, reached only for what stagedSidecars above did not find on this machine
				'^/ftorrent\\.[a-z0-9_.]+\\.json$': { target: origin, changeOrigin: true },
			},
		},
	},

	themeConfig: {
		origin,// not a theme setting; themeConfig is just how VitePress hands a value of our own to the app, where useData().theme reads it

		// https://vitepress.dev/reference/default-theme-config
		outline: { level: [2, 3] },

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
					{ text: 'Documentation Website', link: 'https://github.com/zootella/ftorrent/blob/master/docs/README.md' }
				]
			},
			{
				text: 'Notes and Planning (TODO)',
				items: [
					{ text: 'Decentralized Infrastructure', link: '/decentralized-infrastructure' },
					{ text: 'Magnet Link User Guide', link: '/magnet-link-user-guide' },
					{ text: 'How the DHT Works', link: '/how-the-dht-works' },
					{ text: 'Desktop Client Planning', link: '/desktop-client-planning' }
				]
			},
			{
				text: 'Examples (TODO)',
				items: [
					{ text: 'Markdown Examples', link: '/markdown-examples' },
					{ text: 'Runtime API Examples', link: '/api-examples' }
				]
			}
		],

		socialLinks: [
			{ icon: 'github', link: 'https://github.com/zootella/ftorrent' }
		]
	}
})
