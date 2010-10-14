
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








//TODO now you're thinking that Data.torrents should be a vector that holds pointers to torrentitems
//new and delete are worth being able to return null or a pointer to an object
//maybe not, just use Find() and Get() in order and it's the same thing


//TODO make a function that takes a hash value and finds the torrentitem in Data.torrents, false if not found







/*
// Run a snippet of test code
void Test() {

	/*
	CString path = L"C:\\Documents\\no.doc";
	libtorrent::big_number hash;
	CString name;
	bool result = Parse Torrent(torrent, &hash, &name);
	if (!result) { log(L"false"); return; }
	log(L"hash: ", convertBigNumberToC(hash));
	log(L"name: ", name);



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

	Add Torrent(
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



}
*/




void StorePulse() {

	// Only do this once
	if (State.restored) return;
	State.restored = true;

	// Add all the torrents from last time the program ran
	std::set<libtorrent::big_number> hashes;
	finditem f(PathRunningFolder());
	while (f.result()) { // Loop for each file in the folder this exe is running in
		if (!f.folder()) {
			CString s = f.info.cFileName;
			if (length(s) == length(L"optn.") + 40 + length(L".db") && // Look for "optn.infohash.db"
				starts(s, L"optn.", Matching) && trails(s, L".db", Matching)) {

				s = clip(s, length(L"optn."), 40);
				libtorrent::big_number hash = convertRtoBigNumber(s);
				if (!hash.is_all_zeros()) hashes.insert(hash); // Only collect unique nonzero hashes
			}
		}
	}
	for (std::set<libtorrent::big_number>::const_iterator i = hashes.begin(); i != hashes.end(); i++) AddStore(*i);

	// Add the torrent or magnet the system launched this program with
	CString s = State.command;
	if (starts(s, L"\"")) { // Parse the command like ["C:\Folder\file.torrent" /more /arguments]
		s = after(s, L"\"");
		if (has(s, L"\"")) {
			s = before(s, L"\"");
			if (starts(s, L"magnet:", Matching)) AddMagnet(s, false); // Look for magnet first because link text might also end torrent
			else if (trails(s, L".torrent", Matching)) AddTorrent(s, false);
		}
	}
}




void ListPulse() {



	for (int i = 0; i < (int)Data.torrents.size(); i++)
		Data.torrents[i].Edit();




}


// Edit the list view row to match the information in this torrent item
void torrentitem::Edit() {

	// Update the cells that have different text
	ListEdit(
		Handle.list,
		5,
		(LPARAM)Hash(),
		ComposeStatusIcon(),
		ComposeStatus(),
		ComposeNameIcon(),
		ComposeName(),
		ComposeSize(),
		ComposeHash(),
		ComposePath(),
		L"");
}







int torrentitem::ComposeStatusIcon() {
	return Handle.icon.clear;
}

CString torrentitem::ComposeStatus() {
	return L"status text";
}

int torrentitem::ComposeNameIcon() {
	return Handle.icon.clear;
}

// This torrent's name
CString torrentitem::ComposeName() {
	return widenStoC(handle.name());
}

CString torrentitem::ComposeSize() {

	sbig done = handle.status().total_done; // libtorrent::size_type and sbig are both __int64
	sbig size = handle.get_torrent_info().total_size();

	return make(InsertCommas(sbigtoC(done)), L"/", InsertCommas(sbigtoC(size)), L" bytes");
}

// This torrent's infohash in base 16
CString torrentitem::ComposeHash() {

	return convertBigNumberToC(handle.info_hash());
}

CString torrentitem::ComposePath() {
	return L"path text";
}

// The first 4 bytes of the infohash in a DWORD
DWORD torrentitem::Hash() {

	return HashStart(handle.info_hash());
}




// Save the folder, name, and trackers in this torrent item to a file like "optn.infohash.db" next to the running exe
void torrentitem::Save() {

	// Copy the torrent item's set into a libtorrent bencoded list
	libtorrent::entry::list_type l;
	for (std::set<CString>::const_iterator i = trackers.begin(); i != trackers.end(); i++)
		l.push_back(narrowRtoS(*i));

	// Make and fill the bencoded dictionary
	libtorrent::entry::dictionary_type d;
	d[narrowRtoS(L"folder")] = narrowRtoS(folder);
	d[narrowRtoS(L"name")] = narrowRtoS(name);
	d[narrowRtoS(L"trackers")] = l;

	// Save it to disk
	SaveEntry(PathTorrentOption(handle.info_hash()), d);
}

// Given an infohash, load the folder, name, and trackers from "optn.infohash.db" to this torrent item
bool torrentitem::Load(libtorrent::big_number hash) {

	// Look for a file named with the given hash, and read the bencoded data inside
	libtorrent::entry d;
	if (!LoadEntry(PathTorrentOption(hash), d)) return false;

	// Load the folder path and name into this torrent item
	folder = widenStoC(d[narrowRtoS(L"folder")].string()); // Not found returns blank
	name = widenStoC(d[narrowRtoS(L"name")].string());

	// Load in the list of trackers
	trackers.clear();
	libtorrent::entry::list_type l = d[narrowRtoS(L"trackers")].list();
	for (libtorrent::entry::list_type::const_iterator i = l.begin(); i != l.end(); i++)
		trackers.insert(widenStoC(i->string()));
	return true;
}









