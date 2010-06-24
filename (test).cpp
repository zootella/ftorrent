
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
	try {

		//add
		if (step == 0) {
			step++;

			libtorrent::add_torrent_params p;
			p.save_path = boost::filesystem::path(narrowRtoS(L"C:\\Documents\\test"));
			p.ti = new libtorrent::torrent_info(boost::filesystem::path(narrowRtoS(L"C:\\Documents\\my.torrent")));
			libtorrent::torrent_handle h = Handle.session->add_torrent(p);
			log(L"add done");

		//Ask for fast resume data
		} else if (step == 1) {
			step++;

			LibraryClose1();



		} else if (step == 2) {
			step++;


		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
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
