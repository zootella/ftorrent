import {parse as parseToml} from 'smol-toml'
import {brandName, brandStem, urlHome} from './brand.js'

/*
The settings file, ftorrent.toml, and the only place a setting is defined. Everything about the file is here: the schema below, which names every setting with its factory value and the comment that explains it; parsing, which turns the file's text into a settings object and a list of anything it had to turn away; and rendering, which turns a settings object back into the complete text of the file. The store in stores/settings.js does the reading and writing around these, and the rest of the app reads settings from the store. Rust never learns what a setting is: it hands the page the file's path, writes bytes when asked, and holds the text to write once more when ftorrent exits. The window's size, place, and maximized flag are settings like the rest, and window.js reads them from the store to place the window before showing it.

The file is TOML rather than JSON for the person who opens it in Notepad. Every setting is written out, always, under a comment saying what it does, with its factory value at the end of the line, so the file is the documentation of every knob ftorrent has; JSON has no comments, so a JSON settings file can never explain itself. And TOML forgives what JSON punishes: no trailing-comma trap, no whole-file failure over one quote. We stay in the plain dialect, sections with simple keys, strings, numbers, booleans, and flat arrays, which is all a settings file needs and all a Notepad user should meet.

Three rules about what parsing does with a file. A value it can use replaces the factory value; a value of the wrong type or outside its check is reported and the factory value stays; a key the schema doesn't know is reported as a typo rather than ignored, since the file lists every setting and an unknown name is a mistake, not a default showing through. A file that won't parse at all leaves everything at factory and is reported, and the store never writes over it: the settings in it are the user's and may be one typo from right, so the file stays exactly as it is until the user fixes it. A file that parses is repaired instead, a bad value put back to its factory value and a missing setting added.
*/

export const settingsFileName = `${brandStem}.toml`//in the data folder paths.rs worked out, which is portable/ beside a portable copy and the user's local application data for an installed one
const settingsHeader = `# ${brandStem}.toml — ${brandName} reads this file when it starts and writes it when a setting changes, and again as it closes
# edit this file only while ${brandName} isn't running: while it runs, including hidden in the tray, it writes the file once more as it closes, over any change made here
# with ${brandName} not running, edit the values freely; the comments and the layout are regenerated every time, so notes of your own here will not survive`//the same words on every platform, whose menus say quit or exit, so a portable copy's file that travels between them is never rewritten over wording

//the platform this copy runs on, by the name its users know it by: one of ftorrent's few platform checks, on purpose, for the windows conventions below and the few words that differ by platform, like Options. The web view's user agent names the platform: WebView2's always says Windows, and WebKit's on a Mac says Macintosh
export const platformName = navigator.userAgent.includes('Windows') ? 'Windows' : navigator.userAgent.includes('Macintosh') ? 'macOS' : 'Linux'
const onWindows = platformName == 'Windows'

//the web view the page runs in, for the about page: the engine, whose version tauri reports, and the system component that carries it. One per platform, since tauri has one web view on each: chromium through microsoft edge webview2 on windows, whose version is a chromium number like 141.0.3537.57; webkit through apple's wkwebview on a mac, whose version is the framework's build number like 20621.3.11.11.3; and webkit through webkitgtk on linux, whose version is that port's release like 2.46.3
export const webviewWords = {Windows: {engine: 'Chromium', host: 'Microsoft Edge WebView2'}, macOS: {engine: 'WebKit', host: 'Apple WKWebView'}, Linux: {engine: 'WebKit', host: 'WebKitGTK'}}[platformName]

//the face the System Font choice draws in, and who made it, for its answer on the settings page; undefined on linux, where it's whatever the desktop is set to
export const systemFace = {Windows: {face: 'Segoe UI', maker: 'Microsoft'}, macOS: {face: 'San Francisco', maker: 'Apple'}}[platformName]

//what ftorrent calls its settings, in the menu and at the top of their page: options on windows, where a program's classic menu is tools, options, as it is in qbittorrent, and settings everywhere else, the word macos has used since ventura
export const settingsName = onWindows ? 'Options' : 'Settings'

//the words for starting at login, which say startup, the word people use for it, though the system starts ftorrent as each user logs in; and when the system differs from the answer, the link to its own page for it, by the name each platform gives that page
export const loginWords = {question: `Run ${brandName} at startup`, confirm: onWindows ? 'Confirm in Startup Apps' : 'Confirm in Login Items & Extensions'}
//the typefaces this copy offers: verdana is a throwback to windows programs around 2000, standing in for the tahoma they were set in, and style.css says why; offered on windows and the mac, which both install it with the system, and not on linux, where it's only there if someone added it
export const fontsOffered = platformName == 'Linux' ? ['system', 'inter'] : ['system', 'inter', 'verdana']

