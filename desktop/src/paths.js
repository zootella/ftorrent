//./src/paths.js

import {invoke} from '@tauri-apps/api/core'

//where everything is: paths.rs works it out once at startup, before the page exists, and is the long version

export function pathsStatus() { return invoke('paths_status') }//installed, portable, or translocated; the program's location, the home folder, the data folder and the files in it, and any trouble

//where a download folder setting points on this machine; the setting is written one of three ways, and paths.rs has the long version of what each means
export function resolveFolder(setting, location, home) {
	setting = setting.replace(/\\/g, '/')//a path pasted from windows works the same as one written the documented way
	let windows = location.includes('\\')//the anchors come from rust with the platform's own separators, so that is what the result gets too
	let [anchor, rest] =
		setting == '~' || setting.startsWith('~/') ? [home,     setting.slice(1)] ://relative to whoever is signed in
		setting == '.' || setting.startsWith('./') ? [location, setting.slice(1)] ://relative to the program, so it follows a portable copy onto whatever drive it lands on
		isAbsolute(setting)                        ? [null,     setting]          ://exactly the place it names, which on a machine without it is simply a folder that isn't there
		                                             [location, '/' + setting]     //a bare relative path, like downloads, is read as ./downloads rather than relative to whatever folder the program happened to be started from
	if (anchor == null) return windows ? setting.replace(/\//g, '\\') : setting
	if (anchor == '') return ''//an anchor rust couldn't find, so the setting names nowhere on this machine, rather than a folder at the root of the drive
	let parts = rest.split('/').filter(part => part != '')//one part at a time, so the separators come out native and a doubled slash costs nothing
	let separator = windows ? '\\' : '/'
	return [anchor, ...parts].join(separator)
}

function isAbsolute(setting) {//does a setting name one exact place, on any platform: C:/Games is absolute on a mac too, where it names a drive that isn't there, rather than a folder called C: beside the program
	return setting.startsWith('/') || /^[A-Za-z]:/.test(setting)
}
