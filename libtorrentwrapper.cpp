
#include "boost/shared_ptr.hpp"
#include "boost/asio/ip/address.hpp"
#include "boost/filesystem/path.hpp"

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

#include "libtorrent/extensions/metadata_transfer.hpp"
#include "libtorrent/extensions/ut_metadata.hpp"
#include "libtorrent/extensions/ut_pex.hpp"
#include "libtorrent/extensions/smart_ban.hpp"

#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>

#include "resource.h"
#include "program.h"
#include "object.h"
#include "top.h"
#include "function.h"

extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;



libtorrent::session *session = NULL;

typedef libtorrent::big_number sha1_hash;
typedef boost::asio::ip::address address;

#define PIECE_DOWNLOADED 'x'
#define PIECE_PARTIAL 'p'
#define PIECE_PENDING '0'
#define PIECE_ACTIVE 'a'
#define PIECE_UNAVAILABLE 'U'
#define PIECE_UNAVAILABLE_PARTIAL 'u'
#define PIECE_QUEUED 'q'

#ifdef NO_ERROR
#define EXTERN_RET int
#define wTHROW(x) return 0;
#else
#define EXTERN_RET wrapper_exception*
#define wTHROW(x) if (last_error != NULL) { delete last_error; last_error=NULL;} last_error = x; return last_error;
#endif

#ifdef _DLL
#define EXTERN_HEADER extern "C" __declspec(dllexport)
#define WINAPI __stdcall
#else
#define EXTERN_HEADER extern "C"
#endif

#define EXCEPTION_UNKNOWN_RETHROWN 0
#define EXCEPTION_RETHROWN 1
#define EXCEPTION_MANUALLY_THROWN 2


wchar_t *mywcsdup(const wchar_t *str) {
	int len = wcslen(str);
	wchar_t *copy = new wchar_t[len + 1];
	wcsncpy_s(copy, len + 1, str, len + 1); // Changed from wcsncpy(copy, str, len + 1)
	return copy;
}

char *mystrdup(const char *str) {
	int len = std::strlen(str);
	char *copy = new char[len + 1];
	strncpy_s(copy, len + 1, str, len + 1); // Changed from strncpy(copy, str, len + 1)
	return copy;
}

struct wrapper_exception {
	int type;
	const char *message;

	wrapper_exception(std::exception &e) {
		this->type = EXCEPTION_RETHROWN;
		this->message = e.what();
	}

	wrapper_exception(char *message) {
		this->type = EXCEPTION_MANUALLY_THROWN;
		this->message = message;
	}

	wrapper_exception() {
		this->type = EXCEPTION_UNKNOWN_RETHROWN;
		this->message = "unfilled";
	}
};

wrapper_exception *last_error = NULL;

struct wrapper_torrent_status {
	long long total_done;
	long long total_wanted_done;
	long long total_wanted;
	long long total_download;
	long long total_upload;
	long long total_payload_download;
	long long total_payload_upload;
	long long all_time_payload_download;
	long long all_time_payload_upload;
	float download_rate;
	float upload_rate;
	float download_payload_rate;
	float upload_payload_rate;
	int num_peers;
	int num_uploads;
	int num_seeds;
	int num_connections;
	int state;
	float progress;
	int paused;
	int finished;
	int valid;
	int auto_managed;
	int seeding_time;
	int active_time;
	const char *error;
	const char *current_tracker;
	int num_complete;
	int num_incomplete;
	long long total_failed_bytes;
};

struct wrapper_announce_entry {
	const char *url;
	int tier;

	~wrapper_announce_entry() {
		delete[] url;
	}
};

struct wrapper_proxy_settings {
	char *hostname;
	int port;
	char *username;
	char *password;
	int type;
};

struct wrapper_torrent_info {
	const char *sha1;
	long long total_size;
	int piece_length;
	wrapper_announce_entry *trackers;
	int num_trackers;
	wrapper_announce_entry *seeds;
	int num_seeds;
	const char *created_by;
	const char *comment;
};

