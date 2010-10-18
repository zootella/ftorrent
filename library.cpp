
// Include boost
#include "boost/shared_ptr.hpp"
#include "boost/asio/ip/address.hpp"
#include "boost/filesystem/path.hpp"

// Include libtorrent
#include "libtorrent/utf8.hpp"
#include "libtorrent/config.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/peer_info.hpp"
#include "libtorrent/alert.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/peer_id.hpp"
#include "libtorrent/size_type.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/identify_client.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/ip_filter.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/bitfield.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/socket.hpp"

// Include libtorrent extensions
#include "libtorrent/extensions/metadata_transfer.hpp"
#include "libtorrent/extensions/ut_metadata.hpp"
#include "libtorrent/extensions/ut_pex.hpp"
#include "libtorrent/extensions/smart_ban.hpp"

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

/*
Write out a number as text
Use base 10 or 16
Add leading zeros to make the text width numerals, width 0 to leave unchanged

DWORD  4 byte  unsigned
int    4 byte  signed
ubig   8 byte  unsigned
sbig   8 byte  signed
*/
CString numerals(DWORD n, int base, int width) { WCHAR s[MAX_PATH];   _ultow_s(n, s, MAX_PATH, base); return AddLeadingZeroes(s, width); }
CString numerals(int n,   int base, int width) { WCHAR s[MAX_PATH];    _itow_s(n, s, MAX_PATH, base); return AddLeadingZeroes(s, width); }
CString numerals(ubig n,  int base, int width) { WCHAR s[MAX_PATH]; _ui64tow_s(n, s, MAX_PATH, base); return AddLeadingZeroes(s, width); }
CString numerals(sbig n,  int base, int width) { WCHAR s[MAX_PATH];  _i64tow_s(n, s, MAX_PATH, base); return AddLeadingZeroes(s, width); }

CString base16(DWORD n) { return numerals(n, 16,  8); } // 4 bytes written in 8 characters
CString base16(int n)   { return numerals(n, 16,  8); }
CString base16(ubig n)  { return numerals(n, 16, 16); } // 8 bytes written in 16 characters
CString base16(sbig n)  { return numerals(n, 16, 16); }

CString AddLeadingZeroes(CString s, int width) {
	if (width > 0) {
		while (length(s) < width) // Loop until we've put enough 0s at the start to make the whole thing width
			s = L"0" + s;
	}
	return s;
}

// Parse 40 characters of base 16 text into a 20 byte number
hbig ParseHash(read r) {
	std::stringstream stream;
	hbig n;
	stream << narrowRtoS(r).c_str();
	stream >> n;
	return n;
}

// Write out the given 20 byte number as 40 characters of base 16 text
CString base16(hbig n) {
	std::stringstream stream;
	stream << n;
	return widenStoC(stream.str());
}

// The first 4 bytes of the given 20 byte number as a DWORD
DWORD HashStart(hbig n) {

	return
		(((DWORD)n[0]) << 24) |
		(((DWORD)n[1]) << 16) |
		(((DWORD)n[2]) <<  8) |
		 ((DWORD)n[3]);
}

/*
Text

P  const char *     narrow  in
S  std::string      narrow  in  out
R  const wchar_t *  wide    in
W  std::wstring     wide    in  out
C  CString          wide        out
*/
std::string convertPtoS(const char *p) { return p; }
std::wstring convertRtoW(const wchar_t *r) { return r; }
CString convertRtoC(const wchar_t *r) { return r; }
CString convertWtoC(std::wstring w) { return w.c_str(); }

CString widenPtoC(const char *p) { return widenStoW(p).c_str(); } // Popular
CString widenStoC(std::string s) { return widenStoW(s).c_str(); } // Popular
std::wstring widenPtoW(const char *p) { return widenStoW(p); }
std::wstring widenStoW(std::string s) {

	std::wstring w;
	libtorrent::utf8_wchar(s, w);
	return w;
}

std::string narrowRtoS(const wchar_t *r) { return narrowWtoS(r); } // Popular
std::string narrowWtoS(std::wstring w) {

	std::string s;
	libtorrent::wchar_utf8(w, s);
	return s;
}

// Given the text of a torrent infohash, look up and return the libtorrent torrent handle object
libtorrent::torrent_handle FindTorrentHandle(const char *id) {

	hbig hash = ParseHash(widenPtoC(id));
	libtorrent::torrent_handle h = Handle.session->find_torrent(hash);
	return h;
}

