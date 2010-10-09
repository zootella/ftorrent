
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

	libtorrent::torrent_handle handle;

	LibraryAddTorrent(
		&handle,
		L"C:\\Documents\\test",
		L"",
		L"C:\\Documents\\my.torrent");



}














// The user clicked to add the torrent file at the given path, or blank to ask the user
// Returns error message text for the user, or blank on cancel or success
CString AddTorrent(CString torrent) {

	// Have the user pick the torrent to open
	if (isblank(torrent)) torrent = DialogOpen(); // Show the file open box to have the user choose a torrent file
	if (isblank(torrent)) return L""; // Canceled the file open dialog

	// Parse the torrent file on the disk
	libtorrent::big_number hash;
	CString name;
	std::set<CString> trackers;
	if (!ParseTorrent(torrent, &hash, &name, &trackers))
		return L"Unable to read the torrent file. Check how you saved or downloaded it, and try again."; // Corrupt torrent file

	// Avoid a duplicate
	if (FindTorrent(hash)) {
		AddTrackers(hash, trackers);
		Blink(hash);
		return L""; // Added trackers from duplicate
	}

	// Choose the download folder
	CString folder = Data.folder;
	if (Data.ask) folder = DialogBrowse(make(L"Choose a folder to download '", name, L"'."));
	if (isblank(folder)) return L""; // User canceled the browse dialog
	if (!CheckFolder(folder))
		return L"Unable to save files to the folder at '" + folder + "'. Check the path and try again."; // Can't write to folder

	// Add the torrent file to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddTorrent(&handle, folder, L"", torrent))
		return L"Cannot add this torrent. Check how you saved or downloaded it, and try again."; // libtorrent error

	AddData(handle, folder, name, trackers); // Make a torrent item in data
	AddTrackers(hash, trackers);             // Add trackers to the torrent item and handle
	AddRow(hash);                            // Make a row in the list view
	AddMeta(hash, torrent);                  // Copy the torrent file to "meta.infohash.db"
	AddOption(hash);                         // Save the torrent item to "optn.infohash.db"
	return L""; // Success
}

// The user clicked to add the given magnet link
// Returns error message text for the user, or blank on cancel or success
CString AddMagnet(read magnet) {

	// Parse the text of the magnet link
	libtorrent::big_number hash;
	CString name;
	std::set<CString> trackers;
	if (!ParseMagnet(magnet, &hash, &name, &trackers))
		return L"Not a valid magnet link. Check the link and try again."; // Inavlid text or missing parts

	// Avoid a duplicate
	if (FindTorrent(hash)) {
		AddTrackers(hash, trackers);
		Blink(hash);
		return L""; // Added trackers from duplicate
	}

	// Choose the download folder
	CString folder = Data.folder;
	if (Data.ask) folder = DialogBrowse(make(L"Choose a folder to download '", name, L"'."));
	if (isblank(folder)) return L""; // User canceled the browse dialog
	if (!CheckFolder(folder))
		return L"Unable to save files to the folder at '" + folder + "'. Check the path and try again."; // Can't write to folder

	// Add the torrent to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddMagnet(&handle, folder, L"", hash, name))
		return L"Cannot add this magnet link. Check the link and try again."; // libtorrent error

	AddData(handle, folder, name, trackers); // Make a torrent item in data
	AddTrackers(hash, trackers);             // Add trackers to the torrent item and handle
	AddRow(hash);                            // Make a row in the list view
	AddOption(hash);                         // Save the torrent item to "optn.infohash.db"
	return L""; // Success
}

// Restore the torrent with the given infohash to this new session from files saved in the previous one
void AddStore(libtorrent::big_number hash) {

	// Read the folder, name, and trackers from the options file
	torrentitem o;
	o.Load(hash);
	if (isblank(o.folder)) return; // Hash and folder are required

	// Parse the torrent file on the disk
	libtorrent::big_number mhash;
	CString mname;
	std::set<CString> mtrackers;
	bool hastorrent = ParseTorrent(PathTorrentMeta(hash), &mhash, &mname, &mtrackers);
	if (hastorrent && hash != mhash) hastorrent = false; // Hash inside must match

	// Avoid a duplicate
	if (FindTorrent(hash)) {
		AddTrackers(hash, o.trackers);
		AddTrackers(hash, mtrackers);
		return; // Added trackers from this duplicate
	}

	// Add to libtorrent
	libtorrent::torrent_handle handle;
	if (hastorrent) {
		if (!LibraryAddTorrent(&handle, o.folder, PathTorrentStore(hash), PathTorrentMeta(hash))) return; // libtorrent error
	} else {
		if (!LibraryAddMagnet(&handle, o.folder, PathTorrentStore(hash), hash, o.name)) return;
	}

	// Add the torrent handle to the data list and window
	AddData(handle, o.folder, o.name, o.trackers); // Make a torrent item in data
	AddTrackers(hash, o.trackers);                 // Add trackers to the torrent item and handle
	AddTrackers(hash, mtrackers);
	AddRow(hash);                                  // Make a row in the list view
}