struct wrapper_session_settings {
	int max_upload_bandwidth;
	int max_download_bandwidth;
	int listen_start_port;
	int listen_end_port;
	float seed_ratio_limit;
	float seed_time_ratio_limit;
	int seed_time_limit;
	int active_downloads_limit;
	int active_seeds_limit;
	int active_limit;
	int alert_mask;
	char *listen_interface;
};

struct wrapper_file_entry {
	int index;
	wchar_t *path;
	long long size;
	long long total_done;
	int priority;
};

struct wrapper_alert_info {
	int category;
	const char *sha1;
	const char *message;
	bool has_data;
	libtorrent::entry *resume_data;

	wrapper_alert_info() {
		category = -1;
		sha1 = NULL;
		message = NULL;
		has_data = 0;
		resume_data = NULL;
	}
};

struct wrapper_torrent_peer {
	int status_flags;
	const char *peer_id;
	const char *ip;
	int source;
	float up_speed;
	float down_speed;
	float payload_up_speed;
	float payload_down_speed;
	float progress;
	char country[3];
	const wchar_t *client_name;
};

const char *getString(const std::stringstream &oss) {
	std::string str = oss.str();
	return mystrdup(str.c_str());
}

const char *getSha1String(const libtorrent::sha1_hash &sha1) {
	std::stringstream oss;
	oss << sha1;
	return getString(oss);
}

const char *getPeerIdString(const libtorrent::peer_id &pid) {
	std::stringstream oss;
	oss << pid;
	return getString(oss);
}

sha1_hash getSha1Hash(const char *sha1String) {
	sha1_hash sha1;
	std::stringstream oss;
	oss << sha1String;
	oss >> sha1;
	return sha1;
}

void get_wrapper_torrent_status(libtorrent::torrent_handle handle, wrapper_torrent_status *stats) {

	libtorrent::torrent_status status = handle.status();

	char *error = mystrdup(status.error.c_str());

	float download_rate = status.download_rate;
	float upload_rate = status.upload_rate;
	float download_payload_rate = status.download_payload_rate;
	float upload_payload_rate = status.upload_payload_rate;

	int num_peers = status.num_peers;
	int num_uploads = status.num_uploads;
	int num_seeds = status.num_seeds;
	int num_connections = status.num_connections;
	int state = status.state;
	float progress = status.progress;
	bool paused = status.paused;
	bool finished = handle.is_finished();
	bool valid = handle.is_valid();
	bool auto_manged = handle.is_auto_managed();

	long long total_done = status.total_done;
	long long total_wanted_done = status.total_wanted_done;
	long long total_wanted = status.total_wanted;
	long long total_download = status.total_download;
	long long total_upload = status.total_upload;
	long long total_payload_download = status.total_payload_download;
	long long total_payload_upload = status.total_payload_upload;
	long long all_time_payload_download = status.all_time_download;
	long long all_time_payload_upload = status.all_time_upload;
	int seeding_time = status.seeding_time;
	int active_time = status.active_time;
	char *current_tracker = mystrdup(status.current_tracker.c_str());
	int num_complete = status.num_complete;
	int num_incomplete = status.num_incomplete;
	long long total_failed_bytes = status.total_failed_bytes;

	stats->total_done = total_done;
	stats->total_wanted_done = total_wanted_done;
	stats->total_wanted = total_wanted;
	stats->total_download = total_download;
	stats->total_upload = total_upload;
	stats->total_payload_download = total_payload_download;
	stats->total_payload_upload = total_payload_upload;
	stats->all_time_payload_download = all_time_payload_download;
	stats->all_time_payload_upload = all_time_payload_upload;
	stats->download_rate = download_rate;
	stats->upload_rate = upload_rate;
	stats->download_payload_rate = download_payload_rate;
	stats->upload_payload_rate = upload_payload_rate;
	stats->num_peers = num_peers;
	stats->num_uploads = num_uploads;
	stats->num_seeds = num_seeds;
	stats->num_connections = num_connections;
	stats->state = state;
	stats->progress = progress;
	stats->paused = paused;
	stats->finished = finished;
	stats->valid = valid;
	stats->auto_managed = auto_manged;
	stats->seeding_time = seeding_time;
	stats->active_time = active_time;
	stats->error = error;
	stats->current_tracker = current_tracker;
	stats->num_complete = num_complete;
	stats->num_incomplete = num_incomplete;
	stats->total_failed_bytes = total_failed_bytes;
}

