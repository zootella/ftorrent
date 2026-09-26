import {ref} from 'vue'
import {diskStat} from './disk.js'
import {registryGet, registrySet, registryNotify} from './registry.js'
import {brandName, brandDescription} from './brand.js'

/*
What ftorrent has told the operating system it can open: two kinds of file, .torrent and .ftorrent, and two kinds of link, magnet: and ftorrent:. The page runs this once at startup, after reading the settings, and it does something only on Windows, and only for a copy running from the folder the installer puts it in; everywhere else it does nothing, and says nothing. Only the copy holding the lock has a page, so ten launches at once make one set of registry writes, not ten racing each other. macOS needs no code, because its declaration is not code: the document types and URL schemes sit in Info.plist inside the .app, Launch Services reads them when it first sees the bundle, and dragging ftorrent.app into Applications is the whole registration. Linux gets its MimeType lines in the .desktop file the packages install. Windows has no such file for an installer app, so an application registers itself, and this is how: the policy here, in plain JavaScript, on three general commands in registry.rs that read a value, write one only if it would change, and tell the shell, and know nothing about what they're reading or writing.

Two things have to be said clearly about the how, because the subject is thick with folklore. The first is what ftorrent writes, all under HKEY_CURRENT_USER and nothing under the machine: a ProgID per kind of file naming the type, its icon, and the command that opens it; that ProgID added to the extension's OpenWithProgids list, which is the offer; the executable's own key with the extensions it supports; a ProgID per URL scheme, marked as a protocol, with its icon and command; and a Capabilities block registered so the Settings app lists ftorrent by name with its types and links, each pointing at one of ftorrent's own ProgIDs. The second is what ftorrent does not write: the extension's own default value, the single line that says .torrent means ftorrent from now on. That line is the one an installer from 1999 would write, it is the one Tauri's bundled NSIS macro still writes, and it is the one Microsoft's own current API for unpackaged apps deliberately does not. Since Windows 8 the default for a file type lives in a hash-sealed UserChoice key that only the user, through the system's own interface, can set. So ftorrent offers and never takes: after this runs, it is in Explorer's Open with menu and listed in Settings under Default apps, and every .torrent on the machine still opens with whatever opened it before. Which is why bundle.fileAssociations is absent from tauri.conf.json and must stay absent: on Windows it inserts the NSIS macro that seizes each type's default at install time, silently.

Links take one step further, and it's where ftorrent is assertive without taking anything. A scheme's default lives in its own sealed UserChoice key too, but when no user has chosen, Windows falls back to the class key named for the scheme, magnet itself, which is shared: any program can write it, and many clients register nothing else. So ftorrent writes that class only while it runs no command or already runs ftorrent's. On a machine with no torrent client, a clicked magnet arrives at ftorrent with no user step; where another program holds the class, ftorrent leaves it alone, says so on the page, and waits to be chosen. A choice of ftorrent names ftorrent's own ProgID, never the shared class, so it holds however often another client rewrites magnet; and a choice of a client that registered only the shared class names magnet itself, which ftorrent then never touches.

The icon a .torrent wears is a file rather than the application: torrent.ico ships beside the executable through bundle.resources, and DefaultIcon names it, with ftorrent's own icon as the fallback if the file is not there. An application icon is meant to be unmistakable in a taskbar, which is exactly the wrong property on a document. The icon studio in the desktop workspace is where it's drawn. The ProgIDs are per type, so a different icon per format costs nothing later.

Two practical notes. It runs on every launch, which is cheap because each value is read before it is written and an unchanged value is not touched; the shell is only notified if something actually moved. And it has one gate, which is the whole of how it tells an installed copy from anything else: the program has to be in the folder the per-user installer puts it in, %LOCALAPPDATA%\ftorrent, which paths.rs reports as a fact beside the program's own location. The command paths come from the executable's own location, so registering a copy anywhere else would point the registry at a file that may move or vanish: a portable copy on a stick that gets ejected, a debug or release build in the repository's target folder that gets rebuilt, a copy on the Desktop. None of them registers, and none needs to say why.
*/

//what ftorrent can open, and the names those things carry in Explorer's Type column and in the Settings app
const fileTypes = [//the extension with its dot, the ProgID, and the name a user reads
	{extension: '.torrent',       program: `${brandName}.torrent`,    name: 'Torrent File'},//what µTorrent and qBittorrent call it too; Transmission's "BitTorrent Metadata File" is a developer's phrase
	{extension: `.${brandName}`,  program: `${brandName}.${brandName}`, name: `${brandName} File`},//reserved early, for the metadata and capabilities ftorrent will add beyond the established standards
]
const linkSchemes = [//the scheme, ftorrent's own ProgID for it, and the name a user reads; the class key named for the scheme itself is written too, so no ProgID may be named bare magnet or ftorrent
	{scheme: 'magnet',  program: `${brandName}.url.magnet`,     name: 'URL:Magnet Link'},
	{scheme: brandName, program: `${brandName}.url.${brandName}`, name: `URL:${brandName} Link`},
]
const applicationName = brandName
const applicationDescription = brandDescription
const documentIcon = 'torrent.ico'//beside the executable, put there by bundle.resources

