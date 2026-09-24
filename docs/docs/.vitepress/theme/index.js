// https://vitepress.dev/guide/custom-theme
import { h } from 'vue'
import DefaultTheme from 'vitepress/theme'
import DownloadLink from './components/DownloadLink.vue'
import DownloadCommand from './components/DownloadCommand.vue'
import './style.css'

/** @type {import('vitepress').Theme} */
export default {
	extends: DefaultTheme,
	Layout: () => {
		return h(DefaultTheme.Layout, null, {
			// https://vitepress.dev/guide/extending-default-theme#layout-slots
		})
	},
	enhanceApp({ app, router, siteData }) {
		//registered globally so markdown can place them: DownloadLink is one installer's box, and DownloadCommand wraps a fenced install command in the markdown and writes the current hash into it when the page opens. downloads.js has the system they belong to
		app.component('DownloadLink', DownloadLink)
		app.component('DownloadCommand', DownloadCommand)
	}
}
