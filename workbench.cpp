
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

// Fill out the given torrent structure with information from the torrent with the given infohash
void GetTorrentInfo(const char *id, torrent_structure *info) {
	try {

		// Find the torrent handle and get its info object
		libtorrent::torrent_handle h = FindTorrentHandle(id);
		libtorrent::torrent_info i = h.get_torrent_info();

		info->created_by = StringToWideCString(i.creator().c_str());
		info->comment    = StringToWideCString(i.comment().c_str());
		info->sha1       = HashToCString(i.info_hash());
		info->total_size = (long long)i.total_size();

		info->piece_length = (int)i.piece_length();

		// Tracker information

		std::vector<libtorrent::announce_entry> announce_entries = i.trackers();
		announce_structure *trackers = new announce_structure[announce_entries.size()];
		std::vector<libtorrent::announce_entry>::iterator iterator1 = announce_entries.begin();




		int index1 = 0;
		while (iterator1 != announce_entries.end()) {

			trackers[index1].url  = CopyString(iterator1->url.c_str());
			trackers[index1].tier = iterator1->tier;

			index1++;
			iterator1++;
		}
		info->trackers = trackers;
		info->num_trackers = announce_entries.size();

		// Seeding information

		std::vector<std::string> seed_entries = i.url_seeds();
		announce_structure *seeds = new announce_structure[seed_entries.size()];
		std::vector<std::string>::iterator iterator2 = seed_entries.begin();

		int index2 = 0;
		while (iterator2 != seed_entries.end()) {

			seeds[index2].url  = CopyString(iterator2->c_str());
			seeds[index2].tier = -1;

			index2++;
			iterator2++;
		}
		info->seeds = seeds;
		info->num_seeds = seed_entries.size();

	} catch (std::exception &e) {
		log(StringToWideCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void FreeTorrentInfo(torrent_structure *info) {
	try {

		delete[] info->trackers;
		delete[] info->seeds;

	} catch (std::exception &e) {
		log(StringToWideCString(e.what()));
	} catch (...) {
		log(L"exception");
	}
}


