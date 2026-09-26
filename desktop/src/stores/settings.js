import {ref, reactive, computed} from 'vue'
import {defineStore} from 'pinia'
import {settingsFactory, settingsParse, settingsRender} from '../settings.js'
import {diskRead, diskWrite, diskMkdir, diskHide, diskStat} from '../disk.js'
import {lockTake} from '../locks.js'
import {useIncomingStore} from './incoming.js'
import {desktopExitHold} from '../desktop.js'
import {resolveFolder} from '../paths.js'
import {brandName} from '../brand.js'

/*
The live settings, and the reading and writing of ftorrent.toml around them. settings.js knows what a setting is; this store knows where the file is, what it last said, and when to write it. The rest of the app imports this store and reads settings.section.key, the way it reads any other store, and the one object is filled in at startup rather than replaced, so a component that grabbed it early is looking at the same thing as one that came later.

Two moments write the file. load, once at startup, writes it if what ftorrent would write differs from what it read: that's what creates a missing file, repairs a bad value, and adds a setting this ftorrent has that the last one didn't. And save, whenever a setting changes by a user's action, writes the whole file at once. Both hand the same text down to Rust as well, which holds it in memory and writes it once more when ftorrent exits; desktop.rs says why that moment needs Rust. Window geometry is the setting that leans on that: the window records its position here in memory as it moves and saves only when it's closed, so the exit write is what catches a user who quits from the tray instead.

A file that can't be read is never written. A missing file is ordinary and gets created; a file that is there and won't open, a lock or a permission, is left exactly alone, because the settings in it are the user's and writing factory values over them would be losing data to a lock. The same goes for a file that opens but won't parse, a hand edit with a typo in it: ftorrent runs on factory values and says so, and the file waits, untouched, for the user to fix it.
*/