//every setting ftorrent has, and the only place any of them is defined; a check, where the type alone isn't enough, has to accept the factory value or an ordinary file would report a problem against itself
export const settingsSchema = [
	{
		section: 'downloads',
		key: 'folders',
		factory: [`~/Downloads/${brandName}`],
		comment: `where torrents go, and where ${brandName} looks for the ones it already has; each folder keeps its own .${brandStem} subfolder with the session data of the torrents in it. ~ is your home folder, ./ is the folder ${brandName} itself is in, so a portable copy can say ./downloads and follow its own drive, and an absolute path like D:/torrents means exactly that place. Written with forward slashes on every platform`,
		check: value => value.every(folder => typeof folder == 'string' && folder.trim() != '' && !/[\x00-\x1f\x7f]/.test(folder)),//text naming a place; a control character in one means a backslash in the file was read as an escape, the way D:\new holds a newline
	}, {
		section: 'associations',
		key: 'default',
		factory: 'ask',
		comment: `whether ${brandName} opens torrent files and links, .torrent and .${brandStem} files and magnet: and ${brandStem}: links: "yes", "no", or "ask". Unless this is "no", a bar at the top of the window asks whenever the system opens any of them with another app, and answering yes there claims them; ${brandName} never claims them on its own. The system keeps the final say, so on Windows yes may open its settings for you to confirm there. Whatever the answer, ${brandName} stays in the system's Open with menu for all four`,
		check: value => ['ask', 'yes', 'no'].includes(value),
	}, {
		section: 'login',
		key: 'start',
		factory: false,
		comment: `whether ${brandName} runs at startup, starting hidden as you log in, with only its icon near the clock: true or false, on a Mac or Windows. Choosing on the settings page also sets it in the system's own settings, Login Items & Extensions on a Mac and Startup Apps on Windows; and when the two differ, as when it's changed there or here in the file, the settings page offers a link to confirm it there`,
		check: value => typeof value == 'boolean',
	}, {
		section: 'log',
		key: 'record',
		factory: false,
		comment: `whether ${brandName} keeps a log, for answering a question about what it did: true writes a file for each time ${brandName} runs into ${brandStem}-logs in your home folder, as things happen. A log names files and links you open, so it's off unless you turn it on`,
		check: value => typeof value == 'boolean',
	}, {
		section: 'update',
		key: 'automatic',
		factory: true,
		comment: `whether ${brandName} asks ${urlHome} on its own which version is newest: true or false, on a Mac or Windows, and never in a portable copy. With false, it asks only when you press Check for Update. Below are when it last heard back and when it will next ask, dates and times in UTC; blank next means at the next start`,
		check: value => typeof value == 'boolean',
	}, {
		section: 'update',
		key: 'last',
		factory: '',
		check: value => value == '' || !isNaN(Date.parse(value)),
	}, {
		section: 'update',
		key: 'next',
		factory: '',
		check: value => value == '' || !isNaN(Date.parse(value)),
	}, {
		section: 'appearance',
		key: 'mode',
		factory: 'system',
		comment: `light or dark: "light" or "dark" keeps ${brandName} that way whatever the system is set to; "system" matches the system's own light or dark setting, and changes when it does`,
		check: value => ['light', 'dark', 'system'].includes(value),
	}, {
		section: 'appearance',
		key: 'font',
		factory: 'system',
		comment: `the typeface: "system" is the one the system sets its own menus and windows in, Segoe UI on Windows and San Francisco on a Mac; "inter" is Inter, with IBM Plex Mono for fixed-width text, both carried by ${brandName}; "verdana" is Verdana, which drawn today looks the way Tahoma did in Windows programs around 2000, with the same Plex Mono for fixed-width text, and is offered on Windows and the Mac`,
		check: value => fontsOffered.includes(value),//so a portable copy's file carried to linux with verdana in it says so once and goes back to system, like any value this copy can't use
	}, {
		section: 'window',
		key: 'x',
		factory: 0,
		comment: `where the window was when ${brandName} last closed it, so it opens there again: the position of its top left corner, then its inner size, in css pixels, which on a Mac are points and on Windows are the screen's pixels divided by its scale. ${brandName} puts it back only on the screen recorded under [screen], still exactly as it was; otherwise it picks a fresh size and place. A portable copy records none of this, and opens somewhere fresh every time. Zeros mean nothing has been recorded yet`,
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
		comment: 'whether the window was maximized, or zoomed on a Mac, so it opens that way again; the four numbers above are the size and place it goes to when restored',
	}, {
		section: 'screen',
		key: 'x',
		factory: 0,
		comment: `the screen the window was on when its place was recorded: its position among your screens and its size, in css pixels, then its scale as a percent, 100 for a plain screen, 200 for a Retina display, 125 for Windows set to 125%. ${brandName} puts the window back only if a screen matches all five exactly, so a new resolution, scale, or rotation, or the screen unplugged or moved among the others, means a fresh place`,
		check: Number.isInteger,
	}, {
		section: 'screen',
		key: 'y',
		factory: 0,
		check: Number.isInteger,
	}, {
		section: 'screen',
		key: 'width',
		factory: 0,
		check: Number.isInteger,
	}, {
		section: 'screen',
		key: 'height',
		factory: 0,
		check: Number.isInteger,
	}, {
		section: 'screen',
		key: 'scale',
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
		if (typeof table != 'object' || Array.isArray(table)) { problems.push(`${section} is not one of ${brandName}'s settings`); continue }
		for (let key of Object.keys(table)) {
			if (!settingsSchema.some(entry => entry.section == section && entry.key == key)) problems.push(`${section}.${key} is not one of ${brandName}'s settings`)
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
