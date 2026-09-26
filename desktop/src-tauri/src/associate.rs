//./src-tauri/src/associate.rs

use std::sync::Mutex;
use tauri::{command, AppHandle, Manager, State};

/*
What ftorrent has told the operating system it can open: two kinds of file, .torrent and .ftorrent, and two kinds of link, magnet: and ftorrent:. This runs once at startup, after the lock is won, and does something only on Windows, and only for a copy running from the folder the installer puts it in; everywhere else it does nothing, and says nothing. macOS needs no code, because its declaration is not code: the document types and URL schemes sit in Info.plist inside the .app, Launch Services reads them when it first sees the bundle, and dragging ftorrent.app into Applications is the whole registration. Linux gets its MimeType lines in the .desktop file the packages install. Windows has no such file for an installer app, so an application registers itself, and this is how.

Two things have to be said clearly about the how, because the subject is thick with folklore. The first is what ftorrent writes, all under HKEY_CURRENT_USER and nothing under the machine: a ProgID per kind of file naming the type, its icon, and the command that opens it; that ProgID added to the extension's OpenWithProgids list, which is the offer; the executable's own key with the extensions it supports; a ProgID per URL scheme, marked as a protocol, with its icon and command; and a Capabilities block registered so the Settings app lists ftorrent by name with its types and links, each pointing at one of ftorrent's own ProgIDs. The second is what ftorrent does not write: the extension's own default value, the single line that says .torrent means ftorrent from now on. That line is the one an installer from 1999 would write, it is the one Tauri's bundled NSIS macro still writes, and it is the one Microsoft's own current API for unpackaged apps deliberately does not. Since Windows 8 the default for a file type lives in a hash-sealed UserChoice key that only the user, through the system's own interface, can set. So ftorrent offers and never takes: after this runs, it is in Explorer's Open with menu and listed in Settings under Default apps, and every .torrent on the machine still opens with whatever opened it before. Which is why bundle.fileAssociations is absent from tauri.conf.json and must stay absent: on Windows it inserts the NSIS macro that seizes each type's default at install time, silently.

Links take one step further, and it's where ftorrent is assertive without taking anything. A scheme's default lives in its own sealed UserChoice key too, but when no user has chosen, Windows falls back to the class key named for the scheme, magnet itself, which is shared: any program can write it, and many clients register nothing else. So ftorrent writes that class only while it runs no command or already runs ftorrent's. On a machine with no torrent client, a clicked magnet arrives at ftorrent with no user step; where another program holds the class, ftorrent leaves it alone, says so on the page, and waits to be chosen. A choice of ftorrent names ftorrent's own ProgID, never the shared class, so it holds however often another client rewrites magnet; and a choice of a client that registered only the shared class names magnet itself, which ftorrent then never touches.

The icon a .torrent wears is a file rather than the application: torrent.ico ships beside the executable through bundle.resources, and DefaultIcon names it, with ftorrent's own icon as the fallback if the file is not there. An application icon is meant to be unmistakable in a taskbar, which is exactly the wrong property on a document. The icon studio in the desktop workspace is where it's drawn. The ProgIDs are per type, so a different icon per format costs nothing later.

Two practical notes. It runs on every launch, which is cheap because each value is read before it is written and an unchanged value is not touched; the shell is only notified if something actually moved. And it has one gate, which is the whole of how it tells an installed copy from anything else: ftorrent.exe has to be in %LOCALAPPDATA%\ftorrent, the one place the per-user installer puts it. The command paths come from the executable's own location, so registering a copy anywhere else would point the registry at a file that may move or vanish: a portable copy on a stick that gets ejected, a debug or release build in the repository's target folder that gets rebuilt, a copy on the Desktop. None of them registers, and none needs to say why. The code still compiles into every Windows build, debug included, so a mistake in it shows up in everyday development rather than only when someone builds a release.
*/

