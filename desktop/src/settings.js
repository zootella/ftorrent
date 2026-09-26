//./src/settings.js

import {parse as parseToml} from 'smol-toml'

/*
The settings file, ftorrent.toml, and the only place a setting is defined. Everything about the file is here: the schema below, which names every setting with its factory value and the comment that explains it; parsing, which turns the file's text into a settings object and a list of anything it had to turn away; and rendering, which turns a settings object back into the complete text of the file. The store in stores/settings.js does the reading and writing around these, and the rest of the app reads settings from the store. Rust never learns what a setting is: it hands the page the file's path, writes bytes when asked, and holds the text to write once more when ftorrent exits. The one exception is the window, whose size, place, and maximized flag settings.rs reads before the page exists, since the window has to be built before there's a page to ask.

The file is TOML rather than JSON for the person who opens it in Notepad. Every setting is written out, always, under a comment saying what it does, with its factory value at the end of the line, so the file is the documentation of every knob ftorrent has; JSON has no comments, so a JSON settings file can never explain itself. And TOML forgives what JSON punishes: no trailing-comma trap, no whole-file failure over one quote. We stay in the plain dialect, sections with simple keys, strings, numbers, booleans, and flat arrays, which is all a settings file needs and all a Notepad user should meet.

Three rules about what parsing does with a file. A value it can use replaces the factory value; a value of the wrong type or outside its check is reported and the factory value stays; a key the schema doesn't know is reported as a typo rather than ignored, since the file lists every setting and an unknown name is a mistake, not a default showing through. A file that won't parse at all leaves everything at factory and is reported, and the store never writes over it: the settings in it are the user's and may be one typo from right, so the file stays exactly as it is until the user fixes it. A file that parses is repaired instead, a bad value put back to its factory value and a missing setting added.
*/

export const settingsFileName = 'ftorrent.toml'//in the data folder paths.rs worked out, which is portable/ beside a portable copy and the user's local application data for an installed one
const settingsHeader = `# ftorrent.toml — ftorrent reads this file when it starts and writes it when a setting changes, and again when it quits
# quit ftorrent before editing this file: while ftorrent is running, including hidden in the tray, it writes the file once more as it quits, over any change made here
# with ftorrent quit, edit the values freely; the comments and the layout are regenerated every time, so notes of your own here will not survive`

//every setting ftorrent has, and the only place any of them is defined; a check, where the type alone isn't enough, has to accept the factory value or an ordinary file would report a problem against itself
export const settingsSchema = [
	{
		section: 'note',
		key: 'text',
		factory: '',
		comment: 'a note to yourself, kept here and shown on the main page; it does nothing, and exists to prove that a setting survives quitting and starting again',
	}, {
		section: 'downloads',
		key: 'folders',
		factory: ['~/Downloads/ftorrent'],
		comment: 'where torrents go, and where ftorrent looks for the ones it already has; each folder keeps its own .ftorrent subfolder with the session data of the torrents in it. ~ is your home folder, ./ is the folder ftorrent itself is in, so a portable copy can say ./downloads and follow its own drive, and an absolute path like D:/torrents means exactly that place. Written with forward slashes on every platform',
		check: value => value.every(folder => typeof folder == 'string' && folder.trim() != '' && !/[\x00-\x1f\x7f]/.test(folder)),//text naming a place; a control character in one means a backslash in the file was read as an escape, the way D:\new holds a newline
	}, {
		section: 'window',
		key: 'x',
		factory: 0,
		comment: 'where the window was when ftorrent last closed it, in css pixels, so it opens there again: the position of its top left corner, then its inner size. ftorrent replays these only when the screen below still has the size recorded under [screen]; otherwise it picks a fresh size and place. Zeros mean nothing has been recorded yet',
		check: Number.isInteger,
	}, {
		section: 'window',
		key: 'y',
		factory: 0,
		check: Number.isInteger,
	}, {
		section: 'window',
		key: 'width',
		factory: 0,
		check: Number.isInteger,
	}, {
		section: 'window',
		key: 'height',
		factory: 0,
		check: Number.isInteger,
	}, {
		section: 'window',
		key: 'maximized',
		factory: false,
		comment: 'whether the window was maximized, so it opens maximized again; the four numbers above are the size and place it goes to when restored',
	}, {
		section: 'screen',
		key: 'width',
		factory: 0,
		comment: 'the size, in css pixels, of the screen the window was on when its position was recorded; the fingerprint that decides whether the position above still means anything',
		check: Number.isInteger,
	}, {
		section: 'screen',
		key: 'height',
		factory: 0,
		check: Number.isInteger,
	},
]

