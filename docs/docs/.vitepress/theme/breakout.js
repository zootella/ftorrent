//Breakouts: an element marked class="breakout", a table's figure or a div around a code fence, breaks out of the prose column and may grow rightward across the outline's column and on to the edge of the article's region, from 1280px, where VitePress gives the outline a column of its own. The outline fades while a breakout is on screen, so the two never overlap. style.css does the drawing, beside the figure rules; this component draws nothing, and measures how far a breakout may reach and watches what's on screen
import { onMounted, onUnmounted } from 'vue'
import { onContentUpdated } from 'vitepress'

export default {
	setup() {
		let observer = null
		let sizes = null//watches the page's width, made on mount because setup also runs in the build, where there's no browser
		const showing = new Set()//the breakouts on screen right now

		function measure() {//how far the article's region reaches past the prose column on the right, across the outline's column and any margin beyond it, which is how much further a breakout may reach; 0 when the outline is hidden, below 1280px
			const column = document.querySelector('.VPDoc .content-container')//the prose column
			const region = document.querySelector('.VPDoc .container')//the prose column and the outline together, inside the page's padding
			const aside = document.querySelector('.VPDoc .aside')
			let reach = 0
			if (column && region && aside && getComputedStyle(aside).display != 'none') reach = Math.max(0, region.getBoundingClientRect().right - column.getBoundingClientRect().right)
			document.documentElement.style.setProperty('--breakout-reach', `${reach}px`)
		}

		function watchBreakouts() {//find this page's breakouts and fade the outline while any of them is on screen
			observer?.disconnect()
			showing.clear()
			document.documentElement.classList.remove('breakout-in-view')
			const breakouts = document.querySelectorAll('.vp-doc .breakout')
			if (!breakouts.length) return
			measure()
			observer = new IntersectionObserver(entries => {//the browser calls this as a breakout enters or leaves the window
				for (const entry of entries) entry.isIntersecting ? showing.add(entry.target) : showing.delete(entry.target)
				document.documentElement.classList.toggle('breakout-in-view', showing.size > 0)
			})
			for (const breakout of breakouts) observer.observe(breakout)
		}

		onContentUpdated(watchBreakouts)//VitePress moves between pages without reloading, and calls this each time a page's content lands
		onMounted(() => {
			sizes = new ResizeObserver(measure)//the page's width changes when the window does, and also when a vertical scrollbar appears and takes its share, which no resize event reports
			sizes.observe(document.documentElement)
			watchBreakouts()
		})
		onUnmounted(() => {
			sizes?.disconnect()
			observer?.disconnect()
		})
		return () => null
	}
}
