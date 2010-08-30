
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


	ListAdd(Handle.list, 5, (LPARAM)0xaabbccdd, Handle.icon.clear, L"a", Handle.icon.clear, L"bb", L"ccc", L"dddd", L"eeeee", NULL);


	log(ListText(Handle.list, 0, 0));
	log(ListText(Handle.list, 0, 1));
	log(ListText(Handle.list, 0, 2));
	log(ListText(Handle.list, 0, 3));
	log(ListText(Handle.list, 0, 4));






	/*

	AddTorrent(
		L"C:\\Documents\\test",       // folder
		L"C:\\Documents\\my.torrent", // torrent
		NULL,  // magnet hash
		NULL,  //        name
		NULL,  //        tracker
		NULL); // store file from before

		*/






}