//what ftorrent can open, and the names those things carry in Explorer's Type column and in the Settings app; read only by the windows code below
#[cfg(target_os = "windows")]
const FILE_TYPES: [(&str, &str, &str); 2] = [//the extension with its dot, the ProgID, and the name a user reads
	(".torrent",  "ftorrent.torrent",  "Torrent File"),//what µTorrent and qBittorrent call it too; Transmission's "BitTorrent Metadata File" is a developer's phrase
	(".ftorrent", "ftorrent.ftorrent", "ftorrent File"),//reserved early, for the metadata and capabilities ftorrent will add beyond the established standards
];
#[cfg(target_os = "windows")]
const URL_SCHEMES: [(&str, &str, &str); 2] = [//the scheme, ftorrent's own ProgID for it, and the name a user reads; the class key named for the scheme itself is written too, so no ProgID may be named bare magnet or ftorrent
	("magnet",   "ftorrent.url.magnet",   "URL:Magnet Link"),
	("ftorrent", "ftorrent.url.ftorrent", "URL:ftorrent Link"),
];
#[cfg(target_os = "windows")]
const APPLICATION_NAME: &str = "ftorrent";
#[cfg(target_os = "windows")]
const APPLICATION_DESCRIPTION: &str = "Next-generation client for the decentralized web";//the same line tauri.conf.json carries
#[cfg(target_os = "windows")]
const DOCUMENT_ICON: &str = "torrent.ico";//beside the executable, put there by bundle.resources

/// What registration did this launch, for the page: one line, blank where there was nothing to do
#[derive(Default)]
pub struct Associate(pub Mutex<String>);

/// Tell the operating system what ftorrent can open, taking nothing another program holds; called once from setup, after the lock, and the page asks for the answer
pub fn associate_register(app: &AppHandle) {
	let line = register().unwrap_or_else(|trouble| trouble);//trouble is reported the same way as a result, as a line on the page
	*app.state::<Associate>().0.lock().unwrap_or_else(|poisoned| poisoned.into_inner()) = line;
}

/// The registration line for the page
#[command]
pub fn associate_status(associate: State<'_, Associate>) -> String {
	associate.0.lock().unwrap_or_else(|poisoned| poisoned.into_inner()).clone()
}

#[cfg(not(target_os = "windows"))]
fn register() -> Result<String, String> {
	Ok(String::new())//blank, so the page shows nothing: on macOS and Linux the declarations ship inside the package, and there's nothing for ftorrent to do
}

