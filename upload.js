import {execFileSync} from 'node:child_process'
import {existsSync} from 'node:fs'
import {join} from 'node:path'
import {fileURLToPath} from 'node:url'

/*
Ships a static site: the build output of one workspace, mirrored into its directory on the server over rsync. Four workspaces use it, site, docs, open's page and good's page, which is why it is one file at the repository root rather than a line in each; each workspace's pnpm upload builds and then calls in here with its name. desktop/scripts.js is the other publisher, for the installers, and says why it is a file of its own: a different account, a different protocol, and one product.

rsync mirrors with --delete, so a file dropped from a build leaves the server too, and goes as the account that administers the server. The server's side is unchanged by any of this: the same files arrive in the same directories over the same port as before.

## Running this against your own server

The destination comes from .env at the repository root, which is gitignored, so a fresh clone will not have one and this will say which values are missing and stop. Write it yourself, no logic, the three the sites share and one directory per site:

	DEPLOY_HOST=example.com
	DEPLOY_PORT=22
	DEPLOY_SITES_USER=deploy
	DEPLOY_SITE_PATH=/var/www/example.com/
	DEPLOY_DOCS_PATH=/var/www/docs.example.com/
	DEPLOY_OPEN_PATH=/var/www/open.example.com/
	DEPLOY_GOOD_PATH=/var/www/good.example.com/

Keep the trailing slashes. The same file carries the installer values desktop/scripts.js reads, named DEPLOY_FILES_*, so one file names the server once.

The file sits at the root, above every workspace, on purpose. Vite loads a .env from the workspace it builds and copies any VITE_-prefixed value into the client bundle, which would publish these on the site itself, and Nuxt, VitePress and the Tauri CLI each read one from their own folder the same way; a .env above all of them is outside what any of them looks at. Node does not read it on its own: the package.json scripts pass --env-file-if-exists, which is why this runs as pnpm upload rather than node upload.js. The tolerant spelling is deliberate, because plain --env-file makes node refuse to start when the file is absent, before this can say anything useful.
*/

const root = fileURLToPath(new URL('.', import.meta.url))//every path is built from this file's own location, never from the working directory, because each workspace calls it from its own folder

const sites = {//each static workspace: what its build writes, and the env value naming the directory on the server it mirrors into
	site: {built: 'site/.output/public',       path: 'DEPLOY_SITE_PATH'},
	docs: {built: 'docs/docs/.vitepress/dist', path: 'DEPLOY_DOCS_PATH'},
	open: {built: 'open/page/dist',            path: 'DEPLOY_OPEN_PATH'},
	good: {built: 'good/page/dist',            path: 'DEPLOY_GOOD_PATH'},
}

function say(line) { console.log(line) }

function readServer(site) {//gather the destination from the environment, naming whatever the env file did not fill
	let required = ['DEPLOY_HOST', 'DEPLOY_PORT', 'DEPLOY_SITES_USER', site.path]
	let missing = required.filter(name => !process.env[name])
	if (missing.length) throw new Error(`.env at the repository root is missing ${missing.join(', ')}; see the comment atop upload.js, and run this as pnpm upload rather than node, so the env file is passed`)
	return {host: process.env.DEPLOY_HOST, port: process.env.DEPLOY_PORT, user: process.env.DEPLOY_SITES_USER, path: process.env[site.path]}
}

function upload(name) {
	let site = sites[name]
	if (!site) throw new Error('say which site: ' + Object.keys(sites).join(', '))
	if (process.platform == 'win32') throw new Error('the sites ship from the Mac; Windows has no rsync, and this would fail with a spawn error rather than a sentence')
	let built = join(root, site.built)
	if (!existsSync(built)) throw new Error('no build to upload; run pnpm build first: ' + built)
	let server = readServer(site)
	execFileSync('rsync', [
		'-avz', '--delete',
		built + '/',//the slash sends the folder's contents rather than the folder
		`${server.user}@${server.host}:${server.path}`,
		'-e', `ssh -p ${server.port}`,//rsync spells the port inside the ssh command, lowercase -p
	], {stdio: 'inherit'})
	say(`sent     ${name}, mirrored`)
}

try { upload(process.argv[2]) } catch (e) { console.error('🚧 Error:', e.message || e); process.exitCode = 1 }
