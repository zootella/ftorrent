//./src/window.js

import {getCurrentWindow, currentMonitor, LogicalSize} from '@tauri-apps/api/window'

/*
The window starts hidden — tauri.conf.json sets visible false — and this module gives it a size that fits the desktop before anything reveals it. That order is the whole point: the window appears once, already right, instead of flashing at a boilerplate size and then jumping.

Note what this deliberately does not do. It never sets a position. Where a window opens is the operating system's job, and leaving it there is what makes a second copy of the app land beside the first rather than exactly on top of it — which matters here, because an installed copy and a portable copy are meant to run side by side.

The sizing runs in the gap between the app mounting and the window appearing, so nothing in it may throw. An error that escaped would leave a running app with no window at all — a process alive with nothing on screen. A wrong-sized window is the better failure, so the reveal happens no matter what the sizing did.
*/

async function _sizeWindow(appWindow) {//fit the window to the usable desktop; never throws, and leaves the fallback size from tauri.conf.json if anything goes wrong
	try {
		if (await appWindow.isVisible()) return//size once at startup only; a hot reload in development mounts the app again, and by then the window is already up
		let monitor = await currentMonitor()
		if (!monitor) return//tauri couldn't say which monitor we're on, so keep the fallback size
		let scale  = monitor.scaleFactor//the work area arrives in backing pixels while the resize api speaks logical ones, so every measurement below is divided by this
		let width  = Math.round((monitor.workArea.size.width  / scale) * 0.6)//the work area is the monitor minus the chrome the os keeps for itself: menu bars, docks, the windows taskbar
		let height = Math.round((monitor.workArea.size.height / scale) * 0.8)
		await appWindow.setSize(new LogicalSize(width, height))
	} catch (e) { console.error('sizeWindow:', e) }
}

export async function revealWindow() {//size the hidden window to the desktop and show it; call once, after the app has mounted and there's something to see
	let appWindow = getCurrentWindow()
	await _sizeWindow(appWindow)
	try {
		await appWindow.show()
	} catch (e) { console.error('revealWindow:', e) }
}
