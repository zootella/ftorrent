
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

	// Read the folder, name, and trackers from the options file
	torrentitem o;
	o.Load(hash);
	if (isblank(o.folder)) return; // Only hash and folder are required

	// Parse the torrent file on the disk
	CString torrent = PathTorrentMeta(hash);
	libtorrent::big_number torrenthash;
	CString torrentname;
	std::set<CString> torrenttrackers;
	bool hastorrent = ParseTorrent(torrent, &torrenthash, &torrentname, &torrenttrackers);
	if (hastorrent && hash != torrenthash) hastorrent = false; // Make sure the hash inside matches

	// Avoid a duplicate
	torrentitem *d = FindTorrent(hash);
	if (d) {
		AddTrackers(d, o.trackers);
		AddTrackers(d, torrenttrackers);
		return; // Added trackers from this duplicate
	}

	libtorrent::torrent_handle handle;
	if (hastorrent) {
		if (!LibraryAddTorrent(&handle, folder, PathTorrentStore(hash), torrent)) return; // libtorrent error
	} else {
		if (!LibraryAddMagnet(&handle, folder, PathTorrentStore(hash), hash, name)) return;
	}

	// Add the torrent handle to the data list, window, and make store files
	AddList(user, handle, folder, L"", magnet);

	torrentitem *t = FindTorrent(hash); // Find the torrent we just added
	if (t) {
		AddTrackers(t, o.trackers);
		AddTrackers(t, torrenttrackers);
	}
}




//TODO have teh functions that call these pick torrent and folder before you get here
// have these return error messages, or blank on success, and then the caller can decide to show them or not
// then, you might be able to combine them into one


/*
Add a torrent file

user     true if the user clicked to do this, so we should show message boxes
store    path to libtorrent resume data from this torrent running in a previous session, or blank if this is the first time
folder   path to the save folder, like "C:\Documents\Torrents" without a trailing slash or the name of the torrent folder like "Torrent Name" on the end
torrent  path to the torrent file on the disk, or blank to ask them
*/
void AddTorrent(bool user, CString store, CString folder, CString torrent) {

	// Have the user pick the torrent to open
	if (torrent == L"<ask>") torrent = DialogOpen(); // Show the file open box to have the user choose a torrent file
	if (isblank(torrent)) return; // Canceled the file open dialog

	// Parse the torrent file on the disk
	libtorrent::big_number hash;
	CString name;
	std::set<CString> trackers;
	if (!ParseTorrent(torrent, &hash, &name, &trackers)) {
		Message(user, MB_ICONSTOP | MB_OK, L"Unable to read the infohash. This torrent file may be corrupted. Check how you saved or downloaded it, and try again.");
		return; // Corrupt torrent file
	}

	// Avoid a duplicate
	torrentitem *t = FindTorrent(hash);
	if (t) {
		AddTrackers(t, trackers);
		Blink(user, t);
		return; // Added trackers from duplicate
	}

	// Find the download folder, asking the user if necessary
	if (isblank(folder)) folder = ChooseFolder(name);
	if (isblank(folder)) return; // User canceled browse for folder dialog

	// Make sure we can write in the folder
	if (!CheckFolder(folder)) {
		Message(user, MB_ICONSTOP | MB_OK, L"Unable to save files to the folder at '" + folder + "'. Check the path and try again.");
		return; // Can't write to folder
	}

	// Add the torrent file to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddTorrent(&handle, folder, store, torrent)) return; // libtorrent error

	// Add the torrent to the data list, window, and make store files
	AddList(user, handle, folder, torrent, L"");
	AddStore();
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
	std::set<CString> trackers;
	if (!ParseMagnet(magnet, &hash, &name, &trackers)) return; // Inavlid text or missing parts

	// Avoid a duplicate
	torrentitem *t = FindTorrent(hash);
	if (t) {
		AddTrackers(t, trackers);
		Blink(user, t);
		return; // Added trackers from duplicate
	}

	// Find the download folder, asking the user if necessary
	if (isblank(folder)) folder = ChooseFolder(name);
	if (isblank(folder)) return; // User canceled browse for folder dialog

	// Add the torrent to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddMagnet(&handle, folder, store, hash, name)) return; // libtorrent error
	AddTrackers(t, trackers); // Add the trackers we parsed

	// Add the torrent handle to the data list, window, and make store files
	AddList(user, handle, folder, L"", magnet);
	AddStore();
}



/*
Add a magnet link

user    true if the user clicked to do this, so we should show message boxes
store   path to libtorrent resume data from this torrent running in a previous session, or blank if this is the first time
folder  path to the save folder, like "C:\Documents\Torrents" without a trailing slash or the name of the torrent folder like "Torrent Name" on the end


*/
void AddParts(bool user, CString store, CString folder, libtorrent::big_number hash, CString name, std::set<CString> trackers) {

}









// Add the given new trackers in the add list to both the torrent item and the libtorrent torrent handle
void AddTrackers(torrentitem *t, std::set<CString> add) {

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
void Blink(bool user, torrentitem *t) {

	if (!user) return; // Only do this if the user is here
}

// Find the download folder, asking the user if necessary
// Returns blank if the user cancels the box
CString ChooseFolder(read name) {
	if (Data.ask) return DialogBrowse(make(L"Choose a folder to download '", name, L"'."));
	else          return Data.folder;
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
void AddList(bool user, libtorrent::torrent_handle handle, read folder, read torrent, read magnet) {

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

void AddStore() {


/*
	if (is(torrent));


	//format for a torrent's optn file
	//folder
	//trackers - set of trackers from all previous magnets and torrents



	std::set<CString> s;
	s.insert(

*/

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
	if (!foundhash) return false; // Hash is required
	if (!foundname)
		*name = L"(Untitled)"; // Name is optional
	return true;
}

// Read the infohash and name from the torrent file at the given
bool ParseTorrent(read torrent, libtorrent::big_number *hash, CString *name, std::set<CString> *trackers) {
	try {

		libtorrent::torrent_info info(boost::filesystem::path(narrowRtoS(torrent)));
		*hash = info.info_hash();
		if (hash->is_all_zeros()) return false; // Make sure the hash looks valid
		*name = widenPtoC(info.name().c_str());
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


