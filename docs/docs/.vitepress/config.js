import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
	title: "ftorrent",
	description: "docs.ftorrent.com",
	markdown: {
		// Open external links in the same tab so the browser back button works.
		// These docs hold no state (no forms), so leaving and returning is safe.
		externalLinks: { target: '_self' }
	},
	themeConfig: {
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
					{ text: 'Tracker Load', link: '/tracker-load' },
					{ text: 'Magnet Link User Guide', link: '/magnet-link-user-guide' }
				]
			},
			{
				text: 'Repository Documentation',
				items: [
					{ text: 'Dockerizing Aquatic', link: 'https://github.com/zootella/ftorrent/blob/master/open/README.md' },
					{ text: 'Tracker Page', link: 'https://github.com/zootella/ftorrent/blob/master/open/page/README.md' },
					{ text: 'Tracker Gauge', link: 'https://github.com/zootella/ftorrent/blob/master/open/gauge/README.md' },
					{ text: 'Tracker Circuit Breaker', link: 'https://github.com/zootella/ftorrent/blob/master/open/breaker/README.md' }
				]
			},
			{
				text: 'Notes and Planning',
				items: [
					{ text: 'Desktop Client Planning', link: '/desktop-client-planning' }
				]
			},
			{
				text: 'Examples',
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
