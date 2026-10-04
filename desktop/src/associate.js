import {diskStat} from './disk.js'
import {registryGet, registrySet, registryDelete, registryDeleteKey, registryNotify, registryOpens} from './registry.js'
import {launchOpens, launchClaim} from './launch.js'
import {brandName, brandStem, brandDescription} from './brand.js'
import {platformName} from './settings.js'
import {log} from './log.js'

/*
ftorrent handles its file types and links on Windows, the .torrent file and the magnet link above all, to be simple, modern, polite, and assertive, and to keep the user in control, which point for point differs from how most Windows programs have done it for twenty-five years. Windows keeps what opens each type in the registry at two levels. The lower is the legacy registry association, the only one before Windows 8: a value any program may write, so a program that wanted its files wrote it at install and checked it at every launch, and rival programs have fought over it ever since. Windows 8 put the sealed registry association above it, which holds the user's pick with a hash only Windows' own screens compute, so only the user can set it, and a program that plays by Windows' rules can't forge it; where both are present and disagree, the sealed one wins. Much association code in the wild is still the old habit of writing the legacy one, carried past the system it was built for.

Simple and modern are how it's built. Simple means one plain pass, the same writes in the same order every time, on general registry commands that know nothing about torrents, with one setting deciding which of them happen; where association code tends to sprawl into strategies as each release of Windows changes the rules, this never checks the Windows version, never tries one method with another behind it, and never computes the sealed one's hash. Modern means the program registers itself, per user, in the shape Microsoft's own current API for unpackaged apps writes, rather than an installer script writing defaults, often for the whole machine; so the installer does nothing about associations, and the way to the default runs through Windows' Settings, where Microsoft says it belongs.

Polite, assertive, and the user in control are how it behaves. Polite means ftorrent offers itself as one more program that can open each of its four types and, until the user says yes, leaves whatever opens them now exactly as it is. Assertive means the offer is renewed every time, so anything missing comes back; a yes claims all four, whoever had them; and a sealed association the user makes for it in Windows names ftorrent's own ProgID, which only ftorrent writes, so nothing quietly undoes it. But ftorrent claims only on the user's yes, at that moment or, for a yes given again on Windows, on the user's return from the Settings it opens. Afterward it only reads, and when another program holds a type again, which may be the user's own doing, the bar asks again; taking a type back whenever the window is clicked is the 1990s habit that leaves a user fighting their own software. The answer covers all four alike, on purpose and for simplicity: one stance for the group, recorded once, and the same code for every type in it. Two of them are almost always contested, since someone getting ftorrent most likely has another BitTorrent client already, and two almost certainly aren't, since .ftorrent files and ftorrent: links won't be out in the world for years; treating them alike means almost nothing changes on the day they are. The one place they part is on Windows, in the order a yes given again writes them, ftorrent's own two at once and the contested two after a trip to Settings, which stores/associations.js tells; the own flag on those two types below is the one thing to change on that day. The bar and the Settings page name only .torrent and magnet:, the two a user knows, while the answer they give covers all four. The user in control means installing or running ftorrent is never taken as permission. ftorrent asks, in a bar across the top of its window whenever there's something to ask, and keeps the answer in one setting, associations.default, "ask" until the user answers and changeable any time on the Settings page. The system has the last word, so ftorrent reads it and follows: a system that already opens all four with ftorrent leaves nothing to ask, and a yes that Windows doesn't share opens its Settings for the user to finish, as does a yes given again after another program has taken a type back, which goes there first, so the sealed association the user makes there ends the duel, and writes the legacy ones for the contested two only when the user comes back. Polite and assertive pull against each other, and the design is in holding both: ftorrent takes nothing that belongs to another program or to the user, and lets go of nothing that belongs to it.

The mechanics, in brief. ftorrent opens two kinds of file, .torrent and .ftorrent, and two kinds of link, magnet: and ftorrent:. On macOS the .app declares them in its Info.plist, which Launch Services reads when it first sees the bundle, so dragging it to Applications is the whole registration, and on Linux the .desktop file the packages install carries MimeType lines. A Windows app an installer places has no such file, so it registers itself, all of it under HKEY_CURRENT_USER, and only a copy running from the installer's folder does. Beneath the two levels is a third thing, ftorrent's offer, which decides nothing and takes nothing from anyone: ftorrent's own ProgIDs, its place in each extension's Open with list, its own key under Applications, and a Capabilities block that lists it by name in Settings; ftorrent writes it always. The legacy registry association is the extension's own default value under Software\Classes for a file, naming a ProgID, and for a link the class key named for the scheme itself, holding the command; ftorrent writes it for all four when the user says yes, and gives it back when the user says no, only while it still names ftorrent. The sealed registry association is the UserChoice key under Explorer's FileExts for a file and under Shell\Associations\UrlAssociations for a link, a ProgID and its hash; Windows falls back to the legacy one only where there's no sealed one, or where the sealed one names a shared class like magnet itself. ftorrent never writes the sealed one and only reads where things stand, by asking the shell what it would open each type with, the same lookup Explorer and Settings make. This file is the policy, and stores/associations.js runs it: the offer and a read at startup and whenever the window comes back into focus, as when the user returns from Windows' Settings, and the claim or the giving back only when the user answers, or for a yes given again, on the return from Settings.

On the Mac the same stance takes fewer moves. The offer is the Info.plist in src-tauri/macos, which never changes with the answer, so ftorrent stays in Finder's Open With menu whatever the user says. The claim is one call per type through launch.rs, handed this copy's path. macOS records ftorrent's bundle identifier rather than the path, and when it knows several copies, the one in /Applications beside a build in the repository or a mounted disk image, it picks which one opens; with copies of the same version, we measured it picking the one in /Applications. macOS makes the change without asking the user anything, for files and links alike, which makes the user's yes in ftorrent the whole of the consent, and the reason ftorrent asks at all. A yes claims each of the four that macOS opens with anything else, once, at that moment. Here the rule that ftorrent never claims on its own matters most, since macOS records a choice the user makes through another app's button exactly as it records that app's own claim, and nothing tells the two apart. And since a default on the Mac always names some app, and nothing unsets one, ask and no write nothing and give nothing back. A no means the same on every platform, that ftorrent stops reaching and gives back what the system lets it give back cleanly; on Windows that's each legacy association still naming ftorrent, and on the Mac it's nothing, since giving a type up there would mean choosing its next app, which is the user's to choose. So a type still opening with ftorrent after a no stays that way, and the user can change it in Finder's Get Info for a file, or from another app for a link. Only a copy in /Applications claims anything; a download macOS runs from a random folder until it's moved, or a build in the repository, never does.

Two things elsewhere have to stay in step with this. bundle.fileAssociations stays absent from tauri.conf.json, since Tauri's bundlers would register each type at install, silently. And win-setup/registry.js, from whose list of extensions and schemes the installer's uninstall.exe builds the list it walks, removes everything this writes, each legacy association only while it still names ftorrent, so a type added here is added to the list there, and a registration of a new kind needs its removal added to the rules beneath it.
*/

