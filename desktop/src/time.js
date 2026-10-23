import {Time, sayDuration, sayTick, textToTick} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go

//a tick as UTC text a person can read in a box or a file, like 2026-10-08T14:22:00Z: ergo's iso form without the milliseconds
export function sayMoment(tick) {
	return sayTick(tick, 'YYYY-MM-DDThh24:mm:ssZ')
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
