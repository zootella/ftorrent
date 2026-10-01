import {diskStat} from './disk.js'
import {registryGet, registrySet, registryDelete, registryDeleteKey, registryNotify, registryOpens} from './registry.js'
import {launchOpens, launchClaim} from './launch.js'
import {brandName, brandDescription} from './brand.js'
import {platformName} from './settings.js'
import {log} from './log.js'

/*
ftorrent handles its file types and links on Windows, the .torrent file and the magnet link above all, to be simple, modern, polite, and assertive, and to keep the user in control, which point for point differs from how most Windows programs have done it for twenty-five years. The usual way made sense when it began: before Windows 8 the default for a file type was a registry value any program could write, so a program that wanted its files wrote it at install and checked it at every launch. Windows 8 sealed the default with a hash that only the user can set, through the system's own screens, and much association code in the wild is that old habit, carried past the system it was built for.

Simple and modern are how it's built. Simple means one plain pass, the same writes in the same order every time, on general registry commands that know nothing about torrents, with one setting deciding which of them happen; where association code tends to sprawl into strategies as each release of Windows changes the rules, this never checks the Windows version, never tries one method with another behind it, and never computes the hash. Modern means the program registers itself, per user, in the shape Microsoft's own current API for unpackaged apps writes, rather than an installer script writing defaults, often for the whole machine; so the installer does nothing about associations, and the way to the default runs through Windows' Settings, where Microsoft says it belongs.

Polite, assertive, and the user in control are how it behaves. Polite means ftorrent offers itself as one more program that can open each of its four types and, until the user says yes, leaves whatever opens them now exactly as it is. Assertive means the offer is renewed every time, so anything missing comes back; a yes claims all four, whoever had them; and a choice the user saves for it in Windows names something only ftorrent writes, so nothing quietly undoes it. But ftorrent claims only at the moment the user says yes. Afterward it only reads, and when another program holds a type again, which may be the user's own doing, the bar asks again; taking a type back whenever the window is clicked is the 1990s habit that leaves a user fighting their own software. The answer covers all four alike: .ftorrent and ftorrent: are ftorrent's own and nobody else wants them today, but nothing here counts on that staying true. The user in control means installing or running ftorrent is never taken as permission. ftorrent asks, in a bar across the top of its window whenever there's something to ask, and keeps the answer in one setting, associations.default, "ask" until the user answers and changeable any time on the Settings page. The system has the last word, so ftorrent reads it and follows: a system that already opens all four with ftorrent leaves nothing to ask, and a yes that Windows doesn't share opens its Settings for the user to finish. Polite and assertive pull against each other, and the design is in holding both: ftorrent takes nothing that belongs to another program or to the user's choice, and lets go of nothing that belongs to it.

The mechanics, in brief. ftorrent opens two kinds of file, .torrent and .ftorrent, and two kinds of link, magnet: and ftorrent:. On macOS the .app declares them in its Info.plist, which Launch Services reads when it first sees the bundle, so dragging it to Applications is the whole registration, and on Linux the .desktop file the packages install carries MimeType lines. A Windows app an installer places has no such file, so it registers itself, all of it under HKEY_CURRENT_USER, and only a copy running from the installer's folder does. Three layers decide what opens a type. The offer, written always: ftorrent's own ProgIDs, its place in each extension's Open with list, its own key under Applications, and a Capabilities block that lists it by name in Settings. The fallback, which Windows uses where the user has saved no choice or has chosen a shared class: the extension's own default value for a file, and for a link the class named for the scheme, which any program may write; ftorrent writes it for all four when the user says yes, and gives it back when the user says no, only while it still names ftorrent. And above both the user's saved choice, sealed, which ftorrent never writes and only reads, by asking the shell what it would open each type with, the same lookup Explorer and Settings make. This file is the policy, and stores/associations.js runs it: the offer and a read at startup and whenever the window comes back into focus, as when the user returns from Windows' Settings, and the claim or the giving back only when the user answers.

On the Mac the same stance takes fewer moves. The offer is the Info.plist in src-tauri/macos, which never changes with the answer, so ftorrent stays in Finder's Open With menu whatever the user says. The claim is one call per type through launch.rs, naming this copy by its path, so the one in /Applications is the one claimed and never a stray copy Launch Services also knows about; macOS makes the change without asking the user anything, for files and links alike, which makes the user's yes in ftorrent the whole of the consent, and the reason ftorrent asks at all. A yes claims each of the four that macOS opens with anything else, once, at that moment. Here the rule that ftorrent never claims on its own matters most, since macOS records a choice the user makes through another app's button exactly as it records that app's own claim, and nothing tells the two apart. And since a default on the Mac always names some app, and nothing unsets one, ask and no write nothing and give nothing back: a type still opening with ftorrent after a no stays that way, since no means ftorrent stops trying, not that it undoes anything, and the user can change it in Finder's Get Info for a file, or from another app for a link. Only a copy in /Applications claims anything; a download macOS runs from a random folder until it's moved, or a build in the repository, never does.

Two things elsewhere have to stay in step with this. bundle.fileAssociations stays absent from tauri.conf.json, since on Windows it adds the NSIS macro that seizes each type's default at install, silently. And the uninstall hook in src-tauri/windows/hooks.nsh removes everything this writes, each fallback only while it still names ftorrent, so a registration added here needs its removal added there.
*/

