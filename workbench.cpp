
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
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void UpdateSettings(settings_structure *settings) {
	try {

		libtorrent::session_settings s;
		s.use_dht_as_fallback   = false;
		s.share_ratio_limit     = settings->seed_ratio_limit;
		s.seed_time_ratio_limit = settings->seed_time_ratio_limit;
		s.seed_time_limit       = settings->seed_time_limit;
		s.active_downloads      = settings->active_downloads_limit;
		s.active_seeds          = settings->active_seeds_limit;
		s.active_limit          = settings->active_limit;

		Handle.session->set_settings(s);
		Handle.session->set_alert_mask(settings->alert_mask); // Tell libtorrent what kinds of alerts we want to find out about
		Handle.session->listen_on(std::make_pair(settings->listen_start_port, settings->listen_end_port), narrowRtoS(settings->listen_interface).c_str());

		Handle.session->set_upload_rate_limit(settings->max_upload_bandwidth);
		Handle.session->set_download_rate_limit(settings->max_download_bandwidth);

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}





