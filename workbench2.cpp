
// Include libtorrent
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/alert_types.hpp"

// Include platform
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>

// Include program
#include "resource.h"
#include "define.h"
#include "object.h"
#include "library.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;


/*

// Wraps a registry key, taking care of closing it
class CRegistry {
public:

	// The handle to the registry key
	HKEY Key;

	// Open a registry key and store its handle in this object
	bool Open(HKEY root, CString path, bool write);
	void Close() { if (Key) RegCloseKey(Key); Key = NULL; }

	// Make a new local CRegistry object, and delete it when it goes out of scope
	CRegistry() { Key = NULL; }
	~CRegistry() { Close(); }
};



// Takes a root key handle name, a key path, and true to make keys and get write access
// Opens or creates and opens the key with full access
// Returns false on error
bool CRegistry::Open(HKEY root, CString path, bool write) {

	// Variables for opening the key
	HKEY key;
	DWORD info;
	int result;

	// If the caller wants write access, create the key if it isn't there
	if (write) {

		// Open or create and open the key
		result = RegCreateKeyEx(
			root,                    // Handle to open root key
			path,                    // Subkey name
			0,
			"",
			REG_OPTION_NON_VOLATILE, // Save information in the registry file
			KEY_ALL_ACCESS,          // Get access to read and write values in the key we're making and opening
			NULL,
			&key,                    // The opened or created key handle is put here
			&info);                  // Tells if the key was opened or created and opened

	// If the caller only wants read access, don't create the key when trying to open it
	} else {

		// Open the key
		result = RegOpenKeyEx(
			root,     // Handle to open root key
			path,     // Subkey name
			0,
			KEY_READ, // We only need to read the key we're opening
			&key);    // The opened key handle is put here
	}

	// Check for an error from opening or making and opening the key
	if (result != ERROR_SUCCESS) return false;

	// Save the open key in this CRegistry object
	Key = key;
	return true;
}

// Read text in the registry
bool RegistryRead(HKEY root, CString path, CString name, CString *value) {

	// Open the key
	CRegistry registry;
	if (!registry.Open(root, path, false)) return false;

	// Get the size required
	DWORD size;
	int result = RegQueryValueEx(
		registry.Key, // Handle to an open key
		name,         // Name of the value to read
		0,
		NULL,
		NULL,         // No data buffer, we're requesting the size
		&size);       // Required size in bytes including the null terminator
	if (result != ERROR_SUCCESS) return false;

	// Open a string
	CString s;
	LPTSTR buffer = s.GetBuffer(size); // How many characters we'll write, including the null terminator

	// Read the binary data
	result = RegQueryValueEx(
		registry.Key,   // Handle to an open key
		name,           // Name of the value to read
		0,
		NULL,
		(LPBYTE)buffer, // Data buffer, writes the null terminator
		&size);         // Size of data buffer in bytes
	s.ReleaseBuffer();
	if (result != ERROR_SUCCESS) return false;

	// Save the text
	*value = s;
	return true;
}

// Write text to the registry
bool RegistryWrite(HKEY root, CString path, CString name, CString value) {

	// Open the key
	CRegistry registry;
	if (!registry.Open(root, path, true)) return false;

	// Set or make and set the text value
	int result = RegSetValueEx(
		registry.Key,                 // Handle to an open key
		name,                         // Name of the value to set or make and set
		0,
		REG_SZ,                       // Variable type is a null-terminated string
		(const BYTE *)(LPCTSTR)value, // Address of the value data to load
		(lstrlen(value) + 1));        // Size of the value data in bytes, add 1 to write the null terminator
	if (result != ERROR_SUCCESS) return false;
	return true;
}

// Delete a registry key
bool RegistryDelete(HKEY base, CString path) {
	int result = RegDeleteKey(base, path);
	if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) return false;
	return true;
}



// List the program in Add or Remove Programs
void SetupList() {
	RegistryWrite(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + PROGRAM_NAME, "DisplayName", PROGRAM_NAME);
	RegistryWrite(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + PROGRAM_NAME, "UninstallString", PathLaunch() + " /addremove");
}

// Remove our listing in Add or Remove programs
CString RemoveList() {
	bool result = RegistryDelete(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + PROGRAM_NAME);
	return result ? "" : "Unable to remove Add or Remove Programs listing";
}


*/



