// Save bencoded entry e to the file at path, overwriting a file already there
// Returns false on error
bool SaveEntry(read path, const libtorrent::entry &e) {
	try {

		boost::filesystem::wpath p(convertRtoW(path)); // Make a boost path object
		boost::filesystem::ofstream f(p, std::ios_base::binary); // Open a file for writing
		if (f.fail()) { log(L"saveentry fail ", path); return false; }
		f.unsetf(std::ios_base::skipws); // Include whitespace

		libtorrent::bencode(std::ostream_iterator<char>(f), e);  // Serialize the bencoded information to a file

		f.close();
		return true;

	} catch (std::exception &e) {
		log(L"saveentry std exception", widenPtoC(e.what()));
	} catch (...) {
		log(L"saveentry exception");
	}
	return false;
}

// Load the file at path into the given bencoded entry e
// Returns false on error
bool LoadEntry(read path, libtorrent::entry &e) {
	try {

		boost::filesystem::wpath p(convertRtoW(path)); // Make a boost path object
		boost::filesystem::ifstream f(p, std::ios_base::binary); // Open a file for reading
		if (f.fail()) { log(L"loadentry fail ", path); return false; }
		f.unsetf(std::ios_base::skipws); // Include whitespace

		e = libtorrent::bdecode(std::istream_iterator<char>(f), std::istream_iterator<char>()); // Read the contents and bencode them

		f.close();
		return true;

	} catch (std::exception &e) {
		log(L"loadentry std exception ", widenPtoC(e.what()));
	} catch (...) {
		log(L"loadentry exception");
	}
	return false;
}

// Copy the contents of the bencoded file at path to c
// Returns false on error
bool LoadVector(read path, std::vector<char> &c) {
	try {

		boost::filesystem::wpath p(convertRtoW(path)); // Make a boost path object
		boost::filesystem::ifstream f(p, std::ios_base::binary); // Open a file for reading
		if (f.fail()) { log(L"loadvector fail ", path); return false; }
		f.unsetf(std::ios_base::skipws); // Include whitespace

		std::istream_iterator<char> fileiterator(f); // Copy the file contents into c
		std::istream_iterator<char> streamiterator;
		std::copy(fileiterator, streamiterator, std::back_inserter(c));

		f.close();
		return true;

	} catch (std::exception &e) {
		log(L"loadvector std exception", widenPtoC(e.what()));
	} catch (...) {
		log(L"loadvector exception");
	}
	return false;
}

