
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

	std::vector<int> weights;
	weights.push_back(21);
	weights.push_back(22);
	weights.push_back(23);
	weights.push_back(24);
	weights.push_back(25);

	std::vector<int> widths = SizeColumnsAdvanced(weights);



	/*
	// add it to libtorrent

	//add
	log(L"add before");
	torrentitem t;
	t = AddTorrent(
		L"C:\\Documents\\test",       // folder
		L"C:\\Documents\\my.torrent", // torrent
		NULL,  // magnet hash
		NULL,  //        name
		NULL,  //        tracker
		NULL); // store
	log(L"add after");

	// add it to the program's list
	Data.torrentlist.push_back(t);

	// add it to the list view

	ListAdd(Handle.list
	*/




}
