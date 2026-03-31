
#include "heavy.h"
extern app App; // Access global object

// Takes a settings structure to look at, or null to not update settings
// Starts libtorrent and adds extensions
void InitializeLibtorrent(settings_structure *info) {
	try {

		App.session = new libtorrent::session; // Make the libtorrent session object

		if (info) // Added this
			UpdateSettings(info); // Update settings if a structure was given

		App.session->add_extension(&libtorrent::create_metadata_plugin); // Tell libtorrent which plugins to use
		App.session->add_extension(&libtorrent::create_ut_metadata_plugin);
		App.session->add_extension(&libtorrent::create_ut_pex_plugin);
		App.session->add_extension(&libtorrent::create_smart_ban_plugin);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Given settings in info, apply them to our libtorrent session
void UpdateSettings(settings_structure *info) {
	try {

		// Apply settings in info to libtorrent
		libtorrent::session_settings s; // Make a new libtorrent session settings object
		s.use_dht_as_fallback   = false; // didn't turn this off
		s.share_ratio_limit     = info->seed_ratio_limit; // Copy info into a libtorrent session settings object
		s.seed_time_ratio_limit = info->seed_time_ratio_limit;
		s.seed_time_limit       = info->seed_time_limit;
		s.active_downloads      = info->active_downloads_limit;
		s.active_seeds          = info->active_seeds_limit;
		s.active_limit          = info->active_limit;
		App.session->set_settings(s); // Apply the settings to libtorrent

		// Tell libtorrent what kinds of alerts we want to find out about
		App.session->set_alert_mask(info->alert_mask);

		// Give libtorrent connection settings
		App.session->listen_on(
			std::make_pair(info->listen_start_port, info->listen_end_port), // Port range to listen on
			narrowRtoS(info->listen_interface).c_str());                    // Network interface to use
		App.session->set_upload_rate_limit(info->max_upload_bandwidth);     // Upload and download speed limits
		App.session->set_download_rate_limit(info->max_download_bandwidth);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Pause the libtorrent session and get resume data from each torrent
// Ported from http://www.rasterbar.com/products/libtorrent/manual.html#save-resume-data
void FreezeAndSaveAllFastResumeData() {
	try {

		// Count how many torrents will give us resume data
		int n = 0;

		// Pausing the whole session is better than pausing torrents individually
		App.session->pause();

		// Loop through all the torrent handles
		std::vector<libtorrent::torrent_handle> handles = App.session->get_torrents();
		for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin(); i != handles.end(); ++i) {
			libtorrent::torrent_handle &h = *i;
			if (!h.has_metadata()) continue; // Skip this one unless it has filename and piece hash metadata
			if (!h.is_valid()) continue; // And isn't uninitialized or aborted

			// Tell the torrent to generate resume data
			h.save_resume_data(); // Returns immediately, we'll get the data later after libtorrent gives us an alert
			n++; // Count one more torrent that will give us resume data
		}

		// Loop until we've gotten as many resume data alerts as we expect
		while (n > 0) {

			// Wait here for an alert
			const libtorrent::alert *alert = App.session->wait_for_alert(libtorrent::seconds(10)); // Wait up to 10 seconds
			if (alert == NULL) break; // Didn't get one, leave

			// Got an alert
			std::auto_ptr<libtorrent::alert> p = App.session->pop_alert(); // Tell libtorrent we've got it
			alert_structure info;
			ProcessAlert(alert, &info); // Copy information from alert into info

			// Only count this alert if it has resume data
			if (info.has_data) n--;
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Tell libtorent to shut down
void AbortTorrents() {
	try {

		if (App.session)
			App.session->abort(); // deleted the object instead of aborting it

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Save the resume data info knows about to a new file at path
void SaveFastResumeData(alert_structure *info, wchar_t *path) {
	try {

		// Open a new file at path
		std::wstring w(path);                                    // Convert path into a wide string
		boost::filesystem::wpath p(w);                           // Make that into a path object
		boost::filesystem::ofstream f(p, std::ios_base::binary); // Make an output file stream
		f.unsetf(std::ios_base::skipws);                         // Don't have the file skip whitespace

		// Write data in the file
		libtorrent::entry *e = info->resume_data;                // Point e at the resume data
		libtorrent::bencode(std::ostream_iterator<char>(f), *e); // Bencode the data at e into the file

		// Close the file
		f.close();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Save DHT information in a file at path to look at next time so it starts up faster
void SaveDhtState(const wchar_t *path) {
	try {

		// Open the file at path for writing
		std::wstring w(path);
		boost::filesystem::wpath p(w);
		boost::filesystem::ofstream f(p, std::ios_base::binary);
		f.unsetf(std::ios_base::skipws);

		libtorrent::entry e = App.session->dht_state(); // Get the DHT state as a bencoded object
		libtorrent::bencode(std::ostream_iterator<char>(f), e); // Serialize the bencoded information to a file

		// Close the file
		f.close();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Take all the alerts libtorrent is waiting to give us and look at each one
void GetAlerts() {
	try {

		// Get an alert
		std::auto_ptr<libtorrent::alert> p = App.session->pop_alert(); // Move an alert from libtorrent to p
		while (p.get()) { // If p contans an alert
			libtorrent::alert *alert = p.get(); // Get it

			// Look at alert, filling info with information about it and copied from it
			alert_structure info;
			ProcessAlert(alert, &info);

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
void ProcessAlert(const libtorrent::alert *alert, alert_structure *info) {

	// Get the category and the message
	info->category = alert->category();
	info->message = widenStoC(alert->message());

	// If it's a torrent alert
	const libtorrent::torrent_alert *a = dynamic_cast<const libtorrent::torrent_alert *>(alert);
	if (a) {

		// Get the torrent handle and make sure that torrent is initialized and not yet aborted
		libtorrent::torrent_handle h = a->handle;
		if (h.is_valid()) {

			// Get the infohash
			info->sha1 = base16(h.info_hash());

			// If the alert is for save resume data
			const libtorrent::save_resume_data_alert *a1 = dynamic_cast<const libtorrent::save_resume_data_alert *>(alert);
			if (a1) {

				// Get the pointer to the resume data
				const boost::shared_ptr<libtorrent::entry> resume_ptr = a1->resume_data;
				info->has_data = 1; // Mark that this info structure has resume data
				info->resume_data = resume_ptr.get(); // Copy across the pointer to the resume data
				return;
			}

			// If the alert is for save resume data failed
			const libtorrent::save_resume_data_failed_alert *a2 = dynamic_cast<const libtorrent::save_resume_data_failed_alert *>(alert);
			if (a2) {

				// Get the error message
				info->message = widenStoC(a2->msg);
				return;
			}

			// If the alert is for fast resume rejected
			const libtorrent::fastresume_rejected_alert *a3 = dynamic_cast<const libtorrent::fastresume_rejected_alert *>(alert);
			if (a3) {

				// Get the error message
				info->message = widenStoC(a3->msg);
				return;
			}
		}
	}
}

// Takes infohash,    the SHA1 infohash of the torrent file in base 16 ASCII characters
// Takes trackerurl,  the URL of the torrent's tracker
// Takes torrentpath, the path to the .torrent file on the disk
// Takes savepath,    the path to where the program should save the torrent on the disk
// Takes resumepath,  the path to a file of resume information we saved earlier and can use now
// Adds the torrent to libtorrent
void AddTorrentWrap(char *infohash, char *trackerurl, wchar_t *torrentpath, wchar_t *savepath, wchar_t *resumepath) {
	try {

		// Output the information this function received
		log(L"adding torrent");
		log(make(L"infohash:         ", widenPtoC(infohash)));
		log(make(L"tracker url:      ", widenPtoC(trackerurl)));
		log(make(L"torrent path:     ", torrentpath));
		log(make(L"resume file path: ", resumepath));

		// Fill out a torrent parameters object
		libtorrent::add_torrent_params p;
		p.save_path          = boost::filesystem::path(narrowRtoS(savepath));
		p.info_hash          = ParseHash(widenPtoC(infohash));
		p.tracker_url        = trackerurl;
		p.auto_managed       = false; // changed this to true
		p.duplicate_is_error = true;

		// If we were given the path to a torrent file on the disk
		if (torrentpath) {

			boost::filesystem::ifstream f1(torrentpath, std::ios_base::binary); // Try to open it
			if (!f1.fail()) { // Opening it worked

				p.ti = new libtorrent::torrent_info(boost::filesystem::path(narrowRtoS(torrentpath))); // Add the path to the torrent parameters we're filling out

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
				std::istream_iterator<char> streamiterator;
				std::istream_iterator<char> fileiterator(f2);
				std::copy(fileiterator, streamiterator, std::back_inserter(resumebuffer));

				p.resume_data = &resumebuffer; // Add the resumebuffer to the torrent parameters we're filling out
				f2.close();                    // Close the disk file we opened

			} else {
				log(L"could not find fast resume file");
			}
		}

		// Add the new torrent we made to our libtorrent session
		libtorrent::torrent_handle h = App.session->add_torrent(p);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Remove the torrent with the given infohash from the program
void RemoveTorrentWrap(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.pause();
		App.session->remove_torrent(h); //TODO another option here would delete the files on the disk, too

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
		libtorrent::storage_interface *storage = h.get_storage_impl();    // Access the torrent's storage implementation
		storage->release_files();                                         // Release all the file handles the torrent has open
		storage->move_storage(boost::filesystem::path(narrowRtoS(path))); // Move all the saved files to path
		h.resume(); // Resume the torrent

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
		info->created_by   = widenStoC(i.creator());
		info->comment      = widenStoC(i.comment());
		info->sha1         = base16(i.info_hash());
		info->total_size   = (long long)i.total_size();
		info->piece_length = (int)i.piece_length();

		// List trackers
		announce_structure a;
		std::vector<libtorrent::announce_entry> v1 = i.trackers();
		std::vector<libtorrent::announce_entry>::iterator iterator1 = v1.begin();
		while (iterator1 != v1.end()) {

			a.url = widenStoC(iterator1->url);
			a.tier = iterator1->tier;
			info->trackers.push_back(a);

			iterator1++;
		}

		// List web seeds
		std::vector<std::string> v2 = i.url_seeds();
		std::vector<std::string>::iterator iterator2 = v2.begin();
		while (iterator2 != v2.end()) {

			a.url = widenPtoC(iterator2->c_str());
			a.tier = -1;
			info->seeds.push_back(a);

			iterator2++;
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
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
		info->error                     = widenStoC(s.error);
		info->current_tracker           = widenStoC(s.current_tracker);
		info->num_complete              = s.num_complete;
		info->num_incomplete            = s.num_incomplete;
		info->total_failed_bytes        = s.total_failed_bytes;

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Add a tracker to the torrent with the given infohash
void AddTracker(const char *id, char *url, int tier) {
	try {
		
		// Get the torent's list of trackers
		libtorrent::torrent_handle h = FindTorrentHandle(id); // Get the torrent handle
		std::vector<libtorrent::announce_entry> trackers = h.trackers(); // Get its list of trackers

		// Make a new announce entry object and fill it out
		libtorrent::announce_entry e(url);
		e.tier = tier;

		// Add it to the list and give the edited list back to the torrent
		trackers.push_back(e);
		h.replace_trackers(trackers);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Remove the given tracker from the torrent with the given infohash
void RemoveTracker(const char *id, char *url, int tier) {
	try {
		
		// Get the torrent's list of trackers and make a new empty list
		libtorrent::torrent_handle h = FindTorrentHandle(id); // Get the torrent handle
		std::vector<libtorrent::announce_entry> trackers = h.trackers(); // Get the current list of trackers
		std::vector<libtorrent::announce_entry> trackers2; // Make a new empty list that will hold trackers

		// Loop down the torrent's list of trackers
		std::vector<libtorrent::announce_entry>::iterator i = trackers.begin();
		while (i != trackers.end()) {
			libtorrent::announce_entry tracker = *i;

			if (tracker.tier != tier || strcmp(tracker.url.c_str(), url) != 0) // This tracker from the list doesn't match the one we're trying to remove
				trackers2.push_back(tracker); // Add it to the new list to save it

			i++;
		}

		// Give the torrent the new list with the one we wanted to remove not there
		h.replace_trackers(trackers2);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Find out how many trackers the torrent with the given infohash has
void GetNumTrackers(const char *id, int &n) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id); // Get the torrent handle

		std::vector<libtorrent::announce_entry> trackers;
		try {
			trackers = h.trackers(); // Get the torrent's list of trackers
		}
		catch (libtorrent::invalid_handle e) { return; } // Leave without changing the value of n
		catch (std::exception e) { return; }

		n = trackers.size(); // Save how many trackers are in the list to the variable we were given access to

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Takes the infohash of a torrent, a pointer to memory for an array of announce structures, and the number of structures that fit in it
// Write information about the torrent's trackers in the given array of structures
void GetTrackers(const char *id, announce_structure **torrent_trackers, int n) {
	try {
		
		libtorrent::torrent_handle h = FindTorrentHandle(id); // Get the torrent handle
		std::vector<libtorrent::announce_entry> trackers = h.trackers(); // Get the list of trackers
		
		std::vector<libtorrent::announce_entry>::iterator i = trackers.begin(); // Make an iterator to loop down the trackers

		announce_structure **a = torrent_trackers; // Point a at the start of the given array of memory

		// Loop for each tracker
		int index = 0;
		while (i != trackers.end()) {
			libtorrent::announce_entry tracker = *i;
			
			announce_structure *info = *a; // Point info at the announce structure under a
			a++; // Move a forward for the next loop
			
			info->tier = tracker.tier; // Copy across the tier number
			info->url = widenStoC(tracker.url); // Copy the tracker address URL
		
			// Make sure we don't loop more than we've got memory to do so
			index++;
			if (index >= n) break;

			i++; // Move to the next tracker in the list
		}
		
	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Takes the infohash of a torrent, and a pointer an array of file structures
// Writes information about the files that make up the torrent with the given infohash
void GetFiles(const char *id, file_structure **file_entries) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id); // Get the torrent handle
		libtorrent::torrent_info i = h.get_torrent_info();    // From it get the torrent info object
		libtorrent::file_storage f = i.files();               // And from that the file storage object

		// Find out how much of each file is saved
		std::vector<libtorrent::size_type> progress;
		h.file_progress(progress); // Slow, faster to tell it to just summarize pieces

		// Get the priority of each file
		std::vector<int> priorities = h.file_priorities();

		// Loop for each file entry in the torrent's file storage vector
		int index = 0;
		std::vector<libtorrent::file_entry>::const_iterator iterator = f.begin();
		while (iterator != f.end()) {
			file_structure *info = *file_entries; // Point info at the file structure at file entries

			// Copy across information
			info->index = index;                             // Save the index
			info->path = widenStoC(iterator->path.string()); // Get the file path
			info->size = iterator->size;                     // Size of the file in bytes
			info->total_done = progress[index];              // How many bytes of the file are saved
			info->priority = priorities[index];              // The file's download priority

			file_entries++; // Point file entries forward the size of one file structure
			index++;        // Move to the next index
			iterator++;     // Move iterator to the next file entry in the file storage vector
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
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
			p.ip                 = widenStoC(i->ip.address().to_string());
			p.source             = i->source;
			p.up_speed           = (float)i->up_speed;
			p.down_speed         = (float)i->down_speed;
			p.payload_up_speed   = (float)i->payload_up_speed;
			p.payload_down_speed = (float)i->payload_down_speed;
			p.peer_id            = base16(i->pid);
			p.progress           = i->progress;

			// Get the country code
			WCHAR c[3];
			c[0] = (WCHAR)i->country[0];
			c[1] = (WCHAR)i->country[1];
			c[2] = L'\0';
			p.country = c;

			// Get the client name
			try {
				p.client_name = widenStoC(i->client);
			} catch (std::runtime_error &e) {
				log(widenPtoC(e.what()));
				p.client_name = L"";			
			}

			// Add the peer_structure we filled out to the vector we're supposed to fill
			v->push_back(p);
			
			i++;
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Get the stripe pattern of pieces for the torrent with the given infohash
void GetPiecesStatus(const char *id, pieces_structure *info) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id); // Get the torrent handle
		libtorrent::bitfield b = h.status().pieces; // Get the bitfield of all the pieces
		int n = b.size(); // Find out how many pieces there are

		info->completed = 0;

		// Make a string with a "0" for each piece
		CString s;
		for (int i = 0; i < n; i++)
			s += L'0'; // Piece pending

		// Loop through all the torrent's peers
		std::vector<libtorrent::peer_info> peers;
		h.get_peer_info(peers); // Get information about all the torrent's peers
		std::vector<libtorrent::peer_info>::iterator iterator1 = peers.begin();
		while (iterator1 != peers.end()) {

			if (iterator1->downloading_piece_index > -1 && iterator1->downloading_piece_index < n) // This peer is download a piece
				s.SetAt(iterator1->downloading_piece_index, L'a'); // Mark the piece active

			iterator1++;
		}

		// Loop through all the torrent's partial pieces
		std::vector<libtorrent::partial_piece_info> pieces;
		h.get_download_queue(pieces); // Get information about all the torrent's download requests and downloads in progress
		std::vector<libtorrent::partial_piece_info>::iterator iterator2 = pieces.begin();
		while (iterator2 != pieces.end()) {
			libtorrent::partial_piece_info p = *iterator2;

			if (p.piece_index > -1 && p.piece_index < n) { // If the partial piece info structure has a valid piece index

				if (s[p.piece_index] != L'a') { // Mark the piece active in our record of them

					if      (p.writing > 0)                     s.SetAt(p.piece_index, L'a'); // Piece active
					else if (p.requested > 0 && p.finished > 0) s.SetAt(p.piece_index, L'p'); // Piece partial
					else if (p.finished == 0)                   s.SetAt(p.piece_index, L'q'); // Piece queued
				}
			}

			iterator2++;
		}

		// Loop through all the pieces that aren't active in our record of them
		for (int i = 0; i < n; i++) {
			if (s[i] != L'a') { // Piece not active

				// The bitfield indicates we have this piece
				if (b[i]) {

					s.SetAt(i, L'x'); // Piece downloaded
					info->completed++; // Count one more piece downloaded

				// The bitfield indicates we don't have this piece
				} else {

					// Loop through all the peers
					bool available = false; // True when we've found one that has this piece
					iterator1 = peers.begin();
					while (iterator1 != peers.end()) {

						if (iterator1->pieces[i]) { // The peer under iterator has piece i
							available = true; // Found it
							break; // Don't need to look anymore
						}
						iterator1++;
					}

					// No peer has the piece
					if (!available) {

						if (s[i] == L'p') // Piece partial, we've saved some of it so far
							s.SetAt(i, L'u'); // Piece unavailable partial, the rest of it is nowhere in the swarm
						else
							s.SetAt(i, L'U'); // Piece unavailable, it's entirely missing
					}
				}
			}
		}

		// Copy the string we made into the given structure
		info->pieces = s;

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Takes the infohash of a torrent, the index of a file in it, and the desired priority for that file
// Sets the priority to the file in the torrent
void SetFilePriority(const char *id, int index, int priority) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.file_priority(index, priority);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Takes the infohash of a torrent, a pointer to a block of ints, and the number of ints in that block
// Sets those priorities on the files in the torrent
void SetFilePriorities(const char *id, int *priorities, int n) {
	try {

		// Convert the given array of ints into a vector of ints
		std::vector<int> v;
		for (int i = 0; i < n; i++) {
			int priority = *priorities;
			v.push_back(priority); // Add it to the end of the list
			priorities++; // Move the pointer forward to the next int
		}

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.prioritize_files(v);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Tell libtorrent the host name and port number of a peer on the DHT
void AddDhtNode(const char *address, int port) {
	try {

		App.session->add_dht_node(std::pair<std::string, int>(std::string(address), port));

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Tell libtorrent the domain name and port number of a DHT bootstrapping router
void AddDhtRouter(const char *address, int port) {
	try {

		App.session->add_dht_router(std::pair<std::string, int>(std::string(address), port));

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Takes the path to DHT information saved from a previous session
// Starts the DHT, using the file if it's there
void StartDht(const wchar_t *path) {
	try {

		// Open the file
		std::wstring w(path);
		boost::filesystem::wpath p(w);
		boost::filesystem::ifstream f(p, std::ios_base::binary);

		// Initialize variables
		libtorrent::entry e = 0; // The bencoded DHT state information from the file
		bool b = false; // True once we've gotten information from the file

		if (!f.fail()) {
			try {

				// Read the file, parsing its contents into a libtorrent entry object
				e = libtorrent::bdecode(std::istream_iterator<char>(f), std::istream_iterator<char>());
				b = true; // Record that worked without an exception

			} catch (std::exception &e) { // No file or bad information inside, we'll just start the DHT without resume data
				log(widenPtoC(e.what())); // Log a note but keep going
			}
		}

		// Start the DHT
		if (b) App.session->start_dht(e); // With the file information we got
		else   App.session->start_dht();  // Without any information from a previous session

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Have libtorrent disconnect from the DHT
void StopDht() {
	try {

		App.session->stop_dht();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Start the local service directory, which uses multicast to find peers on the LAN
void StartLsd() {
	try {

		App.session->start_lsd();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Stop the local service directory
void StopLsd() {
	try {

		App.session->stop_lsd();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Create port mappings on a UPnP device on the LAN
void StartUpnp() {
	try {

		App.session->start_upnp(); // Returns an object you could use

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Stop the UPnP service
void StopUpnp() {
	try {

		App.session->stop_upnp();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Use NAT-PMP to map ports on the router
void StartNatpmp() {
	try {

		App.session->start_natpmp(); // Returns an object you could use

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

// Stop the NAT-PMP service
void StopNatpmp() {
	try {

		App.session->stop_natpmp();

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}
