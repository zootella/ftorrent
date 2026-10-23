import {ref} from 'vue'
import {dialogOpen, dialogSave} from './dialog.js'
import {documentsFolder, downloadsFolder} from './paths.js'

/*
A spike for the system's boxes: the home page's buttons and File, Open… each put one up a different way, and spikeStatus says what came back, so a person can check each platform's open, folder, and save boxes by eye. Scaffolding, like log and stick, for while the boxes are built and checked, gone once the features that need them use them. We checked them by eye on Windows and the Mac in October 2026; Linux waits for a Linux machine, where the Flatpak's boxes should reach any folder, a microSD card under /run/media included.
*/

export const spikeStatus = ref('')//what the last box answered, in words

const torrents = {kind: 'Torrent files', extensions: ['torrent']}//the filter a torrent box offers

//each button's words and the box it puts up, a different way each, and each naming the folder it opens in, since a blank one can leave a Windows box in the program's own folder
export const spikeBoxes = [
	{label: 'Open torrents',         box: async () => dialogOpen({multiple: true, start: await downloadsFolder(), ...torrents})},//several torrent files from Downloads, as File, Open… asks
	{label: 'Open any file',         box: async () => dialogOpen({start: await documentsFolder()})},//one file of any kind, from Documents
	{label: 'Choose a folder',       box: async () => dialogOpen({folder: true, start: await documentsFolder()})},//one folder, from Documents
	{label: 'Open a file or folder', box: async () => dialogOpen({either: true, start: await documentsFolder()})},//either in one box on a mac, and a file box elsewhere, from Documents
	{label: 'Choose folders',        box: async () => dialogOpen({folder: true, multiple: true, start: await downloadsFolder()})},//several folders, from Downloads
	{label: 'Save a torrent',        box: async () => dialogSave({start: await documentsFolder(), name: 'new.torrent', ...torrents})},//a place for a new torrent file, suggesting a name, from Documents
]

//put a box up and say what came back: cancelled, the paths chosen, or the error
export async function spikeRun(label, box) {
	spikeStatus.value = `${label}: the box is up`
	try {
		let paths = [await box()].flat().filter(Boolean)//the open box answers a list, the save box one path or blank
		spikeStatus.value = `${label}: ${paths.length ? `chose ${paths.join(', ')}` : 'cancelled'}`
	} catch (error) {
		spikeStatus.value = `${label}: ${error}`
	}
}
