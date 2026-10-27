import {readFileSync} from 'node:fs'
import {fileURLToPath, URL} from 'node:url'

import {defineConfig} from 'vite'
import vue from '@vitejs/plugin-vue'
import tailwindcss from '@tailwindcss/vite'
import vueDevTools from 'vite-plugin-vue-devtools'

/*
The dashboard's data, page.json, on this machine. On the server the gauge writes it every minute into its own data directory, and the proxy answers /page.json from there, apart from the static files this workspace builds. Here the gauge's folder stands in for that directory: open/gauge/data/public/page.json is a committed sample, with round numbers and a 90-day history, and a local run of the gauge writes over it.

Read at request time and never copied. Kept out of public/, the sample is in no build, so the server's static files can never hold a page.json to serve in place of the gauge's live one, and the upload has nothing to leave out. The same answer serves pnpm preview, which serves the built dist/ and would otherwise find no page.json at all.
*/
function gaugeData() {
	let file = fileURLToPath(new URL('../gauge/data/public/page.json', import.meta.url))//from this file's own location, so the dev server finds it whichever folder it was started from
	let answer = (request, response, next) => {
		if (!/^\/page\.json(?:[?#]|$)/.test(request.url || '')) return next()
		response.setHeader('content-type', 'application/json')
		response.setHeader('cache-control', 'no-store')//a local gauge rewrites the file every minute, so a reload always reads the latest
		response.end(readFileSync(file))
	}
	return {
		name: 'ftorrent-gauge-data',
		configureServer(server) { server.middlewares.use(answer) },//added here rather than in a returned function, so it answers ahead of vite's own middlewares
		configurePreviewServer(server) { server.middlewares.use(answer) },
	}
}

//https://vite.dev/config/
export default defineConfig({
	plugins: [
		gaugeData(),
		vue(),
		tailwindcss(),
		vueDevTools(),
	],
})
