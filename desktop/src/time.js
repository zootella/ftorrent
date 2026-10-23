import {Time, sayDuration, sayTick, textToTick} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go

//a tick as text a person can read in a box or a file, like 2026oct23 Fri 12:53p 49.123s, on this computer's own clock: the minute as a person says it, with a and p for am and pm, then the seconds to the millisecond; the s at the end isn't a tag, since ergo has none for a lone s, so it's written as it is
export function sayMoment(tick) {
	let local = tick - new Date(tick).getTimezoneOffset()*Time.minute//shifted by this computer's offset from UTC at that moment, daylight saving included, since sayTick writes every tick as it falls in UTC
	return sayTick(local, 'YYYYmmmD Ddd h12:mma ss.ttts')
}

//a tick read back from ergo's short form, like 2026oct22.1830, or a person's shorter 2026dec or 2027 in the settings file, or false for blank or anything else; a bottom gate, since textToTick throws on text that isn't exactly that form, and a file a person edits can hold anything
export function readTick(text) {
	try { return textToTick(text) } catch (e) { return false }
}

//how long ago, from the milliseconds since, in round words to follow a phrase like last checked: just now, 5 minutes ago, 1 week ago, 2 months ago; a moment in the future, from a clock set back, reads as just now
export function sayAgo(duration) {
	let units = [['years', Time.year], ['months', Time.month], ['weeks', Time.week], ['days', Time.day], ['hours', Time.hour], ['minutes', Time.minute]]//largest first, so the first that fits is the one used
	for (let [name, length] of units) {
		if (duration >= length) return sayDuration(duration, `# ${name} ago`)//in whole units, and singular for exactly one
	}
	return 'just now'//under a minute
}

//the day a tick falls on in UTC, the way a release is dated, like 2026-Oct-7
export function sayDay(tick) {
	return sayTick(tick, 'YYYY-Mmm-D')
}
