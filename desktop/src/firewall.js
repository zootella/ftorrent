import {invoke} from '@tauri-apps/api/core'
import {platformName} from './settings.js'
import {processOpen, processStart} from './process.js'
import {pathsStatus} from './paths.js'

//the windows firewall, read through its COM interface; firewall.rs is the long version. These two reads reject off windows, so the page asks only there

export function firewallProfiles()  { return invoke('firewall_profiles') }      //the firewall's three profiles, as [{name, current, enabled, block_all, inbound}]: Domain, Private, or Public; whether the current network is in it; whether the firewall is on for it; whether it refuses every incoming connection; and block or allow, what it does to an incoming connection no rule names
export function firewallRules(path) { return invoke('firewall_rules', {path}) } //every rule naming the program at this path, as [{name, direction, action, enabled, profiles, protocol, ports}]: the rule's name, in or out, allow or block, whether it's on, the profiles it applies in, TCP, UDP, any, or a number, and the local ports it covers as the firewall writes them, a list, * for any, or blank for a protocol without ports; an empty list for a program the firewall has never been asked about

const firewallPane = 'x-apple.systempreferences:com.apple.Network-Settings.extension?Firewall'//System Settings' Network pane on a Mac, with Firewall, the anchor System Settings itself lists for that pane when AppleScript asks for its anchors. macOS 27 accepts the anchor and opens Network without going on to it, whether from this link or from AppleScript's reveal, so the person lands one row above Firewall, whose Options… button holds block all, stealth mode, and the list of programs; a version that honors the anchor lands on Firewall itself

//open the page where a person lets a program through the firewall, for the settings page's link and the Connection tab's Firewall settings… button. On a Mac the firewall's pane in System Settings, where the person turns the firewall on or off and, under Options…, lets a program in; on Windows Control Panel's Allow an app through Windows Firewall, by control.exe from the folder Windows keeps its own programs in, since the page has no address of its own to open. Linux has no one page to open, and the callers offer this only on a Mac and Windows
export async function firewallSettings() {
	if (platformName == 'macOS') return processOpen(firewallPane)
	if (platformName == 'Windows') {
		let paths = await pathsStatus()
		return processStart(`${paths.system}\\control.exe`, ['/name', 'Microsoft.WindowsFirewall', '/page', 'pageConfigureApps'])
	}
}
