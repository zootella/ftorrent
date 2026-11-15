<script setup>
import {ref} from 'vue'
import {Time} from '@zootella/ergo'

let props = defineProps({
	text: String,//what goes on the clipboard
	label: {type: String, default: 'Copy'},
	primary: Boolean,//the filled button, for the one copy a page is mostly for
})
let copied = ref(false)//true for a moment after a press, so the button can say so
async function copy() {
	await navigator.clipboard.writeText(props.text)
	copied.value = true
	setTimeout(() => { copied.value = false }, 1.5*Time.second)
}
</script>

<template>
	<button
		type="button"
		@click="copy"
		class="inline-flex shrink-0 items-center gap-1.5 rounded-md px-2.5 py-1 text-sm font-medium transition-colors"
		:class="primary
			? 'bg-neutral-900 text-white hover:bg-neutral-700 dark:bg-white dark:text-neutral-900 dark:hover:bg-neutral-200'
			: 'border border-neutral-200 text-neutral-600 hover:bg-neutral-100 dark:border-neutral-700 dark:text-neutral-300 dark:hover:bg-neutral-800'">
		<svg v-if="!copied" class="size-3.5" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true"><rect x="5.5" y="5.5" width="8" height="8" rx="1.5"/><path d="M10.5 5.5V3.5A1.5 1.5 0 0 0 9 2H3.5A1.5 1.5 0 0 0 2 3.5V9A1.5 1.5 0 0 0 3.5 10.5H5.5"/></svg>
		<svg v-else class="size-3.5" viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="2" aria-hidden="true"><path d="M3 8.5L6.5 12L13 4.5"/></svg>
		<span>{{ copied ? 'Copied' : label }}</span>
	</button>
</template>
