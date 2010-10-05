
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













//TODO now you're thinking that Data.torrents should be a vector that holds pointers to torrentitems
//new and delete are worth being able to return null or a pointer to an object
//maybe not, just use Find() and Get() in order and it's the same thing


//TODO make a function that takes a hash value and finds the torrentitem in Data.torrents, false if not found







/*
// Run a snippet of test code
void Test() {

	/*
	CString path = L"C:\\Documents\\no.doc";
	libtorrent::big_number hash;
	CString name;
	bool result = LookPath(path, &hash, &name);
	if (!result) { log(L"false"); return; }
	log(L"hash: ", convertBigNumberToC(hash));
	log(L"name: ", name);



	/*

	m.
	std::map<CString, CString>::iterator i = m.find(L"color");
	if (i != stringCounts.end()) {

		log(i->first, i->second);

	}




	/*
	libtorrent::entry e;
	libtorrent::entry::dictionary_type d = e.dict();
	d.insert(

	libtorrent::entry::dictionary_type::const_iterator i;
	i = dict.find("announce");
	if (i != dict.end()) {

		std::string tracker_url = i->second.string();
		std::cout << tracker_url << "\n";
	}







	/*

	SaveEntry(PathOption(), d);


	/*
	int choice = Dialog(L"DIALOG_REGISTER"); // Enter IDYES, IDNO, or Ask X Escape are IDCANCEL
	log(L"choice is ", numerals(choice));



	/*

	AddTorrent(
		L"C:\\Documents\\test",       // folder
		L"C:\\Documents\\my.torrent", // torrent
		NULL,  // magnet hash
		NULL,  //        name
		NULL,  //        tracker
		NULL); // store file from before

	/*
	// convert some big numbers to text
	sbig n = 0xffffffffffffffff;
	log(L"the signed number in base 10 is ", sbigtoC(n, 10));
	log(L"the signed number in base 16 is ", sbigtoC(n, 16));

	ubig n2 = 0xffffffffffffffff;
	log(L"the unsigned number in base 10 is ", ubigtoC(n2, 10));
	log(L"the unsigned number in base 16 is ", ubigtoC(n2, 16));

	/*
	//let's move 4 unsigned char into a dword

	BYTE c1 = 0x01;
	BYTE c2 = 0x02;
	BYTE c3 = 0x03;
	BYTE c4 = 0x04;

	DWORD d = (((DWORD)c1) << 24) | (((DWORD)c2) << 16) | (((DWORD)c3) << 8) | ((DWORD)c4);

	log(base16(d));

	/*
	std::vector<torrentitem> v;

	torrentitem t1, t2, t3;
	t1.id = 1;
	t2.id = 2;
	t3.id = 3;
	v.push_back(t1);
	v.push_back(t2);
	v.push_back(t3);
	print(v);

	v.erase(v.begin() + 1);
	print(v);
	*/

	/*
	ListAdd(Handle.list, 5, (LPARAM)0xaabbccdd, Handle.icon.clear, L"a", Handle.icon.clear, L"bb", L"ccc", L"dddd", L"eeeee", NULL);

	log(ListText(Handle.list, 0, 0));
	log(ListText(Handle.list, 0, 1));
	log(ListText(Handle.list, 0, 2));
	log(ListText(Handle.list, 0, 3));
	log(ListText(Handle.list, 0, 4));



}
*/




































void ListPulse() {


	for (int i = 0; i < (int)Data.torrents.size(); i++)
		Data.torrents[i].Edit();




}




// Edit the list view row to match the information in this torrent item
void torrentitem::Edit() {

	// Update the cells that have different text
	ListEdit(
		Handle.list,
		5,
		(LPARAM)Hash(),
		ComposeStatusIcon(),
		ComposeStatus(),
		ComposeNameIcon(),
		ComposeName(),
		ComposeSize(),
		ComposeHash(),
		ComposePath(),
		L"");
}







int torrentitem::ComposeStatusIcon() {
	return Handle.icon.clear;
}

CString torrentitem::ComposeStatus() {
	return L"status text";
}

int torrentitem::ComposeNameIcon() {
	return Handle.icon.clear;
}

// This torrent's name
CString torrentitem::ComposeName() {
	return widenStoC(handle.name());
}

CString torrentitem::ComposeSize() {

	sbig done = handle.status().total_done; // libtorrent::size_type and sbig are both __int64
	sbig size = handle.get_torrent_info().total_size();

	return make(InsertCommas(sbigtoC(done)), L"/", InsertCommas(sbigtoC(size)), L" bytes");
}

// This torrent's infohash in base 16
CString torrentitem::ComposeHash() {

	return convertBigNumberToC(handle.info_hash());
}

CString torrentitem::ComposePath() {
	return L"path text";
}

// The first 4 bytes of the infohash in a DWORD
DWORD torrentitem::Hash() {

	return HashStart(handle.info_hash());
}







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



















