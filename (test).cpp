
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


//TODO now you're thinking that Data.torrents should be a vector that holds pointers to torrentitems
//new and delete are worth being able to return null or a pointer to an object


//TODO make a function that takes a hash value and finds the torrentitem in Data.torrents, false if not found

//if its the default path, consider blanking it in case it changes again




// Run a snippet of test code
void Test() {


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














