
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

// Run a snippet of test code
void Test() {

}



//TODO add
//factor all add paths in for open, link, and restore session to branch together
//show the user messages while you can prove he's there
//get hashes as early as possible and gracefully deal with duplicates
//deal with a first add pops a message and a second add arrives




void AddTorrent(bool user, read torrent) {

	if (!torrent) {



	}




}

void AddMagnet(bool user, read magnet) {



}

void AddRestore(bool user, libtorrent::big_number hash) {



}



// The user clicked Tools, Open
void CommandOpen() {

	// Show the file open box to have the user choose a torrent file
	CString path = DialogOpen();
	if (isblank(path)) return; // Canceled the file open dialog

	// With a path, what remains is the same as if the user double-clicked the torrent to open it
	CommandPath(path);
}

// The user clicked to open the torrent file at path
void CommandPath(read path) {

	// Read the infohash and name from the torrent file on the disk
	libtorrent::big_number hash;
	CString name;
	if (!LookPath(path, &hash, &name)) {
		Message(MB_ICONSTOP | MB_OK, L"Unable to read the infohash. This torrent file may be corrupted. Check how you saved or downloaded it, and try again.");
		return;
	}

	// If the ask option is checked, ask the user where to download the torrent
	CString folder;
	if (Data.ask) folder = DialogBrowse(L"Choose a folder to download '" + name + L"'.");
	else          folder = Data.folder;

	// Make sure we can write in the folder
	if (!CheckFolder(folder)) {
		Message(MB_ICONSTOP | MB_OK, L"Unable to save files to the folder at '" + folder + "'. Check the path and try again.");
		return;
	}

	// What remains is the same as restoring torrents from the last session
	EnterTorrent(folder, path); // Now we can't show messages to the user anymore
}










//program loads stuff from last time
void CommandRestore(libtorrent::big_number hash) {



	CString pathmeta = PathTorrentMeta(hash);
	CString pathstore = PathTorrentStore(hash);
	CString pathoption = PathTorrentOption(hash);


	libtorrent::entry entrymeta, entrystore, entryoption;
	bool hasmeta = LoadEntry(pathmeta, entrymeta);
	bool hasstore = LoadEntry(pathstore, entrystore);
	bool hasoption = LoadEntry(pathoption, entryoption);



	read folder = NULL;
	read torrent = NULL;
	read hash = NULL;
	read name = NULL;
	std::vector<CString> trackers;
	read store = NULL;


	if (true) {

		EnterTorrent(hash, path, folder);



	} else {

		EnterMagnet(link);



	}



//	Add(read folder, read torrent, read hash, read name, std::vector<CString> trackers, read store) {









}




bool EnterTorrent(read folder, libtorrent::big_number hash, read path) {

	if (FindTorrent(hash)) { // Duplicate



		return true;
	}

	if (!LibraryAddTorrent(&handle, folder, L"", path)) return false;


	AddToList(handle);


	return true;
}

bool EnterMagnet(read folder, read link) {

	libtorrent::big_number hash;
	CString name;
	std::vector<CString> trackers;
	
	if (!LookLink(link, &hash, &name, &trackers)) return false;

	if (FindTorrent(hash)) { // Duplicate

		return true;
	}

	if (!LibraryAddMagnet(&handle, folder, L"", hash, name)) return false;

	for (int i = 0; i < (int)trackers.size(); i++) {
		if (!LibraryAddTracker(handle, trackers[i])) { return false };
	}

	AddToList(handle);

	read tracker = NULL;
	Add(Data.folder, NULL, hash, name, tracker, NULL);
}





// Find the torrent with the given infohash in our list, or null if not a duplicate
torrentitem *FindTorrent(libtorrent::big_number hash) {

	// Loop through all the torrents loaded into the program and library
	for (int i = 0; i < (int)Data.torrents.size(); i++) {
		torrentitem *t = &(Data.torrents[i]); // Point t at the torrentitem that is copied into the list
		if (hash == t->handle.info_hash()) return t; // Compare the 20 byte hash values
	}

	// Not found, you can add hash without creating a duplicate
	return NULL;
}





void AddToList(libtorrent::torrent_handle handle) {

	torrentitem t(handle);
	Data.torrents.push_back(t);

	// add it to the list view
	ListAdd(
		Handle.list,
		5,
		(LPARAM)t.Hash(),
		t.ComposeStatusIcon(),
		t.ComposeStatus(),
		t.ComposeNameIcon(),
		t.ComposeName(),
		t.ComposeSize(),
		t.ComposeHash(),
		t.ComposePath(),
		L"");
}




// Add a torrent to the libtorrent session from a torrent file on the disk
bool LibraryAddTorrent(libtorrent::torrent_handle *handle, read folder, read store, read torrent) {
	try {
		libtorrent::add_torrent_params p; // Object to fill out

		// Set folder
		p.save_path = boost::filesystem::path(narrowRtoS(folder));

		// Set store
		std::vector<char> c;
		if (is(store)) LoadVector(store, c);
		p.resume_data = &c;

		// Set torrent
		libtorrent::torrent_info info(boost::filesystem::path(narrowRtoS(torrent)));
		p.ti = info;

		// Add and save handle
		*handle = Handle.session->add_torrent(p);
		if (!handle->is_valid()) { log(L"invalid add torrent"); return false; }

		return true;
	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
	return false;
}

// Add a torrent to the libtorrent session from information parsed from a magnet link
bool LibraryAddMagnet(libtorrent::torrent_handle *handle, read folder, read store, libtorrent::big_number hash, read name) {
	try {
		libtorrent::add_torrent_params p; // Object to fill out

		// Set folder
		p.save_path = boost::filesystem::path(narrowRtoS(folder));

		// Set store
		std::vector<char> c;
		if (is(store)) LoadVector(store, c);
		p.resume_data = &c;

		// Set hash
		p.info_hash = hash;

		// Set name
		std::string n = narrowRtoS(name);
		p.name = n.c_str();

		// Add and save handle
		*handle = Handle.session->add_torrent(p);
		if (!handle->is_valid()) { log(L"invalid add magnet"); return false; }

		return true;
	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
	return false;
}

// Add the given tracker to the given torrent in the libtorrent session
bool LibraryAddTracker(libtorrent::torrent_handle handle, read tracker) {
	try {

		// Add tracker
		libtorrent::announce_entry a(narrowRtoS(tracker));
		handle->add_tracker(a);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}







