import {Time, sayTick, sayTimeRemaining, isoToTick} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go

/*
What the page knows about trackers.json, the report list.ftorrent.com publishes each hour, kept here apart from the components so they stay thin. The report is a record of hours: for each tracker in each family, every hour of six horizons is exactly one outcome, by a capital letter, and its last 180 days are a string of one character a day. The functions here turn those into what a reader sees: an uptime as a share of the hours with a reading, a calendar of days laid out by week, and dates and durations in words.

Where a function hands back Tailwind classes, they're written out whole, like 'bg-emerald-500', never assembled from parts, since Tailwind finds the classes a page uses by reading its source for them.
*/

const horizonNames = {//how the page heads each horizon trackers.json names by its lowercase letter, and says it in a sentence
	h: {head: '1h', phrase: 'hour'},
	d: {head: '1d', phrase: 'day'},
	w: {head: '7d', phrase: '7 days'},
	m: {head: '30d', phrase: '30 days'},
	q: {head: '90d', phrase: '90 days'},
	y: {head: '1y', phrase: 'year'},
}
export function horizonHead(horizon) { return horizonNames[horizon]?.head || horizon }//a horizon a later report adds still shows, under its own letter
export function horizonPhrase(horizon) { return horizonNames[horizon]?.phrase || horizon }

export const outcomes = {//each outcome's letter, in the order trackers.json lists them, and what happened, in words to place inside a sentence, which sayOutcome capitalizes for a label; no reading is an hour list's own monitor has no result for, which says nothing about the tracker
	U: {word: 'up'},
	D: {word: 'name doesn\'t resolve'},
	N: {word: 'no address'},
	T: {word: 'timed out'},
	R: {word: 'connection refused or reset'},
	C: {word: 'certificate didn\'t verify'},
	B: {word: 'bad reply'},
	Z: {word: 'no reading'},
}
export function sayOutcome(letter) {//an outcome's words to start a sentence or a label, like Timed out
	let word = outcomes[letter]?.word || outcomes.Z.word
	return word[0].toUpperCase() + word.slice(1)
}

export const families = {ipv4: 'IPv4', ipv6: 'IPv6'}//each family's key in trackers.json, and how the page writes it

const outcomeFills = {//each outcome's color, for its dot and its stretch of a bar: green for up; yellow, orange, and red for a reply that wasn't a tracker's, a certificate that didn't verify, and a connection turned away; blue for no answer at all, the tracker frozen; purple for a name that's gone; light gray for no address in the family; and an outline for no reading
	U: 'bg-emerald-500',
	B: 'bg-amber-400',
	C: 'bg-orange-500',
	R: 'bg-red-500',
	T: 'bg-blue-500',
	D: 'bg-purple-500',
	N: 'bg-neutral-300 dark:bg-neutral-600',
	Z: 'ring-1 ring-inset ring-neutral-300 dark:ring-neutral-700',
}
export function outcomeDot(letter) { return outcomeFills[letter] || outcomeFills.Z }

export const outcomeOrder = ['U', 'B', 'C', 'R', 'T', 'D', 'N', 'Z']//the outcomes from up to gone, the order a bar of them runs

//the hours a horizon adds up to, from its {letter: hours}
export function horizonHours(hours) {
	return Object.values(hours).reduce((sum, n) => sum + n, 0)
}

//one horizon's uptime, from its {letter: hours}, as the share of the hours with a reading that were up, or -1 when no hour had a reading, or every one found no address in this family, since there's nothing to judge
export function uptime(hours) {
	let awake = horizonHours(hours) - (hours.Z || 0)
	if (awake == 0 || hours.N == awake) return -1
	return (hours.U || 0)/awake
}

//the same as text: 100% only when every hour with a reading was up, so 719 of 720 reads 99%, never a rounded-up 100%; blank when there's nothing to judge
export function sayUptime(hours) {
	let share = uptime(hours)
	if (share < 0) return ''
	if (share == 1) return '100%'
	return Math.floor(share*100) + '%'
}

//the one outcome of a family's last hour, the letter its h horizon holds
export function lastHour(column) {
	return Object.keys(column.h)[0] || 'Z'
}

