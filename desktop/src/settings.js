import {parse as parseToml} from 'smol-toml'
import {brandName, brandStem} from './brand.js'
import {readTick} from './time.js'
import factoryText from './ftorrent.toml?raw'//the settings file exactly as a first run writes it, as text built into the JavaScript; the essay below says why it isn't a file the app reads

/*
The settings file, ftorrent.toml. Its factory form is a file of its own, ftorrent.toml beside this one, and that file is where a setting is defined: its section, its name, the comment that explains it, and its factory value, in the order the file lists them, so a change to a factory setting shows in a diff as a change to that file. This module reads the factory file once, as it loads, and holds everything else about settings: the checks a value must pass, which toml can't carry; parsing, which turns the user's file into a settings object and a list of anything it had to turn away; and rendering, which turns a settings object back into the complete text of the file, the factory file's own header, comments, and order with the user's values in place. The store in stores/settings.js decides when to read and write, and Rust's disk commands do the reading and writing; the rest of the app reads settings from the store. Rust never learns what a setting is: it hands JavaScript the file's path, writes bytes when asked, and holds the text to write once more when ftorrent exits. The window's size, place, and maximized flag are settings like the rest, and window.js reads them from the store to place the window before showing it.

The factory values are code, as they always were, written as TOML so they read like the settings file. Vite's ?raw import builds the factory file's text into the JavaScript, so when ftorrent runs it's a string, never a file to find or read, and it can't go missing; the only file ftorrent reads or writes is the user's, through Rust's disk commands. Tauri's bundle.resources, which places real files beside the executable, is for what has to exist on disk, like the engine Rust starts; the factory text never does. The factory file is exactly what rendering every factory value produces, byte for byte, so a first run writes the factory file itself, and settingsFactoryDrift below says if the two ever differ. Every write after that renders from the same template, which is how a later release adds a new setting to a file a person already has and rewords its comments: only the values are theirs.

The file is TOML rather than JSON for the person who opens it in Notepad. Every setting is written out, always, under a comment saying what it does, with its factory value at the end of the line, so the file is the documentation of every knob ftorrent has; JSON has no comments, so a JSON settings file can never explain itself. And TOML forgives what JSON punishes: no trailing-comma trap, no whole-file failure over one quote. We stay in the plain dialect, sections with simple keys, strings, whole numbers, booleans, and flat arrays, a list written one item to a line, which is all a settings file needs and all a Notepad user should meet.

Three rules about what parsing does with a file. A value it can use replaces the factory value; a value of the wrong type or outside its check is reported and the factory value stays; a key the factory file doesn't have is reported as a typo rather than ignored, since the file lists every setting and an unknown name is a mistake, not a default showing through. A file that won't parse at all leaves everything at factory and is reported, and the store never writes over it: the settings in it are the user's and may be one typo from right, so the file stays exactly as it is until the user fixes it. A file that parses is repaired instead, a bad value put back to its factory value and a missing setting added.
*/

export const settingsFileName = `${brandStem}.toml`//in the data folder paths.rs worked out, which is portable/ beside a portable copy and the user's local application data for an installed one

//the platform this copy runs on, by the name its users know it by: one of ftorrent's few platform checks, on purpose, for the windows conventions below and the few words that differ by platform, like Options. The web view's user agent names the platform: WebView2's always says Windows, and WebKit's on a Mac says Macintosh
export const platformName = navigator.userAgent.includes('Windows') ? 'Windows' : navigator.userAgent.includes('Macintosh') ? 'macOS' : 'Linux'
const onWindows = platformName == 'Windows'

//the web view, for the About panel: the engine, whose version tauri reports, and the system component that carries it. One per platform, since tauri has one web view on each: chromium through microsoft edge webview2 on windows, whose version is the runtime's own, in edge's numbering like 141.0.3537.57, with chromium's major in front; webkit through apple's wkwebview on a mac, whose version is the framework's build number like 20621.3.11.11.3; and webkit through webkitgtk on linux, whose version is that port's release like 2.46.3
export const webviewWords = {Windows: {engine: 'Chromium', host: 'Microsoft Edge WebView2'}, macOS: {engine: 'WebKit', host: 'Apple WKWebView'}, Linux: {engine: 'WebKit', host: 'WebKitGTK'}}[platformName]

