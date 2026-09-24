<script setup>//./docs/.vitepress/theme/components/DownloadCommand.vue
import { ref, computed, onMounted } from 'vue'
import { useData } from 'vitepress'
import { fetchSidecar } from '../downloads.js'
import { data } from '../curl.data.js'

/*
One install command: a code block that fetches an installer, checks its hash, and installs it, with the hash of the current build written into it. downloads.js has the system it belongs to, where a sidecar comes from and why the fetch happens on mount, and DownloadLink.vue is the box that shows the same hash beside the link.

The installing page places two of these, one for the dmg and one for the exe:

	<DownloadCommand file="ftorrent.dmg" />

The commands themselves, and why they are highlighted during the build rather than in the browser, are in curl.data.js. That loader renders each one through VitePress's own markdown renderer, with a run of zeros where the hash goes, so a block arrives here as the finished HTML a fence produces, spans, colors, copy button and all, and this component's whole job is to fetch the sidecar and put the hash where the zeros are.

The hash is the reason this is a component rather than a fence. A fence would name the hash of whatever was published the day the site was built, and the arrangement is that publishing an installer changes what the site says without a rebuild.
*/

let props = defineProps({
	file: String,//the published name, ftorrent.dmg or ftorrent.exe; its sidecar sits beside it under the same name, and its command is in curl.data.js
})
if (!data.html[props.file]) throw new Error('no install command for ' + props.file)//a file the loader has no command for is a mistake in the markdown, caught here rather than as a blank block

let { theme } = useData()//carries origin, the one place config.js writes the download host

let sidecar = ref(false)//everything the build wrote down, or false until it arrives, and false for good if this file is not published
let loaded = ref(false)//the fetch has settled, so a missing sidecar now means unpublished rather than unread
let html = computed(() => sidecar.value ? data.html[props.file].replace(data.placeholder, sidecar.value.sha256) : '')//the finished block with the real hash in it. replace swaps the first occurrence, and the loader checked there is exactly one

onMounted(async () => {
	sidecar.value = await fetchSidecar(props.file, theme.value.origin)
	loaded.value = true
})
</script>

<template>
<!-- the block is a fence's own html, so the copy button inside it works through the theme's one click listener; the wrapping div is only because v-html needs an element -->
<div v-if="sidecar" v-html="html"></div>
<div v-else-if="loaded" class="detail">Not yet published</div>
</template>
