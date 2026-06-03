import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
	title: "ftorrent",
	description: "docs.ftorrent.com",
	themeConfig: {
		// https://vitepress.dev/reference/default-theme-config
		nav: [
			{ text: 'Home', link: '/' }
		],

		sidebar: [
			{
				text: 'Project',
				items: [
					{ text: 'Roadmap in Reverse', link: '/roadmap-in-reverse' },
					{ text: 'Tracker Load', link: '/tracker-load' }
				]
			},
			{
				text: 'Guides',
				items: [
					{ text: 'Magnet Link User Guide', link: '/magnet-link-user-guide' }
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
