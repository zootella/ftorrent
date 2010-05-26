
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
std::string w2s(std::wstring w) {

	std::string s;
	libtorrent::wchar_utf8(w, s); // Use libtorrent
	return s;
}

// Convert a STL UTF8 byte string into a STL wide character string
std::wstring s2w(std::string s) {

	std::wstring w;
	libtorrent::utf8_wchar(s, w);
	return w;
}

// Convert a STL UTF8 byte string into an ATL wide CString
CString s2c(std::string s) {

	return s2w(s).c_str();
}

// Convert a STL wide character string into an ATL wide CString
CString w2c(std::wstring w) {

	return w.c_str();
}
