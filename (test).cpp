
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
#include "program.h"
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



	//test handle to text and back


	CString s = L"e6a56670baae316ebf5d3ce91be729e8688f7256";
	CString s2;

	libtorrent::big_number b = convertRtoBigNumber(s);
	s2 = convertBigNumberToC(b);
	log(s);
	log(s2);

	libtorrent::sha1_hash h = convertRtoSha1Hash(s);
	s2 = convertSha1HashToC(h);
	log(s);
	log(s2);


	/*


	libtorrent::big_number convertRtoBigNumber(read r);
	libtorrent::sha1_hash convertRtoSha1Hash(read r);
	libtorrent::big_number convertPtoBigNumber(const char *p);
	libtorrent::sha1_hash convertPtoSha1Hash(const char *p);
	CString convertSha1HashToC(const libtorrent::sha1_hash &h);
	CString convertBigNumberToC(const libtorrent::big_number &n);

	*/





	/*
	//add
	log(L"add before");
	libtorrent::torrent_handle h;
	bool result = AddTorrent(
		L"C:\\Documents\\test",       // folder
		L"C:\\Documents\\my.torrent", // torrent
		NULL, // magnet hash
		NULL, //        name
		NULL, //        tracker
		NULL, // store
		h);  // handle
	log(L"add after");
	*/



}