export function settingsFactory() {//a settings object with every value at its factory setting
	let s = {}
	for (let entry of settingsSchema) (s[entry.section] ??= {})[entry.key] = sayCopy(entry.factory)
	return s
}

export function settingsParse(text) {//the settings the given file text describes, a list of anything in it ftorrent had to turn away, and whether the text was toml at all
	let settings = settingsFactory()//only a usable value in the file replaces one of these
	let problems = []
	let parsed
	try {
		parsed = parseToml(text)
	} catch (error) {
		problems.push(`could not read the file, ${error.message}`)
		return {settings, problems, parsed: false}//nothing in there is usable, so everything stays factory, and the store leaves the file alone
	}

	for (let entry of settingsSchema) {
		let value = parsed[entry.section]?.[entry.key]
		if (value == undefined) continue//not in the file, which is ordinary; rendering puts the line back
		let name = `${entry.section}.${entry.key}`//only for the two complaints below
		if (!sameType(value, entry.factory))     { problems.push(`${name} has to be ${sayType(entry.factory)}, so ${sayValue(value)} was ignored`); continue }
		if (entry.check && !entry.check(value))   { problems.push(`${name} cannot be ${sayValue(value)}, so it was ignored`);                         continue }
		settings[entry.section][entry.key] = sayCopy(value)
	}
	for (let [section, table] of Object.entries(parsed)) {//the file lists every setting ftorrent has, so a name it doesn't know is a typo rather than a default quietly showing through, and worth saying out loud
		if (typeof table != 'object' || Array.isArray(table)) { problems.push(`${section} is not an ftorrent setting`); continue }
		for (let key of Object.keys(table)) {
			if (!settingsSchema.some(entry => entry.section == section && entry.key == key)) problems.push(`${section}.${key} is not an ftorrent setting`)
		}
	}
	return {settings, problems, parsed: true}
}

export function settingsRender(settings) {//the complete text of the file for these settings, and the only place that text ever comes from
	let lines = [settingsHeader]
	for (let section of new Set(settingsSchema.map(entry => entry.section))) {
		let entries = settingsSchema.filter(entry => entry.section == section)
		let keyWidth   = Math.max(...entries.map(entry => entry.key.length))//pad within the section, so a long name in one doesn't push the others out
		let valueWidth = Math.max(...entries.map(entry => sayValue(settings[section][entry.key]).length))

		lines.push('', `[${section}]`)
		for (let entry of entries) {
			if (entry.comment) lines.push(`# ${entry.comment}`)//above the setting, not trailing it: these run long, and a soft wrapped comment beside a value would fold across the next line
			lines.push(`${entry.key.padEnd(keyWidth)} = ${sayValue(settings[section][entry.key]).padEnd(valueWidth)} # factory ${sayValue(entry.factory)}`)
		}
	}
	return lines.join('\n')+'\n'
}

function sayValue(value) {//a value as the toml text that means it
	if (typeof value == 'boolean') return value ? 'true' : 'false'
	if (typeof value == 'number')  return String(value)
	if (Array.isArray(value))      return `[${value.map(sayValue).join(', ')}]`//a flat array of the values above; the one shape of array the schema uses
	return JSON.stringify(String(value)).replace(/\x7f/g, '\\u007f')//a basic string: toml took its string escapes from json, so a json string is a toml one, with quotes, backslashes, and control characters like a newline all escaped; toml also wants DEL escaped, which json leaves bare
}

function sayType(value) {//the word for a value's type in a complaint, so a list reads as a list rather than as an object
	return Array.isArray(value) ? 'a list' : typeof value
}

function sameType(value, factory) {//does a value from the file have the shape of the factory value: a list where the factory is a list, and otherwise the same typeof
	if (Array.isArray(factory)) return Array.isArray(value)
	return typeof value == typeof factory && !Array.isArray(value)
}

function sayCopy(value) {//a value of its own, so a factory array handed into a settings object is never the schema's array itself
	return Array.isArray(value) ? [...value] : value
}
