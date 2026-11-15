import {useRoute, useRouter} from 'vue-router'

/*
Which trackers the summary shows, kept as values in the address's query, like ?show=cut&scheme=https,wss&family=ipv6. A view can be bookmarked or shared that way, and the back button returns to it. A value at its default leaves the address, so the plain summary is just /.
*/
export function useQuery() {
	let route = useRoute()
	let router = useRouter()
	function read(key) { return typeof route.query[key] == 'string' ? route.query[key] : '' }//a value, or blank when the address doesn't say
	function set(change) {
		let query = {...route.query, ...change}
		for (let key in query) if (!query[key]) delete query[key]
		router.replace({query})
	}
	function clear() { router.replace({query: {}}) }
	return {read, set, clear}
}
