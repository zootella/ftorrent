import {invoke} from '@tauri-apps/api/core'

//the menu bar, which lifecycle.rs builds; this is the one thing the page asks of it

export function menuEnable(menu, item, enabled) { return invoke('menu_enable', {menu, item, enabled}) }//light or gray one item of the menu bar, by the words the bar shows for its menu and for it; the mac's Edit menu is the one that asks, and off the mac rust answers that there is no such menu
