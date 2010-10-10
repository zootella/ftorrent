
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
#include <netfw.h>

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

// Takes a root key handle name, a key path, and a registry variable name
// Gets the information from the registry
// Returns true if it works, and writes the value
bool RegistryReadNumber(HKEY root, read path, read name, DWORD *value) {

	// Open the key
	registryitem registry;
	if (!registry.Open(root, path, false)) return false;

	// Read the number value
	DWORD d;
	DWORD size = sizeof(DWORD);
	int result = RegQueryValueEx(
		registry.Key, // Handle to an open key
		name,         // Name of the value to read
		0,
		NULL,
		(LPBYTE)&d,   // Data buffer
		&size);       // Size of data buffer
	if (result != ERROR_SUCCESS) { error(L"regqueryvalueex number ", numerals(result)); return false; }

	// Write the number
	*value = d;
	return true;
}

// Takes a root key handle name, a key path, and a registry variable name
// Gets the information from the registry
// Returns true if it works, and writes the value
bool RegistryReadText(HKEY root, read path, read name, CString *value) {

	// Open the key
	registryitem registry;
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
	if (result != ERROR_SUCCESS) { error(L"regqueryvalueex text size ", numerals(result)); return false; }

	// Open a string
	CString s;
	LPWSTR buffer = s.GetBuffer(size / sizeof(WCHAR)); // How many characters we'll write, including the null terminator

	// Read the binary data
	result = RegQueryValueEx(
		registry.Key,   // Handle to an open key
		name,           // Name of the value to read
		0,
		NULL,
		(LPBYTE)buffer, // Data buffer, writes the null terminator
		&size);         // Size of data buffer in bytes
	s.ReleaseBuffer();
	if (result != ERROR_SUCCESS) { error(L"regqueryvalueex text ", numerals(result)); return false; }

	// Write the string
	*value = s;
	return true;
}

// Takes a root key handle name, a key path, a registry variable name, and an integer
// Stores the information in the registry
// Returns false on error
bool RegistryWriteNumber(HKEY root, read path, read name, int value) {

	// Open the key
	registryitem registry;
	if (!registry.Open(root, path, true)) return false;

	// Set or make and set the number value
	int result = RegSetValueEx(
		registry.Key,         // Handle to an open key
		name,                 // Name of the value to set or make and set
		0,
		REG_DWORD,            // Variable type is a 32-bit number
		(const BYTE *)&value, // Address of the value data to load
		sizeof(DWORD));       // Size of the value data
	if (result != ERROR_SUCCESS) { error(L"regsetvalueex number ", numerals(result)); return false; }
	return true;
}

// Takes a root key handle name, a key path, a registry variable name, and value text
// Stores the information in the registry
// Returns false on error
bool RegistryWriteText(HKEY root, read path, read name, read value) {

	// Open the key
	registryitem registry;
	if (!registry.Open(root, path, true)) return false;

	// Set or make and set the text value
	int result = RegSetValueEx(
		registry.Key,                          // Handle to an open key
		name,                                  // Name of the value to set or make and set
		0,
		REG_SZ,                                // Variable type is a null-terminated string
		(const BYTE *)value,                   // Address of the value data to load
		(lstrlen(value) + 1) * sizeof(WCHAR)); // Size of the value data in bytes, add 1 to write the null terminator
	if (result != ERROR_SUCCESS) { error(L"regsetvalueex text ", numerals(result)); return false; }
	return true;
}

// Takes a root key handle name or open base key, and the path to a key beneath it
// Deletes the key from the registry, including its subkeys
// Returns false on error
bool RegistryDelete(HKEY base, read path) {

	// Open the key
	registryitem key;
	if (!key.Open(base, path, true)) return false;

	// Loop for each subkey, deleting them all
	DWORD size;
	WCHAR subkey[MAX_PATH];
	int result;
	while (true) {

		// Get the name of the first subkey
		size = MAX_PATH;
		result = RegEnumKeyEx(key.Key, 0, subkey, &size, NULL, NULL, NULL, NULL);
		if (result == ERROR_NO_MORE_ITEMS) break; // There are no subkeys
		else if (result != ERROR_SUCCESS) { error(L"regenumkeyex ", numerals(result)); return false; } // RegEnumKeyEx returned an error

		// Delete it, making the next subkey the new first one
		if (!RegistryDelete(key.Key, subkey)) return false;
	}

	// We've cleared this key of subkeys, close it and delete it
	key.Close();
	result = RegDeleteKey(base, path);
	if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) { error(L"regdeletekey ", numerals(result)); return false; }
	return true;
}

// Takes a root key handle name, a key path, and true to make keys and get write access
// Opens or creates and opens the key and returns true
bool registryitem::Open(HKEY root, read path, bool write) {

	// Make sure we were given a key and path
	if (!root || isblank(path)) return false;

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
			L"",
			REG_OPTION_NON_VOLATILE, // Save information in the registry file
			KEY_ALL_ACCESS,          // Get access to read and write values in the key we're making and opening
			NULL,
			&key,                    // The opened or created key handle is put here
			&info);                  // Tells if the key was opened or created and opened
		if (result != ERROR_SUCCESS) { error(L"regcreatekeyex ", numerals(result)); return false; }

	// If the caller only wants read access, don't create the key when trying to open it
	} else {

		// Open the key
		result = RegOpenKeyEx(
			root,     // Handle to open root key
			path,     // Subkey name
			0,
			KEY_READ, // We only need to read the key we're opening
			&key);    // The opened key handle is put here
		if (result != ERROR_SUCCESS) { error(L"regopenkeyex ", numerals(result)); return false; }
	}

	// Save the open key in this registryitem object
	Key = key;
	return true;
}
