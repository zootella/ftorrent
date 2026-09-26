import {ref} from 'vue'
import {defineStore} from 'pinia'

/*
The app's first store, and the smallest honest example of what a store is for: it remembers the name the user typed into the greet form.

A component's state dies with the component. This name used to live in MainPage.vue as a plain ref, so a trip to the about page and back came back to an empty box — the router unmounted the page, and mounting it again ran its setup from scratch. Keeping the ref here instead moves the value out from under the component: the page reads and writes the same ref every time it mounts, and clicking between pages costs nothing.

Wrapping the router outlet in <KeepAlive> would also have kept the box filled, and it's the more common reflex. It preserves the component rather than moving the state out of it, so the value stays welded to the one component that owns it, and a second component that wants the same fact still can't reach it.

This first one is deliberately about nothing underneath: it never calls Rust, and there's nothing to save. It's the interface remembering something about itself.
*/

export const useGreetStore = defineStore('greet', () => {//the setup form of a store: a function returning what the store holds, which reads like the <script setup> components around it
	let name = ref('')//what the user has typed into the greet form, held here so it survives leaving the page and coming back
	return {name}
})
