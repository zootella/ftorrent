
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
		log(StringToCString(e.what()));
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
		log(StringToCString(e.what()));
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
		Handle.session->listen_on(std::make_pair(settings->listen_start_port, settings->listen_end_port), settings->listen_interface);

		Handle.session->set_upload_rate_limit(settings->max_upload_bandwidth);
		Handle.session->set_download_rate_limit(settings->max_download_bandwidth);

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void InitializeLibtorrent(settings_structure *setting) {
	try {

		Handle.session = new libtorrent::session;

		if (setting) // Added this
			UpdateSettings(setting);

		Handle.session->add_extension(&libtorrent::create_metadata_plugin);
		Handle.session->add_extension(&libtorrent::create_ut_metadata_plugin);
		Handle.session->add_extension(&libtorrent::create_ut_pex_plugin);
		Handle.session->add_extension(&libtorrent::create_smart_ban_plugin);

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void AbortTorrents() {
	try {

		log(L"abort torrents called");

		if (Handle.session) {

			log(L"session->abort");

			Handle.session->abort();


			log(L"abort finished");
		}

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void MoveTorrent(const char *id, wchar_t *path) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.pause();
		libtorrent::storage_interface *storage = h.get_storage_impl();
		storage->release_files();

		std::string s1;
		libtorrent::wchar_utf8(path, s1);
		storage->move_storage(boost::filesystem::path(s1));
		h.resume();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void AddTorrent(char *sha1String, char *trackerURI, wchar_t *torrentPath, wchar_t *savePath, wchar_t *fastResumePath) {
	try {

		log(L"adding torrent");
		log(make(L"sha1String", StringToCString(sha1String)));
		log(make(L"trackerURI", StringToCString(trackerURI)));
		log(make(L"torrentPath: ", torrentPath));
		log(make(L"resumeFilePath: ", fastResumePath));

		libtorrent::big_number sha1 = StringToHash(sha1String);

		libtorrent::add_torrent_params torrent_params;
		std::string s1;
		libtorrent::wchar_utf8(savePath, s1);
		torrent_params.save_path = boost::filesystem::path(s1);
		torrent_params.info_hash = sha1;
		torrent_params.tracker_url = trackerURI;
		torrent_params.auto_managed = false; //TODO change this to true
		torrent_params.duplicate_is_error = true;

		std::vector<char> resume_buf;

		if (torrentPath) {
			boost::filesystem::ifstream torrent_file(torrentPath, std::ios_base::binary);
			if (!torrent_file.fail()) {
				std::string s2;
				libtorrent::wchar_utf8(torrentPath, s2);
				torrent_params.ti = new libtorrent::torrent_info(boost::filesystem::path(s2));
			} else {
				log(L"could not find torrent file");
			}
			torrent_file.close();
		}

		if (fastResumePath) {
			boost::filesystem::ifstream resume_file(fastResumePath, std::ios_base::binary);

			if (!resume_file.fail()) {
				resume_file.unsetf(std::ios_base::skipws);

				std::istream_iterator<char> ios_iter;
				std::istream_iterator<char> iter(resume_file);

				std::copy(iter, ios_iter, std::back_inserter(resume_buf));

				torrent_params.resume_data = &resume_buf;
				resume_file.close();
			} else {
				log(L"could not find fast resume file");
			}
		}

		libtorrent::torrent_handle h = Handle.session->add_torrent(torrent_params);

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void PauseTorrent(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.pause();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void SetAutoManagedTorrent(const char *id, bool auto_managed) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.auto_managed(auto_managed);

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void RemoveTorrent(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.pause();
		Handle.session->remove_torrent(h);

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void ResumeTorrent(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.resume();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void ForceReannounce(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.force_reannounce();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void ScrapeTracker(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.scrape_tracker();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetTorrentStatus(const char *id, status_structure *stats) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		GetWrapperTorrentStatus(h, stats);

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void FreeTorrentStatus(status_structure *info) {
	try {

		delete[] info->error;
		delete[] info->current_tracker;

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetTorrentInfo(const char *id, torrent_structure *info) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		libtorrent::torrent_info i = h.get_torrent_info();

//		char *created_by = CopyString(i.creator().c_str());
		char *comment = CopyString(i.comment().c_str());

		info->created_by = StringToCString(i.creator().c_str());
		info->comment = comment;

		info->sha1 = HashToString(i.info_hash());

		long long total_size = i.total_size();
		info->total_size = total_size;

		int piece_length = i.piece_length();
		info->piece_length = piece_length;

		//add tracker information to the torrent info
		std::vector<libtorrent::announce_entry> announce_entries = i.trackers();
		int num_trackers = announce_entries.size();
		announce_structure *trackers = new announce_structure[num_trackers];
		std::vector<libtorrent::announce_entry>::iterator iter = announce_entries.begin();

		int tracker_index = 0;
		while (iter != announce_entries.end()) {
			libtorrent::announce_entry entry = *iter;
			trackers[tracker_index].url = CopyString(entry.url.c_str());
			trackers[tracker_index].tier = entry.tier;
			tracker_index++;
			iter++;
		}
		info->trackers = trackers;
		info->num_trackers= num_trackers;

		//add seeding information to the torrent info
		std::vector<std::string> seed_entries = i.url_seeds();
		int num_seeds = seed_entries.size();
		announce_structure *seeds = new announce_structure[num_seeds];
		std::vector<std::string>::iterator iter2 = seed_entries.begin();

		int seed_index = 0;
		while (iter2 != seed_entries.end()) {
			seeds[seed_index].url = CopyString(iter2->c_str());
			seeds[seed_index].tier = -1;
			seed_index++;
			iter2++;
		}
		info->seeds = seeds;
		info->num_seeds=num_seeds;

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void FreeTorrentInfo(torrent_structure *info) {
	try {

		delete[] info->sha1;
//		delete[] info->created_by;
		delete[] info->comment;
		delete[] info->trackers;
		delete[] info->seeds;

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void SignalFastResumeDataRequest(const char *id) {
	try {

		log(make(L"signal fast resume data request: ", StringToCString(id)));

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		if (h.has_metadata()) {
			log(L"has metadata");
			h.save_resume_data();
			log(make(L"save resume data called on torrent handle: ", StringToCString(id)));
		}

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void ClearErrorAndRetry(const char *id) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.force_recheck();
		h.clear_error();
		h.resume();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetNumPeers(const char *id, int &num_peers) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);

		std::vector<libtorrent::peer_info> peers;
		try {
			h.get_peer_info(peers);
		} catch (libtorrent::invalid_handle e) {
			return;
		} catch (std::exception e) {
			return;
		}
		num_peers = peers.size();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void HasMetadata(const char *id, int &has_metadata) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		has_metadata = h.has_metadata();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void IsValid(const char *id, int &is_valid) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		is_valid = h.is_valid();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetPeers(const char *id, peer_structure **torrent_peers, int numPeers) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);

		std::vector<libtorrent::peer_info> peers;
		h.get_peer_info(peers);

		std::vector<libtorrent::peer_info>::iterator iter = peers.begin();

		int index = 0;
		while (iter != peers.end()) {

			libtorrent::peer_info peer = *iter;
			std::string address = peer.ip.address().to_string();
			log(make(L"peer:", StringToCString(address)));

			peer_structure *torrent_peer = *torrent_peers;
			torrent_peers++;
			torrent_peer->status_flags = peer.flags;
			torrent_peer->ip = CopyString(address.c_str());
			torrent_peer->source = peer.source;
			torrent_peer->up_speed = (float)peer.up_speed;
			torrent_peer->down_speed = (float)peer.down_speed;
			torrent_peer->payload_up_speed = (float)peer.payload_up_speed;
			torrent_peer->payload_down_speed = (float)peer.payload_down_speed;
			torrent_peer->peer_id = PeerIdToString(peer.pid);
			torrent_peer->progress = peer.progress;

			torrent_peer->country[0] = peer.country[0];
			torrent_peer->country[1] = peer.country[1];
			torrent_peer->country[2] = '\0';

			try {
				std::wstring w;
				libtorrent::utf8_wchar(peer.client.c_str(), w);
				torrent_peer->client_name = CopyWideString(w.c_str());
			} catch (std::runtime_error &e) {
				log(StringToCString(e.what()));
				torrent_peer->client_name = 0;			
			}
			

			index++;
			if(index >= numPeers) {
				break;
			}
			iter++;
		}

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void FreePeers(peer_structure **torrent_peers, int numPeers) {
	try {

		for (int i = 0; i < numPeers; i++) {
			peer_structure *torrent_peer = *torrent_peers;
			delete[] torrent_peer->ip;
			delete[] torrent_peer->peer_id;
			delete[] torrent_peer->client_name;
			torrent_peers++;
		}

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
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
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void SetSeedRatio(const char *id, float seed_ratio) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.set_ratio(seed_ratio);

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetNumFiles(const char *id, int &num_files) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		libtorrent::torrent_info info = h.get_torrent_info();
		libtorrent::file_storage files = info.files();
		num_files = files.num_files();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
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
			std::wstring wpath;
			libtorrent::utf8_wchar(path.string(), wpath);
			file_entry->path = CopyWideString(wpath.c_str());
			file_entry->size = iter->size;
			file_entry->total_done = progress[index];
			file_entry->priority = priorities[index];
			file_entries++;
			index++;
			iter++;
		}

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
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
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void SetFilePriority(const char *id, int index, int priority) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);
		h.file_priority(index, priority);

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StartDht(const wchar_t *dht_state_file_path) {
	try {

		std::wstring file(dht_state_file_path);
		boost::filesystem::wpath full_path(file);

		boost::filesystem::ifstream dht_state_file(
							full_path,
							std::ios_base::binary);

		libtorrent::entry dht_state = 0;
		bool state_loaded = false;
		if(!dht_state_file.fail()) {
			try {
				dht_state = libtorrent::bdecode(std::istream_iterator<char>(dht_state_file), std::istream_iterator<char>());
				state_loaded = true;
			} catch (std::exception& e) {
				log(StringToCString(e.what()));
				//no dht to resume will start dht without a prebuilt state
			}
		}

		if(state_loaded) {
			Handle.session->start_dht(dht_state);
		} else {
			Handle.session->start_dht();
		}

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void AddDhtRouter(const char *address, int port) {
	try {

		Handle.session->add_dht_router(std::pair<std::string, int>(std::string(address), port));

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void AddDhtNode(const char *address, int port) {
	try {

		Handle.session->add_dht_node(std::pair<std::string, int>(std::string(address), port));

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
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
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StopDht() {
	try {

		Handle.session->stop_dht();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StartUpnp() {
	try {

		Handle.session->start_upnp();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StopUpnp() {
	try {

		Handle.session->stop_upnp();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StartLsd() {
	try {

		Handle.session->start_lsd();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StopLsd() {
	try {

		Handle.session->stop_lsd();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StartNatpmp() {
	try {

		Handle.session->start_natpmp();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void StopNatpmp() {
	try {

		Handle.session->stop_natpmp();

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void FreePiecesInfo(pieces_structure *info) {
	try {

		delete[] info->pieces;

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void GetPiecesStatus(const char *id, pieces_structure *info) {
	try {

		libtorrent::torrent_handle h = FindTorrentHandle(id);

		libtorrent::bitfield piece_downloaded_info = h.status().pieces;

		int num_pieces = piece_downloaded_info.size();

		char *pieces = new char[num_pieces+1];
		info->pieces = pieces;
		info->completed = 0;

		// Clear the array
		for (int i = 0; i < num_pieces; i++) {
			pieces[i] = '0'; // Piece pending
		}
		pieces[num_pieces] = '\0';

		std::vector<libtorrent::peer_info> peers;
		h.get_peer_info(peers);
		std::vector<libtorrent::peer_info>::iterator iter = peers.begin();
		while (iter != peers.end()) {
			if (iter->downloading_piece_index > -1 && iter->downloading_piece_index < num_pieces) {
				// Mark downloading pieces
				pieces[iter->downloading_piece_index] = 'a'; // Piece active
			}
			iter++;
		}

		std::vector<libtorrent::partial_piece_info> download_queue;
		h.get_download_queue(download_queue);
		std::vector<libtorrent::partial_piece_info>::iterator queue_iter = download_queue.begin();
		while (queue_iter != download_queue.end()) {
			libtorrent::partial_piece_info piece = *queue_iter;
			if (piece.piece_index > -1 && piece.piece_index < num_pieces) {
				if (pieces[piece.piece_index] != 'a') { // Piece active
					if (piece.writing > 0) {
						pieces[piece.piece_index] = 'a'; // Piece active
					} else if (piece.requested > 0 && piece.finished > 0) {
						pieces[piece.piece_index] = 'p'; // Piece partial
					} else if (piece.finished == 0) {
						pieces[piece.piece_index] = 'q'; // Piece queued
					}
				}
			}

			queue_iter++;
		}

		for (int i = 0; i < num_pieces; i++) {
			if (pieces[i] != 'a') { // Piece active
				if (piece_downloaded_info[i]) {
					// Mark downloaded pieces
					pieces[i] = 'x'; // Piece downloaded
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
						if (pieces[i] == 'p') { // Piece partial
							pieces[i] = 'u'; // Piece unavailable partial
						} else {
							pieces[i] = 'U'; // Piece unavailable
						}
					}
				}
			}
		}

	} catch (std::exception &e) {
		log(StringToCString(e.what()));
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
		log(StringToCString(e.what()));
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
		log(StringToCString(e.what()));
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
		log(StringToCString(e.what()));
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
		log(StringToCString(e.what()));
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
		log(StringToCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}
