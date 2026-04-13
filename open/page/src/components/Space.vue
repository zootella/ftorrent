<script setup>
import { onMounted, onUnmounted, ref } from 'vue'
import * as THREE from 'three'

const DRAG_ENABLED = false
const TEST_HOUR = null // set to 0-24 to override, null for live UTC
const TICK_MS = 1000 // how often the globe updates rotation
const TIME_SCALE = 1 // 1 = real time, 10 = 10x faster, etc.

const container = ref(null)
let renderer, scene, camera, globe, tickInterval
const startTime = Date.now()
const startHours = new Date().getUTCHours() + new Date().getUTCMinutes() / 60 + new Date().getUTCSeconds() / 3600

onMounted(() => {
	scene = new THREE.Scene()
	scene.background = new THREE.Color(0x000000)

	const el = container.value
	const FOV = 10
	const GLOBE_RADIUS = 1
	const aspect = el.clientWidth / el.clientHeight
	const halfFovRad = THREE.MathUtils.degToRad(FOV / 2)

	// Position camera so globe width matches container width
	const CAMERA_Z = GLOBE_RADIUS / (Math.tan(halfFovRad) * aspect) * 0.5
	camera = new THREE.PerspectiveCamera(FOV, aspect, 0.1, 100)
	camera.position.set(0, 0, CAMERA_Z)

	// Position globe so its top touches the top-center of the container
	const visibleHalfHeight = CAMERA_Z * Math.tan(halfFovRad)
	const GAP = 0.025
	const globeY = visibleHalfHeight - GLOBE_RADIUS - GAP

	renderer = new THREE.WebGLRenderer({ antialias: true })
	renderer.setSize(el.clientWidth, el.clientHeight)
	renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
	container.value.appendChild(renderer.domElement)

	// Globe — texture chosen by the browser's local month at load time.
	// The URL is built at runtime from a string, so Vite has no compile-time
	// reference to any of the twelve files: only the chosen month is fetched,
	// and nothing in /earth/ is preloaded, prefetched, or bundled.
	const MONTHS = ['jan', 'feb', 'mar', 'apr', 'may', 'jun', 'jul', 'aug', 'sep', 'oct', 'nov', 'dec']
	const monthName = MONTHS[new Date().getMonth()]
	const geometry = new THREE.SphereGeometry(1, 64, 64)
	const texture = new THREE.TextureLoader().load(`/earth/${monthName}.jpg`, () => {
		render()
	})
	texture.colorSpace = THREE.SRGBColorSpace
	const material = new THREE.MeshStandardMaterial({ map: texture })
	globe = new THREE.Mesh(geometry, material)
	globe.position.set(0, globeY, 0)

	// Tilt axis 17° to show equatorial/southern latitudes
	globe.rotation.x = -17 * Math.PI / 180

	// Set rotation based on current UTC time
	updateRotation()

	scene.add(globe)

	// Light — fixed sun direction
	const light = new THREE.DirectionalLight(0xffffff, 3)
	light.position.set(5, 3, 5)
	scene.add(light)

	const ambient = new THREE.AmbientLight(0xffffff, 0.3)
	scene.add(ambient)

	// Drag to rotate globe manually (for testing)
	if (DRAG_ENABLED) {
		let isDragging = false
		let lastX = 0
		const canvas = renderer.domElement
		canvas.addEventListener('pointerdown', (e) => {
			isDragging = true
			lastX = e.clientX
		})
		canvas.addEventListener('pointermove', (e) => {
			if (!isDragging) return
			const dx = e.clientX - lastX
			globe.rotation.y += dx * 0.005
			lastX = e.clientX
			render()
		})
		canvas.addEventListener('pointerup', () => { isDragging = false })
		canvas.addEventListener('pointerleave', () => { isDragging = false })
	}

	// Update rotation periodically
	tickInterval = setInterval(() => {
		updateRotation()
		render()
	}, TICK_MS)

	window.addEventListener('resize', onResize)
})

function updateRotation() {
	if (!globe) return
	let hours
	if (TEST_HOUR !== null) {
		hours = TEST_HOUR
	} else {
		const elapsed = (Date.now() - startTime) / 3600000 // real hours elapsed
		hours = startHours + elapsed * TIME_SCALE
	}
	globe.rotation.y = Math.PI / 2 + (hours / 24) * Math.PI * 2
}

function render() {
	renderer.render(scene, camera)
}

function onResize() {
	const el = container.value
	const FOV = 10
	const GLOBE_RADIUS = 1
	const aspect = el.clientWidth / el.clientHeight
	const halfFovRad = THREE.MathUtils.degToRad(FOV / 2)
	const z = GLOBE_RADIUS / (Math.tan(halfFovRad) * aspect) * 0.5
	camera.aspect = aspect
	camera.position.z = z
	camera.updateProjectionMatrix()

	const visibleHalfHeight = z * Math.tan(halfFovRad)
	globe.position.y = visibleHalfHeight - GLOBE_RADIUS - 0.025

	renderer.setSize(el.clientWidth, el.clientHeight)
	render()
}

onUnmounted(() => {
	clearInterval(tickInterval)
	window.removeEventListener('resize', onResize)
	renderer?.dispose()
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
