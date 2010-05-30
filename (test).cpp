
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

		if (step == 0) {

			InitializeLibtorrent(NULL);
			step = 1;

			log(L"init done");

		} else if (step == 1) {

			libtorrent::add_torrent_params torrent_params;

			torrent_params.save_path = boost::filesystem::path(WideToString(L"C:\\Documents\\test"));
			torrent_params.ti = new libtorrent::torrent_info(boost::filesystem::path(WideToString(L"C:\\Documents\\my.torrent")));
			libtorrent::torrent_handle h = Handle.session->add_torrent(torrent_params);

			log(L"add done");
		}

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}