//the face the System Font choice draws in, and who made it, for its answer on the Settings panel; undefined on linux, where it's whatever the desktop is set to
export const systemFace = {Windows: {face: 'Segoe UI', maker: 'Microsoft'}, macOS: {face: 'San Francisco', maker: 'Apple'}}[platformName]

//what ftorrent calls its settings, in the menu and at the top of their panel: options on windows, where a program's classic menu is tools, options, as it is in qbittorrent, and settings everywhere else, the word macos has used since ventura
export const settingsName = onWindows ? 'Options' : 'Settings'

//the words for starting at login, which say startup, the word people use for it, though the system starts ftorrent as each user logs in; and when the system differs from the answer, the link to its own page for it, by the name each platform gives that page
export const loginWords = {
	Windows: {question: `Run ${brandName} at startup`, confirm: 'Confirm in Startup Apps'},
	macOS:   {question: `Run ${brandName} at startup`, confirm: 'Confirm in Login Items & Extensions'},
}[platformName]//undefined on linux, where ftorrent doesn't start at login, so the Settings panel doesn't ask
//the words for the link to the page where a person lets ftorrent through the firewall, which name where the link goes rather than an action, since following it changes nothing: System Settings at the Network pane on a Mac, a row above Firewall, and Windows Firewall's page of allowed apps in Control Panel
export const firewallWords = {Windows: 'Windows Firewall', macOS: 'System Firewall Settings'}[platformName]//undefined on linux, which has no one page for it, so the Settings panel shows no link there
//the typefaces this copy offers: verdana is a throwback to windows programs around 2000, standing in for the tahoma they were set in, and style.css says why; offered on windows and the mac, which both install it with the system, and not on linux, where it's only there if someone added it
export const fontsOffered = platformName == 'Linux' ? ['system', 'inter'] : ['system', 'inter', 'verdana']