#[cfg(target_os = "windows")]
fn register() -> Result<String, String> {
	let executable = std::env::current_exe().map_err(|e| format!("associations: {e}"))?;
	let installed_folder = std::env::var_os("LOCALAPPDATA").map(|local| std::path::PathBuf::from(local).join(APPLICATION_NAME));//where the per-user installer puts ftorrent.exe, %LOCALAPPDATA%\ftorrent
	let here = executable.parent().map(|folder| folder.to_string_lossy().to_lowercase());//windows paths ignore case
	if installed_folder.map(|folder| folder.to_string_lossy().to_lowercase()) != here { return Ok(String::new()) }//not where the installer puts ftorrent, so not an installed copy, whatever else it is; skipped without a word, since the reason doesn't matter
	let file = executable.file_name().and_then(|n| n.to_str()).ok_or("associations: ftorrent's own file name is not utf-8")?.to_string();//ftorrent.exe, which is the key windows expects under Applications
	let path = executable.to_str().ok_or("associations: the path to ftorrent is not utf-8")?.to_string();
	let command = format!("\"{path}\" \"%1\"");//quoted, because a file's path will contain spaces; %1 is where windows puts the file or the link
	let application_icon = format!("{path},0");//the executable's own icon, its first
	let beside = executable.with_file_name(DOCUMENT_ICON);
	let document_icon = match beside.to_str() {
		Some(found) if beside.exists() => format!("{found},0"),
		_ => application_icon.clone(),//still better than none
	};//neither is quoted, which is safe because windows reads an icon location by splitting at the last comma rather than at a space

	let application = format!("Software\\Classes\\Applications\\{file}");
	let capabilities = format!("Software\\{APPLICATION_NAME}\\Capabilities");
	let mut changed = 0;
	for (extension, program, name) in FILE_TYPES {
		changed += windows_set(&format!("Software\\Classes\\{program}"), "", name)?;//the ProgID: what this kind of file is called
		changed += windows_set(&format!("Software\\Classes\\{program}\\DefaultIcon"), "", &document_icon)?;//what explorer draws on one
		changed += windows_set(&format!("Software\\Classes\\{program}\\shell\\open\\command"), "", &command)?;//and what opens it
		changed += windows_set(&format!("Software\\Classes\\{extension}\\OpenWithProgids"), program, "")?;//ftorrent joins the list of what could open this extension, which is the offer; the value is empty and only the name matters
		changed += windows_set(&format!("{application}\\SupportedTypes"), extension, "")?;//so ftorrent is offered for these and not for everything else
		changed += windows_set(&format!("{capabilities}\\FileAssociations"), extension, program)?;//and so the settings app can list ftorrent's types
	}
	let mut left = Vec::new();//schemes whose shared class another program holds, named on the page
	for (scheme, program, name) in URL_SCHEMES {
		let mut classes = vec![format!("Software\\Classes\\{program}")];//ftorrent's own ProgID, which no other program writes, so a user's choice that names it stays ftorrent's
		if windows_claimable(scheme, &command) { classes.push(format!("Software\\Classes\\{scheme}")) } else { left.push(scheme) }//and the shared class named for the scheme, which windows falls back to when no user has chosen, but only while it's empty or already ours
		for class in classes {//the same four values under each
			changed += windows_set(&class, "", name)?;
			changed += windows_set(&class, "URL Protocol", "")?;//the empty value that marks a class as a url scheme rather than a file type
			changed += windows_set(&format!("{class}\\DefaultIcon"), "", &application_icon)?;
			changed += windows_set(&format!("{class}\\shell\\open\\command"), "", &command)?;
		}
		changed += windows_set(&format!("{capabilities}\\URLAssociations"), scheme, program)?;//so the settings app lists ftorrent for this kind of link, and a choice there names ftorrent's own ProgID rather than the shared class
	}
	changed += windows_set(&application, "FriendlyAppName", APPLICATION_NAME)?;
	changed += windows_set(&format!("{application}\\shell\\open\\command"), "", &command)?;
	changed += windows_set(&capabilities, "ApplicationName", APPLICATION_NAME)?;
	changed += windows_set(&capabilities, "ApplicationDescription", APPLICATION_DESCRIPTION)?;
	changed += windows_set("Software\\RegisteredApplications", APPLICATION_NAME, &capabilities)?;//the line that puts ftorrent in the settings app by name, and last on purpose: any write above can fail and take the whole call with it, so publishing ftorrent to Settings is the step that only happens once everything it points at is there. The next launch starts again from the top and finishes the job

	if changed > 0 { windows_notify() }//only when something moved, because this runs on every launch and almost always writes nothing
	let note = if left.is_empty() { String::new() } else { format!("; {} left to the program that has it", left.join(" and ")) };
	Ok(format!("associations: {} file types and {} link types registered, {changed} values written{note}", FILE_TYPES.len(), URL_SCHEMES.len()))
}

/// Text the way windows takes it, utf-16 ending in a zero; bind the result to a variable before handing windows a pointer into it, because a pointer into a temporary dangles
#[cfg(target_os = "windows")]
fn wide(text: &str) -> Vec<u16> {
	text.encode_utf16().chain(std::iter::once(0)).collect()
}