// Add the given trackers in the add list to both the torrent item and the libtorrent torrent handle
void AddTrackers(libtorrent::big_number hash, std::set<CString> add) {

	// Find the torrent item in the data list
	torrentitem *t = FindTorrent(hash);
	if (!t) return;

	// Insert the given add trackers into the list the given torrentitem keeps
	for (std::set<CString>::const_iterator i = add.begin(); i != add.end(); i++) {
		t->trackers.insert(*i); // The set will keep duplicates out
	}

	// If there are any we haven't told libtorrent about yet, add them there too
	for (std::set<CString>::const_iterator i = t->trackers.begin(); i != t->trackers.end(); i++) {
		if (!LibraryHasTracker(t->handle, *i)) { // Avoid duplicates because libtorrent uses a vector instead of a set
			LibraryAddTracker(t->handle, *i);
		}
	}
}

// True if tracker is already in libtorrent's list of trackers for handle
bool LibraryHasTracker(libtorrent::torrent_handle handle, read tracker) {
	try {

		for (int i = 0; i < (int)handle.trackers().size(); i++) {

			libtorrent::announce_entry a = handle.trackers()[i];
			CString s = widenStoC(a.url);
			if (same(s, tracker)) return true;
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
	return false; // Not found
}

// Add the given tracker to libtorrent's list of trackers for handle
void LibraryAddTracker(libtorrent::torrent_handle handle, read tracker) {
	try {

		libtorrent::announce_entry a(narrowRtoS(tracker));
		handle.add_tracker(a);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}






// Blink the selection of t in the list view to draw the users attention to it
void Blink(libtorrent::big_number hash) {

	//TODO
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



// Copy the given information into a new torrent handle in data
void AddData(libtorrent::torrent_handle handle, read folder, read name, std::set<CString> trackers) {

	// Never add a zero or duplicate hash
	if (handle.info_hash().is_all_zeros() || FindTorrent(handle.info_hash())) return;

	// Make a new empty torrent item and copy in the given information
	torrentitem t;
	t.handle = handle;
	t.folder = folder;
	t.name = name;
	t.trackers = trackers;

	// Copy the local torrent item t into a new one at the end of the program's list
	Data.torrents.push_back(t);
}

// Add a new row to the window's list view control for the torrent item in data with the given hash
void AddRow(libtorrent::big_number hash) {

	// Find the torrent item in data
	torrentitem *t = FindTorrent(hash);
	if (!t) return;

	// Make a new row for it at the bottom of the list view
	ListAdd(
		Handle.list,
		5,
		(LPARAM)t->Hash(),
		t->ComposeStatusIcon(),
		t->ComposeStatus(),
		t->ComposeNameIcon(),
		t->ComposeName(),
		t->ComposeSize(),
		t->ComposeHash(),
		t->ComposePath(),
		L"");
}

// Copy the torrent file at the given path to "meta.infohash.db" next to this running exe if not there already
void AddMeta(libtorrent::big_number hash, read torrent) {

	if (same(torrent, PathTorrentMeta(hash), Matching)) return; // Don't copy a file onto itself
	CopyFile(torrent, PathTorrentMeta(hash), true); // True to not overwrite
}

// Have the torrent item with hash in the data list save "optn.infohash.db" next to this running exe
void AddOption(libtorrent::big_number hash) {

	torrentitem *t = FindTorrent(hash);
	if (!t) return;
	t->Save(); // Overwrite a file already there
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








// Parse the infohash, name, and trackers from the given magnet link
// Returns true with nonzero hash and nonblank name, or false
bool ParseMagnet(read magnet, libtorrent::big_number *hash, CString *name, std::set<CString> *trackers) {

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

			trackers->insert(ReplacePercent(a));
		}
	}

	// See what we found
	if (!foundhash || hash->is_all_zeros()) return false; // Hash cannot be zero
	if (!foundname || isblank(*name))
		*name = L"Untitled " + base16(HashStart(*hash)); // Name never blank
	return true;
}

// Read the infohash and name from the torrent file at the given
// Returns true with nonzero hash and nonblank name, or false
bool ParseTorrent(read torrent, libtorrent::big_number *hash, CString *name, std::set<CString> *trackers) {
	try {

		libtorrent::torrent_info info(boost::filesystem::path(narrowRtoS(torrent)));
		*hash = info.info_hash();
		if (hash->is_all_zeros()) return false; // Hash cannot be zero
		*name = widenPtoC(info.name().c_str());
		if (isblank(*name)) *name = L"Untitled " + base16(HashStart(*hash)); // Name never blank
		for (int i = 0; i < (int)info.trackers().size(); i++)
			trackers->insert(widenPtoC(info.trackers()[i].url.c_str()));
		return true;

	} catch (std::exception &e) {
		log(widenPtoC(e.what())); // Throws an exception like "invalid bencoding"
	} catch (...) {
		log(L"exception");
	}
	return false;
}


