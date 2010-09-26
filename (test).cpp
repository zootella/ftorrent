
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







void print(std::vector<torrentitem> v) {
	log(L"print:");

	for (int i = 0; i < (int)v.size(); i++) {
		torrentitem *t = &(v[i]);

		log(numerals(t->id), L" is at ", base16((LPARAM)t));
	}
}


//make a function that takes a hash value and finds the torrentitem in Data.torrents, false if not found


// Call when the program starts up
// Reads the optn.db file next to this running exe, and loads values in Data
void LoadOption() {

	libtorrent::entry d;
	if (LoadEntry(PathOption(), d)) { // Loaded

		Data.path = d[narrowRtoS(L"path")];
		log(L"loaded: ", Data.path);

	} else { // Not loaded, use factory defaults

		Data.path = PathDocuments() + L"\\Torrents";
		log(L"factory default: ", Data.path);
	}
}

// Call when the program is shutting down
// Saves values from Data to optn.db next to this running exe
void SaveOption() {

	libtorrent::entry::dictionary_type d;
	d[narrowRtoS(L"path")] = narrowRtoS(Data.path);
	SaveEntry(PathOption(), d);
}



// Run a snippet of test code
void Test() {


	SaveOption();

	LoadOption();

	/*

	m.
	std::map<CString, CString>::iterator i = m.find(L"color");
	if (i != stringCounts.end()) {

		log(i->first, i->second);

	}




	/*
	libtorrent::entry e;
	libtorrent::entry::dictionary_type d = e.dict();
	d.insert(

	libtorrent::entry::dictionary_type::const_iterator i;
	i = dict.find("announce");
	if (i != dict.end()) {

		std::string tracker_url = i->second.string();
		std::cout << tracker_url << "\n";
	}







	/*

	SaveEntry(PathOption(), d);


	/*
	int choice = Dialog(L"DIALOG_REGISTER"); // Enter IDYES, IDNO, or Ask X Escape are IDCANCEL
	log(L"choice is ", numerals(choice));



	/*

	AddTorrent(
		L"C:\\Documents\\test",       // folder
		L"C:\\Documents\\my.torrent", // torrent
		NULL,  // magnet hash
		NULL,  //        name
		NULL,  //        tracker
		NULL); // store file from before

	/*
	// convert some big numbers to text
	sbig n = 0xffffffffffffffff;
	log(L"the signed number in base 10 is ", sbigtoC(n, 10));
	log(L"the signed number in base 16 is ", sbigtoC(n, 16));

	ubig n2 = 0xffffffffffffffff;
	log(L"the unsigned number in base 10 is ", ubigtoC(n2, 10));
	log(L"the unsigned number in base 16 is ", ubigtoC(n2, 16));

	/*
	//let's move 4 unsigned char into a dword

	BYTE c1 = 0x01;
	BYTE c2 = 0x02;
	BYTE c3 = 0x03;
	BYTE c4 = 0x04;

	DWORD d = (((DWORD)c1) << 24) | (((DWORD)c2) << 16) | (((DWORD)c3) << 8) | ((DWORD)c4);

	log(base16(d));

	/*
	std::vector<torrentitem> v;

	torrentitem t1, t2, t3;
	t1.id = 1;
	t2.id = 2;
	t3.id = 3;
	v.push_back(t1);
	v.push_back(t2);
	v.push_back(t3);
	print(v);

	v.erase(v.begin() + 1);
	print(v);
	*/

	/*
	ListAdd(Handle.list, 5, (LPARAM)0xaabbccdd, Handle.icon.clear, L"a", Handle.icon.clear, L"bb", L"ccc", L"dddd", L"eeeee", NULL);

	log(ListText(Handle.list, 0, 0));
	log(ListText(Handle.list, 0, 1));
	log(ListText(Handle.list, 0, 2));
	log(ListText(Handle.list, 0, 3));
	log(ListText(Handle.list, 0, 4));
	*/



}




/*
void LoadOption() {

	/*

	CString option = PathOption();

	libtorrent::entry e;
	e.construct(dictionary_type);

	LoadEntry(PathOption(), e));

	e.dict(


	Data.path = L"C:";
	*/

/*
}
*/

/*

void SaveOption() {

	libtorrent::entry::string_type name, value;
	name = narrowRtoS(L"path");
	value = narrowRtoS(L"documents");

	libtorrent::entry::dictionary_type d;
	d.insert(name, value);




}

*/
