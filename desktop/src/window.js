import {getCurrentWindow, currentMonitor, primaryMonitor, monitorFromPoint, LogicalPosition, LogicalSize} from '@tauri-apps/api/window'
import {invoke} from '@tauri-apps/api/core'

/*
The window's size and place, both halves: where it opens, and remembering where the user left it. Rust makes the window hidden, at any size, and window.rs says why it's made in code; everything about where it goes is here, in one file, so the rule that records a place and the rule that replays it can't drift apart.

Where it opens. ftorrent is single window, so it decides both size and place every time and never lets the operating system choose. After a session the settings file holds six numbers, all in CSS pixels, the window's outer position and inner size under [window] and under [screen] the size of the monitor it was on, and beside them whether it was maximized. At startup the monitor under the middle of the saved window is asked for; the middle rather than the corner, because Windows counts an invisible resize border as part of the window, so a window flush against the left edge of the screen has its corner a few pixels off it. If there's such a monitor and it's the size the file says, the ground beneath the user's choice is the same, and the rectangle is replayed exactly, so a window parked against the taskbar comes back there tomorrow. Anything else means a fresh place: no monitor under the point because a display was unplugged, or a different size because the resolution, rotation, or scaling changed. That's the whole guard, a fingerprint of the ground rather than reasoning about it.

A fresh place, which is also the first run, is five eighths of the primary screen wide and half as tall, dropped at a random spot inside a field centered on the screen that's three quarters of it each way. The margin the field leaves, an eighth of the screen on every side, is deeper than any taskbar, menu bar, or dock, so the window misses the operating system's chrome without asking where it is, and the randomness keeps an installed copy and a portable one from opening exactly on top of each other. Monitors report physical pixels and the file holds CSS pixels, so the saved middle is scaled by the primary monitor's scale before the monitor under it is asked for; on monitors with different scales that can land a little off, and the worst case is a fresh place, which is the fallback anyway.

The window is placed while it's hidden, maximized if it was, and only then shown, so it appears once, where it belongs. Maximizing goes last, beside showing, because on Windows maximizing a hidden window shows it. Then the page tells Rust the window is revealed, which is what lets a second launch, the tray, or the Dock bring it forward from then on. A place that can't be worked out still ends in showing the window, wherever the builder left it, since a window that never appears is worse than one in the wrong spot.

Recording and writing are separate on purpose. Every move and resize updates the store in memory, and hands the rendered file down to Rust to write when ftorrent exits, which is free; nothing touches the disk while the user drags. The file is written when the window is closed with its X, which in ftorrent hides it rather than quitting, and again by Rust at exit, which covers the user who quits from the tray or the File menu without ever closing the window. A user who parks the window just so and then loses power before either has lost the position and drags it once more; that's the trade for never debouncing.

Everything recorded is in CSS pixels. Tauri reports positions and sizes in physical pixels and hands over the scale factor to divide by, and dividing here is what makes the numbers in the file mean the same thing on a Retina display and a plain one. The outer position is recorded because that's where the frame sits on the screen; the inner size because that's what the builder takes, and mixing the two would grow the window by a title bar every launch. A maximized window records only that it's maximized, and leaves the position and size where the window was before, so the file always holds a normal size and place: the window opens there and is maximized, and restoring it goes back there. Fullscreen isn't recorded or brought back, on purpose; see recordWindow.
*/

//how a fresh window is sized and placed, as fractions of the primary screen: the window is dropped uniformly at random wherever it fits inside the field. These are code rather than settings; a user changes the window by moving it, and the settings file records the result
const fieldFraction  = 0.75  //the field a new window is dropped into, a box centered on the screen this fraction of it each way; the margin left over is what keeps the window off the taskbar, menu bar, and dock
const widthFraction  = 0.625 //the window itself, five eighths of the screen wide
const heightFraction = 0.5   //and half as tall
const fallbackScreen = {x: 0, y: 0, width: 1280, height: 800}//a screen to size against when none can be named, so there's still a window

export async function revealWindow(settings) {//place the hidden window where the settings remember or somewhere fresh, maximize it if it was, show it, and tell rust it's revealed; call once, after the settings have loaded, with the store's settings, which hold factory values when the file couldn't be read
	let appWindow = getCurrentWindow()
	try {
		let place = await replayPlace(settings) || await freshPlace()
		await appWindow.setSize(new LogicalSize(place.width, place.height))
		await appWindow.setPosition(new LogicalPosition(place.x, place.y))
		if (settings.window.maximized) await appWindow.maximize()//last, beside showing, since on windows maximizing a hidden window shows it
	} finally {
		await appWindow.show()//whatever happened above, a window that never appears is the worst outcome
		await invoke('window_revealed')
	}
}