//what a setting's value must be beyond its factory value's type, by section and name; a check has to accept the factory value, or an ordinary file would report a problem against itself, and a setting without one is held to its type alone, a number also to being whole
const isTick = value => value == '' || readTick(value) !== false//ergo's short form of a tick, read back exactly, or blank; !== since 1970 reads as 0
const isTrackers = value => value.every(url => typeof url == 'string' && /^(udp|https?|wss?):\/\/[^\s/?#]+/i.test(url))//announce addresses of the five kinds libtorrent speaks, each naming a host
const settingsChecks = {
	'downloads.folders': value => value.every(folder => typeof folder == 'string' && folder.trim() != '' && !/[\x00-\x1f\x7f]/.test(folder)),//text naming a place; a control character in one means a backslash in the file was read as an escape, the way D:\new holds a newline
	'trackers.lists': value => value.every(url => typeof url == 'string' && /^https:\/\/[^\s/?#]+/i.test(url)),//web addresses over https, the only scheme net_get in net.rs fetches
	'trackers.always': isTrackers,
	'trackers.answered': isTrackers,
	'trackers.checked': isTick,
	'associations.default': value => ['ask', 'yes', 'no'].includes(value),
	'update.last': isTick,
	'update.next': isTick,
	'appearance.mode': value => ['light', 'dark', 'system'].includes(value),
	'appearance.font': value => fontsOffered.includes(value),//so a portable copy's file carried to linux with verdana in it says so once and goes back to system, like any value this copy can't use
}

//every setting, read once from the factory file, in its order: the lines of comment that open the file, then for each setting its section, its name, its factory value, the lines of comment above it, and its check
export const settingsSchema = readFactory(factoryText)

function readFactory(text) {//the factory file's layout, from its lines, with the values toml reads from it; a list's items are values toml already has, so the lines between its brackets are passed over
	let values = parseToml(text)
	let header = []
	let entries = []
	let section = ''
	let comment = []
	let inList = false
	for (let line of text.split('\n')) {
		if (inList) { if (line.trim().startsWith(']')) inList = false; continue }
		let heading = line.match(/^\[(\w+)\]$/)
		let setting = line.match(/^(\w+)\s*=\s*(.*)$/)
		if (heading) section = heading[1]
		else if (line.startsWith('#')) (section ? comment : header).push(line)//before the first section, the file's own header; after it, the comment above the next setting
		else if (setting) {
			entries.push({section, key: setting[1], factory: values[section][setting[1]], comment, check: settingsChecks[`${section}.${setting[1]}`]})
			comment = []
			if (setting[2].startsWith('[') && !setting[2].includes(']')) inList = true//a list written one item to a line, which ends at the line that starts with its closing bracket
		}
	}
	return {header, entries}
}

export function settingsFactory() {//a settings object with every value at its factory setting
	let s = {}
	for (let entry of settingsSchema.entries) (s[entry.section] ??= {})[entry.key] = sayCopy(entry.factory)
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

	for (let entry of settingsSchema.entries) {
		let value = parsed[entry.section]?.[entry.key]
		if (value == undefined) continue//not in the file, which is ordinary; rendering puts the line back
		let name = `${entry.section}.${entry.key}`//only for the two complaints below
		if (!sameType(value, entry.factory))     { problems.push(`${name} has to be ${sayType(entry.factory)}, so ${sayValue(value)} was ignored`); continue }
		if ((entry.check && !entry.check(value)) || (typeof value == 'number' && !Number.isInteger(value))) { problems.push(`${name} cannot be ${sayValue(value)}, so it was ignored`); continue }
		settings[entry.section][entry.key] = sayCopy(value)
	}
	for (let [section, table] of Object.entries(parsed)) {//the file lists every setting ftorrent has, so a name it doesn't know is a typo rather than a default quietly showing through, and worth saying out loud
		if (typeof table != 'object' || Array.isArray(table)) { problems.push(`${section} is not one of ${brandName}'s settings`); continue }
		for (let key of Object.keys(table)) {
			if (!settingsSchema.entries.some(entry => entry.section == section && entry.key == key)) problems.push(`${section}.${key} is not one of ${brandName}'s settings`)
		}
	}
	return {settings, problems, parsed: true}
}

export function settingsRender(settings) {//the complete text of the file for these settings, the factory file's header, comments, and order with these values in place, and the only place that text ever comes from
	let lines = [...settingsSchema.header]
	for (let section of new Set(settingsSchema.entries.map(entry => entry.section))) {
		let entries = settingsSchema.entries.filter(entry => entry.section == section)
		let keyWidth   = Math.max(...entries.map(entry => entry.key.length))//pad within the section, so a long name in one doesn't push the others out
		let valueWidth = Math.max(0, ...entries.filter(entry => !Array.isArray(entry.factory)).map(entry => sayValue(settings[section][entry.key]).length))//lists take lines of their own, so only the values beside their names line up

		lines.push('', `[${section}]`)
		for (let entry of entries) {
			lines.push(...entry.comment)//above the setting, not trailing it: these run long, and a soft wrapped comment beside a value would fold across the next line
			let value = settings[section][entry.key]
			if (!Array.isArray(entry.factory)) lines.push(`${entry.key.padEnd(keyWidth)} = ${sayValue(value).padEnd(valueWidth)} # factory ${sayValue(entry.factory)}`)
			else if (!value.length) lines.push(`${entry.key.padEnd(keyWidth)} = []`)
			else lines.push(`${entry.key.padEnd(keyWidth)} = [`, ...value.map(item => `\t${sayValue(item)},`), ']')//one item to a line, indented with a tab, each with its comma, the last one too, which toml allows, so adding a line never means touching the one above; a list's factory value is the factory file's own, too long to repeat beside it
		}
	}
	return lines.join('\n')+'\n'
}

export const settingsFactoryDrift = settingsRender(settingsFactory()) != factoryText//true when the factory file isn't exactly what rendering every factory value produces, which the store logs; edit the factory file in the rendered form, padding and all

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
