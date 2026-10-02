import {getCurrentWindow, currentMonitor, primaryMonitor, availableMonitors, LogicalPosition, LogicalSize} from '@tauri-apps/api/window'
import {invoke} from '@tauri-apps/api/core'

/*
The window's size and place, both halves: where it opens, and remembering where the user left it. Rust makes the window hidden, at any size, and window.rs says why it's made in code; everything about where it goes is here, in one file, so the rule that records a place and the rule that replays it can't drift apart.

What it's for. A user who sizes and places ftorrent just so, then quits, or restarts the computer, finds it right there the next time, and maximized, or zoomed on a Mac, if it was. A user who has changed the screen under it since, a new resolution, scale, or rotation, or the screen unplugged or moved among the others, gets a fresh place instead, as on the first run; ftorrent doesn't try to work out where the old place went, it starts over. Minimized and fullscreen are never recorded, so ftorrent never opens either way; recordWindow says why. And a portable copy is a guest on every desktop it runs on: it records no place, and opens somewhere fresh every time, never maximized.

What's recorded. Under [window] in the settings file, the window's outer position and inner size, and whether it's maximized; under [screen], the screen it was on, its position among the screens, its size, and its scale as a percent. The outer position because that's where the frame sits on the screen; the inner size because that's what setSize takes, and mixing the two would grow the window by a title bar every launch. Every position and size is in CSS pixels, the page's own unit, which isn't the same thing everywhere: on a Mac a CSS pixel is a point, the unit macOS lays out every screen in, two physical pixels across on a Retina display; on Windows it's a physical pixel divided by the screen's scale, so at 125% five physical pixels make four CSS ones. Tauri reports windows and screens in physical pixels, along with the scale that divides them back. A maximized window records only that it's maximized, and leaves the rest where the window was before, so the file always holds a normal size and place: the window opens there and is maximized, and restoring it goes back there.

Where it opens. The place is put back only onto the screen it was recorded on, found among the monitors by all five of its numbers matching exactly. The scale being one of them is what makes CSS pixels safe on Windows: dividing by a scale when the place is recorded and multiplying by the same scale when it's put back returns the same physical pixels, give or take one at a scale like 125%. Matching exactly is also the whole guard, skittish in one direction only: anything that changed that screen means a fresh place, while a screen nobody touched is never second-guessed, whatever happened to the others.

Tauri turns CSS pixels into physical ones by the scale of the screen the window is on at that moment, and the hidden window starts wherever the builder put it, usually on the primary screen. So before it's placed, the window is moved, still hidden, to the top left corner of the screen it's going to, in the physical pixels Tauri measures screens in, and only then sized and placed in CSS pixels, the place last so the top left corner is where the window ends up. On Windows with screens at different scales, that first move is what has the conversion use the right screen's scale; it relies on Windows giving a hidden window the scale of the screen it's moved to, as it does a visible one. On a Mac the first move changes nothing that matters: macOS places windows in points, which is exactly what the file holds, so the window lands in the same spot wherever the first move left it. With one screen, or every screen at one scale, the first move doesn't change the arithmetic at all.

A fresh place, which is also the first run, is five eighths of the primary screen wide and half as tall, though never more than three times as wide as it is tall, dropped at a random spot inside a field centered on the screen that's three quarters of it each way. The margin the field leaves, an eighth of the screen on every side, is deeper than any taskbar, menu bar, or dock, so the window misses the operating system's chrome without asking where it is, and the randomness keeps an installed copy and a portable one from opening exactly on top of each other.

The window is placed while it's hidden, given its light or dark theme and its font, maximized if it was, and only then shown, so it appears once, where it belongs and in the right colors and type. Maximizing goes last, beside showing, because on Windows maximizing a hidden window shows it. Then the page tells Rust the window is revealed, which is what lets a second launch, the tray, or the Dock bring it forward from then on. A place that can't be worked out still ends in showing the window, wherever the builder left it, since a window that never appears is worse than one in the wrong spot.

Recording and writing are separate on purpose. Every move and resize updates the store in memory, and hands the rendered file down to Rust to write when ftorrent exits, which is free; nothing touches the disk while the user drags. The file is written when the window is closed with its X, which in ftorrent hides it rather than quitting, and again by Rust at exit, which covers the user who quits from the tray or the File menu without ever closing the window, and a restart of Windows or a logout on the Mac, which reach the same exit. A user who parks the window just so and then loses power before either has lost the position and drags it once more; that's the trade for never debouncing.
*/

