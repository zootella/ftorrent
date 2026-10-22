<script setup>
import {ref, onMounted, onUnmounted} from 'vue'
import {getCurrentWindow} from '@tauri-apps/api/window'
import {windowResizeDrag} from '../window.js'
import {platformName} from '../settings.js'

/*
The size grip: a square in the window's bottom-right corner that resizes the window from that corner, as Windows programs' status bars have had one since Windows 95, so the corner is a target as big as a line of text rather than a border a few pixels wide. It's Windows' alone. A Mac has drawn none since OS X Lion, and modern Linux desktops none either, each resizing from the window's edges, so there ftorrent leaves the corner to the system as every other app does.

A press hands the drag to Windows, which runs the same sizing loop a press on the window's border starts, so it resizes exactly as the border would. The pointer over the grip is the system's own diagonal resize arrow, the one the window's corner border shows just past it, so it doesn't change as it crosses from one to the other. And the grip is there only while the window is a normal one: maximized or full screen, the window has no corner to drag, and Windows' own status bars hide their grip then too.
*/

//the dots, a pyramid of six, one in the top row, two, then three, after the grip Windows 10's status bars draw, on a grid of 6 where every dot and every gap is one unit wide: dot, gap, dot, gap, dot, gap, so the pyramid touches the square's top and left edges, and the last gap keeps the bottom-right corner clear for a window that rounds it, as Windows 11's do. The grid stretches over the whole square, one line of text on a side, and the circles are drawn as vectors at whatever size that is on the screen, as sharp at 150% as at 100%
const dots = [[4.5, 0.5], [2.5, 2.5], [4.5, 2.5], [0.5, 4.5], [2.5, 4.5], [4.5, 4.5]]

let shown = ref(false)//whether the window is normal, neither maximized nor full screen, so a corner to drag exists
let unlisten = null//stops hearing the window's resizes
let gone = false//this grip left the page, perhaps before its listening had started

//look again at whether the window is normal; a maximize, a restore, and going full screen each resize the window, so the resize event brings each one here
async function look() {
	let appWindow = getCurrentWindow()
	let [maximized, fullscreen] = await Promise.all([appWindow.isMaximized(), appWindow.isFullscreen()])
	shown.value = !maximized && !fullscreen
}
onMounted(async () => {
	if (platformName != 'Windows') return//the grip is Windows' alone, so elsewhere there's nothing to watch for
	let stop = await getCurrentWindow().onResized(look)
	if (gone) stop()//the page moved on while the listening was starting
	else unlisten = stop
	look()
})
onUnmounted(() => { gone = true; unlisten?.() })

//the primary button, as a window's border answers, hands the drag to Windows, which runs it until the button comes up
function press(event) {
	if (event.button == 0) windowResizeDrag('SouthEast').catch(() => {})//a press Windows didn't take leaves the window as it was
}
</script>

<template>
	<!-- ./src/components/SizeGrip.vue -->
	<!-- fixed to the window rather than the page, so the page area's padding and scrolling leave it in the corner; size-grip is one line of the system's text, on both sides; select-none so a press here never starts selecting the page's text -->
	<div v-if="platformName == 'Windows' && shown" class="fixed right-0 bottom-0 size-grip cursor-nwse-resize select-none" @pointerdown="press">
		<svg viewBox="0 0 6 6" class="size-full fill-grip"><circle v-for="[x, y] in dots" :key="`${x},${y}`" :cx="x" :cy="y" r="0.5" /></svg><!-- the whole square takes the press, and the dots only show where it is -->
	</div>
</template>
