
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




// Restore the torrent with the given infohash to this new session from files saved in the previous one
void AddRestore(libtorrent::big_number hash) {

	// Compose paths to the torrent, store, and option files
	CString torrent = PathTorrentMeta(hash);
	CString store   = PathTorrentStore(hash);
	CString option  = PathTorrentOption(hash);
	libtorrent::entry e1, e2, e3;
	if (!LoadEntry(torrent, e1)) torrent = L""; // Blank paths that have corrupted or missing files
	if (!LoadEntry(store,   e2)) store   = L"";
	if (!LoadEntry(option,  e3)) option  = L"";

	// Read the folder path and magnet link from the options file
	CString folder, magnet; // Blank if not found
	if (is(option)) {
		folder = widenStoC(e3[narrowRtoS(L"folder")].string());
		magnet = widenStoC(e3[narrowRtoS(L"magnet")].string());
	}

	// If we have a magnet link, parse the trackers from it
	libtorrent::big_number e4;
	CString e5;
	std::vector<CString> trackers;
	if (is(magnet)) ParseMagnet(magnet, &e4, &e5, &trackers);

	// Add the torrent from last time
	if (is(folder) && is(torrent)) {
		AddTorrent(false, store, folder, torrent);

		// If options also contained a magnet link, add the additional trackers
		torrentitem *t = FindTorrent(hash);
		if (t) LibraryAddTrackers(t->handle, trackers);

	// Add the magnet from last time
	} else if (is(folder) && is(magnet)) {
		AddMagnet(false, store, folder, magnet);
	}
}

/*
Add a torrent file

user     true if the user clicked to do this, so we should show message boxes
store    path to libtorrent resume data from this torrent running in a previous session, or blank if this is the first time
folder   path to the save folder, like "C:\Documents\Torrents" without a trailing slash or the name of the torrent folder like "Torrent Name" on the end
torrent  path to the torrent file on the disk, or blank to ask them
*/
void AddTorrent(bool user, CString store, CString folder, CString torrent) {

	// Have the user pick the torrent to open
	if (isblank(torrent) && user) torrent = DialogOpen(); // Show the file open box to have the user choose a torrent file
	if (isblank(torrent)) return; // Canceled the file open dialog

	// Parse the torrent file on the disk
	libtorrent::big_number hash;
	CString name;
	std::vector<CString> trackers;
	if (!ParseTorrent(torrent, &hash, &name, &trackers)) {
		Message(user, MB_ICONSTOP | MB_OK, L"Unable to read the infohash. This torrent file may be corrupted. Check how you saved or downloaded it, and try again.");
		return; // Corrupt torrent file
	}

	// Avoid a duplicate
	torrentitem *t = FindTorrent(hash);
	if (t) {
		LibraryAddTrackers(t->handle, trackers);
		Blink(user, t);
		return; // Added trackers from duplicate
	}

	// Find the download folder, asking the user if necessary
	if (isblank(folder)) folder = ChooseFolder(user, name);
	if (isblank(folder)) return; // User canceled browse for folder dialog

	// Make sure we can write in the folder
	if (!CheckFolder(folder)) {
		Message(user, MB_ICONSTOP | MB_OK, L"Unable to save files to the folder at '" + folder + "'. Check the path and try again.");
		return; // Can't write to folder
	}

	// Add the torrent file to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddTorrent(&handle, folder, store, torrent)) return; // libtorrent error

	// Add the torrent handle to the data list and window
	AddList(handle);
}

/*
Add a magnet link

user    true if the user clicked to do this, so we should show message boxes
store   path to libtorrent resume data from this torrent running in a previous session, or blank if this is the first time
folder  path to the save folder, like "C:\Documents\Torrents" without a trailing slash or the name of the torrent folder like "Torrent Name" on the end
magnet  text of the magnet link
*/
void AddMagnet(bool user, CString store, CString folder, CString magnet) {

	// Parse the text of the magnet link
	libtorrent::big_number hash;
	CString name;
	std::vector<CString> trackers;
	if (!ParseMagnet(magnet, &hash, &name, &trackers)) return; // Inavlid text or missing parts

	// Avoid a duplicate
	torrentitem *t = FindTorrent(hash);
	if (t) {
		LibraryAddTrackers(t->handle, trackers);
		Blink(user, t);
		return; // Added trackers from duplicate
	}

	// Find the download folder, asking the user if necessary
	if (isblank(folder)) folder = ChooseFolder(user, name);
	if (isblank(folder)) return; // User canceled browse for folder dialog

	// Add the torrent to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddMagnet(&handle, folder, store, hash, name)) return; // libtorrent error
	LibraryAddTrackers(handle, trackers); // Add the trackers we parsed right afterwards

	// Add the torrent handle to the data list and window
	AddList(handle);
}