//how a fresh window is sized and placed, as fractions of the primary screen: the window is dropped uniformly at random wherever it fits inside the field. These are code rather than settings; a user changes the window by moving it, and the settings file records the result
const fieldFraction  = 0.75  //the field a new window is dropped into, a box centered on the screen this fraction of it each way; the margin left over is what keeps the window off the taskbar, menu bar, and dock
const widthFraction  = 0.625 //the window itself, five eighths of the screen wide
const heightFraction = 0.5   //and half as tall
const widthLimit     = 3     //but never wider than three times its height, so a super wide monitor gets a window, not a banner. A 16:9 screen makes a window about 2.2 to 1 and a 21:9 one just under 3, so only the 32:9 screens meet this
const fallbackScreen = {x: 0, y: 0, width: 1280, height: 800}//a screen to size against when none can be named, so there's still a window

export async function revealWindow(store) {//place the hidden window where the settings remember or somewhere fresh, give it its theme and font, maximize it if it was, show it, and tell rust it's revealed; call once, after the settings have loaded, with the settings store, whose settings hold factory values when the file couldn't be read
	let appWindow = getCurrentWindow()
	let guest = portable(store)
	try {
		let {monitor, place} = (!guest && await replayPlace(store.settings)) || await freshPlace()
		if (monitor) await appWindow.setPosition(monitor.position)//first to the top left corner of the screen it's going to, in the physical pixels tauri measures screens in, so the two calls below convert by that screen's scale
		await appWindow.setSize(new LogicalSize(place.width, place.height))
		await appWindow.setPosition(new LogicalPosition(place.x, place.y))//after the size, so the last word on where the window sits is its top left corner
		await themeWindow(store.settings.appearance.mode)//while still hidden, so it appears in its colors and its type; after the place, so a look that fails to take still leaves the window where it belongs
		await fontWindow(store.settings.appearance.font)
		if (!guest && store.settings.window.maximized) await appWindow.maximize()//last, beside showing, since on windows maximizing a hidden window shows it
	} finally {
		await appWindow.show()//whatever happened above, a window that never appears is the worst outcome
		await invoke('window_revealed')
	}
}

async function replayPlace(s) {//the place the settings remember and the monitor it goes on, if a place was recorded and the screen it was recorded on is still there exactly as it was; or false, which means find a fresh place
	if (!(s.window.width > 0 && s.window.height > 0)) return false//zeros are how the file says nothing has been recorded yet
	let monitor = (await availableMonitors()).find(m => Object.entries(screenOf(m)).every(([key, value]) => value == s.screen[key]))//all five numbers, exactly
	if (!monitor) return false//that screen is gone, or changed: a new resolution, scale, or rotation, or moved among the others
	return {monitor, place: {x: s.window.x, y: s.window.y, width: s.window.width, height: s.window.height}}
}

async function freshPlace() {//a fraction of the primary screen, dropped at random inside the centered field, and the monitor it goes on
	let monitor = await primaryMonitor()
	let screen = monitor ? screenOf(monitor) : fallbackScreen
	let height = Math.round(screen.height * heightFraction)
	let width  = Math.min(Math.round(screen.width * widthFraction), height * widthLimit)
	let fieldWidth  = screen.width  * fieldFraction
	let fieldHeight = screen.height * fieldFraction
	let fieldX = screen.x + (screen.width  - fieldWidth)  / 2//the field's top left corner, an eighth of the screen in from the edges
	let fieldY = screen.y + (screen.height - fieldHeight) / 2
	let x = Math.round(fieldX + Math.random() * Math.max(0, fieldWidth  - width))
	let y = Math.round(fieldY + Math.random() * Math.max(0, fieldHeight - height))
	return {monitor, place: {x, y, width, height}}
}