libtorrent::torrent_handle findTorrentHandle(const char *sha1String) {

	sha1_hash sha1 = getSha1Hash(sha1String);
	libtorrent::torrent_handle torrent_handle = session->find_torrent(sha1);
	return torrent_handle;
}

void process_save_resume_data_alert(libtorrent::torrent_handle handle, libtorrent::save_resume_data_alert const *alert, wrapper_alert_info *alertInfo) {

	const boost::shared_ptr<libtorrent::entry> resume_ptr = alert->resume_data;
	alertInfo->has_data = 1;
	alertInfo->resume_data=resume_ptr.get();
}

void process_alert(libtorrent::alert const *alert, wrapper_alert_info *alertInfo) {

	alertInfo->category = alert->category();
	alertInfo->message = mystrdup(alert->message().c_str());

	libtorrent::torrent_alert const *torrentAlert;

	if ((torrentAlert = dynamic_cast<libtorrent::torrent_alert const*> (alert))) {

		libtorrent::torrent_handle handle = torrentAlert->handle;

		if (handle.is_valid()) {
			alertInfo->sha1 = getSha1String(handle.info_hash());

			libtorrent::save_resume_data_alert const *srd_alert = dynamic_cast<libtorrent::save_resume_data_alert const*> (alert);
			if (srd_alert) {
				process_save_resume_data_alert(handle, srd_alert, alertInfo);
				return;
			}

			libtorrent::save_resume_data_failed_alert const *srdf_alert = dynamic_cast<libtorrent::save_resume_data_failed_alert const*> (alert);
			if (srdf_alert) {
				//std::cout << "save_resume_data_failed_alert (" << srdf_alert->msg << ")" << std::endl;
				delete[] alertInfo->message;
				alertInfo->message = mystrdup(srdf_alert->msg.c_str());
				return;
			}

			libtorrent::fastresume_rejected_alert const *fra_alert = dynamic_cast<libtorrent::fastresume_rejected_alert const*> (alert);
			if (fra_alert) {
				//std::cout << "fastresume_rejected_alert (" << fra_alert->msg << ")" << std::endl;
				delete[] alertInfo->message;
				alertInfo->message = mystrdup(fra_alert->msg.c_str());
				return;
			}
		}
	}
}

extern "C" int save_fast_resume_data(wrapper_alert_info *alert, wchar_t *filePath) {
	try {

		std::wstring file(filePath);
		boost::filesystem::wpath full_path(file);
		boost::filesystem::ofstream out(full_path, std::ios_base::binary);
		out.unsetf(std::ios_base::skipws);
		libtorrent::entry *resume_data = alert->resume_data;
		libtorrent::bencode(std::ostream_iterator<char>(out), *resume_data);
		out.close();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		log(L"exception");
	}
	return 0;
}

// Ported from http://www.rasterbar.com/products/libtorrent/manual.html#save-resume-data
extern "C" int freeze_and_save_all_fast_resume_data(void(*alertCallback)(void*)) {
	try {

		int num_resume_data = 0;

		std::vector<libtorrent::torrent_handle> handles = session->get_torrents();
		session->pause();

		for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin(); i != handles.end(); ++i) {
			libtorrent::torrent_handle &h = *i;
			if (!h.has_metadata())
				continue;
			if (!h.is_valid())
				continue;

			h.save_resume_data();
			++num_resume_data;
			//std::cout << "num_resume: " << num_resume_data << std::endl;
		}

		while (num_resume_data > 0) {
			//std::cout << "waiting for resume: " << num_resume_data << std::endl;

			libtorrent::alert const *alert = session->wait_for_alert(libtorrent::seconds(10));

			// if we don't get an alert within 10 seconds, abort
			if (alert == NULL)
				break;

			std::auto_ptr<libtorrent::alert> holder = session->pop_alert();

			wrapper_alert_info *alertInfo = new wrapper_alert_info();
			process_alert(alert, alertInfo);

			const char *sha1 = alertInfo->sha1;
			const char *message = alertInfo->message;
			alertCallback(alertInfo);

			if (alertInfo->has_data) {
				//std::cout << "resume_found: " << std::endl;
				--num_resume_data;
			}
			delete[] sha1;
			delete[] message;
			delete alertInfo;
		}

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}



