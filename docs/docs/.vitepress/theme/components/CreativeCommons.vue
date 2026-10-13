<script setup>
// The line under every article: the Creative Commons badge, the 88 by 31 button of the mid-2000s web, the 2002 original, the first button Creative Commons made, which it still hosts as somerights.gif, served by this site from docs/public rather than fetched from Creative Commons on every view, beside one sentence saying the page's text is CC BY 4.0. The button says Some Rights Reserved and not which license; the link beside it does. Both links carry rel="license", the markup Creative Commons recommends, so a crawler reads the license from the page. The text only: the research articles' data stays under each source's terms, which the articles state, and the code is under the repository's own license. The layout places this through its doc-after slot, after the previous and next links, so it is the last thing on the page.
</script>

<template>
	<div class="license-line">
		<a rel="license" href="https://creativecommons.org/licenses/by/4.0/"><img src="/somerights.gif" alt="Some rights reserved: Creative Commons Attribution 4.0 International" width="88" height="31"></a>
		<p>ftorrent licenses the text of this page under <a rel="license" href="https://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International</a>.</p>
	</div>
</template>