async function replayPlace(s) {//the rectangle the settings remember, if one was recorded and the monitor under its middle is still the size recorded with it, or false, which means find a fresh place
	if (!(s.window.width > 0 && s.window.height > 0)) return false//zeros are how the file says nothing has been recorded yet
	let scale = (await primaryMonitor())?.scaleFactor ?? 1//css to physical by the primary monitor's scale, the approximation the essay names
	let monitor = await monitorFromPoint((s.window.x + s.window.width / 2) * scale, (s.window.y + s.window.height / 2) * scale)
	if (!monitor) return false//no monitor there: the ground is gone
	let width  = Math.round(monitor.size.width  / monitor.scaleFactor)
	let height = Math.round(monitor.size.height / monitor.scaleFactor)
	if (width != s.screen.width || height != s.screen.height) return false//a different screen than the one the place was recorded on, so the place means nothing
	return {x: s.window.x, y: s.window.y, width: s.window.width, height: s.window.height}
}

async function freshPlace() {//a fraction of the primary screen, dropped at random inside the centered field
	let monitor = await primaryMonitor()
	let screen = monitor ? {
		x: monitor.position.x / monitor.scaleFactor, y: monitor.position.y / monitor.scaleFactor,//monitors are measured in physical pixels, and the window is placed in css ones
		width: monitor.size.width / monitor.scaleFactor, height: monitor.size.height / monitor.scaleFactor,
	} : fallbackScreen
	let width  = Math.round(screen.width  * widthFraction)
	let height = Math.round(screen.height * heightFraction)
	let fieldWidth  = screen.width  * fieldFraction
	let fieldHeight = screen.height * fieldFraction
	let fieldX = screen.x + (screen.width  - fieldWidth)  / 2//the field's top left corner, an eighth of the screen in from the edges
	let fieldY = screen.y + (screen.height - fieldHeight) / 2
	let x = Math.round(fieldX + Math.random() * Math.max(0, fieldWidth  - width))
	let y = Math.round(fieldY + Math.random() * Math.max(0, fieldHeight - height))
	return {x, y, width, height}
}

export async function watchWindow(store) {//keep the settings store told where the window is, and write the file when the window is closed; call once, after the store has loaded
	let appWindow = getCurrentWindow()
	await appWindow.onMoved(() => recordWindow(appWindow, store))
	await appWindow.onResized(() => recordWindow(appWindow, store))
	await appWindow.onCloseRequested(event => { event.preventDefault(); return store.save() })//the x: lifecycle.rs hides the window, and this writes where it was. Without preventDefault, tauri's handler goes on to destroy the window once this returns
	await recordWindow(appWindow, store)//once now, because the two events report only changes, and a window that's never moved still has a place worth remembering
}

async function recordWindow(appWindow, store) {//the window's place and size and the screen it's on, into the store's [window] and [screen], in css pixels, and whether it's maximized
	if (await appWindow.isMinimized() || await appWindow.isFullscreen()) return//a minimized window reports a position like -32000, -32000 on windows, and a fullscreen one fills a screen it wasn't sized to. Fullscreen also isn't brought back, on purpose: on macOS it's a Space of its own, somewhere a user steps into for a while, and an application that opened into a new Space at launch would feel like it had taken over the screen. Maximized is different, the everyday way to work on Windows and zoom on the Mac, so that one is recorded below and comes back
	let s = store.settings
	s.window.maximized = await appWindow.isMaximized()
	if (s.window.maximized) { store.remember(); return }//a maximized window fills its screen, so leave the numbers below where the window was before: that's where restoring it goes
	let monitor = await currentMonitor()
	if (!monitor) return//tauri couldn't say which screen we're on, so there's no fingerprint to record against
	let scale = await appWindow.scaleFactor()
	let position = (await appWindow.outerPosition()).toLogical(scale)
	let size = (await appWindow.innerSize()).toLogical(scale)
	s.window.x = Math.round(position.x)
	s.window.y = Math.round(position.y)
	s.window.width = Math.round(size.width)
	s.window.height = Math.round(size.height)
	s.screen.width = Math.round(monitor.size.width / monitor.scaleFactor)
	s.screen.height = Math.round(monitor.size.height / monitor.scaleFactor)
	store.remember()//in memory and down to rust for the exit write, not to disk
}
