import {invoke} from '@tauri-apps/api/core'
import {registryGet, registryGetBinary, registrySet, registryDelete} from './registry.js'
import {brandName} from './brand.js'
import {platformName} from './settings.js'

/*
Starting at login, on Windows and the Mac, where two things are kept apart: what the user wants, the login.start setting, which only the user's answer on the settings page changes; and what the system will do at the next login, which ftorrent reads. Each system has its own screen where a user can change it too, so the two can differ, and when they do ftorrent says so beside the answer, with a link to that screen, rather than changing either one on its own. This file is how each platform writes ftorrent's entry, takes it away, and reads where things stand; stores/login.js is when.

ftorrent writes to the system only when the user answers. A yes puts the entry there and a no takes it away, and a read follows each, and every other time ftorrent only reads. That matters because no read can tell why an entry is missing: the user removing it in System Settings and the entry never having been made look exactly alike, so an app that put back what it found missing would undo the user's own choice, often within a second, as we measured on the Mac.

On the Mac the entry is the app itself, as a login item through SMAppService, and macOS answers one of four words, which login.rs has the long version of: enabled is on; requiresApproval is an entry there that waits on the user's approval in System Settings, so it won't start ftorrent yet; and notRegistered and notFound mean no entry, which is also what removing it from Open at Login leaves. On Windows the entry is a value under the user's Run key, named brandName, whose command starts this executable with --login, the argument login.rs recognizes to start hidden. Windows keeps the user's switch apart from it, in Task Manager's Startup apps and in Settings, under StartupApproved, where the value of the same name holds a few bytes, the first of them odd when the user switched it off; Microsoft doesn't document the format, so ftorrent only reads it. A Run value with any other command, an older path or argument, counts as missing. win-setup/registry.js takes both values away at uninstall.
*/

export function loginLaunch()     { return invoke('login_launch')     }//whether the system started this process at login, and the argument a windows registration carries to say so
export function loginStatus()     { return invoke('login_status')     }//on the mac, what macOS says about this app as a login item
export function loginRegister()   { return invoke('login_register')   }//on the mac, make this app a login item
export function loginUnregister() { return invoke('login_unregister') }//on the mac, take it off
export function loginSettings()   { return invoke('login_settings')   }//on the mac, open Login Items in System Settings

const runKey = 'Software\\Microsoft\\Windows\\CurrentVersion\\Run'//the user's own list of what starts at sign-in, under HKEY_CURRENT_USER, which needs no administrator
const approvedKey = 'Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run'//where windows keeps the user's switch for each of them
const valueName = brandName//the name of ftorrent's value under both, which win-setup/registry.js removes by the same name

function runCommand(paths, argument) { return `"${paths.executable}" ${argument}` }//quoted, since the path may hold spaces

export async function loginRead(paths, argument) {//where things stand: on; off, which is ftorrent's entry there with the system holding it back, by the user's switch on windows or awaiting approval on the mac; or absent, which means there's no entry of ftorrent's
	if (platformName == 'macOS') {
		let status = await loginStatus()
		return status == 'enabled' ? 'on' : status == 'requiresApproval' ? 'off' : 'absent'
	}
	if (await registryGet('user', runKey, valueName) != runCommand(paths, argument)) return 'absent'
	let approved = await registryGetBinary('user', approvedKey, valueName)//no value at all is how windows says the user has never touched the switch, which is on
	return approved?.length && approved[0] & 1 ? 'off' : 'on'//2 and 6 are the on values seen in the wild, 3 and 7 the off ones, so the low bit is the switch
}

export async function loginWrite(paths, argument) {//put ftorrent's entry there
	if (platformName == 'macOS') return loginRegister()
	await registrySet(runKey, valueName, runCommand(paths, argument))
}

export async function loginRemove() {//take ftorrent's entry away, if there is one; answers whether there was
	if (platformName == 'macOS') {
		if (!['enabled', 'requiresApproval'].includes(await loginStatus())) return false//nothing to take away, and macOS refuses to unregister what isn't there
		await loginUnregister()
		return true
	}
	return registryDelete(runKey, valueName)
}