/// Create a key under HKEY_CURRENT_USER if it isn't there and set one of its values, answering 1 if that changed anything and 0 if the value already said this; a blank name means the key's own default value, which is how the registry spells "the value of this key itself"
#[cfg(target_os = "windows")]
fn windows_set(path: &str, name: &str, value: &str) -> Result<u32, String> {
	use windows::core::PCWSTR;
	use windows::Win32::System::Registry::{RegCloseKey, RegCreateKeyExW, RegQueryValueExW, RegSetValueExW, HKEY, HKEY_CURRENT_USER, KEY_QUERY_VALUE, KEY_SET_VALUE, REG_OPTION_NON_VOLATILE, REG_SZ};

	let wide_path = wide(path);
	let wide_name = wide(name);
	let bytes = wide(value).iter().flat_map(|u| u.to_le_bytes()).collect::<Vec<u8>>();//REG_SZ is utf-16 little endian including its terminating zero, handed to windows as bytes

	let mut key = HKEY::default();
	let opened = unsafe { RegCreateKeyExW(HKEY_CURRENT_USER, PCWSTR(wide_path.as_ptr()), None, PCWSTR::null(), REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, None, &mut key, None) };//opens the key, creating it and any missing parents along the way; every pointer here is into a local that outlives the call
	if opened.is_err() { return Err(format!("associations: could not open {path}, windows error {}", opened.0)) }

	//read what is there now; a value that already says this is left alone, so a launch that changed nothing does not go on to tell the shell that something did
	let mut buffer = [0u8; 2048];
	let mut size = buffer.len() as u32;
	let read = unsafe { RegQueryValueExW(key, PCWSTR(wide_name.as_ptr()), None, None, Some(buffer.as_mut_ptr()), Some(&mut size)) };
	let same = read.is_ok() && buffer[..size as usize] == bytes[..];//a value too long for the buffer fails the read rather than reporting a size past its end, so the slice is safe once the read succeeds. Only the bytes are compared and not the type, so a value some other program wrote as REG_NONE is rewritten once and matches from then on

	let mut answer = Ok(0);
	if !same {
		let written = unsafe { RegSetValueExW(key, PCWSTR(wide_name.as_ptr()), None, REG_SZ, Some(&bytes)) };
		answer = if written.is_ok() { Ok(1) } else { Err(format!("associations: could not write {path}, windows error {}", written.0)) };
	}
	let _ = unsafe { RegCloseKey(key) };
	answer
}

/// Whether the class named for this scheme is ftorrent's to write: it runs no command yet, or already runs ftorrent's. Read through HKEY_CLASSES_ROOT, the view windows itself uses, which lays the user's classes over the machine's, so a client installed for everyone counts as holding it too
#[cfg(target_os = "windows")]
fn windows_claimable(scheme: &str, command: &str) -> bool {
	use windows::core::PCWSTR;
	use windows::Win32::Foundation::ERROR_FILE_NOT_FOUND;
	use windows::Win32::System::Registry::{RegGetValueW, HKEY_CLASSES_ROOT, RRF_NOEXPAND, RRF_RT_REG_EXPAND_SZ, RRF_RT_REG_SZ};

	let wide_path = wide(&format!("{scheme}\\shell\\open\\command"));
	let mut buffer = [0u16; 1024];//far longer than any command of ftorrent's, so a value that doesn't fit is somebody else's
	let mut size = std::mem::size_of_val(&buffer) as u32;//the registry counts in bytes
	let read = unsafe { RegGetValueW(HKEY_CLASSES_ROOT, PCWSTR(wide_path.as_ptr()), PCWSTR::null(), RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, None, Some(buffer.as_mut_ptr().cast()), Some(&mut size)) };//the key's default value, which is the command; unlike RegQueryValueExW, this always ends a string with its terminating zero
	if read == ERROR_FILE_NOT_FOUND { return true }//no class, or a class with no command: empty, so writing it takes nothing from anyone
	if read.is_err() { return false }//too long or unreadable, so not ftorrent's
	let found = String::from_utf16_lossy(&buffer[..(size as usize / 2).saturating_sub(1)]);//characters, less the terminating zero; on success size never exceeds the buffer
	found.is_empty() || found == command
}

/// Tell the shell that associations have changed, so explorer's menus and icons catch up without a sign-out
#[cfg(target_os = "windows")]
fn windows_notify() {
	use windows::Win32::UI::Shell::{SHChangeNotify, SHCNE_ASSOCCHANGED, SHCNF_IDLIST};
	unsafe { SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, None, None) }
}