//what ftorrent can open, and the names those things carry in Explorer's Type column and in the Settings app; ftorrent offers itself for all four always, and claims all four only on the user's yes
const fileTypes = [//the extension with its dot, the ProgID, and the name a user reads; an extension is brandStem, a ProgID begins with brandName, the way Windows' Settings shows it
	{extension: '.torrent',       program: `${brandName}.torrent`,      name: 'Torrent File'},//what µTorrent and qBittorrent call it too; Transmission's "BitTorrent Metadata File" is a developer's phrase
	{extension: `.${brandStem}`,  program: `${brandName}.${brandStem}`, name: `${brandName} File`, own: true},//reserved early, for the metadata and capabilities ftorrent will add beyond the established standards
]
const linkSchemes = [//the scheme, ftorrent's own ProgID for it, and the name a user reads; the class key named for the scheme itself is written too, so no ProgID may be named bare magnet or ftorrent
	{scheme: 'magnet',  program: `${brandName}.url.magnet`,       name: 'URL:Magnet Link'},
	{scheme: brandStem, program: `${brandName}.url.${brandStem}`, name: `URL:${brandName} Link`, own: true},
]//own marks the two no other program wants yet, which a yes given again on windows claims at once rather than after a trip to Settings; the essay says why, and it's the flag to take off the day another client reaches for them
export const typeNames = [...fileTypes, ...linkSchemes].map(type => type.extension ?? type.scheme)//all four by name, .torrent, .ftorrent, magnet, and ftorrent, which one answer covers and the bar's rule checks; the words name only the first and third, the two a user knows, on purpose, as the essay says
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

