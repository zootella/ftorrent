import {reactive} from 'vue'
import {Time} from '@zootella/ergo'

/*
The page's tooltips. A browser draws an element's title after a delay of its own, often a whole second, which no page can change, and that's slow for running the pointer along a calendar. So an element with something to say on hover carries it in a data-tip attribute instead, and one listener on the document shows it in a single box Page.vue draws: after a short wait for the first, so a pointer passing over the page doesn't flash tips, and at once as the pointer moves on to the next tipped element.
*/
export const tip = reactive({text: '', x: 0, y: 0, below: false})//what the box says, and where: centered on x, its bottom edge at y, or its top edge there when below
let wait = 150*Time.millisecond//how long the pointer rests on the first element before its tip shows
let showing = false
let timer = null

export function installTips() {
	document.addEventListener('mouseover', event => {//fires as the pointer enters any element, children included, so closest finds the tipped element it's over, if any
		let target = event.target.closest('[data-tip]')
		clearTimeout(timer)
		if (!target) return hide()
		if (showing) place(target)
		else timer = setTimeout(() => place(target), wait)
	})
	window.addEventListener('scroll', hide, {passive: true})//a tip left behind by a scroll would point at nothing
}

function place(target) {//show the target's tip above it, or below it when it sits near the top of the window
	let box = target.getBoundingClientRect()
	tip.text = target.dataset.tip
	tip.x = Math.min(Math.max(box.left + box.width/2, 160), window.innerWidth - 160)//half the box's widest, so a tip near either edge stays inside the window
	tip.below = box.top < 64
	tip.y = tip.below ? box.bottom + 6 : box.top - 6
	showing = true
}

function hide() {
	clearTimeout(timer)
	tip.text = ''
	showing = false
}
