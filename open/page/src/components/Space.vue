<script setup>
import { onMounted, onUnmounted, ref } from 'vue'
import * as THREE from 'three'

const container = ref(null)
let renderer, scene, camera, globe

onMounted(() => {
	scene = new THREE.Scene()
	scene.background = new THREE.Color(0x000000)

	const FOV = 10
	const GLOBE_RADIUS = 1
	const aspect = window.innerWidth / window.innerHeight
	const halfFovRad = THREE.MathUtils.degToRad(FOV / 2)

	// Position camera so globe width matches viewport width
	const CAMERA_Z = GLOBE_RADIUS / (Math.tan(halfFovRad) * aspect) * 0.5
	camera = new THREE.PerspectiveCamera(FOV, aspect, 0.1, 100)
	camera.position.set(0, 0, CAMERA_Z)

	// Position globe so its top touches the top-center of the viewport
	const visibleHalfHeight = CAMERA_Z * Math.tan(halfFovRad)
	const GAP = 0.025
	const globeY = visibleHalfHeight - GLOBE_RADIUS - GAP

	renderer = new THREE.WebGLRenderer({ antialias: true })
	renderer.setSize(window.innerWidth, window.innerHeight)
	renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
	container.value.appendChild(renderer.domElement)

	// Globe
	const geometry = new THREE.SphereGeometry(1, 64, 64)
	const texture = new THREE.TextureLoader().load('/textures/earth-8k-q80.jpg', () => {
		render()
	})
	texture.colorSpace = THREE.SRGBColorSpace
	const material = new THREE.MeshStandardMaterial({ map: texture })
	globe = new THREE.Mesh(geometry, material)
	globe.position.set(0, globeY, 0)

	// Axis straight up (no tilt for now)
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

	// Drag to rotate globe manually
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

	window.addEventListener('resize', onResize)
})

function updateRotation() {
	if (!globe) return
	const now = new Date()
	const hours = now.getUTCHours() + now.getUTCMinutes() / 60
	// Full rotation (2π) per 24 hours
	globe.rotation.y = (hours / 24) * Math.PI * 2
}

function render() {
	renderer.render(scene, camera)
}

function onResize() {
	const FOV = 10
	const GLOBE_RADIUS = 1
	const aspect = window.innerWidth / window.innerHeight
	const halfFovRad = THREE.MathUtils.degToRad(FOV / 2)
	const z = GLOBE_RADIUS / (Math.tan(halfFovRad) * aspect) * 0.5
	camera.aspect = aspect
	camera.position.z = z
	camera.updateProjectionMatrix()

	const visibleHalfHeight = z * Math.tan(halfFovRad)
	globe.position.y = visibleHalfHeight - GLOBE_RADIUS - 0.025

	renderer.setSize(window.innerWidth, window.innerHeight)
	render()
}

onUnmounted(() => {
	window.removeEventListener('resize', onResize)
	renderer?.dispose()
})
</script>

<template>
	<div ref="container" class="space"></div>
</template>

<style scoped>
.space {
	width: 100vw;
	height: 100vh;
	overflow: hidden;
}
</style>
