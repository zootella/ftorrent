import {mkdirSync, readFileSync, writeFileSync} from 'node:fs'
import {dirname, join} from 'node:path'
import {fileURLToPath} from 'node:url'

/*
The Mac's application icon from Tahoe on: the gasket, brand.svg's mark, as the one layer of an Icon Composer icon. From Tahoe on, macOS draws every app's icon on a rounded square tile of its own, and an icon that doesn't fill the tile, like the free-standing pill the .icns carries, is shrunk onto a gray one. An Icon Composer icon is made for the tile: AppIcon.icon in src-tauri/icons holds icon.json, which gives the tile a white fill and draws it flat, with no glass, sheen, or shadow, and this layer, the gasket alone on a clear 1024 canvas. pnpm icons compiles the two into Assets.car with Xcode's actool, and the README has the rest.

The gasket is 820 pixels wide, 80 percent of the canvas, the width round logos like Chrome's sit at on Tahoe's tile, measured on macOS 27.0.1, so ftorrent sits beside them at their size. brand.svg's mark is 16 units wide on its 16 unit grid, so every coordinate is multiplied by 51.25 and moved 102 pixels in, which centers the grid on the canvas and the mark with it. The numbers are written out rather than left to a transform, so whatever reads the layer gets plain coordinates.

Nothing in the build runs this script. Run it on any change to brand.svg, then pnpm icons on a Mac with Xcode, and commit the layer and Assets.car with the drawing.
*/

const studio  = dirname(fileURLToPath(import.meta.url))//this folder, so it runs from anywhere, and the paths below are the studio's
const drawing = join(studio, 'brand.svg')
const layer   = join(studio, '../src-tauri/icons/AppIcon.icon/Assets/gasket.svg')

const canvas = 1024//Icon Composer's canvas, in pixels
const width  = 820//the gasket's width on it
const grid   = 16//brand.svg's grid, square, with the mark its full width
const scale  = width / grid//51.25 pixels to a unit
const inset  = (canvas - width) / 2//102 pixels in from each edge, the same down as across, since the grid is square and the mark sits at its middle

let place = u => figure(inset + u * scale)//a coordinate on the grid to one on the canvas
let size  = u => figure(u * scale)//a length on the grid, like a radius, to one on the canvas
function figure(n) { return String(+n.toFixed(3)) }//three places at most, and no trailing zeros

//one path's d, every point moved onto the canvas; it reads the absolute commands brand.svg uses, M, L, H, V, A, and Z, and stops at any other
function placePath(d) {
	let tokens = d.match(/[A-Za-z]|-?[0-9.]+/g)
	let out = [], command = '', numbers = []
	let flush = () => {
		if (!command) return
		if (command == 'M' || command == 'L') {
			let pairs = []
			for (let i = 0; i < numbers.length; i += 2) pairs.push(`${place(numbers[i])} ${place(numbers[i + 1])}`)
			out.push(command + pairs.join(' '))//a second pair after M or L continues it as a line, so pairs keep a space between them
		}
		else if (command == 'H' || command == 'V') out.push(command + numbers.map(place).join(' '))
		else if (command == 'A') for (let i = 0; i < numbers.length; i += 7) {
			let [rx, ry, turn, large, sweep, x, y] = numbers.slice(i, i + 7)
			out.push(`A${size(rx)} ${size(ry)} ${turn} ${large} ${sweep} ${place(x)} ${place(y)}`)//the flags and the turn stay as they are, since scaling alike on both axes changes neither
		}
		else if (command == 'Z') out.push('Z')
	}
	for (let token of tokens) {
		if (/[A-Za-z]/.test(token)) {
			if (!'MLHVAZ'.includes(token)) throw new Error(`brand.svg's path uses ${token}, which this script doesn't read; it reads M, L, H, V, A, and Z`)
			flush()
			command = token, numbers = []
		} else numbers.push(Number(token))
	}
	flush()
	return out.join('')
}

function main() {
	let svg = readFileSync(drawing, 'utf8')
	if (!svg.includes('viewBox="0 0 16 16"')) throw new Error('brand.svg is no longer on its 16 unit grid, which this script places')
	let shapes = svg.match(/<(rect|path|circle|ellipse|line|polygon|polyline|g|use|text)\b/g) || []
	let rect = svg.match(/<rect x="([0-9.]+)" y="([0-9.]+)" width="([0-9.]+)" height="([0-9.]+)" rx="([0-9.]+)" fill="([^"]+)"\/>/)
	let path = svg.match(/<path d="([^"]+)" fill="([^"]+)"\/>/)
	if (shapes.length != 2 || !rect || !path) throw new Error('brand.svg is no longer one rect and one path, the two shapes this script places')

	let [x, y, w, h, rx] = rect.slice(1, 6).map(Number), orange = rect[6]
	let gasket = [
		`<svg width="${canvas}" height="${canvas}" viewBox="0 0 ${canvas} ${canvas}" xmlns="http://www.w3.org/2000/svg">`,
		`<rect x="${place(x)}" y="${place(y)}" width="${size(w)}" height="${size(h)}" rx="${size(rx)}" fill="${orange}"/>`,
		`<path d="${placePath(path[1])}" fill="${path[2]}"/>`,
		'</svg>',
	].join('\n') + '\n'
	mkdirSync(dirname(layer), {recursive: true})
	writeFileSync(layer, gasket)
	console.log(`gasket.svg: brand.svg's mark ${size(grid)} pixels wide on a ${canvas} pixel canvas, in src-tauri/icons/AppIcon.icon/Assets`)
}

try { main() } catch (e) { console.error('🚧 Error:', e.message || e); process.exitCode = 1 }
