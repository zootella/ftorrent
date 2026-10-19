import {Time} from '@zootella/ergo'//small helpers from the ergo project, the same in javascript, python, rust, and go

//a moment as UTC text a person can read in a file, like 2026-10-08T14:22:00Z: toISOString without the milliseconds
export function sayMoment(time) {
	return new Date(time).toISOString().replace(/\.\d+Z$/, 'Z')
}

//how long ago, from the milliseconds since, in round words to follow a phrase like last checked: just now, 5 minutes ago, 1 week ago, 2 months ago; a moment in the future, from a clock set back, reads as just now
export function sayAgo(duration) {
	let units = [['year', Time.year], ['month', Time.month], ['week', Time.week], ['day', Time.day], ['hour', Time.hour], ['minute', Time.minute]]//largest first, so the first that fits is the one used
	for (let [name, length] of units) {
		let count = Math.floor(duration / length)
		if (count >= 1) return `${count} ${name}${count == 1 ? '' : 's'} ago`
	}
	return 'just now'//under a minute
}

//a day written 2026-10-07, as a sidecar dates a release, in the words a person reads, 2026-Oct-7; split rather than parsed, so no time zone moves it to the day before
export function sayDay(date) {
	let [year, month, day] = date.split('-').map(Number)
	return `${year}-${['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'][month - 1]}-${day}`
}