//what ftorrent can open, and the names those things carry in Explorer's Type column and in the Settings app; ftorrent offers itself for all four always, and claims all four only on the user's yes
const fileTypes = [//the extension with its dot, the ProgID, and the name a user reads
	{extension: '.torrent',       program: `${brandName}.torrent`,      name: 'Torrent File'},//what µTorrent and qBittorrent call it too; Transmission's "BitTorrent Metadata File" is a developer's phrase
	{extension: `.${brandName}`,  program: `${brandName}.${brandName}`, name: `${brandName} File`},//reserved early, for the metadata and capabilities ftorrent will add beyond the established standards
]
const linkSchemes = [//the scheme, ftorrent's own ProgID for it, and the name a user reads; the class key named for the scheme itself is written too, so no ProgID may be named bare magnet or ftorrent
	{scheme: 'magnet',  program: `${brandName}.url.magnet`,       name: 'URL:Magnet Link'},
	{scheme: brandName, program: `${brandName}.url.${brandName}`, name: `URL:${brandName} Link`},
]
export const typeNames = [...fileTypes, ...linkSchemes].map(type => type.extension ?? type.scheme)//all four by name, .torrent, .ftorrent, magnet, and ftorrent, which the setting and the banner are about
export const applicationName = brandName//the name under RegisteredApplications, which is also the name Windows 11's Settings link takes to open ftorrent's own page
const applicationDescription = brandDescription
const documentIcon = 'torrent.ico'//what a .torrent wears: a file of its own, beside the executable where bundle.resources puts it, rather than the application's icon, which is made to stand out in a taskbar, the wrong thing for a document to do. The icon studio in the desktop workspace draws it, and with a ProgID per type another format can have another icon later, at no cost

export function installedCopy(paths) {//whether this copy runs from the folder an install puts ftorrent in, %LOCALAPPDATA%\ftorrent on windows and /Applications on the mac, which paths.rs reports beside the program's own location; the one gate on everything here. What gets registered or claimed names this copy's path, so a copy anywhere else would point the system at a file that may move or vanish, a portable copy on a stick, a build in the repository's target folder, a copy on the Desktop, and none of them registers or needs to say why. The installer's folder is blank on linux, so never there
	return !!paths?.installer && paths.location.toLowerCase() == paths.installer.toLowerCase()
}

export function thisCopy(paths) {//what the system names when this copy is what opens a type: on windows the executable, and on the mac the .app around it, which is how launch services knows an app
	let bundle = paths.executable.lastIndexOf('.app/')
	return platformName == 'macOS' && bundle >= 0 ? paths.executable.slice(0, bundle + '.app'.length) : paths.executable
}

async function claimOnMac(paths, answering) {//the mac's half: when the user has just said yes, make this copy the default for each of the four that macOS opens with anything else; otherwise nothing, since the offer is the Info.plist and there's no default to give back. Answers how many it claimed
	if (answering != 'yes') return 0
	let app = thisCopy(paths)
	let changed = 0
	for (let name of typeNames) {
		let was = await launchOpens(name)
		if (was == app) continue//already this copy's, so nothing to do
		await launchClaim(name, app)
		log(`associations: claimed ${name} from ${was || 'nothing'} for ${app}`)
		changed++
	}
	return changed
}

