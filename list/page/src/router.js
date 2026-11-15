import {createRouter, createWebHistory} from 'vue-router'
import {trackerUrl} from './report.js'
import Home from './components/Home.vue'

/*
The page's two routes: the summary at /, and the summary with one tracker's details open below it, at the tracker's URL with the :// turned into a slash, like list.ftorrent.com/udp/tracker.example.com:6969/announce. Both draw the same component, so choosing a tracker adds its details to the page in place, and the address bookmarks the whole view.

These are real paths, which vue-router calls history mode: moving between pages, the router changes the address and draws the new page without asking the server for anything. A reload, or a link someone shares, does ask the server for the path, and no file is there, so the web server answers any path that isn't a file with 200.html, a copy of index.html the build writes, and the router reads the path from there.
*/
const router = createRouter({
	history: createWebHistory(),
	routes: [
		{path: '/', name: 'home', component: Home},
		{path: '/:scheme(udp|http|https|ws|wss)/:rest(.*)', name: 'tracker', component: Home},//the scheme, then everything after the slash it became, slashes and all
		{path: '/:pathMatch(.*)*', redirect: '/'},//anything else goes to the summary
	],
	scrollBehavior(to, from, saved) {//the back and forward buttons return to where the reader was; otherwise the page stays put, and brings a chosen tracker's details into view itself
		return saved || false
	},
})
router.afterEach(to => {//the tab names the tracker on its page
	document.title = to.name == 'tracker' ? `${trackerUrl(to.params.scheme, to.params.rest)} - list.ftorrent.com` : 'list.ftorrent.com'
})
export default router
