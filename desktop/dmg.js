import {spawnSync} from 'node:child_process'
import {mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync} from 'node:fs'
import {tmpdir} from 'node:os'
import {join} from 'node:path'
import {fileURLToPath} from 'node:url'

/*
The mac installer, made without Finder.

Tauri can make a dmg itself, and ftorrent let it until October 2026. Its way is a fork of the create-dmg shell script, which mounts the new image and runs an AppleScript asking Finder to open the window, place the icons, set the background, close the window again, and save what it saw as .DS_Store — then waits, with no limit, until that file appears. So every build stole keyboard focus with a window, two builds at once fought over the one Finder, a Finder showing hidden files stored every icon in the wrong place, and a window lost among others stalled the build until somebody closed it.

All Finder ever contributed was that one .DS_Store, and dmgbuild writes it directly. It mounts the image with -nobrowse, so Finder never learns the volume exists, writes the window, icon view, background alias and icon positions as records, then detaches and compresses. A build takes about five seconds, touches nothing on screen, and comes out the same whatever Finder is showing. Mounted beside Tauri's dmg, its records match the ones Finder wrote; what differs is where the background sits, /.background.jpg rather than .background/ceiling.jpg, and Finder's own bookmarks to the picture, which dmgbuild does not write and Finder does not need.

## Where the numbers come from

Measured on macOS Sequoia in October 2026, with a checkerboard of 200-point squares and a diamond touching each edge in the picture's place, so a clipped edge or an icon off its corner showed at a glance. Four things were learned that the code cannot say:

- The window size is the frame, title bar included. Showing all 800 points of picture takes 828, because Sequoia's title bar is 28 with the toolbar hidden. Tahoe redrew window chrome, and if its title bar is another height the bottom of the picture is clipped or letterboxed by the difference.
- The picture is drawn at its own size from the top left of the area under the title bar, and never scaled to fit the window.
- An icon's position is the center of its picture, measured from that same top left. The label hangs below and is not counted, so y 400 puts the icon's center on the middle line of the picture exactly.
- The window's position runs from the bottom left of the screen to the bottom left of the frame, with y up — Cocoa's coordinates, not AppleScript's. So no number means the same place on every screen, and tools that take a position from the top, create-dmg among them, quietly convert it on the screen of the Mac that ran the build. A window whose top would run past the menu bar is clamped just beneath it, which is what happens on most screens.

## The pins

dmgbuild is python, run through uvx, so nothing is installed and uv is the one requirement, on the mac that builds the installer and nowhere else — the same uv that freezes the engine. It is pinned, and so are ds_store and mac_alias, the two libraries that write the records and the alias measured above; dmgbuild itself asks only for some version or later of each, which would let a new release change the bytes of a dmg with nothing here changing.

The extension of ftorrent.app is not hidden. Hiding it sets a FinderInfo attribute on the bundle, and codesign --verify --strict rejects a bundle carrying one. Tauri's dmg never managed to hide it either, and nothing is lost: the label follows each viewer's own Finder setting, ftorrent.app with every extension shown and ftorrent without, the same as every application in /Applications, none of which carries the flag.
*/

//every path is built from this file's own location, for the reason scripts.js gives
const here    = fileURLToPath(new URL('.', import.meta.url))
const tauri   = join(here, 'src-tauri')
const bundled = join(tauri, 'target/release/bundle')//tauri leaves ftorrent.app in macos/ here, and this puts the dmg in dmg/ beside it, where hash in scripts.js looks

const dmgbuild     = 'dmgbuild@1.6.7'                     //pinned together, for the reason the essay gives
const dependencies = ['ds_store==1.3.3', 'mac_alias==2.2.3']

//the window a mac user meets: the whole picture and nothing around it, with each icon centered in its half. every number is in CSS pixels, and the essay says how each was measured
const layout = {
	window:       {position: {x: 400, y: 400}, size: {width: 800, height: 828}},//800 of picture under a 28 title bar. the position is from the bottom left of the screen, so the top would sit 1228 up; on any screen shorter than that and the menu bar, which is most, macOS clamps it just beneath the menu bar, 400 from the left
	iconSize:     128,//the label under it is 16, dmgbuild's default, which the json form of its settings cannot change
	background:   join(tauri, 'dmg/ceiling.jpg'),//1600 pixels tagged at 144 dpi, so 800 CSS pixels on an sRGB panel and a Retina one alike
	volumeIcon:   join(tauri, 'icons/mac/icon.icns'),//the dock icon, the same file tauri gave the volume
	app:          {x: 200, y: 400},//the center of each half, and the middle line of the picture
	applications: {x: 600, y: 400},
}

