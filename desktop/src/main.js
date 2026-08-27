//./src/main.js
import {createApp} from 'vue'
import {createPinia} from 'pinia'
import App from './App.vue'
import router from './router/index.js'
import {revealWindow} from './window.js'
import './index.css'

createApp(App).use(createPinia()).use(router).mount('#app')//one pinia for the life of the app: this process opens a single window once and closes it once, so there is never a second store to keep in step
revealWindow()//the window starts hidden; size it to the desktop and show it, now that there's something in it to see
