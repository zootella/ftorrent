//./src/window.js

import {getCurrentWindow, currentMonitor} from '@tauri-apps/api/window'

/*
The page's half of the window. Rust makes it, hidden, at the size and place the settings file remembers or a fresh one; window.rs is the long version of how that's decided. This module does the two things only the page can: show the window once there's something drawn in it, and keep the settings store told where the window is, so that what Rust reads next time is where the user left it.

Recording and writing are separate on purpose. Every move and resize updates the store in memory, and hands the rendered file down to Rust to write when ftorrent exits, which is free; nothing touches the disk while the user drags. The file is written when the window is closed with its X, which in ftorrent hides it rather than quitting, and again by Rust at exit, which covers the user who quits from the tray or the File menu without ever closing the window. A user who parks the window just so and then loses power before either has lost the position and drags it once more; that's the trade for never debouncing.

Everything recorded is in CSS pixels. Tauri reports positions and sizes in physical pixels and hands over the scale factor to divide by, and dividing here is what makes the numbers in the file mean the same thing on a Retina display and a plain one. The outer position is recorded because that's where the frame sits on the screen; the inner size because that's what the builder takes, and mixing the two would grow the window by a title bar every launch. A maximized window records only that it's maximized, and leaves the position and size where the window was before, so the file always holds a normal size and place: Rust builds the window there and maximizes it, and restoring it goes back there.
*/

export async function revealWindow() {//show the window, which rust made hidden; call once the app has mounted and there's something to see
	await getCurrentWindow().show()
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