export async function register(paths, answering) {//tell the system what this installed copy can open, and when the user has just answered, claim all four for a yes or give back the fallbacks for a no; answering is that answer, or blank on a pass that only renews the offer. Answers how many values changed. Only the copy holding the lock has a page to call this, so ten launches at once make one set of writes, not ten racing each other
	if (platformName == 'macOS') return claimOnMac(paths, answering)//the mac has its own, much shorter pass, above
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
	let set    = async (key, name, value) => { if (await registrySet(key, name, value)) { changed++; log(`associations: wrote ${key} ${name || '(default)'} = ${value}`) } }//each value read first and written only if it would change, so a pass that changed nothing knows it, and the log names every value that moved
	let unset  = async (key, name)        => { if (await registryDelete(key, name))    { changed++; log(`associations: removed ${key} ${name || '(default)'}`) } }//and deleting something already gone isn't a change either
	let unsetKey = async key              => { if (await registryDeleteKey(key))       { changed++; log(`associations: removed ${key}`) } }
	let holds  = async (key, value) => { try { return await registryGet('user', key, '') == value } catch { return false } }//whether a key's default value still says what ftorrent wrote there; one too long or of another type isn't ftorrent's, and mustn't stop the pass

	for (let type of fileTypes) {
		let {extension, program, name} = type
		await set(`Software\\Classes\\${program}`, '', name)//the ProgID: what this kind of file is called
		await set(`Software\\Classes\\${program}\\DefaultIcon`, '', fileIcon)//what explorer draws on one
		await set(`Software\\Classes\\${program}\\shell\\open\\command`, '', command)//and what opens it
		await set(`Software\\Classes\\${extension}\\OpenWithProgids`, program, '')//ftorrent joins the list of what could open this extension, which is the offer; the value is empty and only the name matters
		await set(`${application}\\SupportedTypes`, extension, '')//so ftorrent is offered for these and not for everything else
		await set(`${capabilities}\\FileAssociations`, extension, program)//and so the settings app can list ftorrent's types
		let fallback = `Software\\Classes\\${extension}`//the extension's own key, whose default value names the ProgID windows uses when the user has saved no choice. That value is the single line that says .torrent means ftorrent from now on, which an installer from 1999 writes at install, Tauri's NSIS macro still writes, and Microsoft's own current API for unpackaged apps deliberately doesn't; here it waits for the user's yes
		if (answering == 'yes') await set(fallback, '', program)//claimed, whoever wrote it last
		else if (answering == 'no' && await holds(fallback, program)) await unset(fallback, '')//given back, but only while it still names ftorrent
	}
	for (let type of linkSchemes) {
		let {scheme, program, name} = type
		let classes = [`Software\\Classes\\${program}`]//ftorrent's own ProgID, which no other program writes, so a user's choice that names it stays ftorrent's
		let shared = `Software\\Classes\\${scheme}`//the class named for the scheme, which windows falls back to when no user has chosen; any program may write it, and many clients register nothing else, so a saved choice can name it too and then opens whichever program wrote it last
		if (answering == 'yes') classes.push(shared)//claimed with the same four values as ftorrent's own, whoever wrote it last
		else if (answering == 'no' && await holds(`${shared}\\shell\\open\\command`, command)) await unsetKey(shared)//given back whole, but only while it still runs ftorrent; what another program had there before the yes is gone, so the class waits empty for whichever program writes it next
		for (let key of classes) {//the same four values under each
			await set(key, '', name)
			await set(key, 'URL Protocol', '')//the empty value that marks a class as a url scheme rather than a file type
			await set(`${key}\\DefaultIcon`, '', applicationIcon)
			await set(`${key}\\shell\\open\\command`, '', command)
		}
		await set(`${capabilities}\\URLAssociations`, scheme, program)//so the settings app lists ftorrent for this kind of link, and a choice there names ftorrent's own ProgID rather than the shared class, which holds however often another client rewrites it
	}
	await set(application, 'FriendlyAppName', applicationName)
	await set(`${application}\\shell\\open\\command`, '', command)
	await set(capabilities, 'ApplicationName', applicationName)
	await set(capabilities, 'ApplicationDescription', applicationDescription)
	await set('Software\\RegisteredApplications', applicationName, capabilities)//the line that puts ftorrent in the settings app by name, and last on purpose: any write above can fail and stop the whole pass, so publishing ftorrent to Settings is the step that only happens once everything it points at is there. The next pass starts again from the top and finishes the job

	if (changed > 0) await registryNotify()//only when something moved, because this runs often and almost always changes nothing
	return changed
}

export async function whoOpens() {//what the system would open each of the four with right now, by name, as {program, executable}, or null when nothing would. On windows that's registry_opens, the ProgID and the path it runs, the user's saved choice first and the fallbacks after, the answer windows' own Settings shows; on the mac, launch services' answer
	let found = {}
	for (let name of typeNames) {
		if (platformName != 'macOS') { found[name] = await registryOpens(name); continue }
		let app = await launchOpens(name)//on the mac, the path of the .app, which stands in for both what windows calls the program and the executable it runs
		found[name] = app ? {program: app.split('/').pop(), executable: app} : null
	}
	return found
}