extern "C" int update_settings(wrapper_session_settings *settings) {
	try {

		libtorrent::session_settings *s = new libtorrent::session_settings;
		s->use_dht_as_fallback   = false;
		s->share_ratio_limit     = settings->seed_ratio_limit;
		s->seed_time_ratio_limit = settings->seed_time_ratio_limit;
		s->seed_time_limit       = settings->seed_time_limit;
		s->active_downloads      = settings->active_downloads_limit;
		s->active_seeds          = settings->active_seeds_limit;
		s->active_limit          = settings->active_limit;

		session->set_settings(*s);
		session->set_alert_mask(settings->alert_mask);
		session->listen_on(std::make_pair(settings->listen_start_port, settings->listen_end_port), settings->listen_interface);

		session->set_upload_rate_limit(settings->max_upload_bandwidth);
		session->set_download_rate_limit(settings->max_download_bandwidth);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int init(wrapper_session_settings *setting) {
	try {

		session = new libtorrent::session;

		if (setting) // Added this
			update_settings(setting);

		session->add_extension(&libtorrent::create_metadata_plugin);
		session->add_extension(&libtorrent::create_ut_metadata_plugin);
		session->add_extension(&libtorrent::create_ut_pex_plugin);
		session->add_extension(&libtorrent::create_smart_ban_plugin);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}


int step = 0;



void mytest() {
	try {

		if (step == 0) {

			init(NULL);
			step = 1;

			log(L"init done");

		} else if (step == 1) {

			libtorrent::add_torrent_params torrent_params;

			torrent_params.save_path = boost::filesystem::path(w2s(L"C:\\Documents\\test"));
			torrent_params.ti = new libtorrent::torrent_info(boost::filesystem::path(w2s(L"C:\\Documents\\my.torrent")));
			libtorrent::torrent_handle h = session->add_torrent(torrent_params);

			log(L"add done");
		}

	} catch (std::exception &e) {
		log(s2c(e.what()));
	} catch (...) {
		log(L"exception");
	}
}


extern "C" int abort_torrents() {
	try {

		//std::cout << "abort_torrents() called" << std::endl;

		if(session) {

			//std::cout << "session->abort" << std::endl;

			session->abort();

			// clean up ip_filter callback method objects, if necessary
			// set_ip_filter_internal(NULL);

			//std::cout << "abort finished" << std::endl;
		}

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int move_torrent(const char *id, wchar_t *path) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.pause();
		libtorrent::storage_interface *storage = h.get_storage_impl();
		storage->release_files();

		std::string s1;
		libtorrent::wchar_utf8(path, s1);
		storage->move_storage(boost::filesystem::path(s1));
		h.resume();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

int my_add_torrent() {
	try {

		/*
		libtorrent::add_torrent_params torrent_params;
		torrent_params.save_path = boost::filesystem::path(libtorrent::wchar_utf8("C:\\Documents\\test");
		torrent_params.ti = new libtorrent::torrent_info(boost::filesystem::path(libtorrent::wchar_utf8("C:\\Documents\\creative commons.torrent"));
		libtorrent::torrent_handle h = session->add_torrent(torrent_params);
		*/

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}


extern "C" int add_torrent(char *sha1String, char *trackerURI, wchar_t *torrentPath, wchar_t *savePath, wchar_t *fastResumePath) {
	try {

		//std::cout << "adding torrent" << std::endl;
		//std::cout << "sha1String" << sha1String << std::endl;
		//std::cout << "trackerURI" << trackerURI << std::endl;
		//std::cout << "torrentPath: " << torrentPath << std::endl;
		//std::cout << "resumeFilePath: " << fastResumePath << std::endl;

		sha1_hash sha1 = getSha1Hash(sha1String);

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
			}
			//else {
			//	std::cout << "could not find torrent file" << std::endl;
			//}
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
			}
			//else {
			//	std::cout << "could not find fast resume file" << std::endl;
			//}
		}

		libtorrent::torrent_handle h = session->add_torrent(torrent_params);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int pause_torrent(const char *id) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.pause();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int set_auto_managed_torrent(const char *id, bool auto_managed) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.auto_managed(auto_managed);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int remove_torrent(const char *id) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.pause();
		session->remove_torrent(h);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int resume_torrent(const char *id) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.resume();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int force_reannounce(const char *id) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.force_reannounce();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int scrape_tracker(const char *id) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.scrape_tracker();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int get_torrent_status(const char *id, wrapper_torrent_status *stats) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		get_wrapper_torrent_status(h, stats);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int free_torrent_status(wrapper_torrent_status *info) {
	try {

		delete[] info->error;
		delete[] info->current_tracker;

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int get_torrent_info(const char *id, wrapper_torrent_info *info) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		libtorrent::torrent_info i = h.get_torrent_info();
		char *created_by = mystrdup(i.creator().c_str());
		char *comment = mystrdup(i.comment().c_str());
		info->created_by = created_by;
		info->comment = comment;
		info->sha1 = getSha1String(i.info_hash());
		long long total_size = i.total_size();
		info->total_size = total_size;

		int piece_length = i.piece_length();
		info->piece_length = piece_length;

		//add tracker information to the torrent info
		std::vector<libtorrent::announce_entry> announce_entries = i.trackers();
		int num_trackers = announce_entries.size();
		wrapper_announce_entry *trackers = new wrapper_announce_entry[num_trackers];
		std::vector<libtorrent::announce_entry>::iterator iter = announce_entries.begin();

		int tracker_index = 0;
		while (iter != announce_entries.end()) {
			libtorrent::announce_entry entry = *iter;
			trackers[tracker_index].url = mystrdup(entry.url.c_str());
			trackers[tracker_index].tier = entry.tier;
			tracker_index++;
			iter++;
		}
		info->trackers = trackers;
		info->num_trackers= num_trackers;

		//add seeding information to the torrent info
		std::vector<std::string> seed_entries = i.url_seeds();
		int num_seeds = seed_entries.size();
		wrapper_announce_entry *seeds = new wrapper_announce_entry[num_seeds];
		std::vector<std::string>::iterator iter2 = seed_entries.begin();

		int seed_index = 0;
		while (iter2 != seed_entries.end()) {
			seeds[seed_index].url = mystrdup(iter2->c_str());
			seeds[seed_index].tier = -1;
			seed_index++;
			iter2++;
		}
		info->seeds = seeds;
		info->num_seeds=num_seeds;

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int free_torrent_info(wrapper_torrent_info *info) {
	try {

		delete[] info->sha1;
		delete[] info->created_by;
		delete[] info->comment;
		delete[] info->trackers;
		delete[] info->seeds;

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int signal_fast_resume_data_request(const char *id) {
	try {

		//std::cout << "signal_fast_resume_data_request: " << id << std::endl;

		libtorrent::torrent_handle h = findTorrentHandle(id);
		if (h.has_metadata()) {
			//std::cout << "has_metadata" << std::endl;
			h.save_resume_data();
			//std::cout << "save_resume_data called on torrent handle: " << id << std::endl;
		}

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int clear_error_and_retry(const char *id) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.force_recheck();
		h.clear_error();
		h.resume();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int get_num_peers(const char *id, int &num_peers) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);

		std::vector<libtorrent::peer_info> peers;
		try {
			h.get_peer_info(peers);
		} catch (libtorrent::invalid_handle e) {
			return 0;
		} catch (std::exception e) {
			return 0;
		}
		num_peers = peers.size();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int has_metadata(const char *id, int &has_metadata) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		has_metadata = h.has_metadata();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int is_valid(const char *id, int &is_valid) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		is_valid = h.is_valid();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int get_peers(const char *id, wrapper_torrent_peer **torrent_peers, int numPeers) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);

		std::vector<libtorrent::peer_info> peers;
		h.get_peer_info(peers);

		std::vector<libtorrent::peer_info>::iterator iter = peers.begin();

		int index = 0;
		while (iter != peers.end()) {

			libtorrent::peer_info peer = *iter;
			std::string address = peer.ip.address().to_string();
			//std::cout << "peer:" << address << std::endl;

			wrapper_torrent_peer *torrent_peer = *torrent_peers;
			torrent_peers++;
			torrent_peer->status_flags = peer.flags;
			torrent_peer->ip = mystrdup(address.c_str());
			torrent_peer->source = peer.source;
			torrent_peer->up_speed=peer.up_speed;
			torrent_peer->down_speed=peer.down_speed;
			torrent_peer->payload_up_speed=peer.payload_up_speed;
			torrent_peer->payload_down_speed=peer.payload_down_speed;
			torrent_peer->peer_id = getPeerIdString(peer.pid);
			torrent_peer->progress = peer.progress;

			torrent_peer->country[0] = peer.country[0];
			torrent_peer->country[1] = peer.country[1];
			torrent_peer->country[2] = '\0';

			try {
				std::wstring w;
				libtorrent::utf8_wchar(peer.client.c_str(), w);
				torrent_peer->client_name = mywcsdup(w.c_str());
			} catch (std::runtime_error &e) {
				OutputDebugStringA(e.what());
				torrent_peer->client_name = 0;			
			}
			

			index++;
			if(index >= numPeers) {
				break;
			}
			iter++;
		}

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int free_peers(wrapper_torrent_peer **torrent_peers, int numPeers) {
	try {

		for (int i = 0; i < numPeers; i++) {
			wrapper_torrent_peer *torrent_peer = *torrent_peers;
			delete[] torrent_peer->ip;
			delete[] torrent_peer->peer_id;
			delete[] torrent_peer->client_name;
			torrent_peers++;
		}

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int get_alerts(void(*alertCallback)(void*)) {
	try {

		std::auto_ptr<libtorrent::alert> alerts;

		alerts = session->pop_alert();

		while (alerts.get()) {

			libtorrent::alert *alert = alerts.get();

			wrapper_alert_info *alertInfo = new wrapper_alert_info();

			process_alert(alert, alertInfo);
			const char *sha1 = alertInfo->sha1;
			const char *message = alertInfo->message;
			alertCallback(alertInfo);
			delete[] sha1;
			delete[] message;
			delete alertInfo;

			alerts = session->pop_alert();
		}

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int set_seed_ratio(const char *id, float seed_ratio) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.set_ratio(seed_ratio);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int get_num_files(const char *id, int &num_files) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		libtorrent::torrent_info info = h.get_torrent_info();
		libtorrent::file_storage files = info.files();
		num_files = files.num_files();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int get_files(const char *id, wrapper_file_entry **file_entries) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		libtorrent::torrent_info info = h.get_torrent_info();
		libtorrent::file_storage files = info.files();

		int index = 0;
		std::vector<libtorrent::size_type> progress;
		h.file_progress(progress);
		std::vector<int> priorities = h.file_priorities();

		std::vector<libtorrent::file_entry>::const_iterator iter = files.begin();
		while (iter != files.end()) {
			boost::filesystem::path path = iter->path;
			wrapper_file_entry *file_entry = *file_entries;
			file_entry->index = index;
			std::wstring wpath;
			libtorrent::utf8_wchar(path.string(), wpath);
			file_entry->path = mywcsdup(wpath.c_str());
			file_entry->size = iter->size;
			file_entry->total_done = progress[index];
			file_entry->priority = priorities[index];
			file_entries++;
			index++;
			iter++;
		}

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int set_file_priorities(const char *id, int *priorities, int num_priorities) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		std::vector<int> priorities_vector;

		for(int i = 0; i < num_priorities; i++) {
			int priority = *priorities;
			priorities_vector.push_back(priority);
			priorities++;
		}

		h.prioritize_files(priorities_vector);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int set_file_priority(const char *id, int index, int priority) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);
		h.file_priority(index, priority);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int start_dht(const wchar_t *dht_state_file_path) {
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
				OutputDebugStringA(e.what());
				//no dht to resume will start dht without a prebuilt state
			}
		}

		if(state_loaded) {
			session->start_dht(dht_state);
		} else {
			session->start_dht();
		}

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int add_dht_router(const char *address, int port) {
	try {

		session->add_dht_router(std::pair<std::string, int>(std::string(address), port));

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int add_dht_node(const char *address, int port) {
	try {

		session->add_dht_node(std::pair<std::string, int>(std::string(address), port));

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int save_dht_state(const wchar_t *dht_state_file_path) {
	try {

		std::wstring file(dht_state_file_path);
		boost::filesystem::wpath full_path(file);
		boost::filesystem::ofstream out(full_path, std::ios_base::binary);
		out.unsetf(std::ios_base::skipws);
		libtorrent::entry dht_state = session->dht_state();
		libtorrent::bencode(std::ostream_iterator<char>(out), dht_state);
		out.close();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int stop_dht() {
	try {

		session->stop_dht();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int start_upnp() {
	try {

		session->start_upnp();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int stop_upnp() {
	try {

		session->stop_upnp();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int start_lsd() {
	try {

		session->start_lsd();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int stop_lsd() {
	try {

		session->stop_lsd();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int start_natpmp() {
	try {

		session->start_natpmp();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int stop_natpmp() {
	try {

		session->stop_natpmp();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int set_peer_proxy(wrapper_proxy_settings *proxy) {
	try {

		libtorrent::proxy_settings proxy_settings;
		proxy_settings.type = libtorrent::proxy_settings::proxy_type(proxy->type);
		proxy_settings.hostname = proxy->hostname;
		proxy_settings.port = proxy->port;
		proxy_settings.username = proxy->username;
		proxy_settings.password = proxy->password;
		session->set_peer_proxy(proxy_settings);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int set_web_seed_proxy(wrapper_proxy_settings *proxy) {
	try {

		libtorrent::proxy_settings proxy_settings;
		proxy_settings.type = libtorrent::proxy_settings::proxy_type(proxy->type);
		proxy_settings.hostname = proxy->hostname;
		proxy_settings.port = proxy->port;
		proxy_settings.username = proxy->username;
		proxy_settings.password = proxy->password;
		session->set_web_seed_proxy(proxy_settings);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int set_tracker_proxy(wrapper_proxy_settings *proxy) {
	try {

		libtorrent::proxy_settings proxy_settings;
		proxy_settings.type = libtorrent::proxy_settings::proxy_type(proxy->type);
		proxy_settings.hostname = proxy->hostname;
		proxy_settings.port = proxy->port;
		proxy_settings.username = proxy->username;
		proxy_settings.password = proxy->password;
		session->set_tracker_proxy(proxy_settings);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int set_dht_proxy(wrapper_proxy_settings *proxy) {
	try {

		libtorrent::proxy_settings proxy_settings;
		proxy_settings.type = libtorrent::proxy_settings::proxy_type(proxy->type);
		proxy_settings.hostname = proxy->hostname;
		proxy_settings.port = proxy->port;
		proxy_settings.username = proxy->username;
		proxy_settings.password = proxy->password;
		session->set_tracker_proxy(proxy_settings);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int echo(char *message) {
	try {

		std::cout << "message:" << message << std::endl;

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

struct pieces_info {
	int completed;
	char *pieces;
};

extern "C" int free_pieces_info(pieces_info *info) {
	try {

		delete[] info->pieces;

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int get_pieces_status(const char *id, pieces_info *info) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);

		libtorrent::bitfield piece_downloaded_info = h.status().pieces;

		int num_pieces = piece_downloaded_info.size();

		char *pieces = new char[num_pieces+1];
		info->pieces = pieces;
		info->completed = 0;

		// Clear the array
		for (int i = 0; i < num_pieces; i++) {
			pieces[i] = PIECE_PENDING;
		}
		pieces[num_pieces] = '\0';

		std::vector<libtorrent::peer_info> peers;
		h.get_peer_info(peers);
		std::vector<libtorrent::peer_info>::iterator iter = peers.begin();
		while (iter != peers.end()) {
			if (iter->downloading_piece_index > -1 && iter->downloading_piece_index < num_pieces) {
				// Mark downloading pieces
				pieces[iter->downloading_piece_index] = PIECE_ACTIVE;
			}
			iter++;
		}

		std::vector<libtorrent::partial_piece_info> download_queue;
		h.get_download_queue(download_queue);
		std::vector<libtorrent::partial_piece_info>::iterator queue_iter = download_queue.begin();
		while (queue_iter != download_queue.end()) {
			libtorrent::partial_piece_info piece = *queue_iter;
			if (piece.piece_index > -1 && piece.piece_index < num_pieces) {
				if (pieces[piece.piece_index] != PIECE_ACTIVE) {
					if (piece.writing > 0) {
						pieces[piece.piece_index] = PIECE_ACTIVE;
					} else if (piece.requested > 0 && piece.finished > 0) {
						pieces[piece.piece_index] = PIECE_PARTIAL;
					} else if (piece.finished == 0) {
						pieces[piece.piece_index] = PIECE_QUEUED;
					}
				}
			}

			queue_iter++;
		}

		for (int i = 0; i < num_pieces; i++) {
			if (pieces[i] != PIECE_ACTIVE) {
				if (piece_downloaded_info[i]) {
					// Mark downloaded pieces
					pieces[i] = PIECE_DOWNLOADED;
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
						if (pieces[i] == PIECE_PARTIAL) {
							pieces[i] = PIECE_UNAVAILABLE_PARTIAL;
						} else {
							pieces[i] = PIECE_UNAVAILABLE;
						}
					}
				}
			}
		}

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int add_tracker(const char *id, char *url, int tier) {
	try {
		
		libtorrent::torrent_handle h = findTorrentHandle(id);
		std::vector<libtorrent::announce_entry> trackers = h.trackers();
		
		libtorrent::announce_entry e(url);
		e.tier = tier;
		trackers.push_back(e);

		h.replace_trackers(trackers);

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int remove_tracker(const char *id, char *url, int tier) {
	try {
		
		libtorrent::torrent_handle h = findTorrentHandle(id);
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
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int get_num_trackers(const char *id, int &num_trackers) {
	try {

		libtorrent::torrent_handle h = findTorrentHandle(id);

		std::vector<libtorrent::announce_entry> trackers;
		try {
			trackers = h.trackers();
		} catch (libtorrent::invalid_handle e) {
			return 0;
		} catch (std::exception e) {
			return 0;
		}
		num_trackers = trackers.size();

	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}


extern "C" int get_trackers(const char *id, wrapper_announce_entry **torrent_trackers, int numTrackers) {
	try {
		
		libtorrent::torrent_handle h = findTorrentHandle(id);
		std::vector<libtorrent::announce_entry> trackers = h.trackers();
		
		std::vector<libtorrent::announce_entry>::iterator iter = trackers.begin();

		wrapper_announce_entry **current_torrent_tracker = torrent_trackers;

		int index = 0;
		while (iter != trackers.end()) {
			libtorrent::announce_entry tracker = *iter;
			
			wrapper_announce_entry *torrent_tracker = *current_torrent_tracker;
			current_torrent_tracker++;
			
			torrent_tracker->tier = tracker.tier;
			torrent_tracker->url = mystrdup(tracker.url.c_str());
		
			index++;
			if(index >= numTrackers) {
				break;
			}
			iter++;
		}
		
	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}

extern "C" int free_trackers(wrapper_announce_entry **torrent_trackers, int numTrackers) {
	try {
		
		wrapper_announce_entry **current_torrent_tracker = torrent_trackers;
	
		for (int i = 0; i < numTrackers; i++) {
			wrapper_announce_entry *tracker = *current_torrent_tracker;
			delete[] tracker->url;
			current_torrent_tracker++;
		}
		
	} catch (std::exception &e) {
		OutputDebugStringA(e.what());
	} catch (...) {
		OutputDebugString(L"exception");
	}
	return 0;
}