function screenOf(monitor) {//a monitor as [screen] records it: its position among the screens and its size, in css pixels, and its scale as a percent
	let scale = monitor.scaleFactor//tauri measures monitors in physical pixels, and this divides them back
	return {
		x: Math.round(monitor.position.x / scale), y: Math.round(monitor.position.y / scale),
		width: Math.round(monitor.size.width / scale), height: Math.round(monitor.size.height / scale),
		scale: Math.round(scale * 100),
	}
}

function portable(store) {//a portable copy is a guest on every desktop it runs on: it records no place, goes back to none, and never opens maximized
	return store.paths?.mode == 'portable'
}

export async function watchWindow(store) {//keep the settings store told where the window is, and write the file when the window is closed; call once, after the store has loaded
	let appWindow = getCurrentWindow()
	await appWindow.onCloseRequested(event => { event.preventDefault(); return store.save() })//the x: lifecycle.rs hides the window, and this writes the settings. Without preventDefault, tauri's handler goes on to destroy the window once this returns
	if (portable(store)) return//a guest records no place
	await appWindow.onMoved(() => recordWindow(appWindow, store))
	await appWindow.onResized(() => recordWindow(appWindow, store))
	await recordWindow(appWindow, store)//once now, because the two events report only changes, and a window that's never moved still has a place worth remembering
}

async function recordWindow(appWindow, store) {//the window's place and size and the screen it's on, into the store's [window] and [screen], in css pixels, and whether it's maximized
	if (await appWindow.isMinimized() || await appWindow.isFullscreen()) return//a minimized window reports a position like -32000, -32000 on windows, and a fullscreen one fills a screen it wasn't sized to. Fullscreen also isn't brought back, on purpose: on macOS it's a Space of its own, somewhere a user steps into for a while, and an application that opened into a new Space at launch would feel like it had taken over the screen. Maximized is different, the everyday way to work on Windows and zoom on the Mac, so that one is recorded below and comes back
	let s = store.settings
	s.window.maximized = await appWindow.isMaximized()
	if (s.window.maximized) { store.remember(); return }//a maximized window fills its screen, so leave the numbers below where the window was before: that's where restoring it goes
	let monitor = await currentMonitor()
	if (!monitor) return//tauri couldn't say which screen we're on, so there's no screen to record the place against
	let scale = await appWindow.scaleFactor()//what tauri multiplied the window's numbers by, the scale of the screen it's on
	let position = (await appWindow.outerPosition()).toLogical(scale)
	let size = (await appWindow.innerSize()).toLogical(scale)
	s.window.x = Math.round(position.x)
	s.window.y = Math.round(position.y)
	s.window.width = Math.round(size.width)
	s.window.height = Math.round(size.height)
	Object.assign(s.screen, screenOf(monitor))
	store.remember()//in memory and down to rust for the exit write, not to disk
}

//light or dark, for the window's frame and menu bar and for the page inside, from the appearance setting: light, dark, or system. The window's theme is the one switch. Tauri hands it to the web view, which reports it to the page as prefers-color-scheme, where style.css picks its colors by it; and given null, the window follows the system, and passes each change along without any code of ours
export async function themeWindow(mode) {
	await getCurrentWindow().setTheme(mode == 'system' ? null : mode)//tauri's null means follow the system
}

//the page's typefaces, from the appearance setting: system, inter, or verdana. style.css holds each look as a rule under html[data-font], and this sets the attribute; the page's, rather than the window's, but it's here beside the theme because both are how the window looks when it first appears
export async function fontWindow(font) {
	let carried = []//the faces ftorrent carries files for, which this choice draws in: wait for them first, so no text is drawn in a stand-in and then jumps; the system already has the others
	if (font == 'inter') carried.push(document.fonts.load('1em Inter'))
	if (font == 'inter' || font == 'verdana') carried.push(document.fonts.load('1em "IBM Plex Mono"'))//both draw fixed-width text in plex; style.css says why
	await Promise.all(carried)
	document.documentElement.dataset.font = font
}

export function windowWebviewVersion() { return invoke('window_webview_version') }//the version of the web view the page runs in, WebView2's on Windows and WebKit's on the Mac, as the platform reports it
