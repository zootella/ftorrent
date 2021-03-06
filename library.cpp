
#include "heavy.h"
extern app App; // Access global object

/*
Write out a number as text
Use base 10 or 16
Add leading zeros to make the text width numerals, width 0 to leave unchanged

DWORD  4 byte  unsigned
int    4 byte  signed
ubig   8 byte  unsigned
sbig   8 byte  signed
*/
CString numerals(DWORD n, int base, int width) { WCHAR bay[MAX_PATH]; copyr(bay, MAX_PATH, L"");   _ultow_s(n, bay, MAX_PATH, base); return AddLeadingZeroes(bay, width); }
CString numerals(int n,   int base, int width) { WCHAR bay[MAX_PATH]; copyr(bay, MAX_PATH, L"");    _itow_s(n, bay, MAX_PATH, base); return AddLeadingZeroes(bay, width); }
CString numerals(ubig n,  int base, int width) { WCHAR bay[MAX_PATH]; copyr(bay, MAX_PATH, L""); _ui64tow_s(n, bay, MAX_PATH, base); return AddLeadingZeroes(bay, width); }
CString numerals(sbig n,  int base, int width) { WCHAR bay[MAX_PATH]; copyr(bay, MAX_PATH, L"");  _i64tow_s(n, bay, MAX_PATH, base); return AddLeadingZeroes(bay, width); }

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
LPARAM ClipHash(hbig n) {

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

// Number of characters in the text, not including null terminator, like "abc\0" is 3
int lengthp(const char *p) { return lstrlenA(p); }
int lengthr(const wchar_t *r) { return lstrlenW(r); } // Use instead of lstrlen

// Copy the narrow text and null terminator characters at source to the memory location destination that can hold capacity narrow charcters
// For example, char bay[4]; copyp(bay, 4, "abc"); is just enough space and copies 4 bytes
void copyp(char *destination, int capacity, const char *source) {
	if (strcpy_s(destination, capacity, source)) error(L"strcpy_s");
}

// Copy the wide text and null terminator characters at source to the memory location destination that can hold capacity wide characters
// For example, WCHAR bay[4]; copyr(bay, 4, L"abc"); is just enough space and copies 8 bytes
void copyr(wchar_t *destination, int capacity, const wchar_t *source) { // Use instead of lstrcpy
	if (wcscpy_s(destination, capacity, source)) error(L"wcscpy_s");
}

// Given the text of a torrent infohash, look up and return the libtorrent torrent handle
libtorrent::torrent_handle FindTorrentHandle(const char *id) {

	hbig hash = ParseHash(widenPtoC(id));
	libtorrent::torrent_handle h = App.session->find_torrent(hash);
	return h;
}

// Save bencoded entry e to the file at path, overwriting a file already there
// Returns false on error
bool SaveEntry(read path, const libtorrent::entry &e) {
	try {

		boost::filesystem::wpath p(convertRtoW(path)); // Make a boost path
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

		boost::filesystem::wpath p(convertRtoW(path)); // Make a boost path
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

		boost::filesystem::wpath p(convertRtoW(path)); // Make a boost path
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

		// Make our libtorrent session
		App.session = new libtorrent::session(
			libtorrent::fingerprint("ftorrent", 0, 1, 0, 0), // Program name and version numbers separated by commas
			std::pair<int, int>(6881, 6999),                 // Pick a port to listen on in this range
			"0.0.0.0",                                       // Use the default network interface
			libtorrent::session::start_default_features | libtorrent::session::add_default_plugins, // Default features and plugins
			libtorrent::alert::all_categories);              // Subscribe to every category of alerts

		// Load session state from settings file
		libtorrent::entry e;
		if (LoadEntry(PathStore(), e)) App.session->load_state(e);

		// Tell libtorrent to use all the plugins beyond the defaults
		App.session->add_extension(&libtorrent::create_metadata_plugin);    // Magnet links join swarm with just tracker and infohash
		App.session->add_extension(&libtorrent::create_ut_metadata_plugin); // Tracker and infohash swarm joining the uTorrent way
		App.session->add_extension(&libtorrent::create_ut_pex_plugin);      // Peer exchange
		App.session->add_extension(&libtorrent::create_smart_ban_plugin);   // Quickly block peers that send poison data

		// Set libtorrent settings
		libtorrent::session_settings s;           // Make a new libtorrent session settings object
		s.stop_tracker_timeout = EXIT_TIME_LIMIT; // Wait 2 seconds instead of 5 for trackers to respond to our goodbye when the program exits
		App.session->set_settings(s);             // Apply the settings to libtorrent

		// Start libtorrent services
		App.session->start_dht();  // Distributed hash table for trackerless torrents
		App.session->start_lsd();  // Local service discovery to find peers on the LAN
		App.session->start_upnp(); // Universal plug-n-play and NAT-PMP to make a mapping at the router
		App.session->start_natpmp();

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
		App.session->stop_dht();
		App.session->stop_lsd();
		App.session->stop_upnp();
		App.session->stop_natpmp();

		// Pause all the torrents and have automanage not unpause them
		App.session->pause();

		// Save all libtorrent state except for the individual torrents
		libtorrent::entry e;
		App.session->save_state(e);
		SaveEntry(PathStore(), e);

		// Loop through each torrent handle
		std::vector<libtorrent::torrent_handle> handles = App.session->get_torrents();
		for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin(); i != handles.end(); i++) {
			libtorrent::torrent_handle &h = *i;

			// This torrent is initialized and not aborted and has filename and piece hash metadata
			if (h.is_valid() && h.has_metadata()) {

				// Tell the torrent to generate resume data
				h.save_resume_data(); // Returns immediately, we'll get the data later after libtorrent gives us an alert
				App.cycle.expect++;   // Count that we expect one more torrent will give us resume data
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

		delete App.session; // Call the libtorrent session destructor

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

/*
bool LibraryTorrentIsPaused(libtorrent::torrent_handle handle) {
	try {

		return handle.is_paused();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}

	return true;
}

void LibraryTorrentPause(libtorrent::torrent_handle handle) {
	try {

		handle.pause();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void LibraryTorrentResume(libtorrent::torrent_handle handle) {
	try {

		handle.resume();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}
*/






//TODO
CString CreateTorrent(read path, bool ask) {
	return L"";
}
CString DownloadTorrent(read address, bool ask) {

	//TODO the user could also be downloading a skin
	WebDownload(address);

	return L"";
}

// Remove the torrent with the given infohash from the program, true to also delete the downloaded files
void RemoveTorrent(Torrent *t, bool deletefiles) {

	// Copy identifying information from the torrent object to local variables before we disconnect and remove the object
	hbig hash = t->handle.info_hash();
	libtorrent::torrent_handle handle = t->handle;

	// Delete this torrent's disk files from next to this running exe
	DiskDeleteFile(PathTorrentMeta(hash));
	DiskDeleteFile(PathTorrentStore(hash));
	DiskDeleteFile(PathTorrentOption(hash));

	// Remove this torrent's row in the list view control in the window
	int row = ListFind(App.list.torrents.window, ClipHash(hash));
	if (row != -1) ListRemove(App.list.torrents.window, row);

	// Remove this torrent object from our list of them, and delete this object
	int index = -1;
	for (int i = 0; i < (int)App.torrents.size(); i++) { // Loop through our list of torrents to find this one
		Torrent *u = &(App.torrents[i]);                 // Point t at the Torrent that is in the list
		if (hash == u->handle.info_hash()) index = i;    // Compare the 20 byte hash values
	}
	if (index == -1) log(L"removetorrent not found");
	if (index != -1) App.torrents.erase(App.torrents.begin() + index); // Remove the object from the vector, call its destructor, and free its memory

	// Remove this torrent from the current libtorrent session
	LibraryRemoveTorrent(handle, deletefiles);
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
		AddTrackers(hash, trackers); // Add trackers to the torrent object and libtorrent handle
		Blink(hash);
		return L""; // Added trackers from duplicate
	}

	// Choose the download folder
	CString folder = App.option.folder;
	if (ask) folder = DialogBrowse(make(L"Choose a folder to download '", name, L"'."));
	if (isblank(folder)) return L""; // User canceled the browse dialog
	if (!CheckFolder(folder))
		return L"Cannot save files to the folder at '" + folder + "'. Check the path and try again."; // Can't write to folder

	// Add the torrent file to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddTorrent(&handle, folder, L"", torrent, false)) // Add not paused
		return L"Cannot add this torrent. Check how you saved or downloaded it, and try again."; // libtorrent error

	AddData(handle, folder, name, trackers, false); // Make a torrent object in our list of them
	AddTrackers(hash, trackers);                    // Add trackers to the torrent object and libtorrent handle
	AddRow(hash);                                   // Make a row in the list view
	AddMeta(hash, torrent);                         // Copy the torrent file to "infohash.meta.db"
	AddOption(hash);                                // Save torrent options like name and trackers to "infohash.optn.db"
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
		AddTrackers(hash, trackers); // Add trackers to the torrent object and libtorrent handle
		Blink(hash);
		return L""; // Added trackers from duplicate
	}

	// Choose the download folder
	CString folder = App.option.folder;
	if (ask) folder = DialogBrowse(make(L"Choose a folder to download '", name, L"'."));
	if (isblank(folder)) return L""; // User canceled the browse dialog
	if (!CheckFolder(folder))
		return L"Cannot save files to the folder at '" + folder + "'. Check the path and try again."; // Can't write to folder

	// Add the torrent to the libtorrent session
	libtorrent::torrent_handle handle;
	if (!LibraryAddMagnet(&handle, folder, L"", hash, name, false)) // Add not paused
		return L"Cannot add this magnet link. Check the link and try again."; // libtorrent error

	AddData(handle, folder, name, trackers, false); // Make a torrent object in our list of them
	AddTrackers(hash, trackers);                    // Add trackers to the torrent object and libtorrent handle
	AddRow(hash);                                   // Make a row in the list view
	AddOption(hash);                                // Save torrent options like name and trackers to "infohash.optn.db"
	return L""; // Success
}

// Restore the torrent with the given infohash to this new session from files saved in the previous one
void AddStore(hbig hash) {

	// Read the folder, name, and trackers from the options file
	Torrent o;
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
		AddTrackers(hash, o.trackers); // Add trackers to the torrent object and libtorrent handle
		AddTrackers(hash, mtrackers);
		return; // Added trackers from this duplicate
	}

	// Add to libtorrent
	libtorrent::torrent_handle handle;
	if (hastorrent) {
		if (!LibraryAddTorrent(&handle, o.folder, PathTorrentStore(hash), PathTorrentMeta(hash), o.paused)) return; // libtorrent error
	} else {
		if (!LibraryAddMagnet(&handle, o.folder, PathTorrentStore(hash), hash, o.name, o.paused)) return;
	}

	// Add the torrent handle to the data list and window
	AddData(handle, o.folder, o.name, o.trackers, o.paused); // Make a torrent object in our list of them
	AddTrackers(hash, o.trackers);                           // Add trackers to the torrent object and libtorrent handle
	AddTrackers(hash, mtrackers);
	AddRow(hash);                                            // Make a row in the list view
}

// Add the given trackers in the add list to both the torrent in data and the torrent handle in libtorrent
void AddTrackers(hbig hash, std::set<CString> add) {

	// Find the torrent in the data list
	Torrent *t = FindTorrent(hash);
	if (!t) return;

	// Insert the given add trackers into the list the given Torrent keeps
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

// Remove the given torrent handle from the current libtorrent session, and optionally also delete all the saved files on the disk
bool LibraryRemoveTorrent(libtorrent::torrent_handle handle, bool deletefiles) {
	try {

		// Pause, remove, and optionally delete the torrent
		handle.pause();
		if (deletefiles) App.session->remove_torrent(handle, libtorrent::session::delete_files);
		else             App.session->remove_torrent(handle);

		return true;
	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
	return false;
}

// Blink the selection of the torrent with the given hash in the list view to draw the users attention to it
void Blink(hbig hash) {

	//TODO
}

// Find the torrent object underneath the given row number in the torrents list view control, or null if not found
Torrent *ListGetTorrent(int row) {

	// Find the parameter under that row, the first 4 bytes of the torrent infohash
	LPARAM p = ListGet(App.list.torrents.window, row);
	if (!p) { log(L"listgettorrent parameter not found"); return NULL; }

	// Loop through all the torrents loaded into the program and library
	for (int i = 0; i < (int)App.torrents.size(); i++) {
		Torrent *t = &(App.torrents[i]); // Point t at the Torrent that is in the list
		if (p == ClipHash(t->handle.info_hash())) return t; // Compare the first 4 bytes of the 20 byte hash values
	}

	// Not found
	log(L"listgettorrent torrent not found");
	return NULL;
}

// Find the torrent with the given infohash in our list, or null if not found
Torrent *FindTorrent(hbig hash) {

	// Loop through all the torrents loaded into the program and library
	for (int i = 0; i < (int)App.torrents.size(); i++) {
		Torrent *t = &(App.torrents[i]); // Point t at the Torrent that is in the list
		if (hash == t->handle.info_hash()) return t; // Compare the 20 byte hash values
	}

	// Not found
	return NULL;
}

// Copy the given information into a new torrent object in our data list of them
void AddData(libtorrent::torrent_handle handle, read folder, read name, std::set<CString> trackers, bool paused) {

	// Never add a zero or duplicate hash
	if (handle.info_hash().is_all_zeros() || FindTorrent(handle.info_hash())) return;

	// Make a new empty torrent and copy in the given information
	Torrent t;
	t.handle = handle;
	t.folder = folder;
	t.name = name;
	t.trackers = trackers;
	t.paused = paused;

	// Copy the local torrent t into a new one at the end of the program's list
	App.torrents.push_back(t);
}

// Add a new row to the window's list view control for the torrent in data with the given hash
void AddRow(hbig hash) {

	// Find the torrent in data
	Torrent *t = FindTorrent(hash);
	if (!t) return;

	// Add a row of cells in the list view window
	t->Compose();
	CellShow(App.list.torrents.window, t->cells);
}

// Copy the torrent file at the given path to "infohash.meta.db" next to this running exe if not there already
void AddMeta(hbig hash, read torrent) {

	if (same(torrent, PathTorrentMeta(hash), Matching)) return; // Don't copy a file onto itself
	DiskCopyFile(torrent, PathTorrentMeta(hash), false); // Don't overwrite
}

// Have the torrent with hash in the data list save "infohash.optn.db" next to this running exe
void AddOption(hbig hash) {

	Torrent *t = FindTorrent(hash);
	if (!t) return;
	t->Save(); // Overwrite a file already there
}

// Add a torrent to the libtorrent session from a torrent file on the disk
bool LibraryAddTorrent(libtorrent::torrent_handle *handle, read folder, read store, read torrent, bool paused) {
	try {
		libtorrent::add_torrent_params p; // Object to fill out

		// Add paused or not
		p.override_resume_data = true; // Get paused and auto managed from add torrent params, not resume data
		if (paused) {
			p.paused = true;
			p.auto_managed = false;
		} else {
			p.paused = true; // Let libtorrent choose to start this torrent or another one
			p.auto_managed = true;
		}

		// Set folder
		p.save_path = boost::filesystem::path(narrowRtoS(folder));

		// Set store
		std::vector<char> c;
		if (is(store)) LoadVector(store, c);
		if (c.size() > 0) p.resume_data = &c;

		// Set torrent
		p.ti = new libtorrent::torrent_info(boost::filesystem::path(narrowRtoS(torrent))); // Uses boost intrustive pointer

		// Add and save handle
		*handle = App.session->add_torrent(p);
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
bool LibraryAddMagnet(libtorrent::torrent_handle *handle, read folder, read store, hbig hash, read name, bool paused) {
	try {
		libtorrent::add_torrent_params p; // Object to fill out

		// Add paused or not
		p.override_resume_data = true; // Get paused and auto managed from add torrent params, not resume data
		if (paused) {
			p.paused = true;
			p.auto_managed = false;
		} else {
			p.paused = true; // Let libtorrent choose to start this torrent or another one
			p.auto_managed = true;
		}

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
		*handle = App.session->add_torrent(p);
		if (!handle->is_valid()) { log(L"invalid add magnet"); return false; }

		return true;
	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
	return false;
}

// Compose the given infohash, torrent name, and list of trackers into the text of a magnet link
CString ComposeMagnet(hbig hash, read name, std::set<CString> trackers) {

	CString s = L"magnet:?";;
	s += make(L"xt=urn:btih:", base16(hash));
	if (is(name)) s += make(L"&dn=", UriEncode(name));
	for (std::set<CString>::const_iterator i = trackers.begin(); i != trackers.end(); i++)
		s += make(L"&tr=", UriEncode(*i));
	return s;
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

			*name = UriDecode(a);
			foundname = true;

		// Tracker, optional, 1 or several of these
		} else if (same(b, L"tr", Matching)) {

			trackers->insert(UriDecode(a));
		}
	}

	// See what we found
	if (!foundhash || hash->is_all_zeros()) return false; // Hash cannot be zero
	if (!foundname || isblank(*name))
		*name = L"Untitled " + base16(ClipHash(*hash)); // Name never blank
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
		if (isblank(*name)) *name = L"Untitled " + base16(ClipHash(*hash)); // Name never blank
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
	if (App.cycle.exit &&                            // The user began the program exit process, and
		(!App.cycle.expect ||                        // Either we've gotten all the resume data we expect, or
		(App.cycle.exit + 4000 < GetTickCount()))) { // We've waited more than 4 seconds

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
		std::auto_ptr<libtorrent::alert> p = App.session->pop_alert(); // Move an alert from libtorrent to p
		while (p.get()) { // If p contans an alert
			libtorrent::alert *alert = p.get(); // Get it

			// Look at alert, filling info with information about it and copied from it
			AlertLook(alert);

			// Get the next alert
			p = App.session->pop_alert(); // Move the next alert from libtorrent to p and loop until libtorrent runs out
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
				App.cycle.expect--;
				return;
			}

			// If the alert is for save resume data failed
			const libtorrent::save_resume_data_failed_alert *a2 = dynamic_cast<const libtorrent::save_resume_data_failed_alert *>(alert);
			if (a2) {

				// Get the error message
				log(id, L" save resume failed ", widenStoC(a2->msg));
				App.cycle.expect--;
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
