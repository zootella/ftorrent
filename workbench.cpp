
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
			file_entry->path = widenStoC(path.string());
			file_entry->size = iter->size;
			file_entry->total_done = progress[index];
			file_entry->priority = priorities[index];
			file_entries++;
			index++;
			iter++;
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
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
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

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





