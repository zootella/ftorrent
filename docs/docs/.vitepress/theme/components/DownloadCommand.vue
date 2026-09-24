<script setup>//./docs/.vitepress/theme/components/DownloadCommand.vue
import { ref, onMounted } from 'vue'
import { useData } from 'vitepress'
import { fetchSidecar } from '../downloads.js'

/*
One install command with the current hash written into it. The command itself is an ordinary fenced code block on the installing page, highlighted during the build like every other fence, and this component wraps it:

	<DownloadCommand file="ftorrent.exe">

	```powershell
	if ((Get-FileHash ftorrent.exe).Hash -eq '0000000000000000000000000000000000000000000000000000000000000000') { .\ftorrent.exe }
	```

	</DownloadCommand>

The sixty-four zeros stand where the hash goes. When the page opens, this fetches the file's sidecar the way DownloadLink does and swaps the hash into the one text node that holds the zeros. That node sits inside one of the highlighter's spans, so the colors are untouched, and the copy button reads the block's text when it is clicked, so it copies the real hash. The blank lines around the fence are what let markdown render it inside the component's slot.

A fence alone would name the hash of whatever was published the day the site was built, and the arrangement is that publishing an installer changes what the site says without a rebuild.
*/

let props = defineProps({
	file: String,//the published name, ftorrent.exe or ftorrent.dmg; its sidecar sits beside it under the same name
})

const placeholder = '0'.repeat(64)//what the fence holds where the hash goes: one run of text the highlighter keeps in one span, and the width of the hash that replaces it

let { theme } = useData()//carries origin, the one place config.js writes the download host

let box = ref(null)//the element around the fence
let sidecar = ref(false)//everything the build wrote down, or false until it arrives, and false for good if this file is not published
let loaded = ref(false)//the fetch has settled, so a missing sidecar now means unpublished rather than unread

onMounted(async () => {
	sidecar.value = await fetchSidecar(props.file, theme.value.origin)
	loaded.value = true
	if (!sidecar.value) return//unpublished, or unreadable: the zeros stay, the line below says so, and a pasted command refuses to install, which is the right refusal

	let walker = document.createTreeWalker(box.value, NodeFilter.SHOW_TEXT)
	for (let node = walker.nextNode(); node; node = walker.nextNode()) {
		if (!node.nodeValue.includes(placeholder)) continue
		node.nodeValue = node.nodeValue.replace(placeholder, sidecar.value.sha256)
		return
	}
	throw new Error('the fence inside DownloadCommand for ' + props.file + ' holds no run of sixty-four zeros for the hash')//a mistake in the markdown, loud here rather than a command that fails its own check
})
</script>

<template>
<div ref="box"><slot /></div>
<div v-if="loaded && !sidecar" class="detail">Not yet published</div>
</template>
