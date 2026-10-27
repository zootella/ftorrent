<script setup>
import {onMounted, onUnmounted, ref} from 'vue'
import * as Three from 'three'
import {Time, sayTick} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go

const dragEnabled = false//true lets a drag turn the globe, for testing
const testMoment = null//a moment to show in place of now, like '2026-12-21T12:00Z', null for live
const redrawsPerSecond = 6//each redraw shifts the whole globe a fraction of a pixel, and at one a second those shifts read as a pulse; at 6 they're smooth, and 1/6 of a second is a whole number of display frames at every common rate: 5 at 30 Hz, 10 at 60, 15 at 90, 20 at 120, and 24 at 144
const timeScale = 1//1 is real time, 10 is ten times faster, and so on, from now or testMoment

const fieldOfView = 10//the camera's vertical field of view, in degrees
const globeRadius = 1
const gap = 0.025//between the top of the globe and the top of the banner, in the globe's units

const container = ref(null)
let renderer, scene, camera, globe, frameRequest
let dragTurn = 0//radians the globe has been turned by dragging, which updateRotation adds to the clock's rotation, so a drag holds
const startTime = Date.now()

onMounted(() => {
	scene = new Three.Scene()
	scene.background = new Three.Color(0x000000)

	camera = new Three.PerspectiveCamera(fieldOfView, 1, 0.1, 100)//layout() below sets its aspect and distance
	renderer = new Three.WebGLRenderer({antialias: true})
	container.value.appendChild(renderer.domElement)

	//the globe, wearing the texture for the current UTC month, chosen at load time to match the rest of the page, whose clock and day number are UTC too. The URL is built at runtime from a string, so Vite has no compile-time reference to any of the twelve files: only the chosen month is fetched, and nothing in /earth/ is preloaded, prefetched, or bundled
	let monthName = sayTick(Date.now(), 'mmm')//jan through dec, the month's short name in lowercase, as the twelve files are named
	let geometry = new Three.SphereGeometry(globeRadius, 128, 128)//segments around and from pole to pole; at 128 the outline's straight edges stay within a pixel of a true circle, even across a wide retina window
	let texture = new Three.TextureLoader().load(`/earth/${monthName}.jpg`, () => render())//draw as soon as the image arrives, rather than at the next redraw
	texture.colorSpace = Three.SRGBColorSpace
	texture.anisotropy = renderer.capabilities.getMaxAnisotropy()//sharp where the surface turns away from the viewer near the globe's edge, which is most of what the banner shows; the graphics card's maximum, usually 16, and 1, plain filtering, where it has none
	let material = new Three.MeshStandardMaterial({map: texture})

	//fade the globe's outermost pixel into space. The renderer's own antialiasing takes a few samples a pixel, which leaves visible steps along the long, shallow arc where bright land meets black. So after three.js has lit each pixel, scale it by how near it is to the edge: facing is how squarely the surface faces the camera, 1 head-on and 0 at the edge, and fwidth is how much facing changes from one pixel to the next, so near the edge facing over twice fwidth is about how many pixels in this one sits. That ramps the last pixel to black, and since space is pure black, darkening a pixel is the same as blending it into the background
	material.onBeforeCompile = shader => {
		shader.fragmentShader = shader.fragmentShader.replace('#include <opaque_fragment>', `#include <opaque_fragment>
			float facing = dot(normal, normalize(vViewPosition));
			gl_FragColor.rgb *= clamp(facing / (2.0 * fwidth(facing)), 0.0, 1.0);`)
	}

	globe = new Three.Mesh(geometry, material)
	updateRotation()

	scene.add(globe)

	//light from the upper right, there to show the globe's shape; it doesn't stand for the sun, whose place the spin and the tilt keep
	let light = new Three.DirectionalLight(0xffffff, 3)
	light.position.set(5, 3, 5)
	scene.add(light)

	let ambient = new Three.AmbientLight(0xffffff, 0.3)
	scene.add(ambient)

	//drag to turn the globe by hand, for testing
	if (dragEnabled) {
		let isDragging = false
		let lastX = 0
		let canvas = renderer.domElement
		canvas.addEventListener('pointerdown', event => {
			isDragging = true
			lastX = event.clientX
		})
		canvas.addEventListener('pointermove', event => {
			if (!isDragging) return
			dragTurn += (event.clientX - lastX) * 0.005//radians a pixel
			lastX = event.clientX
			updateRotation()
			render()
		})
		canvas.addEventListener('pointerup', () => { isDragging = false })
		canvas.addEventListener('pointerleave', () => { isDragging = false })
	}

	layout()

	//redraw redrawsPerSecond times a second, on the display's own frames. A frame can arrive a hair early, which would push the redraw a whole frame late and make the cadence uneven, so a frame up to slack milliseconds early still counts; that's less than half a frame at any display rate up to 165 Hz, so the frame before never does
	let redrawInterval = Time.second / redrawsPerSecond
	let slack = 3
	let lastRender = 0
	function tick(now) {
		if (now - lastRender >= redrawInterval - slack) {
			if (renderer.getPixelRatio() != window.devicePixelRatio) layout()//the window moved to a screen of another density, which fires no resize
			updateRotation()
			render()
			lastRender = now
		}
		frameRequest = requestAnimationFrame(tick)
	}
	frameRequest = requestAnimationFrame(tick)

	window.addEventListener('resize', layout)
})

