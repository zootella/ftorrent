import {invoke} from '@tauri-apps/api/core'

//the windows firewall, read through its COM interface; firewall.rs is the long version. Both reject off windows, so the page asks only there

export function firewallProfiles()  { return invoke('firewall_profiles') }      //the firewall's three profiles, as [{name, current, enabled, block_all, inbound}]: Domain, Private, or Public; whether the current network is in it; whether the firewall is on for it; whether it refuses every incoming connection; and block or allow, what it does to an incoming connection no rule names
export function firewallRules(path) { return invoke('firewall_rules', {path}) } //every rule naming the program at this path, as [{name, direction, action, enabled, profiles, protocol, ports}]: the rule's name, in or out, allow or block, whether it's on, the profiles it applies in, TCP, UDP, any, or a number, and the local ports it covers as the firewall writes them, a list, * for any, or blank for a protocol without ports; an empty list for a program the firewall has never been asked about
