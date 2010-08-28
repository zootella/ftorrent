
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




	// add it to libtorrent
	torrentitem t = AddTorrent(
		L"C:\\Documents\\test",       // folder
		L"C:\\Documents\\my.torrent", // torrent
		NULL,  // magnet hash
		NULL,  //        name
		NULL,  //        tracker
		NULL); // store file from before

	// add it to the program's list
	Data.torrentlist.push_back(t);

	// add it to the list view
	ListAdd(
		Handle.list,
		5,
		(LPARAM)&t,
		t.ComposeStatusIcon(),
		t.ComposeStatus(),
		t.ComposeNameIcon(),
		t.ComposeName(),
		t.ComposeSize(),
		t.ComposeHash(),
		t.ComposePath(),
		L"");





}
