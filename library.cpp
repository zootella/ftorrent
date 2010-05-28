
// Include libtorrent
#include "libtorrent/create_torrent.hpp"

// Include platform
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>

// Include program
#include "resource.h"
#include "program.h"
#include "object.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;

// Convert a STL wide character string into a STL UTF8 byte string
std::string WideToString(std::wstring w) {

	std::string s;
	libtorrent::wchar_utf8(w, s); // Use libtorrent
	return s;
}

// Convert a STL UTF8 byte string into a STL wide character string
std::wstring StringToWide(std::string s) {

	std::wstring w;
	libtorrent::utf8_wchar(s, w);
	return w;
}

// Convert a STL UTF8 byte string into an ATL wide CString
CString StringToCString(std::string s) {

	return StringToWide(s).c_str();
}

// Convert a STL wide character string into an ATL wide CString
CString WideToCString(std::wstring w) {

	return w.c_str();
}




// Make a new string of allocated memory you have to free from the given hash value
const char *HashToString(const libtorrent::sha1_hash &hash) {

	std::stringstream stream;
	stream << hash;
	return CopyStringFromStream(stream);
}

// Make a new string of allocated memory you have to free from the given peer ID
const char *PeerIdToString(const libtorrent::peer_id &id) {

	std::stringstream stream;
	stream << id;
	return CopyStringFromStream(stream);
}

// Make a new string of allocated memory you have to free from the text in the given stream
const char *CopyStringFromStream(const std::stringstream &stream) {

	std::string s = stream.str();
	return CopyString(s.c_str());
}

// Copy the given byte string into a new memory block you have to free later
char *CopyString(const char *s) {

	int n = std::strlen(s);           // Number of characters not including the null terminator
	char *copy = new char[n + 1];     // Allocate space for the characters and the null terminator
	strncpy_s(copy, n + 1, s, n + 1); // Copy the characters and the null terminator
	return copy;
}

// Copy the given wide string into a new memory block you have to free later
wchar_t *CopyWideString(const wchar_t *s) {

	int n = wcslen(s);                  // Number of characters not including the null terminator
	wchar_t *copy = new wchar_t[n + 1]; // Allocate space for the characters and the null terminator
	wcsncpy_s(copy, n + 1, s, n + 1);   // Copy the characters and the null terminator
	return copy;
}




// Convert the hash value in the given text into a libtorrent big number hash value object
libtorrent::big_number StringToHash(const char *s) {

	libtorrent::big_number hash;
	std::stringstream stream;
	stream << s;
	stream >> hash;
	return hash;
}




