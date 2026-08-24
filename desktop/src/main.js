//./src/main.js
import {createApp} from 'vue'
import App from './App.vue'
import router from './router/index.js'
import {revealWindow} from './window.js'
import './index.css'

createApp(App).use(router).mount('#app')
revealWindow()//the window starts hidden; size it to the desktop and show it, now that there's something in it to see