export const useSettingsStore = defineStore('settings', () => {
	let settings = reactive(settingsFactory())//the live settings the rest of ftorrent reads, filled in by load and never replaced
	let paths = ref(null)//where everything is, as paths.rs worked it out; load takes it, and the page and the folder resolution below read it from here
	let problems = ref([])//what reading or writing the file had to say, for the page to show; empty when the file was fine
	let folderStates = ref({})//how each resolved download folder stands, by path, as folderLock last answered: held, busy, missing, or trouble with the reason after a colon
	let fileText = ''//what ftorrent last read from or wrote to the file, to tell when a write would change nothing
	let heldText = ''//what rust is holding to write at exit, to tell when handing it down again would change nothing
	let unreadable = false//there's no file to use, or it's there and won't open or won't parse, so nothing may be written

	let resolvedFolders = computed(() => {//each download folder setting as the place it names on this machine, in the order the file lists them
		let p = paths.value
		if (!p) return []
		return settings.downloads.folders.map(setting => ({setting, path: resolveFolder(setting, p.location, p.home)}))
	})

	const sessionFolder = `.${brandName}`//inside each download folder, where the session data of the torrents in it lives, and the lock that keeps two copies of ftorrent from using the folder at once
	const lockName = `${brandName}.lock`//inside that, empty, and never written; it exists to be locked
	let heldFolders = computed(() => resolvedFolders.value.map(folder => folder.path).filter(path => folderStates.value[path] == 'held'))//the folders this copy holds, in settings order, which are the only ones the engine is told about

	async function folderLock(path) {//lock one download folder, never making it: missing if it isn't on this machine; otherwise it gets its hidden .ftorrent, if it hasn't one yet, and the lock inside it. Answers held, busy, missing, or trouble with the reason after a colon
		let found
		try { found = await diskStat(path) } catch { return 'missing' }//a drive not plugged in, or a default folder nothing has needed yet
		if (!found.is_dir && !found.is_symlink) return 'trouble: this is a file, not a folder'
		let separator = path.includes('\\') ? '\\' : '/'//the resolved path already has the platform's own separators
		let session = path + separator + sessionFolder
		try {
			try { await diskStat(session) } catch { await diskMkdir(session) }//made the first time this folder is used, beside the downloads, holding one subfolder per torrent
			await diskHide(session)//on windows; the leading dot hides it elsewhere
		} catch (error) {
			return `trouble: could not make ${sessionFolder}, ${error}`
		}
		return await lockTake(session + separator + lockName)//held against every other copy of ftorrent for as long as this one runs; one folder under two spellings is held once
	}

	async function lockFolders() {//take the lock on every download folder that exists, and tell the engine which ones this copy holds; at startup, never making a folder
		for (let folder of resolvedFolders.value) folderStates.value[folder.path] = await folderLock(folder.path)
		await useIncomingStore().send({command: 'folders', folders: heldFolders.value})
	}

	async function prepareFolder(path) {//get one folder ready for a torrent: make it if it isn't there, lock it, and tell the engine; what starting a torrent will call, and the main page's button until then
		let state = await folderLock(path)
		if (state == 'missing') {
			try {
				await diskMkdir(path)
				state = await folderLock(path)
			} catch (error) {
				state = `trouble: could not make the folder, ${error}`
			}
		}
		folderStates.value[path] = state
		await useIncomingStore().send({command: 'folders', folders: heldFolders.value})
		return state
	}

	async function load(loadedPaths) {//read the settings file and leave it exactly as ftorrent would write it, unless it won't open or won't parse; call once, before anything reads a setting
		paths.value = loadedPaths//first, and always, so the page can show where everything is, and explain a copy with no data folder
		let path = loadedPaths.settings
		if (!path) { unreadable = true; return }//no data folder: no file to read, and none to write
		let text = ''
		try {
			text = new TextDecoder().decode(new Uint8Array(await diskRead(path)))
		} catch (error) {
			unreadable = !String(error).includes('os error 2')//both platforms number a missing file 2; anything else is a lock, a permission, or a disk saying no
			problems.value.push(unreadable ? `settings: leaving alone ${path}, because reading it said: ${error}` : `settings: first run, so writing ${path} with every setting at its factory value`)//a missing file is the ordinary first run, and says so plainly; anything else is a file that's there and won't open
		}
		fileText = text

		let {settings: found, problems: complaints, parsed} = settingsParse(text)
		for (let section of Object.keys(found)) for (let key of Object.keys(found[section])) settings[section][key] = found[section][key]//fill the live object rather than replacing it, so importers keep theirs
		for (let complaint of complaints) problems.value.push(`settings: ${complaint}`)
		if (!parsed) {//a hand edit with a typo in it, most likely, and one keystroke from right; writing factory values over it would lose everything else in the file
			unreadable = true
			problems.value.push(`settings: leaving ${path} exactly as it is, and running on factory settings until it's fixed`)
		}

		if (unreadable) return//the settings in there are the user's, still there, and not ours to write over
		let rendered = settingsRender(settings)
		if (rendered != fileText) await write(rendered)//the file is missing, or held a value ftorrent had to repair, or came from an ftorrent with fewer settings than this one
		else hold(rendered)//the file already says this; rust just needs the text to write again on the way out
	}

	async function save() {//write the settings as they are now; call after changing a value by a user's action, the note box's Save button for one
		if (!paths.value || unreadable) return//nothing to write to, or a file we've promised to leave alone
		let rendered = settingsRender(settings)
		if (rendered == fileText) return//nothing changed on disk terms, so nothing to write
		await write(rendered)
	}

	function remember() {//the settings as they are now, to rust for the exit write only, not to disk; window.js calls this as the window moves, so a quit from the tray still saves where it was
		if (!paths.value || unreadable) return
		let rendered = settingsRender(settings)
		if (rendered == heldText) return//rust already holds this, which is what a move event reporting the same place produces
		hold(rendered)
	}

	async function write(text) {//the file, whole, and the same text to rust for the exit write
		try {
			await diskWrite(paths.value.settings, Array.from(new TextEncoder().encode(text)))//disk.rs speaks bytes because it mirrors posix, so this is where text becomes bytes
			fileText = text
		} catch (error) {
			problems.value.push(`settings: writing ${paths.value.settings}: ${error}`)
		}
		hold(text)//after a write that worked this is belt and braces; after one that didn't, it leaves the file with rust to try again on the way out
	}

	function hold(text) {//hand the whole file down to rust, which writes it when ftorrent exits
		heldText = text
		desktopExitHold(paths.value.settings, text).catch(error => problems.value.push(`settings: handing the file down to rust: ${error}`))
	}

	return {settings, paths, problems, folderStates, resolvedFolders, heldFolders, load, save, remember, lockFolders, prepareFolder}
})