// Start libtorrent and get the session handle
// This takes about 50ms, do it after the window is on the screen but before it is responsive
void LibraryStart() {
	try {

		// Make our libtorrent session object
		Handle.session = new libtorrent::session(
			libtorrent::fingerprint("ftorrent", 0, 1, 0, 0), // Program name and version numbers separated by commas
			std::pair<int, int>(6881, 6999),                 // Pick a port to listen on in this range
			"0.0.0.0",                                       // Use the default network interface
			libtorrent::session::start_default_features | libtorrent::session::add_default_plugins, // Default features and plugins
			libtorrent::alert::all_categories);              // Subscribe to every category of alerts

		// Load session state from settings file
		libtorrent::entry e;
		if (LoadEntry(PathStore(), e)) Handle.session->load_state(e);

		// Tell libtorrent to use all the plugins beyond the defaults
		Handle.session->add_extension(&libtorrent::create_metadata_plugin);    // Magnet links join swarm with just tracker and infohash
		Handle.session->add_extension(&libtorrent::create_ut_metadata_plugin); // Tracker and infohash swarm joining the uTorrent way
		Handle.session->add_extension(&libtorrent::create_ut_pex_plugin);      // Peer exchange
		Handle.session->add_extension(&libtorrent::create_smart_ban_plugin);   // Quickly block peers that send poison data

		// Start libtorrent services
		Handle.session->start_dht();  // Distributed hash table for trackerless torrents
		Handle.session->start_lsd();  // Local service discovery to find peers on the LAN
		Handle.session->start_upnp(); // Universal plug-n-play and NAT-PMP to make a mapping at the router
		Handle.session->start_natpmp();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Stop libtorrent services, pause torrents, and ask for resume alerts for each one
// This call takes about 20ms, and then 250ms later the alerts arrive with the resume data, do it after the window has left the screen
void LibraryStop() {
	try {

		// Stop libtorrent services
		Handle.session->stop_dht();
		Handle.session->stop_lsd();
		Handle.session->stop_upnp();
		Handle.session->stop_natpmp();

		// Pause all the torrents and have automanage not unpause them
		Handle.session->pause();

		// Save all libtorrent state except for the individual torrents
		libtorrent::entry e;
		Handle.session->save_state(e);
		SaveEntry(PathStore(), e);

		// Loop through each torrent handle
		std::vector<libtorrent::torrent_handle> handles = Handle.session->get_torrents();
		for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin(); i != handles.end(); i++) {
			libtorrent::torrent_handle &h = *i;

			// This torrent is initialized and not aborted and has filename and piece hash metadata
			if (h.is_valid() && h.has_metadata()) {
				
				// Tell the torrent to generate resume data
				h.save_resume_data(); // Returns immediately, we'll get the data later after libtorrent gives us an alert
				State.expect++;       // Count that we expect one more torrent will give us resume data
			}
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Shut down libtorrent
// This can be slow, 1 to 4 seconds as libtorrent waits for trackers to confirm our goodbye
void LibraryClose() {
	try {

		delete Handle.session; // Call the libtorrent session object destructor

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// The user clicked to add the torrent file at the given path
// Returns error message text for the user, or blank on success or cancel
CString AddTorrent(read torrent, bool ask) {

	// Parse the torrent file on the disk
	hbig hash;
	CString name;
	std::set<CString> trackers;
	if (!ParseTorrent(torrent, &hash, &name, &trackers))
		return L"Cannot read the torrent file. Check how you saved or downloaded it, and try again."; // Corrupt torrent file

	// Avoid a duplicate
	if (FindTorrent(hash)) {
		AddTrackers(hash, trackers);
		Blink(hash);
		return L""; // Added trackers from duplicate
	}

	// Choose the download folder
	CString folder = Data.folder;
	if (ask) folder = DialogBrowse(make(L"Choose a folder to download '", name, L"'."));
	if (isblank(folder)) return L""; // User canceled the browse dialog
	if (!CheckFolder(folder))
		return L"Cannot save files to the folder at '" + folder + "'. Check the path and try again."; // Can't write to folder

	// Add the torrent file to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddTorrent(&handle, folder, L"", torrent))
		return L"Cannot add this torrent. Check how you saved or downloaded it, and try again."; // libtorrent error

	AddData(handle, folder, name, trackers); // Make a torrent item in data
	AddTrackers(hash, trackers);             // Add trackers to the torrent item and handle
	AddRow(hash);                            // Make a row in the list view
	AddMeta(hash, torrent);                  // Copy the torrent file to "infohash.meta.db"
	AddOption(hash);                         // Save the torrent item to "infohash.optn.db"
	return L""; // Success
}

// The user clicked to add the given magnet link
// Returns error message text for the user, or blank on success or cancel
CString AddMagnet(read magnet, bool ask) {

	// Parse the text of the magnet link
	hbig hash;
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
	if (ask) folder = DialogBrowse(make(L"Choose a folder to download '", name, L"'."));
	if (isblank(folder)) return L""; // User canceled the browse dialog
	if (!CheckFolder(folder))
		return L"Cannot save files to the folder at '" + folder + "'. Check the path and try again."; // Can't write to folder

	// Add the torrent to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddMagnet(&handle, folder, L"", hash, name))
		return L"Cannot add this magnet link. Check the link and try again."; // libtorrent error

	AddData(handle, folder, name, trackers); // Make a torrent item in data
	AddTrackers(hash, trackers);             // Add trackers to the torrent item and handle
	AddRow(hash);                            // Make a row in the list view
	AddOption(hash);                         // Save the torrent item to "infohash.optn.db"
	return L""; // Success
}

// Restore the torrent with the given infohash to this new session from files saved in the previous one
void AddStore(hbig hash) {

	// Read the folder, name, and trackers from the options file
	torrentitem o;
	o.Load(hash);
	if (isblank(o.folder)) return; // Hash and folder are required

	// Parse the torrent file on the disk
	hbig mhash;
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
void AddTrackers(hbig hash, std::set<CString> add) {

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
void Blink(hbig hash) {

	//TODO
}

// Find the torrent with the given infohash in our list, or null if not found
torrentitem *FindTorrent(hbig hash) {

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
void AddRow(hbig hash) {

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
void AddMeta(hbig hash, read torrent) {

	if (same(torrent, PathTorrentMeta(hash), Matching)) return; // Don't copy a file onto itself
	CopyFile(torrent, PathTorrentMeta(hash), true); // True to not overwrite
}

// Have the torrent item with hash in the data list save "infohash.optn.db" next to this running exe
void AddOption(hbig hash) {

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
		if (c.size() > 0) p.resume_data = &c;

		// Set torrent
		p.ti = new libtorrent::torrent_info(boost::filesystem::path(narrowRtoS(torrent))); // Uses boost intrustive pointer

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
bool LibraryAddMagnet(libtorrent::torrent_handle *handle, read folder, read store, hbig hash, read name) {
	try {
		libtorrent::add_torrent_params p; // Object to fill out

		// Set folder
		p.save_path = boost::filesystem::path(narrowRtoS(folder));

		// Set store
		std::vector<char> c;
		if (is(store)) LoadVector(store, c);
		if (c.size() > 0) p.resume_data = &c;

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
bool ParseMagnet(read magnet, hbig *hash, CString *name, std::set<CString> *trackers) {

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
				*hash = ParseHash(a);
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
bool ParseTorrent(read torrent, hbig *hash, CString *name, std::set<CString> *trackers) {
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













void LibraryPulse() {

	// If we're done leaving libtorrent, quit the program message loop
	if (State.exit &&                            // The user began the program exit process, and
		(!State.expect ||                        // Either we've gotten all the resume data we expect, or
		(State.exit + 4000 < GetTickCount()))) { // We've waited more than 4 seconds

		PostQuitMessage(0); // Post the quit message to leave the message loop
		return;             // Don't do anything else here
	}

	/*

	//query the session for information

	//query the torrent handles for progress
	torrent_handle.status() returns torrent_status struct




	*/

	// See if there are any new alerts
	AlertLoop();

}

// Take all the alerts libtorrent is waiting to give us and look at each one
void AlertLoop() {
	try {

		// Get an alert
		std::auto_ptr<libtorrent::alert> p = Handle.session->pop_alert(); // Move an alert from libtorrent to p
		while (p.get()) { // If p contans an alert
			libtorrent::alert *alert = p.get(); // Get it

			// Look at alert, filling info with information about it and copied from it
			AlertLook(alert);

			// Get the next alert
			p = Handle.session->pop_alert(); // Move the next alert from libtorrent to p and loop until libtorrent runs out
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Given a libtorrent alert, fill a structure of info about it
// After calling this function, you can look at the information in info to see the alert libtorrent sent you
void AlertLook(const libtorrent::alert *alert) {

	// If it's a torrent alert
	const libtorrent::torrent_alert *a = dynamic_cast<const libtorrent::torrent_alert *>(alert);
	if (a) {

		// Get the torrent handle and make sure that torrent is initialized and not yet aborted
		libtorrent::torrent_handle h = a->handle;
		if (h.is_valid()) {

			// Get the infohash
			CString id = base16(h.info_hash());

			// If the alert is for save resume data
			const libtorrent::save_resume_data_alert *a1 = dynamic_cast<const libtorrent::save_resume_data_alert *>(alert);
			if (a1) {

				// Get the pointer to the resume data
				const boost::shared_ptr<libtorrent::entry> p = a1->resume_data;
				libtorrent::entry *e = p.get(); // Copy across the pointer to the resume data
				SaveEntry(PathTorrentStore(h.info_hash()), *e);
				State.expect--;
				return;
			}

			// If the alert is for save resume data failed
			const libtorrent::save_resume_data_failed_alert *a2 = dynamic_cast<const libtorrent::save_resume_data_failed_alert *>(alert);
			if (a2) {

				// Get the error message
				log(id, L" save resume failed ", widenStoC(a2->msg));
				State.expect--;
				return;
			}

			// If the alert is for fast resume rejected
			const libtorrent::fastresume_rejected_alert *a3 = dynamic_cast<const libtorrent::fastresume_rejected_alert *>(alert);
			if (a3) {

				// Get the error message
				log(id, L" fast resume rejected ", widenStoC(a3->msg));
				return;
			}
		}
	}
}
