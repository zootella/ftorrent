
// Include libtorrent
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/alert.hpp"

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
#include "program.h"
#include "object.h"
#include "library.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;

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




// Add a torrent to our libtorrent session
// folder is the path to the save folder, like "C:\Documents\torrents" without a trailing slash or the name of the torrent folder like "My Torrent" on the end
// torrent is the path to the torrent file on the disk
// or set torrent null and specify hash, name, and tracker from the magnet link
// store is the path to libtorrent resume data from a previous session, or null if this is the first time
// Sets the torrent handle, or returns false on error
// If you add the same infohash twice, sets the existing handle instead of producing an error
bool AddTorrent(read folder, read torrent, read hash, read name, read tracker, read store, libtorrent::torrent_handle &handle) {
	try {

		//TODO have this return a torrentitem that contains the torrent handle, or null if we didn't get one

		// Local objects in memory for the add call below
		std::string namestring, trackerstring;
		if (name)    namestring    = narrowRtoS(name);
		if (tracker) trackerstring = narrowRtoS(tracker);
		std::vector<char> charvector;

		// Make a torrent params object to fill out
		libtorrent::add_torrent_params p;
		p.duplicate_is_error = false; // Return the existing torrent handle instead of producing an error

		// Set folder, the path to the folder where the torrent is or will be saved, required
		p.save_path = boost::filesystem::path(narrowRtoS(folder));

		// Set torrent, the path to the torrent file on the disk
		if (torrent) {

			p.ti = new libtorrent::torrent_info(boost::filesystem::path(narrowRtoS(torrent)));

		// Or, set hash, name, and tracker from a magnet link
		} else {

			p.info_hash = convertPtoSha1Hash(narrowRtoS(hash).c_str());
			if (name)    p.name        = namestring.c_str();
			if (tracker) p.tracker_url = trackerstring.c_str();
		}

		// Specify store data saved from a previous session, optional
		if (store) {

			boost::filesystem::ifstream f(store, std::ios_base::binary); // Try to open the file on the disk
			if (!f.fail()) { // Opening it worked

				// Copy the file contents into charvector
				f.unsetf(std::ios_base::skipws); // Set whitespace option
				std::istream_iterator<char> fileiterator(f);
				std::istream_iterator<char> streamiterator;
				std::copy(fileiterator, streamiterator, std::back_inserter(charvector));

				// Add the charvector to the torrent parameters we're filling out
				p.resume_data = &charvector;

				// Close the disk file we opened
				f.close();
			}
		}

		// Add the torrent to the session and return the torrent handle we get
		handle = Handle.session->add_torrent(p);
		return true;

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
	return false; // Something went wrong
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
			CString id = convertSha1HashToC(h.info_hash());

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