//whether a family's name has no address now, so the page says that instead of an uptime: its last hour found none, or, with no reading last hour, most of the readings over the horizon did; the hours before stay in its calendar and its table
export function noAddress(column, horizon) {
	let last = lastHour(column)
	if (last != 'Z') return last == 'N'
	let hours = column[horizon]
	let awake = horizonHours(hours) - (hours.Z || 0)
	return awake > 0 && (hours.N || 0)*2 > awake
}

//whether a tracker can be reached in a family at all: its name has an address there, and resolves
export function hasAddress(column, horizon) {
	return !noAddress(column, horizon) && lastHour(column) != 'D'
}

export const schemes = ['udp', 'http', 'https', 'ws', 'wss']//the kinds of tracker list checks, in the order the summary offers them

/*
A calendar is a family's last 180 days, which trackers.json writes as 180 characters, oldest first and the report's own day last: U when every hour with a reading was up, a digit when some were, how many tenths of them, the letter of the outcome most of its hours had when none was up, and Z when there was no reading. So a day takes the same color as the outcome it sums, the colors of the last hour's bars: green up, partly up a lighter green the less of the day was up, and a day with nothing up in its outcome's own color. The page lays the days out the way GitHub draws a year of contributions, a column for each week and a row for each day of it, Sunday at the top, so a weekly pattern, like a tracker that's down every weekend, shows as a row across the weeks.
*/
const partly = {//a day partly up, in tenths of its hours, as the up color thinned toward the page, so less up reads paler
	9: 'bg-emerald-500/75',
	8: 'bg-emerald-500/60',
	7: 'bg-emerald-500/60',
	6: 'bg-emerald-500/45',
	5: 'bg-emerald-500/45',
	4: 'bg-emerald-500/45',
	3: 'bg-emerald-500/30',
	2: 'bg-emerald-500/30',
	1: 'bg-emerald-500/30',
	0: 'bg-emerald-500/30',
}
export function dayClass(character) { return partly[character] || outcomeDot(character) }//a digit thins the green, and a letter takes its outcome's color

export const dayLegend = [//the squares a calendar's key shows: up, partly up, and then each outcome a day with nothing up can have, in the last hour's order
	{character: 'U', word: 'Up'},
	{character: '5', word: 'Partly up'},
	...outcomeOrder.filter(letter => letter != 'U').map(letter => ({character: letter, word: sayOutcome(letter)})),
]

//what one day's character means, for the square's tooltip
export function sayDay(character) {
	if (character == 'U') return 'Up every hour'
	if (character == 'Z') return 'No reading'
	if (character in partly) return `Up ${Number(character)*10}% to ${Number(character)*10 + 9}% of its hours`
	return `Nothing up, mostly ${outcomes[character]?.word || 'no reading'}`
}

//a family's days as weeks of seven, each day {tick, character}, with false for the days before the first one in its week, so the first lands on its weekday; and the weeks a month begins in, to label above them
export function calendar(days, hour) {
	let today = hour - hour % Time.day//the report's own day, the last character
	let first = today - (days.length - 1)*Time.day
	let cells = []
	for (let i = 0; i < new Date(first).getUTCDay(); i++) cells.push(false)//0 for Sunday, which starts a week at the top of its column
	for (let i = 0; i < days.length; i++) cells.push({tick: first + i*Time.day, character: days[i]})

	let weeks = []
	let months = []//{week, name} for each week a month begins in
	for (let i = 0; i < cells.length; i += 7) {
		let week = cells.slice(i, i + 7)
		let start = week.find(cell => cell && new Date(cell.tick).getUTCDate() == 1)
		if (start) months.push({week: weeks.length, name: sayTick(start.tick, 'Mmm')})
		weeks.push(week)
	}
	if (!months.length || months[0].week > 2) months.unshift({week: 0, name: sayTick(first, 'Mmm')})//the first week takes its month's name too, when there's room before the next
	return {weeks, months}
}

//a moment as a date, largest to smallest, like 2026-Nov-13, and 2026-Dec-1 for the first of a month, the way a server's status reads; ergo's sayTick writes the fields in UTC, the same for a reader anywhere
export function sayDate(tick) {
	return sayTick(tick, 'YYYY-Mmm-D')
}

