import { createApp } from 'vue'
import App from './App.vue'

const response = await fetch('/page.json')
const page = await response.json()

const app = createApp(App)
app.provide('page', page)
app.mount('#app')
