/*
The desktop client's downloads, as the documentation site sees them: from the machine that built an installer to the page that offers it.

An installer is built where it can be, the exe on Windows and the dmg on a Mac, and `pnpm hash` in the desktop workspace stages it under a published name and writes a small JSON sidecar beside it: file, version, arch, size, sha256, and the moment it was hashed, as tick, ergo's number, and as date, its iso text. `pnpm upload` sends the pair to ftorrent.com, so an installer and its sidecar sit at the apex: ftorrent.com/ftorrent.dmg beside ftorrent.com/ftorrent.dmg.json. Nothing in this build knows a hash. A page fetches the sidecars when it opens, so publishing an installer changes what the site says without the site being rebuilt.

	desktop/scripts.js              pnpm hash writes a sidecar, pnpm upload sends it to ftorrent.com
	  ↓                             or, in development, .vitepress/config.js serves it
	theme/downloads.js              this file: the fetch, a day in words, and the clipboard call
	  ↓
	components/DownloadLink.vue     one download; installing-ftorrent.md places six
	components/DownloadCommand.vue  the hash written into a fenced install command; installing-ftorrent.md places two

This site is docs.ftorrent.com and the sidecars live on ftorrent.com, so in production the fetch crosses origins, and it works only because ftorrent.com sends an Access-Control-Allow-Origin header with its downloads. A download host without that header leaves every sidecar unreadable, the fetch reads that as missing, and the page says Not yet published. Development fetches the same names from its own origin instead, where config.js answers from what is staged on this machine and proxies the rest to production, so nothing cross-origin happens there.

**Fetch on mount, never at module scope.** VitePress prerenders components in Node at build time, and Node has fetch, so a module-scope call would run rather than fail loudly, but nothing is serving these files to a build, and every box would bake in as unpublished.
*/
import { hasInt, hasTick, saySize4, sayTick } from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go

//where a sidecar is fetched from. the one place development and production differ: production asks ftorrent.com, development asks its own server, which config.js arranges to answer
function sidecarAddress(file, origin) {
	if (import.meta.env.DEV) return '/' + file + '.json'
	return origin + '/' + file + '.json'
}

//one sidecar, or false. a missing one means that installer is not published yet, which is an ordinary answer rather than an error. it sits beside its installer under the same name, so callers name the file, and pass the origin config.js keeps in themeConfig
export async function fetchSidecar(file, origin) {
	try {
		let response = await fetch(sidecarAddress(file, origin))
		if (!response.ok) return false
		let sidecar = await response.json()
		if (!sidecar || !sidecar.sha256 || !sidecar.file) return false//malformed reads the same as missing
		return sidecar
	} catch (error) { return false }//the network refused, the browser blocked the cross-origin read, or the body was not json
}

//the day a sidecar's tick falls on in UTC, like "2026 September 22", the same for every reader wherever they are; blank for a sidecar without a tick, so a malformed one shows the rest of its box
export function sayDay(t) {
	if (!hasTick(t)) return ''
	return sayTick(t, 'YYYY Mmmm D')
}

//a sidecar's size in four digits or less, counted by 1024 the way BitTorrent clients and Windows count, so an installer the Mac's browser calls 22 MB reads 21 MB here; blank for a sidecar without a whole count, so a malformed one shows the rest of its box
export function saySize(n) {
	if (!hasInt(n) || n < 0) return ''//saySize4 throws on anything but a whole count
	return saySize4(n)
}

//copy text to the clipboard, answering whether it worked. the clipboard needs a secure context, which https and localhost both are, so a false here is a browser refusing rather than a mistake to fix
export async function copyText(text) {
	try {
		await navigator.clipboard.writeText(text)
		return true
	} catch (error) { return false }
}
