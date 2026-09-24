//./docs/.vitepress/theme/curl.data.js

import { defineLoader, createMarkdownRenderer } from 'vitepress'

/*
The two install commands, highlighted during the build so they match every other code block on the page.

A fenced block in markdown is colored while the site is built: Shiki breaks it into tokens and wraps each in a span carrying the site's light and dark colors, and the browser receives finished spans. DownloadCommand.vue cannot be a fence, because the hash it carries is not known until the page opens and the sidecar arrives, and a block assembled in the browser meets no highlighter. So this loader renders each command as a fence here, in Node during the build, with a run of zeros standing where the hash will go, and hands the finished HTML to the component, which swaps the real hash in. Same renderer, same theme, same copy button, and nothing shipped to the browser that was not already going.

VitePress runs a *.data.js file during the build and in the dev server, and a component imports its result as data. The renderer is reached the way VitePress's own content loader reaches it: the resolved config sits on a global while VitePress is running, and its source directory, markdown options, base and logger are what the pages themselves are rendered with, which is what makes the colors identical rather than similar.
*/

export const placeholder = '0'.repeat(64)//stands where the hash goes, and has to come through highlighting as one run of text so the swap is one replace. a hex-shaped run inside a quoted string does, in both shells, and load checks it rather than assuming

//the command per file, with the download address written in. each downloads into the Downloads folder, checks the hash, and installs only on a match. the windows one names curl.exe because in windows powershell bare curl is an alias for a different command with different flags, and -eq compares the hash without regard to case, so powershell's uppercase matches the sidecar's lowercase. the mac one chains with && so a failed check stops the chain, and mounts at a fixed folder rather than under the image's own name, which lands on "ftorrent 1" when an ftorrent volume is already open
function commands(origin) {
	return {
		'ftorrent.exe': {language: 'powershell', text:
`cd ~\\Downloads
curl.exe -fsSLO ${origin}/ftorrent.exe
if ((Get-FileHash ftorrent.exe).Hash -eq '${placeholder}') { .\\ftorrent.exe } else { 'The hash does not match. ftorrent was not installed.' }`},

		'ftorrent.dmg': {language: 'bash', text:
`cd ~/Downloads && \\
curl -fsSLO ${origin}/ftorrent.dmg && \\
echo "${placeholder}  ftorrent.dmg" | shasum -a 256 -c && \\
hdiutil attach -nobrowse -quiet -mountpoint ftorrent-image ftorrent.dmg && \\
cp -R ftorrent-image/ftorrent.app /Applications/ && \\
hdiutil detach -quiet ftorrent-image`},
	}
}

export default defineLoader({
	async load() {
		let config = globalThis.VITEPRESS_CONFIG
		if (!config) throw new Error('curl.data.js needs a running vitepress, which puts its resolved config on this global')
		let md = await createMarkdownRenderer(config.srcDir, config.markdown, config.site.base, config.logger)

		let html = {}
		for (let [file, command] of Object.entries(commands(config.site.themeConfig.origin))) {
			let rendered = md.render('```' + command.language + '\n' + command.text + '\n```')
			if (rendered.split(placeholder).length != 2) throw new Error('the hash placeholder did not come through highlighting as one run of text in the ' + file + ' command')
			html[file] = rendered
		}
		return {placeholder, html}
	},
})