export async function register(paths, answering) {//tell the system what this installed copy can open, and when the user has just answered, claim all four for a yes or give back the legacy associations for a no; answering is that answer, own to claim only ftorrent's own two, or blank on a pass that only renews the offer. Answers how many values changed. Only the copy holding the lock has a page to call this, so ten launches at once make one set of writes, not ten racing each other
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
	let claims = type => answering == 'yes' || (answering == 'own' && type.own)//whether this pass writes the type's legacy association

	for (let type of fileTypes) {
		let {extension, program, name} = type
		await set(`Software\\Classes\\${program}`, '', name)//the ProgID: what this kind of file is called
		await set(`Software\\Classes\\${program}\\DefaultIcon`, '', fileIcon)//what explorer draws on one
		await set(`Software\\Classes\\${program}\\shell\\open\\command`, '', command)//and what opens it
		await set(`Software\\Classes\\${extension}\\OpenWithProgids`, program, '')//ftorrent joins the list of what could open this extension, which is the offer; the value is empty and only the name matters
		await set(`${application}\\SupportedTypes`, extension, '')//so ftorrent is offered for these and not for everything else
		await set(`${capabilities}\\FileAssociations`, extension, program)//and so the settings app can list ftorrent's types
		let legacy = `Software\\Classes\\${extension}`//the extension's own key, whose default value is the legacy association, the ProgID windows uses where no sealed one says otherwise. That value is the single line that says .torrent means ftorrent from now on, which an installer from 1999 writes at install, Tauri's NSIS macro still writes, and Microsoft's own current API for unpackaged apps deliberately doesn't; here it waits for the user's yes
		if (claims(type)) await set(legacy, '', program)//claimed, whoever wrote it last
		else if (answering == 'no' && await holds(legacy, program)) await unset(legacy, '')//given back, but only while it still names ftorrent
	}
	for (let type of linkSchemes) {
		let {scheme, program, name} = type
		let classes = [`Software\\Classes\\${program}`]//ftorrent's own ProgID, which no other program writes, so a sealed association that names it stays ftorrent's
		let shared = `Software\\Classes\\${scheme}`//the class named for the scheme, the legacy association for a link, which windows uses where no sealed one says otherwise; any program may write it, and many clients register nothing else, so a sealed association can name it too and then opens whichever program wrote it last
		if (claims(type)) classes.push(shared)//claimed with the same four values as ftorrent's own, whoever wrote it last
		else if (answering == 'no' && await holds(`${shared}\\shell\\open\\command`, command)) await unsetKey(shared)//given back whole, but only while it still runs ftorrent; what another program had there before the yes is gone, so the class waits empty for whichever program writes it next
		for (let key of classes) {//the same four values under each
			await set(key, '', name)
			await set(key, 'URL Protocol', '')//the empty value that marks a class as a url scheme rather than a file type
			await set(`${key}\\DefaultIcon`, '', applicationIcon)
			await set(`${key}\\shell\\open\\command`, '', command)
		}
		await set(`${capabilities}\\URLAssociations`, scheme, program)//so the settings app lists ftorrent for this kind of link, and a sealed association made there names ftorrent's own ProgID rather than the shared class, so it holds however often another client rewrites the class
	}
	await set(application, 'FriendlyAppName', applicationName)
	await set(`${application}\\shell\\open\\command`, '', command)
	await set(capabilities, 'ApplicationName', applicationName)
	await set(capabilities, 'ApplicationDescription', applicationDescription)
	await set('Software\\RegisteredApplications', applicationName, capabilities)//the line that puts ftorrent in the settings app by name, and last on purpose: any write above can fail and stop the whole pass, so publishing ftorrent to Settings is the step that only happens once everything it points at is there. The next pass starts again from the top and finishes the job

	if (changed > 0) await registryNotify()//only when something moved, because this runs often and almost always changes nothing
	return changed
}

export async function whoOpens() {//what the system would open each of the four with right now, as found, by name, as {program, executable}, or null when nothing would, beside problems, a line for each type the system couldn't answer for. On windows that's registry_opens, the ProgID and the path it runs, the sealed association first and the legacy one after, the answer windows' own Settings shows; on the mac, launch services' answer
	let found = {}
	let problems = []
	for (let name of typeNames) {
		try {
			if (platformName != 'macOS') { found[name] = await registryOpens(name); continue }
			let app = await launchOpens(name)//on the mac, the path of the .app, which stands in for both what windows calls the program and the executable it runs
			found[name] = app ? {program: app.split('/').pop(), executable: app} : null
		} catch (error) {//a lookup the system couldn't answer counts as nothing opening that type, so one odd answer leaves the other three read and the bar still decided on all four; problems says so, so the report doesn't take it for nothing
			found[name] = null
			problems.push(`could not ask what opens ${name}, ${error}`)
			log(`associations: could not ask what opens ${name}, ${error}`)
		}
	}
	return {found, problems}
}
