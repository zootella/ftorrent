<script setup>
import { inject } from 'vue'
import LcdPanel from './LcdPanel.vue'

const page = inject('page')
</script>

<template>
	<div>
		<h1>open.ftorrent.com</h1>

		<LcdPanel />

		<p>The gauge is a Node.js script that runs in its own Docker container alongside the trackers. Once per minute it scrapes Prometheus metrics from the three Aquatic containers, reads their memory usage from cgroup files, and writes a single JSON file — page.json — that nginx serves as a static file. The Vue frontend fetches page.json and renders the dashboard. The gauge has no HTTP server and no listening ports. It only writes files.</p>

		<p>A JSON file served by nginx is the simplest possible data path. No application server in the request path, no ports to expose, no process that can be crashed by a malformed HTTP request. The gauge writes, nginx serves, the frontend reads. Each piece can fail independently — if the gauge crashes, nginx keeps serving the last good page.json until the gauge restarts.</p>

		<p>The gauge fires once just after the top of each UTC minute, not on a fixed interval. A 60-second setInterval drifts over time and can skip or double-fire at minute boundaries, which would create false gaps in the ring buffer. Clock alignment means exactly one tick per calendar minute.</p>

		<pre>{{ JSON.stringify(page, null, '\t') }}</pre>
	</div>
</template>
