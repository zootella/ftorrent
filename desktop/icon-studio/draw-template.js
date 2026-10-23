import {execFileSync} from 'node:child_process'
import {copyFileSync, readFileSync} from 'node:fs'
import {dirname, join} from 'node:path'
import {fileURLToPath} from 'node:url'

/*
The Mac's menu bar icon. The icon studio's scripts are PowerShell and draw for Windows, but for this one and draw-gasket.js, the Mac's two steps; this one runs there by hand with Node and ImageMagick from Homebrew. "Template" is Apple's word: a template image is one macOS reads for its shape alone, its opaque pixels, and paints in the menu bar's own color, black on a light bar, white on a dark one, and inverted while the menu is open. That is why the Mac needs one file where Windows needs two. The Windows notification area paints an icon's pixels as they are, so the program carries the glyph in black and in white and picks by the taskbar's theme.

Both come from the same drawing. glyph.svg, the stencil, goes to Windows through Draw-Tray.ps1 and Join-Ico.ps1, which make tray-black.ico and tray-white.ico, and to the Mac through this script, which makes tray-template.png. The file is kept twice, here as the record and in src-tauri/icons/, where lifecycle.rs builds it into the program with include_image! and hands it to the system as a template. Nothing in the build runs this script, and Tauri never looks in this folder, so run it after any change to glyph.svg and commit both copies with the drawing.

It rewrites glyph.svg's root element so the 16 unit drawing sits on an 18 unit canvas, one unit in from each edge, at 36 pixels, which is the 18 points the menu bar draws at Retina; the README's section on the menu bar icon says why exactly that size. It pipes the result to ImageMagick, which has its own SVG renderer and needs no other library, with the flags the lines below explain. It reads the file back to make sure the glyph left ink, copies it into src-tauri/icons/, and prints the size, the channels, and how much of the square the glyph covers.
*/

const studio    = dirname(fileURLToPath(import.meta.url))//this folder, so it runs from anywhere, and the paths below are the studio's
const drawing   = join(studio, 'glyph.svg')
const drawn     = join(studio, 'tray-template.png')
const installed = join(studio, '../src-tauri/icons/tray-template.png')

//the root element glyph.svg has, and the one this draws it with: the drawing on an 18 unit canvas, one unit in from each edge, at 36 pixels, which is 18 points at Retina. it also drops the root's fill="none", which ImageMagick's own SVG renderer would otherwise apply over the path's black and render nothing
const rootGiven = 'width="1024" height="1024" viewBox="0 0 16 16" fill="none"'
const rootDrawn = 'width="36" height="36" viewBox="-1 -1 18 18"'

function main() {
	let svg = readFileSync(drawing, 'utf8')
	if (svg.split(rootGiven).length != 2) throw new Error(`glyph.svg's root element has changed: this script expects ${rootGiven}, exactly once`)
	execFileSync('magick', ['-background', 'none', 'svg:-', '-define', 'png:color-type=6', '-strip', drawn], {input: svg.replace(rootGiven, rootDrawn)})//the define writes red, green, blue, and alpha channels, which include_image! insists on, where ImageMagick would otherwise save a black drawing as gray and alpha; and strip drops the dated text chunks ImageMagick would otherwise write, so a rerun on an unchanged drawing writes an identical file

	let inked = Number(execFileSync('magick', [drawn, '-format', '%[fx:mean.a]', 'info:-'], {encoding: 'utf8'}))//the fraction of the square the glyph covers, read back from the file
	if (!(inked > 0)) throw new Error('tray-template.png has nothing in it: the renderer dropped the fill')//the trap above, caught if the renderer ever changes how it reads the root
	copyFileSync(drawn, installed)

	let described = execFileSync('magick', [drawn, '-format', '%w by %h, %[channels]', 'info:-'], {encoding: 'utf8'})
	console.log(`tray-template.png: ${described}, and the glyph covers ${Math.round(inked * 100)} percent of it`)
}

try { main() } catch (e) { console.error('🚧 Error:', e.message || e); process.exitCode = 1 }
