use tauri::command;

/*
The Windows registry, offered to the page the way disk.rs offers the disk: three general commands that any Windows application could use as they are, knowing nothing about what's read or written or why. Which keys, which values, and in what order is the page's; associate.js is the one that uses these today, to register the file types and link schemes an installed copy can open.

registry_get reads a string value through one of two roots: classes, which is HKEY_CLASSES_ROOT, the merged view Windows itself uses to decide what opens what, laying the user's classes over the machine's; or user, which is HKEY_CURRENT_USER. It answers nothing when the key or the value isn't there. registry_set writes a string value, and only ever under HKEY_CURRENT_USER, creating the key if it's missing; it reads first, writes only when the value would change, and answers whether it did, so a caller that runs on every launch can tell the shell only when something moved. registry_notify tells the shell that file associations changed, so Explorer's menus and icons catch up without a sign-out.

A blank value name means the key's own default value, which is how the registry spells "the value of this key itself". The commands take any key under their root and hold no guard, on purpose, the same as disk.rs: the page runs only ftorrent's own code, and it alone knows what a key means. They never write the machine-wide hive. On macOS and Linux there's no registry, and each command answers so.
*/

/// Read a string value; root is classes or user, and a blank name is the key's default value. Answers nothing when the key or the value isn't there
#[command]
pub fn registry_get(root: String, key: String, name: String) -> Result<Option<String>, String> {
	platform::get(&root, &key, &name)
}

/// Write a string value under the current user, creating the key if it's missing, and only if it would change; answers whether it changed
#[command]
pub fn registry_set(key: String, name: String, value: String) -> Result<bool, String> {
	platform::set(&key, &name, &value)
}

/// Tell the shell that file associations changed
#[command]
pub fn registry_notify() -> Result<(), String> {
	platform::notify()
}

#[cfg(target_os = "windows")]
mod platform {
	use windows::core::PCWSTR;
	use windows::Win32::Foundation::ERROR_FILE_NOT_FOUND;
	use windows::Win32::System::Registry::{RegCloseKey, RegCreateKeyExW, RegGetValueW, RegQueryValueExW, RegSetValueExW, HKEY, HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, KEY_QUERY_VALUE, KEY_SET_VALUE, REG_OPTION_NON_VOLATILE, REG_SZ, RRF_NOEXPAND, RRF_RT_REG_EXPAND_SZ, RRF_RT_REG_SZ};
	use windows::Win32::UI::Shell::{SHChangeNotify, SHCNE_ASSOCCHANGED, SHCNF_IDLIST};

	/// Text the way windows takes it, utf-16 ending in a zero; bind the result to a variable before handing windows a pointer into it, because a pointer into a temporary dangles
	fn wide(text: &str) -> Vec<u16> {
		text.encode_utf16().chain(std::iter::once(0)).collect()
	}

	pub fn get(root: &str, key: &str, name: &str) -> Result<Option<String>, String> {
		let hive = match root { "classes" => HKEY_CLASSES_ROOT, "user" => HKEY_CURRENT_USER, _ => return Err(format!("registry: no root named {root}; it's classes or user")) };
		let wide_key = wide(key);
		let wide_name = wide(name);
		let mut buffer = [0u16; 2048];//longer than any value a registration reads; a longer one is reported rather than cut short
		let mut size = std::mem::size_of_val(&buffer) as u32;//the registry counts in bytes
		let read = unsafe { RegGetValueW(hive, PCWSTR(wide_key.as_ptr()), if name.is_empty() { PCWSTR::null() } else { PCWSTR(wide_name.as_ptr()) }, RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, None, Some(buffer.as_mut_ptr().cast()), Some(&mut size)) };//a string, as written; unlike RegQueryValueExW, this always ends one with its terminating zero
		if read == ERROR_FILE_NOT_FOUND { return Ok(None) }//no key, or a key without this value
		if read.is_err() { return Err(format!("registry: could not read {key}, windows error {}", read.0)) }
		Ok(Some(String::from_utf16_lossy(&buffer[..(size as usize / 2).saturating_sub(1)])))//characters, less the terminating zero; on success size never exceeds the buffer
	}

	pub fn set(key: &str, name: &str, value: &str) -> Result<bool, String> {
		let wide_key = wide(key);
		let wide_name = wide(name);
		let bytes = wide(value).iter().flat_map(|u| u.to_le_bytes()).collect::<Vec<u8>>();//REG_SZ is utf-16 little endian including its terminating zero, handed to windows as bytes

		let mut handle = HKEY::default();
		let opened = unsafe { RegCreateKeyExW(HKEY_CURRENT_USER, PCWSTR(wide_key.as_ptr()), None, PCWSTR::null(), REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, None, &mut handle, None) };//opens the key, creating it and any missing parents along the way; every pointer here is into a local that outlives the call
		if opened.is_err() { return Err(format!("registry: could not open {key}, windows error {}", opened.0)) }

		//read what's there now; a value that already says this is left alone, so a caller that changed nothing can know it
		let mut buffer = [0u8; 2048];
		let mut size = buffer.len() as u32;
		let read = unsafe { RegQueryValueExW(handle, PCWSTR(wide_name.as_ptr()), None, None, Some(buffer.as_mut_ptr()), Some(&mut size)) };
		let same = read.is_ok() && buffer[..size as usize] == bytes[..];//a value too long for the buffer fails the read rather than reporting a size past its end, so the slice is safe once the read succeeds. Only the bytes are compared and not the type, so a value some other program wrote as REG_NONE is rewritten once and matches from then on

		let mut answer = Ok(false);
		if !same {
			let written = unsafe { RegSetValueExW(handle, PCWSTR(wide_name.as_ptr()), None, REG_SZ, Some(&bytes)) };
			answer = if written.is_ok() { Ok(true) } else { Err(format!("registry: could not write {key}, windows error {}", written.0)) };
		}
		let _ = unsafe { RegCloseKey(handle) };
		answer
	}

	pub fn notify() -> Result<(), String> {
		unsafe { SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, None, None) }
		Ok(())
	}
}

#[cfg(not(target_os = "windows"))]
mod platform {
	const NONE: &str = "registry: there's no registry on this platform";
	pub fn get(_root: &str, _key: &str, _name: &str) -> Result<Option<String>, String> { Err(NONE.to_string()) }
	pub fn set(_key: &str, _name: &str, _value: &str) -> Result<bool, String> { Err(NONE.to_string()) }
	pub fn notify() -> Result<(), String> { Err(NONE.to_string()) }
}
