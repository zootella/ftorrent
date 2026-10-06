//lengths of time in milliseconds, the unit Date.now() and setInterval speak, so a duration reads as 24*Time.hour rather than a number to work out
export const Time = {second: 1000, minute: 60*1000, hour: 60*60*1000, day: 24*60*60*1000, week: 7*24*60*60*1000, month: 30*24*60*60*1000, year: 365*24*60*60*1000}//a month and a year here are round, which is all a phrase like 2 months ago needs

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
