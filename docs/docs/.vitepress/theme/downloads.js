/*
The desktop client's downloads, as the documentation site sees them: from the machine that built an installer to the page that offers it.

An installer is built where it can be, the exe on Windows and the dmg on a Mac, and `pnpm hash` in the desktop workspace stages it under a published name and writes a small JSON sidecar beside it: file, version, arch, bytes, sha256, date. `pnpm upload` sends the pair to ftorrent.com, so an installer and its sidecar sit at the apex: ftorrent.com/ftorrent.dmg beside ftorrent.com/ftorrent.dmg.json. Nothing in this build knows a hash. A page fetches the sidecars when it opens, so publishing an installer changes what the site says without the site being rebuilt.

	desktop/scripts.js              pnpm hash writes a sidecar, pnpm upload sends it to ftorrent.com
	  ↓                             or, in development, .vitepress/config.js serves it
	theme/downloads.js              this file: the fetch, two small readings and the clipboard call
	  ↓
	components/DownloadLink.vue     one download; installing-ftorrent.md places six
	components/DownloadCommand.vue  the hash written into a fenced install command; installing-ftorrent.md places two

This site is docs.ftorrent.com and the sidecars live on ftorrent.com, so in production the fetch crosses origins, and it works only because ftorrent.com sends an Access-Control-Allow-Origin header with its downloads. A download host without that header leaves every sidecar unreadable, the fetch reads that as missing, and the page says Not yet published. Development fetches the same names from its own origin instead, where config.js answers from what is staged on this machine and proxies the rest to production, so nothing cross-origin happens there.

**Fetch on mount, never at module scope.** VitePress prerenders components in Node at build time, and Node has fetch, so a module-scope call would run rather than fail loudly, but nothing is serving these files to a build, and every box would bake in as unpublished.
*/

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

//"2026-09-22" as "2026 September 22". the parts go into Date as local numbers rather than through its string parser, which reads a bare date as UTC midnight and would render it as the day before for anyone west of Greenwich; the locale is fixed because the rest of the page is English
export function readableDate(iso) {
	let [year, month, day] = iso.split('-').map(Number)
	if (!year || !month || !day) return iso//a date we did not expect shows exactly as it arrived
	return year + ' ' + new Date(year, month - 1, day).toLocaleDateString('en', {month: 'long'}) + ' ' + day
}

//copy text to the clipboard, answering whether it worked. the clipboard needs a secure context, which https and localhost both are, so a false here is a browser refusing rather than a mistake to fix
export async function copyText(text) {
	try {
		await navigator.clipboard.writeText(text)
		return true
	} catch (error) { return false }
}

//whole megabytes, counted by 1000 the way macOS and browsers do; the installers run from about 12 to 26 MB, where a megabyte is fine enough to tell them apart. whole, because a decimal point is a comma in half the world, and a number with no separator is right in both
export function saySize(bytes) {
	return Math.round(bytes / 1_000_000) + ' MB'
}