//the moment the globe shows, which sets both its spin and its tilt: now, or testMoment, run forward at timeScale from when the page loaded
function moment() {
	let start = testMoment == null ? startTime : Date.parse(testMoment)
	return start + (Date.now() - startTime) * timeScale
}

//spin and tilt the globe for the moment it shows. The spin turns the longitude where it's noon to face the viewer, plus any drag. The tilt leans the north pole away from the viewer by the latitude where the sun is overhead, which follows the seasons: 23.44° away at the June solstice, upright at the equinoxes, and 23.44° toward the viewer at the December solstice. The pole leans as a viewer on the far side of the planet from the sun would see it, while the spin still brings the noon side round to face you, and that's chosen for what the banner's northern arc then shows: in June the view leans south, toward the warm latitudes, 31°N to 67°N on a laptop-wide window, and in December it looks down onto the Arctic ice
function updateRotation() {
	let t = moment()//milliseconds since 1970, which the spin and the tilt both read
	globe.rotation.y = Math.PI / 2 + (t % Time.day) / Time.day * Math.PI * 2 + dragTurn
	globe.rotation.x = -Three.MathUtils.degToRad(sunLatitude(t))
}

//the latitude where the sun is directly overhead at a moment, its declination, in degrees: 23.44 north at the June solstice, 0 at the equinoxes, and 23.44 south at the December solstice. This is the U.S. Naval Observatory's formula for the sun's approximate position, good to about an arcminute, a sixtieth of a degree, within two centuries of 2000
function sunLatitude(t) {
	let d = (t - Date.UTC(2000, 0, 1, 12)) / Time.day//days since noon UTC on 2000 January 1, the epoch the formula counts from
	let g = Three.MathUtils.degToRad(357.529 + 0.98560028 * d)//the sun's mean anomaly, how far around Earth's elliptical orbit it is
	let q = 280.459 + 0.98564736 * d//its mean longitude, in degrees
	let l = Three.MathUtils.degToRad(q + 1.915 * Math.sin(g) + 0.020 * Math.sin(2 * g))//its ecliptic longitude, corrected for the orbit's ellipse
	let e = Three.MathUtils.degToRad(23.439 - 0.00000036 * d)//the tilt of Earth's axis, which drifts very slowly
	return Three.MathUtils.radToDeg(Math.asin(Math.sin(e) * Math.sin(l)))
}

function render() {
	renderer.render(scene, camera)
}

//fit the camera, the globe, and the canvas to the banner, at mount, on every resize, and when the screen's density changes, as with browser zoom or a window moved to another display. The canvas takes the screen's full density, 3x on an iPhone, so the globe is never drawn soft and scaled up
function layout() {
	let banner = container.value
	let aspect = banner.clientWidth / banner.clientHeight
	let halfField = Three.MathUtils.degToRad(fieldOfView / 2)//half the vertical field of view, in radians

	//back the camera off until the banner's width spans half the globe's diameter, so at any window width the banner frames the middle of its upper arc
	camera.aspect = aspect
	camera.position.z = globeRadius / (Math.tan(halfField) * aspect) * 0.5
	camera.updateProjectionMatrix()

	//raise the globe until its top sits gap below the banner's top edge
	globe.position.y = camera.position.z * Math.tan(halfField) - globeRadius - gap

	renderer.setPixelRatio(window.devicePixelRatio)
	renderer.setSize(banner.clientWidth, banner.clientHeight)
	render()
}

onUnmounted(() => {
	cancelAnimationFrame(frameRequest)
	window.removeEventListener('resize', layout)
	globe?.geometry.dispose()      //the sphere's vertex and index buffers
	globe?.material.map?.dispose() //the Earth texture, the image on the GPU
	globe?.material.dispose()      //the shader program and its uniforms
	renderer?.dispose()            //the WebGL context and render targets
})
</script>

<template>
	<div ref="container" class="space"></div>
</template>

<style scoped>
.space {
	width: 100%;
	height: 100%;
	overflow: hidden;
}
</style>
