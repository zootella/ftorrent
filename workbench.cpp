
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



void LibraryStart() {
	log(L"library start");
	try {

		// Make our libtorrent session object
		Handle.session = new libtorrent::session(
			libtorrent::fingerprint("ltorrent", 0, 1, 0, 0), // Program name and version numbers separated by commas
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

void LibraryClose1() {
	log(L"library close1");
	try {


		Handle.session->stop_dht();
		Handle.session->stop_lsd();
		Handle.session->stop_upnp();
		Handle.session->stop_natpmp();

		// Pause all the torrents and have automanage not unpause them
		Handle.session->pause();

		// Loop through all the torrent handles
		int n = 0; // Count how many torrents will give us resume data
		std::vector<libtorrent::torrent_handle> handles = Handle.session->get_torrents();
		for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin(); i != handles.end(); i++) {
			libtorrent::torrent_handle &h = *i;

			// This torrent is initialized and not aborted and has filename and piece hash metadata
			if (h.is_valid() && h.has_metadata()) {
				
				// Tell the torrent to generate resume data
				h.save_resume_data(); // Returns immediately, we'll get the data later after libtorrent gives us an alert
				n++; // Count one more torrent that will give us resume data
			}
		}

		log(L"requested save resume data on ", saynumber(n, L"torrent"));



	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}

}

void LibraryClose2() {
	log(L"library close2, a no-op");
	/*

	libtorrent::entry e;
	Handle.session->save_state(e); // Save all libtorrent state except for the individual torrents
	SaveEntry(PathStore(), e);

	log(L"delete session before");
	delete Handle.session;
	log(L"delete session after"); //TODO do this in a thread or figure out how to use abort
	/*
	log(L"a");
	libtorrent::session_proxy p = Handle.session->abort(); // Tell libtorrent to shut down
	log(L"b");
	p.session_proxy::~session_proxy(); // Blocks here until libtorrent is shut down
	log(L"c"); //TODO confirm a and b is quick, b to c is slow, then move delete to after alert
	*/

}

void AddTorrent() {

	/*
	bdencode();
	bencode();
	add_torrent();

	*/



}

void LibraryPulse() {

	/*

	//query the torrent handles for progress
	torrent_handle

	//query the session for information

	//see if there are any new alerts


	*/

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

	// Get the category and the message
//	log(L"alert category ", numerals(alert->category()), L" ", widenStoC(alert->message()));

	// If it's a torrent alert
	const libtorrent::torrent_alert *a = dynamic_cast<const libtorrent::torrent_alert *>(alert);
	if (a) {

		// Get the torrent handle and make sure that torrent is initialized and not yet aborted
		libtorrent::torrent_handle h = a->handle;
		if (h.is_valid()) {

			// Get the infohash
			CString id = HashToString(h.info_hash());

			// If the alert is for save resume data
			const libtorrent::save_resume_data_alert *a1 = dynamic_cast<const libtorrent::save_resume_data_alert *>(alert);
			if (a1) {

				// Get the pointer to the resume data
				const boost::shared_ptr<libtorrent::entry> resume_ptr = a1->resume_data;

				libtorrent::entry *e = resume_ptr.get(); // Copy across the pointer to the resume data


				SaveEntry(PathTorrentStore(h.info_hash()), *e);
				log(L"saved entry for ", id);



				return;
			}

			// If the alert is for save resume data failed
			const libtorrent::save_resume_data_failed_alert *a2 = dynamic_cast<const libtorrent::save_resume_data_failed_alert *>(alert);
			if (a2) {

				// Get the error message
				log(id, L" save resume failed ", widenStoC(a2->msg));
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






