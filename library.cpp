
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

// Text

// P  const char *     narrow  in
// S  std::string      narrow  in  out
// R  const wchar_t *  wide    in
// W  std::wstring     wide    in  out
// C  CString          wide        out

std::string convertPtoS(const char *p) { return p; }
std::wstring convertRtoW(const wchar_t *r) { return r; }
CString convertRtoC(const wchar_t *r) { return r; }
CString convertWtoC(std::wstring w) { return w.c_str(); }

CString widenPtoC(const char *p) { return widenStoW(p).c_str(); }
CString widenStoC(std::string s) { return widenStoW(s).c_str(); }
std::wstring widenPtoW(const char *p) { return widenStoW(p); }
std::wstring widenStoW(std::string s) {

	std::wstring w;
	libtorrent::utf8_wchar(s, w);
	return w;
}

std::string narrowRtoS(const wchar_t *r) { return narrowWtoS(r); }
std::string narrowWtoS(std::wstring w) {

	std::string s;
	libtorrent::wchar_utf8(w, s);
	return s;
}


// Hashes

// Convert a 20 byte hash value between
// base 16 text like "e6a56670baae316ebf5d3ce91be729e8688f7256" and
// libtorrent::big_number and libtorrent::sha1_hash which are the same thing

libtorrent::big_number convertRtoBigNumber(read r) { return convertPtoBigNumber(narrowRtoS(r).c_str()); }
libtorrent::big_number convertPtoBigNumber(const char *p) {

	std::stringstream stream;
	libtorrent::big_number n;
	stream << p;
	stream >> n;
	return n;
}

CString convertBigNumberToC(const libtorrent::big_number &n) {

	std::stringstream stream;
	stream << n;
	return widenStoC(stream.str());
}

// The first 4 bytes of the given hash as a DWORD
DWORD HashStart(libtorrent::big_number hash) {

	return
		(((DWORD)hash[0]) << 24) |
		(((DWORD)hash[1]) << 16) |
		(((DWORD)hash[2]) <<  8) |
		 ((DWORD)hash[3]);
}






// Convert the given peer ID object into text
CString PeerToString(const libtorrent::peer_id &id) {

	std::stringstream stream;
	stream << id;
	return widenStoC(stream.str());
}

// Given the text of a torrent infohash, look up and return the libtorrent torrent handle object
libtorrent::torrent_handle FindTorrentHandle(const char *id) {

	libtorrent::big_number hash = convertPtoBigNumber(id);
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

/*
Add a torrent to the libtorrent session
Writes a pointer to the new torrent handle and returns true, or logs and exception and returns false

- folder is the path to the save folder, like "C:\Documents\torrents" without a trailing slash or the name of the torrent folder like "My Torrent" on the end
- store is the path to libtorrent resume data from this torrent running in a previous session, or blank if this is the first time

Use it to add a torrent file on the disk
- torrent is the path to the torrent file on the disk

Or set torrent blank and use this function to add a torrent with information from a magnet link
- hash
- name
- trackers
*/
/*
bool LibraryAdd(libtorrent::torrent_handle *handle, read folder, read store, read torrent, libtorrent::big_number hash, read name, std::vector<CString> trackers) {
	try {

		libtorrent::add_torrent_params p; // The structure we will fill out, making it fills it with defaults
		libtorrent::torrent_info info(boost::filesystem::path(narrowRtoS(torrent)));
		std::string n = narrowRtoS(name); // Local variables set in braces but still in scope for the add call afterwards
		std::vector<char> c;

		p.save_path = boost::filesystem::path(narrowRtoS(folder)); // Required path to save folder

		if (is(torrent)) { // Torrent file

			p.ti = &info;

		} else { // Magnet link

			p.info_hash = hash;
			p.name = n.c_str();
		}

		if (is(store)) {

			boost::filesystem::ifstream f(store, std::ios_base::binary); // Try to open the file on the disk
			if (!f.fail()) { // Opening it worked

				f.unsetf(std::ios_base::skipws); // Set whitespace option
				std::istream_iterator<char> fileiterator(f);
				std::istream_iterator<char> streamiterator;
				std::copy(fileiterator, streamiterator, std::back_inserter(c)); // Copy the file contents into c
				p.resume_data = &c; // Add the char vector to the torrent parameters we're filling out
				f.close(); // Close the disk file we opened
			}
		}

		*handle = Handle.session->add_torrent(p);
		if (!handle->is_valid()) { log(L"invalid add"); return false; }

		for (int i = 0; i < (int)trackers.size(); i++) {

			libtorrent::announce_entry a(narrowRtoS(trackers[i]));
			handle->add_tracker(a);
		}

		return true;

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
	return false; // There was an exception
}
*/






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
			CString id = convertBigNumberToC(h.info_hash());

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






