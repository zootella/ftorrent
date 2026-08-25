//./src/window.js

import {getCurrentWindow, currentMonitor, LogicalSize} from '@tauri-apps/api/window'

const startingWindowSize = {widthFraction: 0.6, heightFraction: 0.8}//how much of the usable desktop the window takes when it first opens

/*
The window is created hidden — tauri.conf.json sets visible false — and this module sizes it to fit the desktop before revealing it, so it appears once already correct instead of flashing at one size and jumping to another.

Two things it deliberately does not do. It never sets a position: where a window opens is the operating system's job, and leaving it there is what makes a second copy land beside the first rather than exactly on top of it, which matters because an installed copy and a portable copy are meant to run side by side. And it never lets a sizing failure stop the reveal, which is what the finally below is for — the window starts hidden, so an error on the way to show() would leave a process running with nothing on screen at all.

That difference is also why there is one try here rather than two. Failing to measure the desktop has a fallback: the window keeps the size tauri.conf.json gave it when it was created, 800 by 600, which is also Tauri's own default for a window. Failing to show has no fallback, so show() sits outside the catch — if it rejects, the app is broken in a way no handling here improves.
*/

export async function revealWindow() {//size the hidden window to the desktop and show it; call once, after the app has mounted and there's something to see
	let appWindow = getCurrentWindow()
	try {
		if (await appWindow.isVisible()) return//size once at startup only; a hot reload in development mounts the app again against a window that is already up
		let monitor = await currentMonitor()
		if (!monitor) return//tauri couldn't say which monitor we're on, so there's nothing to measure and the fallback size stands
		let scale  = monitor.scaleFactor//the work area arrives in backing pixels while the resize api speaks logical ones, so both measurements are divided by this
		let width  = Math.round((monitor.workArea.size.width  / scale) * startingWindowSize.widthFraction)//workArea is the monitor minus the chrome the os keeps for itself: menu bars, docks, the windows taskbar
		let height = Math.round((monitor.workArea.size.height / scale) * startingWindowSize.heightFraction)
		await appWindow.setSize(new LogicalSize(width, height))
	} catch (e) {
		console.error('sizing the window:', e)//whatever went wrong measuring or resizing, the fallback size stands
	} finally {
		await appWindow.show()//reveal whatever happened above, including the early returns
	}
}