// Blink the selection of t in the list view to draw the users attention to it
void Blink(bool user, torrentitem *t) {

	if (!user) return; // Only do this if the user is here
}

// Find the download folder, asking the user if necessary
// Returns blank if the user cancels the box
CString ChooseFolder(bool user, read name) {
	if (user && Data.ask) return DialogBrowse(make(L"Choose a folder to download '", name, L"'."));
	return Data.folder;
}

// Find the torrent with the given infohash in our list, or null if not found
torrentitem *FindTorrent(libtorrent::big_number hash) {

	// Loop through all the torrents loaded into the program and library
	for (int i = 0; i < (int)Data.torrents.size(); i++) {
		torrentitem *t = &(Data.torrents[i]); // Point t at the torrentitem that is copied into the list
		if (hash == t->handle.info_hash()) return t; // Compare the 20 byte hash values
	}

	// Not found, you can add hash without creating a duplicate
	return NULL;
}

// Place the given libtorrent handle in the data list and on the screen
void AddList(libtorrent::torrent_handle handle) {

	// Add it to the data list
	torrentitem t;              // Make a new empty torrentitem
	t.handle = handle;          // Add the libtorrent handle
	Data.torrents.push_back(t); // Add the torrentitem to the program's list

	// Add it to the list view
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
		p.ti = &info;

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
void LibraryAddTrackers(libtorrent::torrent_handle handle, std::vector<CString> trackers) {
	try {

		for (int i = 0; i < (int)trackers.size(); i++) {

			libtorrent::announce_entry a(narrowRtoS(trackers[i]));
			handle.add_tracker(a);
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}







// Parse the infohash, name, and trackers from the given magnet link
bool ParseMagnet(read magnet, libtorrent::big_number *hash, CString *name, std::vector<CString> *trackers) {

	// Define tags
	CString tag1 = L"magnet:?";
	CString tag2 = L"urn:btih:";

	// Flags to make true when we've found parts
	bool foundhash = false;
	bool foundname = false;

	// Confirm and remove protocol at the start
	if (!starts(magnet, tag1, Matching)) return false;
	CString s = after(magnet, tag1, Forward, Matching);

	// Loop through each part of the query string
	std::vector<CString> v = words(s, L"&");
	CString b, a;
	for (int i = 0; i < (int)v.size(); i++) {
		split(v[i], L"=", &b, &a);

		// Exact topic, required
		if (same(b, L"xt", Matching) && starts(a, tag2, Matching)) {

			a = after(a, tag2, Forward, Matching);
			if (length(a) == 40) {
				*hash = convertRtoBigNumber(a);
				if (hash->is_all_zeros()) return false; // Make sure the hash looks valid
				foundhash = true;
			}

		// Download name, optional
		} else if (same(b, L"dn", Matching)) {

			*name = ReplacePercent(a);
			foundname = true;

		// Tracker, optional, 1 or several of these
		} else if (same(b, L"tr", Matching)) {

			trackers->push_back(ReplacePercent(a));
		}
	}

	// See what we found
	if (!foundhash) return false; // Hash is required
	if (!foundname)
		*name = L"(Untitled)"; // Name is optional
	return true;
}

// Read the infohash and name from the torrent file at the given
bool ParseTorrent(read torrent, libtorrent::big_number *hash, CString *name, std::vector<CString> *trackers) {
	try {

		libtorrent::torrent_info info(boost::filesystem::path(narrowRtoS(torrent)));
		*hash = info.info_hash();
		if (hash->is_all_zeros()) return false; // Make sure the hash looks valid
		*name = widenPtoC(info.name().c_str());
		for (int i = 0; i < (int)info.trackers().size(); i++)
			trackers->push_back(widenPtoC(info.trackers()[i].url.c_str()));
		return true;

	} catch (std::exception &e) {
		log(widenPtoC(e.what())); // Throws an exception like "invalid bencoding"
	} catch (...) {
		log(L"exception");
	}
	return false;
}