export const associations = ref('')//what registration did this launch, one line for the main page, blank where there was nothing to do

export async function associate(paths) {//tell windows what an installed copy can open, taking nothing another program holds; call once at startup, after the settings are read
	if (!paths.installer || paths.location.toLowerCase() != paths.installer.toLowerCase()) return//not where the installer puts ftorrent, so not an installed copy, whatever else it is, and on macOS and linux blank; skipped without a word, since the reason doesn't matter
	let executable = paths.executable
	let file = executable.split('\\').pop()//ftorrent.exe, which is the key windows expects under Applications
	let command = `"${executable}" "%1"`//quoted, because a file's path will contain spaces; %1 is where windows puts the file or the link
	let applicationIcon = `${executable},0`//the executable's own icon, its first
	let beside = `${paths.location}\\${documentIcon}`
	let fileIcon = applicationIcon//still better than none
	try { await diskStat(beside); fileIcon = `${beside},0` } catch {}//neither is quoted, which is safe because windows reads an icon location by splitting at the last comma rather than at a space

	let application = `Software\\Classes\\Applications\\${file}`
	let capabilities = `Software\\${applicationName}\\Capabilities`
	let changed = 0
	let set = async (key, name, value) => { if (await registrySet(key, name, value)) changed++ }//each value read first and written only if it would change, so a launch that changed nothing knows it
	try {
		for (let {extension, program, name} of fileTypes) {
			await set(`Software\\Classes\\${program}`, '', name)//the ProgID: what this kind of file is called
			await set(`Software\\Classes\\${program}\\DefaultIcon`, '', fileIcon)//what explorer draws on one
			await set(`Software\\Classes\\${program}\\shell\\open\\command`, '', command)//and what opens it
			await set(`Software\\Classes\\${extension}\\OpenWithProgids`, program, '')//ftorrent joins the list of what could open this extension, which is the offer; the value is empty and only the name matters
			await set(`${application}\\SupportedTypes`, extension, '')//so ftorrent is offered for these and not for everything else
			await set(`${capabilities}\\FileAssociations`, extension, program)//and so the settings app can list ftorrent's types
		}
		let left = []//schemes whose shared class another program holds, named on the page
		for (let {scheme, program, name} of linkSchemes) {
			let classes = [`Software\\Classes\\${program}`]//ftorrent's own ProgID, which no other program writes, so a user's choice that names it stays ftorrent's
			if (await claimable(scheme, command)) classes.push(`Software\\Classes\\${scheme}`); else left.push(scheme)//and the shared class named for the scheme, which windows falls back to when no user has chosen, but only while it's empty or already ours
			for (let key of classes) {//the same four values under each
				await set(key, '', name)
				await set(key, 'URL Protocol', '')//the empty value that marks a class as a url scheme rather than a file type
				await set(`${key}\\DefaultIcon`, '', applicationIcon)
				await set(`${key}\\shell\\open\\command`, '', command)
			}
			await set(`${capabilities}\\URLAssociations`, scheme, program)//so the settings app lists ftorrent for this kind of link, and a choice there names ftorrent's own ProgID rather than the shared class
		}
		await set(application, 'FriendlyAppName', applicationName)
		await set(`${application}\\shell\\open\\command`, '', command)
		await set(capabilities, 'ApplicationName', applicationName)
		await set(capabilities, 'ApplicationDescription', applicationDescription)
		await set('Software\\RegisteredApplications', applicationName, capabilities)//the line that puts ftorrent in the settings app by name, and last on purpose: any write above can fail and stop the whole run, so publishing ftorrent to Settings is the step that only happens once everything it points at is there. The next launch starts again from the top and finishes the job

		if (changed > 0) await registryNotify()//only when something moved, because this runs on every launch and almost always writes nothing
		let note = left.length > 0 ? `; ${left.join(' and ')} left to the program that has it` : ''
		associations.value = `associations: ${fileTypes.length} file types and ${linkSchemes.length} link types registered, ${changed} values written${note}`
	} catch (error) {
		associations.value = `associations: ${error}`//trouble reads the same way a result does, as a line on the page
	}
}

async function claimable(scheme, command) {//whether the class named for this scheme is ftorrent's to write: it runs no command yet, or already runs ftorrent's. Read through HKEY_CLASSES_ROOT, the view windows itself uses, which lays the user's classes over the machine's, so a client installed for everyone counts as holding it too
	try {
		let found = await registryGet('classes', `${scheme}\\shell\\open\\command`, '')//the key's default value, which is the command
		return !found || found == command//no class, a class with no command, or one that's already ftorrent's
	} catch {
		return false//too long or unreadable, so not ftorrent's
	}
}
