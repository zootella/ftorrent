<script setup>
import { inject } from 'vue'

const page = inject('page')
</script>

<template>
	<code>open.ftorrent.com</code>
	<pre>{{ JSON.stringify(page, null, '\t') }}</pre>
</template>
