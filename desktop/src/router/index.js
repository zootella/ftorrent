import {createRouter, createWebHashHistory} from 'vue-router'
import MainPage from '../pages/MainPage.vue'

/*
The router is this app's table of contents: every page the window can show is named once, here, and the rest of the code navigates by those names rather than by wiring components together by hand.

We run it in hash mode. A router's other mode, history mode, writes real paths like /about and expects a server to answer a request for that path when the page reloads; a Tauri window loads its frontend out of the bundle and has no server behind it, so a reload of /about would find nothing. Hash mode keeps the whole route after a # — the part a browser resolves locally and never requests — which is exactly right here. The user never sees it: the window has no address bar.
*/
const router = createRouter({
	history: createWebHashHistory(),
	routes: [
		{path: '/',      name: 'main',  component: MainPage},//the page the window opens on, so it's imported up front rather than fetched a moment later
		{path: '/about', name: 'about', component: () => import('../pages/AboutPage.vue')},//an arrow function instead of a component makes this a lazy route: the build gives the page its own chunk, and the app loads it the first time someone opens it
		{path: '/:pathMatch(.*)*', redirect: '/'},//anything unrecognized goes home rather than matching no route at all; with no address bar the only ways to get here are a stale hash left in the webview by a development reload or a mistaken navigate in our own code, and either one would otherwise leave the navigation sitting above an empty page
	],
})
export default router