function main() {
	if (process.platform != 'darwin') return//a dmg is the mac's alone; windows finishes with tauri build

	let configuration = JSON.parse(readFileSync(join(tauri, 'tauri.conf.json'), 'utf8'))
	let brandName = configuration.productName//the name people read, which tauri names the .app and the dmg with; brand.js says how it and brandStem are used
	let arch = {arm64: 'aarch64', x64: 'x64'}[process.arch]//spelled the way tauri spells it, since hash reads the architecture out of the filename and into the published sidecar
	if (!arch) throw new Error('no dmg architecture for ' + process.arch)
	let file = `${brandName}_${configuration.version}_${arch}.dmg`

	//empty the folder first, so a dmg from an earlier version never gives hash two candidates. a copy still mounted in Finder is safe: it keeps reading the old file, and opening the new one mounts it beside, as ftorrent 1
	let folder = join(bundled, 'dmg')
	rmSync(folder, {recursive: true, force: true})
	mkdirSync(folder, {recursive: true})

	//the settings in the json form dmgbuild reads, which is appdmg's, in a private temporary folder so two builds at once never share a file
	let scratch = mkdtempSync(join(tmpdir(), 'ftorrent-dmg-'))
	let settings = join(scratch, 'settings.json')
	writeFileSync(settings, JSON.stringify({
		'title':             brandName,
		'icon':              layout.volumeIcon,
		'background':        layout.background,
		'icon-size':         layout.iconSize,
		'compression-level': 9,//zlib's best
		'window':            layout.window,
		'contents': [
			{type: 'file', path: join(bundled, `macos/${brandName}.app`), ...layout.app},
			{type: 'link', path: '/Applications',                    ...layout.applications},
		],
	}, null, '\t'))

	console.log(`bundling ${file} with ${dmgbuild}`)
	//uvx takes its own options before the tool and hands everything after it to the tool. --no-hidpi copies the picture as it is: without it, an @2x file beside ceiling.jpg would make dmgbuild combine the two into a tiff, and the one 144 dpi jpeg already serves both kinds of panel
	let run = spawnSync('uvx', [
		...dependencies.flatMap(dependency => ['--with', dependency]), dmgbuild,
		'--no-hidpi', '-s', settings, brandName, join(folder, file),
	], {stdio: 'inherit'})
	rmSync(scratch, {recursive: true, force: true})
	if (run.error && run.error.code == 'ENOENT') throw new Error('uvx is not on the path; the mac installer needs uv — brew install uv')
	if (run.error) throw run.error
	if (run.status != 0) throw new Error(`dmgbuild exited ${run.status}`)
	console.log('finished ' + join(folder, file))

	//the same app zipped, which hash publishes as ftorrent.app.zip for a running copy to update from without mounting a disk image; ditto, the mac's own archiver, keeps the bundle's signature intact, and --keepParent puts the ftorrent.app folder itself at the top of the zip, where update.js looks for it. In a folder of its own beside dmg/, emptied first for the same reason
	let zipFolder = join(bundled, 'app-zip')
	let zipFile = `${brandName}_${configuration.version}_${arch}.app.zip`
	rmSync(zipFolder, {recursive: true, force: true})
	mkdirSync(zipFolder, {recursive: true})
	let zip = spawnSync('ditto', ['-c', '-k', '--keepParent', join(bundled, `macos/${brandName}.app`), join(zipFolder, zipFile)], {stdio: 'inherit'})
	if (zip.error) throw zip.error
	if (zip.status != 0) throw new Error(`ditto exited ${zip.status}`)
	console.log('finished ' + join(zipFolder, zipFile))
}

try { main() } catch (e) { console.error('🚧 Error:', e.message || e); process.exitCode = 1 }