//a moment to the minute, largest to smallest, like 2026-Nov-13 06:00 UTC
export function sayHour(tick) {
	return sayTick(tick, 'YYYY-Mmm-D hh24:mm') + ' UTC'
}

//a date trackers.json writes as YYYY-MM-DD, like a certificate's expiry, as a tick at the start of that UTC day
export function dayTick(date) {
	return isoToTick(`${date}T00:00:00.000Z`)
}

//a span in the one or two units that matter at its size, like 30m, 1m 59s, or 6h; ergo's sayTimeRemaining picks them, whatever the span is of
export function saySpan(span) {
	return sayTimeRemaining(span)
}

//a tracker's URL in three parts, for showing it with its host first: the scheme, the host, and the rest, a port or a path, like udp, tracker.example.com, and :6969/announce
export function splitUrl(url) {
	let scheme = url.slice(0, url.indexOf('://'))
	let rest = url.slice(scheme.length + 3)
	let end = rest.search(/[:/?]/)//where the host ends
	if (rest.startsWith('[')) end = rest.indexOf(']') + 1//an IPv6 literal, whose brackets hold its colons
	if (end == -1) end = rest.length
	return {scheme, host: rest.slice(0, end), rest: rest.slice(end)}
}

//the route of a tracker's own page, its URL with the :// turned into a /, like /udp/tracker.example.com:6969/announce, so the address bar reads plainly; a ? or # in the URL is escaped, so the router reads it as part of the path rather than the start of a query
export function trackerPath(url) {
	return '/' + url.replace('://', '/').replace(/[?#]/g, character => encodeURIComponent(character))
}

//the tracker a route names, back from its scheme and the rest of its path
export function trackerUrl(scheme, rest) {
	return `${scheme}://${rest}`
}

//a tracker's entry in the report, and whether it's listed in trackers.txt or was cut; false when this hour's report doesn't name it, as for an old link
export function findTracker(report, url) {
	let entry = report.trackers.find(e => e.url == url)
	if (entry) return {entry, listed: true}
	entry = report.cut.find(e => e.url == url)
	if (entry) return {entry, listed: false}
	return false
}

//the report's record of a tracker's extra peers, the peers its replies listed for a hash no one else has, or false when it listed none
export function findExtra(report, url) {
	return report.extra_peers.find(e => e.url == url) || false
}

//what a tracker's replies were seen doing, as a sentence for each family it happened in, said as what was seen and not what it means
export function sayExtra(report, extra, entry) {
	let horizon = report.criterion.horizon
	let lines = []
	for (let [key, name] of Object.entries(families)) {
		let seen = extra[key]
		if (!seen) continue
		let peers = seen.most == 1 ? '1 peer' : `as many as ${seen.most} peers`
		lines.push(`Over ${name}, its replies listed ${peers} for a hash no one else has, in ${seen.hours} of the last ${horizonHours(entry[key][horizon])} hours.`)
	}
	return lines
}

//trackers.txt as the report lists it, one URL to a line with a blank line between, the way a client reads a list of trackers: a blank line starts a new tier, so each tracker in its own tier is one the client announces to on its own
export function listText(report) {
	return report.trackers.map(entry => entry.url).join('\n\n') + '\n'
}

/*
The summary's count of the hour, from the report's trackers, listed and cut together: how each family's last hour went. A tracker is up here when its last hour was, in that family; the 30 days that decide the list show in each tracker's details.
*/

//how each family's last hour went, as {ipv4: {letter: trackers}, ipv6: {...}}, and how many trackers were up in both
export function lastHours(report) {
	let counts = {ipv4: {}, ipv6: {}}
	let both = 0
	for (let entry of [...report.trackers, ...report.cut]) {
		for (let key of Object.keys(counts)) {
			let letter = lastHour(entry[key])
			counts[key][letter] = (counts[key][letter] || 0) + 1
		}
		if (lastHour(entry.ipv4) == 'U' && lastHour(entry.ipv6) == 'U') both++
	}
	return {counts, both}
}

//a share as a CSS width, for a stretch of a bar
export function share(part, whole) {
	return whole > 0 ? `${part*100/whole}%` : '0%'
}
