import {defineConfig} from "vite"
import vue from "@vitejs/plugin-vue"
import tailwindcss from '@tailwindcss/vite'

const host = process.env.TAURI_DEV_HOST

// https://vitejs.dev/config/
export default defineConfig({
	plugins: [
		vue(),
		tailwindcss(),
	],

	// Vite options tailored for Tauri development and only applied in `tauri dev` or `tauri build`
	//
	// 1. prevent vite from obscuring rust errors
	clearScreen: false,
	// 2. tauri expects a fixed port, fail if that port is not available
	server: {
		port: 7100,//rather than the 1420 every app from Tauri's template asks for, so ftorrent's dev server runs beside theirs
		strictPort: true,
		host: host || false,
		hmr: host ? {protocol: "ws", host, port: 7101} : undefined,
		watch: {
			// 3. tell vite to ignore watching `src-tauri`
			ignored: ["**/src-tauri/**", "**/engine/**", "**/linux/**"],//and the engine's freeze and the linux builds, which write thousands of files a page never imports
		},
	},
})
