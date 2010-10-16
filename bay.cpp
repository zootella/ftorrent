
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

// Run a snippet of test code
void Test() {



	std::map<CString, CString> map;
	// map string to a little object that has text and an icon index int, Cell


}






// Takes text
// Reads the text as a number, handling a leading minus sign properly
// Returns the number, or 0 if given blank or alphabetic text
int number(read r) {

	return _wtoi(r); // Use function like atoi
}

// Convert the unsigned 64 bit number into text numerals using base 10 or 16
CString ubigtoC(ubig number, int base) {

	WCHAR bay[MAX_PATH];
	_ui64tow_s(number, bay, MAX_PATH, base);
	return bay;
}

// Convert the signed 64 bit number into text numerals using base 10 or 16
CString sbigtoC(sbig number, int base) {

	WCHAR bay[MAX_PATH];
	_i64tow_s(number, bay, MAX_PATH, base);
	return bay;
}

// Takes a number and width like 3 for 001, base 10 or 16
// Writes the minus sign and number into text
// Returns a string
CString numerals(int number, int width, int base) {

	WCHAR bay[MAX_PATH];
	_itow_s(number, bay, MAX_PATH, base);
	CString s = bay;

	if (width) {
		while (length(s) < width)
			s = L"0" + s; // Loop until we've put enough 0s at the start
	}
	return s;
}

// Turn a 4 byte number into base 16 numerals like "11223344"
CString base16(DWORD number) {

	return numerals(number, 8, 16);
}






// Convert a 20 byte hash value in 40 characters of base 16 text into a libtorrent big number, which is the same thing as a libtorrent SHA1 hash
hbig ParseHash(read r) {

	std::stringstream stream;
	hbig n;
	stream << narrowRtoS(r).c_str();
	stream >> n;
	return n;
}

// Convert a libtorrent big number or SHA1 hash into 40 characters of base 16 text
CString base16(const hbig &n) {

	std::stringstream stream;
	stream << n;
	return widenStoC(stream.str());
}

// The first 4 bytes of the given hash value as a DWORD
DWORD HashStart(hbig hash) {

	return
		(((DWORD)hash[0]) << 24) |
		(((DWORD)hash[1]) << 16) |
		(((DWORD)hash[2]) <<  8) |
		 ((DWORD)hash[3]);
}

//TODO this is the same as hbig
// Convert the given peer ID object into text
CString PeerToString(const hbig &id) {

	std::stringstream stream;
	stream << id;
	return widenStoC(stream.str());
}








