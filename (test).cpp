
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


int step = 0;

// Run a snippet of test code
void Test() {



	if (step == 0) {
		step++;

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

	} else if (step == 1) {
		step++;

		//save
		log(L"stop before");
		LibraryStop();
		log(L"stop after");

	} else if (step == 2) {
		step++;

		//close
		log(L"close before");
		LibraryClose();
		log(L"close after");
	}


	/*
	bool result = false;
	CString path = L"C:\\Documents\\options.db";

	if (!step) {
		step = 1;

		libtorrent::entry e1;
		e1 = narrowRtoS(L"hello");
		result = SaveEntry(path, e1);

	} else {

		libtorrent::entry e2;
		result = LoadEntry(path, e2);

		CString s = widenStoC(e2.string().c_str());
		log(s);
	}

	log(result ? L"true" : L"false");
	*/
}
