
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







void SaveFastResumeData(alert_structure *alert, wchar_t *filePath) {
	try {

		std::wstring file(filePath);
		boost::filesystem::wpath full_path(file);
		boost::filesystem::ofstream out(full_path, std::ios_base::binary);
		out.unsetf(std::ios_base::skipws);
		libtorrent::entry *resume_data = alert->resume_data;
		libtorrent::bencode(std::ostream_iterator<char>(out), *resume_data);
		out.close();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Ported from http://www.rasterbar.com/products/libtorrent/manual.html#save-resume-data
void FreezeAndSaveAllFastResumeData(void(*alertCallback)(void*)) {
	try {

		int num_resume_data = 0;

		std::vector<libtorrent::torrent_handle> handles = Handle.session->get_torrents();
		Handle.session->pause();

		for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin(); i != handles.end(); ++i) {
			libtorrent::torrent_handle &h = *i;
			if (!h.has_metadata())
				continue;
			if (!h.is_valid())
				continue;

			h.save_resume_data();
			++num_resume_data;
			log(make(L"num_resume: ", numerals(num_resume_data)));
		}

		while (num_resume_data > 0) {
			log(make(L"waiting for resume: ", numerals(num_resume_data)));

			libtorrent::alert const *alert = Handle.session->wait_for_alert(libtorrent::seconds(10));

			// if we don't get an alert within 10 seconds, abort
			if (alert == NULL)
				break;

			std::auto_ptr<libtorrent::alert> holder = Handle.session->pop_alert();

			alert_structure *alertInfo = new alert_structure();
			ProcessAlert(alert, alertInfo);

			const char *sha1 = alertInfo->sha1;
			const char *message = alertInfo->message;
			alertCallback(alertInfo);

			if (alertInfo->has_data) {
				log(L"resume_found: ");
				--num_resume_data;
			}
			delete[] sha1;
			delete[] message;
			delete alertInfo;
		}

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void UpdateSettings(settings_structure *settings) {
	try {

		libtorrent::session_settings *s = new libtorrent::session_settings;
		s->use_dht_as_fallback   = false;
		s->share_ratio_limit     = settings->seed_ratio_limit;
		s->seed_time_ratio_limit = settings->seed_time_ratio_limit;
		s->seed_time_limit       = settings->seed_time_limit;
		s->active_downloads      = settings->active_downloads_limit;
		s->active_seeds          = settings->active_seeds_limit;
		s->active_limit          = settings->active_limit;

		Handle.session->set_settings(*s);
		Handle.session->set_alert_mask(settings->alert_mask);
		Handle.session->listen_on(std::make_pair(settings->listen_start_port, settings->listen_end_port), ReadToString(settings->listen_interface).c_str());

		Handle.session->set_upload_rate_limit(settings->max_upload_bandwidth);
		Handle.session->set_download_rate_limit(settings->max_download_bandwidth);

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}


void GetAlerts(void(*alertCallback)(void*)) {
	try {

		std::auto_ptr<libtorrent::alert> alerts;

		alerts = Handle.session->pop_alert();

		while (alerts.get()) {

			libtorrent::alert *alert = alerts.get();

			alert_structure *alertInfo = new alert_structure();

			ProcessAlert(alert, alertInfo);
			const char *sha1 = alertInfo->sha1;
			const char *message = alertInfo->message;
			alertCallback(alertInfo);
			delete[] sha1;
			delete[] message;
			delete alertInfo;

			alerts = Handle.session->pop_alert();
		}

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}


















// Takes a settings structure to look at, or null to not update settings
// Starts libtorrent and adds extensions
void InitializeLibtorrent(settings_structure *info) {
	try {

		Handle.session = new libtorrent::session; // Make the libtorrent session object

		if (info) // Added this
			UpdateSettings(info); // Update settings if a structure was given

		Handle.session->add_extension(&libtorrent::create_metadata_plugin); // Tell libtorrent which plugins to use
		Handle.session->add_extension(&libtorrent::create_ut_metadata_plugin);
		Handle.session->add_extension(&libtorrent::create_ut_pex_plugin);
		Handle.session->add_extension(&libtorrent::create_smart_ban_plugin);

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Tell libtorent to shut down
void AbortTorrents() {
	try {

		if (Handle.session)
			Handle.session->abort(); //TODO Get the session_proxy this returns

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Takes infohash text and a new save to path
// Moves the torrent from where it is now to the given path
void MoveTorrent(const char *id, wchar_t *path) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);

		h.pause();  // Pause the torrent
		libtorrent::storage_interface *storage = h.get_storage_impl(); // Access the torrent's storage implementation
		storage->release_files();                                      // Release all the file handles the torrent has open
		storage->move_storage(WideToPath(path));                       // Move all the saved files to path
		h.resume(); // Resume the torrent

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Takes infohash,    the SHA1 infohash of the torrent file in base 16 ASCII characters
// Takes trackerurl,  the URL of the torrent's tracker
// Takes torrentpath, the path to the .torrent file on the disk
// Takes savepath,    the path to where the program should save the torrent on the disk
// Takes resumepath,  the path to a file of resume information we saved earlier and can use now
// Adds the torrent to libtorrent
void AddTorrent(char *infohash, char *trackerurl, wchar_t *torrentpath, wchar_t *savepath, wchar_t *resumepath) {
	try {

		// Output the information this function received
		log(L"adding torrent");
		log(make(L"infohash:         ", Widen(infohash)));
		log(make(L"tracker url:      ", Widen(trackerurl)));
		log(make(L"torrent path:     ", torrentpath));
		log(make(L"resume file path: ", resumepath));

		// Fill out a torrent parameters object
		libtorrent::add_torrent_params p;
		p.save_path          = WideToPath(savepath);
		p.info_hash          = StringToHash(infohash);
		p.tracker_url        = trackerurl;
		p.auto_managed       = false; //TODO change this to true
		p.duplicate_is_error = true;

		// If we were given the path to a torrent file on the disk
		if (torrentpath) {

			boost::filesystem::ifstream f1(torrentpath, std::ios_base::binary); // Try to open it
			if (!f1.fail()) { // Opening it worked

				p.ti = new libtorrent::torrent_info(WideToPath(torrentpath)); // Add the path to the torrent parameters we're filling out

			} else { // Couldn't open it
				log(L"could not find torrent file");
			}
			f1.close(); // Close the file we tried to open
		}

		std::vector<char> resumebuffer;

		// If we were given the path to a file of resume data on the disk that we saved before and can use now
		if (resumepath) {

			boost::filesystem::ifstream f2(resumepath, std::ios_base::binary); // Try to open it
			if (!f2.fail()) { // Opening it worked

				// Copy the file contents into resumebuffer
				f2.unsetf(std::ios_base::skipws); // Change whitespace option
				std::istream_iterator<char> ios_iter;
				std::istream_iterator<char> iter(f2);
				std::copy(iter, ios_iter, std::back_inserter(resumebuffer));

				p.resume_data = &resumebuffer; // Add the resumebuffer to the torrent parameters we're filling out
				f2.close();                    // Close the disk file we opened

			} else {
				log(L"could not find fast resume file");
			}
		}

		// Add the new torrent we made to our libtorrent session
		libtorrent::torrent_handle h = Handle.session->add_torrent(p);

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Pause the torrent with the given infohash
void PauseTorrent(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.pause();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Turn auto management on or off for the torrent with the given infohash
void SetAutoManagedTorrent(const char *id, bool auto_managed) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.auto_managed(auto_managed);

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Remove the torrent with the given infohash from the program
void RemoveTorrent(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.pause();
		Handle.session->remove_torrent(h); //TODO another option here would delete the files on the disk, too

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Resume the torrent with the given infohash after you paused it
void ResumeTorrent(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.resume();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Reannounce the torrent with the given infohash to its trackers now to find out about more peers
void ForceReannounce(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.force_reannounce();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Ask the tracker for information about the torrent with the given infohash, like how many peers are complete
void ScrapeTracker(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.scrape_tracker();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Fill out the given status structure with current information about the torrent with the given infohash
void GetTorrentStatus(const char *id, status_structure *info) {
	try {

		// Find the torrent handle and get the status object
		libtorrent::torrent_handle h = FindTorrentHandle(id);
		libtorrent::torrent_status s = h.status();

		// Fill out the given status structure
		info->total_done                = s.total_done;
		info->total_wanted_done         = s.total_wanted_done;
		info->total_wanted              = s.total_wanted;
		info->total_download            = s.total_download;
		info->total_upload              = s.total_upload;
		info->total_payload_download    = s.total_payload_download;
		info->total_payload_upload      = s.total_payload_upload;
		info->all_time_payload_download = s.all_time_download;
		info->all_time_payload_upload   = s.all_time_upload;
		info->download_rate             = (float)s.download_rate;
		info->upload_rate               = (float)s.upload_rate;
		info->download_payload_rate     = (float)s.download_payload_rate;
		info->upload_payload_rate       = (float)s.upload_payload_rate;
		info->num_peers                 = s.num_peers;
		info->num_uploads               = s.num_uploads;
		info->num_seeds                 = s.num_seeds;
		info->num_connections           = s.num_connections;
		info->state                     = s.state;
		info->progress                  = s.progress;
		info->paused                    = s.paused;
		info->finished                  = h.is_finished();
		info->valid                     = h.is_valid();
		info->auto_managed              = h.is_auto_managed();
		info->seeding_time              = s.seeding_time;
		info->active_time               = s.active_time;
		info->error                     = Widen(s.error);
		info->current_tracker           = Widen(s.current_tracker);
		info->num_complete              = s.num_complete;
		info->num_incomplete            = s.num_incomplete;
		info->total_failed_bytes        = s.total_failed_bytes;

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Fill out the given torrent structure with information from the torrent with the given infohash
void GetTorrentInfo(const char *id, torrent_structure *info) {
	try {

		// Find the torrent handle and get its info object
		libtorrent::torrent_handle h = FindTorrentHandle(id);
		libtorrent::torrent_info i = h.get_torrent_info();

		// Fill out some information
		info->created_by   = Widen(i.creator().c_str());
		info->comment      = Widen(i.comment().c_str());
		info->sha1         = HashToCString(i.info_hash());
		info->total_size   = (long long)i.total_size();
		info->piece_length = (int)i.piece_length();

		// List trackers
		announce_structure a;
		std::vector<libtorrent::announce_entry> v1 = i.trackers();
		std::vector<libtorrent::announce_entry>::iterator iterator1 = v1.begin();
		while (iterator1 != v1.end()) {

			a.url = Widen(iterator1->url.c_str());
			a.tier = iterator1->tier;
			info->trackers.push_back(a);

			iterator1++;
		}

		// List web seeds
		std::vector<std::string> v2 = i.url_seeds();
		std::vector<std::string>::iterator iterator2 = v2.begin();
		while (iterator2 != v2.end()) {

			a.url = Widen(iterator2->c_str());
			a.tier = -1;
			info->seeds.push_back(a);

			iterator2++;
		}

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Have libtorrent generate resume data for the torrent with the given infohash
// This returns immediately, when the data is ready, you'll get a save resume data alert
void SignalFastResumeDataRequest(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		if (h.has_metadata()) // True if we started with a .torrent file or got equivalent information from the swarm
			h.save_resume_data();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Force restart the torrent with the given infohash that may have stopped because it had an error
void ClearErrorAndRetry(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.force_recheck(); // Disconnect peers and tracker, hash what we have on the disk, and then go back online
		h.clear_error();   // Clear this torrent's error state and start again
		h.resume();        // Unpause, start and go back online

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Find out how many peers the torrent with the given infohash has right now
void GetNumPeers(const char *id, int &num_peers) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);

		std::vector<libtorrent::peer_info> peers; // Make a new empty vector
		try {

			h.get_peer_info(peers); // Have libtorrent fill it with with information about each peer

		} catch (libtorrent::invalid_handle e) {
			log(L"invalid handle exception");
			return;
		} catch (std::exception e) {
			log(L"std exception");
			return;
		}

		num_peers = peers.size();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Find out if the torrent with the given infohash has metadata, either from a .torrent file or downloaded from the swarm
void HasMetadata(const char *id, int &has_metadata) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		has_metadata = h.has_metadata();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Find out if we've got a valid handle for a torrent with the given infohash
void IsValid(const char *id, int &is_valid) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		is_valid = h.is_valid(); // Reasons for invalid include not found, or filesystem character error

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Fill the given vector with information about the peers of the torrent with the given infohash
void GetPeers(const char *id, std::vector<peer_structure> *v) {
	try {

		// Get information about the torrent's current peers
		libtorrent::torrent_handle h = FindTorrentHandle(id); // Look up the torrent handle
		std::vector<libtorrent::peer_info> peers;             // Make a new empty vector that can hold peer_info objects
		h.get_peer_info(peers);                               // Have libtorrent fill it with information about each of this torrent's peers

		// Loop through the peers
		peer_structure p;
		std::vector<libtorrent::peer_info>::iterator i = peers.begin();
		while (i != peers.end()) {

			// Copy information from the peer_info object i points to to our local peer_structure, p
			p.status_flags       = i->flags;
			p.ip                 = Widen(i->ip.address().to_string());
			p.source             = i->source;
			p.up_speed           = (float)i->up_speed;
			p.down_speed         = (float)i->down_speed;
			p.payload_up_speed   = (float)i->payload_up_speed;
			p.payload_down_speed = (float)i->payload_down_speed;
			p.peer_id            = PeerIdToString(i->pid);
			p.progress           = i->progress;

			// Get the country code
			WCHAR c[3];
			c[0] = (WCHAR)i->country[0];
			c[1] = (WCHAR)i->country[1];
			c[2] = L'\0';
			p.country = c;

			// Get the client name
			try {
				p.client_name = Widen(i->client.c_str());
			} catch (std::runtime_error &e) {
				log(Widen(e.what()));
				p.client_name = L"";			
			}

			// Add the peer_structure we filled out to the vector we're supposed to fill
			v->push_back(p);
			
			i++;
		}

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Set the given download to upload seed ratio on the torrent with the given infohash
void SetSeedRatio(const char *id, float seed_ratio) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.set_ratio(seed_ratio);

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Find out how many files the torrent with the given infohash contains
void GetNumFiles(const char *id, int &num_files) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		libtorrent::torrent_info info = h.get_torrent_info();
		libtorrent::file_storage files = info.files();
		num_files = files.num_files();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetFiles(const char *id, file_structure **file_entries) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		libtorrent::torrent_info info = h.get_torrent_info();
		libtorrent::file_storage files = info.files();

		int index = 0;
		std::vector<libtorrent::size_type> progress;
		h.file_progress(progress);
		std::vector<int> priorities = h.file_priorities();

		std::vector<libtorrent::file_entry>::const_iterator iter = files.begin();
		while (iter != files.end()) {
			boost::filesystem::path path = iter->path;
			file_structure *file_entry = *file_entries;
			file_entry->index = index;
			file_entry->path = Widen(path.string());
			file_entry->size = iter->size;
			file_entry->total_done = progress[index];
			file_entry->priority = priorities[index];
			file_entries++;
			index++;
			iter++;
		}

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void SetFilePriorities(const char *id, int *priorities, int num_priorities) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		std::vector<int> priorities_vector;

		for (int i = 0; i < num_priorities; i++) {
			int priority = *priorities;
			priorities_vector.push_back(priority);
			priorities++;
		}

		h.prioritize_files(priorities_vector);

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void SetFilePriority(const char *id, int index, int priority) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.file_priority(index, priority);

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StartDht(const wchar_t *dht_state_file_path) {
	try {

		std::wstring file(dht_state_file_path);
		boost::filesystem::wpath full_path(file);

		boost::filesystem::ifstream dht_state_file(full_path, std::ios_base::binary);

		libtorrent::entry dht_state = 0;
		bool state_loaded = false;
		if(!dht_state_file.fail()) {
			try {
				dht_state = libtorrent::bdecode(std::istream_iterator<char>(dht_state_file), std::istream_iterator<char>());
				state_loaded = true;
			} catch (std::exception& e) {
				log(Widen(e.what()));
				//no dht to resume will start dht without a prebuilt state
			}
		}

		if(state_loaded) {
			Handle.session->start_dht(dht_state);
		} else {
			Handle.session->start_dht();
		}

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void AddDhtRouter(const char *address, int port) {
	try {

		Handle.session->add_dht_router(std::pair<std::string, int>(std::string(address), port));

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void AddDhtNode(const char *address, int port) {
	try {

		Handle.session->add_dht_node(std::pair<std::string, int>(std::string(address), port));

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void SaveDhtState(const wchar_t *dht_state_file_path) {
	try {

		std::wstring file(dht_state_file_path);
		boost::filesystem::wpath full_path(file);
		boost::filesystem::ofstream out(full_path, std::ios_base::binary);
		out.unsetf(std::ios_base::skipws);
		libtorrent::entry dht_state = Handle.session->dht_state();
		libtorrent::bencode(std::ostream_iterator<char>(out), dht_state);
		out.close();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StopDht() {
	try {

		Handle.session->stop_dht();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StartUpnp() {
	try {

		Handle.session->start_upnp();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StopUpnp() {
	try {

		Handle.session->stop_upnp();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StartLsd() {
	try {

		Handle.session->start_lsd();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StopLsd() {
	try {

		Handle.session->stop_lsd();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StartNatpmp() {
	try {

		Handle.session->start_natpmp();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StopNatpmp() {
	try {

		Handle.session->stop_natpmp();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetPiecesStatus(const char *id, pieces_structure *info) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);

		libtorrent::bitfield piece_downloaded_info = h.status().pieces;

		int num_pieces = piece_downloaded_info.size();

		CString p;
		info->completed = 0;

		// Clear the array
		for (int i = 0; i < num_pieces; i++) {
			p += L'0'; // Piece pending
		}

		std::vector<libtorrent::peer_info> peers;
		h.get_peer_info(peers);
		std::vector<libtorrent::peer_info>::iterator iter = peers.begin();
		while (iter != peers.end()) {
			if (iter->downloading_piece_index > -1 && iter->downloading_piece_index < num_pieces) {
				// Mark downloading pieces
				p.SetAt(iter->downloading_piece_index, L'a'); // Piece active
			}
			iter++;
		}

		std::vector<libtorrent::partial_piece_info> download_queue;
		h.get_download_queue(download_queue);
		std::vector<libtorrent::partial_piece_info>::iterator queue_iter = download_queue.begin();
		while (queue_iter != download_queue.end()) {
			libtorrent::partial_piece_info piece = *queue_iter;
			if (piece.piece_index > -1 && piece.piece_index < num_pieces) {
				if (p[piece.piece_index] != L'a') { // Piece active
					if (piece.writing > 0) {
						p.SetAt(piece.piece_index, L'a'); // Piece active
					} else if (piece.requested > 0 && piece.finished > 0) {
						p.SetAt(piece.piece_index, L'p'); // Piece partial
					} else if (piece.finished == 0) {
						p.SetAt(piece.piece_index, L'q'); // Piece queued
					}
				}
			}

			queue_iter++;
		}

		for (int i = 0; i < num_pieces; i++) {
			if (p[i] != L'a') { // Piece active
				if (piece_downloaded_info[i]) {
					// Mark downloaded pieces
					p.SetAt(i, L'x'); // Piece downloaded
					info->completed++;
				} else {
					bool available = false;

					iter = peers.begin();
					while (iter != peers.end()) {
						if (iter->pieces[i]) {
							available = true;
							break;
						}
						iter++;
					}

					if (!available) {
						if (p[i] == L'p') { // Piece partial
							p.SetAt(i, L'u'); // Piece unavailable partial
						} else {
							p.SetAt(i, L'U'); // Piece unavailable
						}
					}
				}
			}
		}

		info->pieces = p;

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void AddTracker(const char *id, char *url, int tier) {
	try {
		
		libtorrent::torrent_handle h = FindTorrentHandle(id);
		std::vector<libtorrent::announce_entry> trackers = h.trackers();
		
		libtorrent::announce_entry e(url);
		e.tier = tier;
		trackers.push_back(e);

		h.replace_trackers(trackers);

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void RemoveTracker(const char *id, char *url, int tier) {
	try {
		
		libtorrent::torrent_handle h = FindTorrentHandle(id);
		std::vector<libtorrent::announce_entry> trackers = h.trackers();
		std::vector<libtorrent::announce_entry> new_trackers;
		
		std::vector<libtorrent::announce_entry>::iterator iter = trackers.begin();

		while (iter != trackers.end()) {
			libtorrent::announce_entry tracker = *iter;
			if (tracker.tier != tier || strcmp(tracker.url.c_str(), url) != 0) {
				new_trackers.push_back(tracker);
			}
			iter++;
		}

		h.replace_trackers(new_trackers);

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetNumTrackers(const char *id, int &num_trackers) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);

		std::vector<libtorrent::announce_entry> trackers;
		try {
			trackers = h.trackers();
		} catch (libtorrent::invalid_handle e) {
			return;
		} catch (std::exception e) {
			return;
		}
		num_trackers = trackers.size();

	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetTrackers(const char *id, announce_structure **torrent_trackers, int numTrackers) {
	try {
		
		libtorrent::torrent_handle h = FindTorrentHandle(id);
		std::vector<libtorrent::announce_entry> trackers = h.trackers();
		
		std::vector<libtorrent::announce_entry>::iterator iter = trackers.begin();

		announce_structure **current_torrent_tracker = torrent_trackers;

		int index = 0;
		while (iter != trackers.end()) {
			libtorrent::announce_entry tracker = *iter;
			
			announce_structure *torrent_tracker = *current_torrent_tracker;
			current_torrent_tracker++;
			
			torrent_tracker->tier = tracker.tier;
			torrent_tracker->url = CopyString(tracker.url.c_str());
		
			index++;
			if(index >= numTrackers) {
				break;
			}
			iter++;
		}
		
	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void FreeTrackers(announce_structure **torrent_trackers, int numTrackers) {
	try {
		
		announce_structure **current_torrent_tracker = torrent_trackers;
	
		for (int i = 0; i < numTrackers; i++) {
			announce_structure *tracker = *current_torrent_tracker;
			delete[] tracker->url;
			current_torrent_tracker++;
		}
		
	} catch (std::exception &e) {
		log(Widen(e.what()));
	} catch (...) {
		log(L"exception");
	}
}
