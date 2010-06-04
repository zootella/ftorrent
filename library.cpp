
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





// P  const char*     narrow  in
// S  std::string     narrow  in  out
// R  const wchar_t*  wide    in
// W  std::wstring    wide    in  out
// C  CString         wide        out

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






CString HashToCString(const libtorrent::sha1_hash &hash) {

	std::stringstream stream;
	stream << hash;
	return widenStoC(stream.str());
}

CString PeerIdToCString(const libtorrent::peer_id &id) {

	std::stringstream stream;
	stream << id;
	return widenStoC(stream.str());
}





// Given the text of a torrent infohash, look up and return the libtorrent torrent handle object
libtorrent::torrent_handle FindTorrentHandle(const char *id) {

	libtorrent::big_number hash = StringToHash(id);
	libtorrent::torrent_handle h = Handle.session->find_torrent(hash);
	return h;
}

// Convert the hash value in the given text into a libtorrent big number hash value object
libtorrent::big_number StringToHash(const char *s) {

	libtorrent::big_number hash;
	std::stringstream stream;
	stream << s;
	stream >> hash;
	return hash;
}






void ProcessSaveResumeDataAlert(libtorrent::torrent_handle handle, libtorrent::save_resume_data_alert const *alert, alert_structure *alertInfo) {

	const boost::shared_ptr<libtorrent::entry> resume_ptr = alert->resume_data;
	alertInfo->has_data = 1;
	alertInfo->resume_data = resume_ptr.get();
}

void ProcessAlert(libtorrent::alert const *alert, alert_structure *alertInfo) {

	alertInfo->category = alert->category();
	alertInfo->message = widenStoC(alert->message());

	libtorrent::torrent_alert const *torrentAlert;

	if ((torrentAlert = dynamic_cast<libtorrent::torrent_alert const*>(alert))) {

		libtorrent::torrent_handle handle = torrentAlert->handle;

		if (handle.is_valid()) {
			alertInfo->sha1 = HashToCString(handle.info_hash());

			libtorrent::save_resume_data_alert const *srd_alert = dynamic_cast<libtorrent::save_resume_data_alert const*>(alert);
			if (srd_alert) {

				ProcessSaveResumeDataAlert(handle, srd_alert, alertInfo);
				return;
			}

			libtorrent::save_resume_data_failed_alert const *srdf_alert = dynamic_cast<libtorrent::save_resume_data_failed_alert const*>(alert);
			if (srdf_alert) {

				log(make(L"save_resume_data_failed_alert (", widenStoC(srdf_alert->msg), L")"));
				alertInfo->message = widenStoC(srdf_alert->msg);
				return;
			}

			libtorrent::fastresume_rejected_alert const *fra_alert = dynamic_cast<libtorrent::fastresume_rejected_alert const*> (alert);
			if (fra_alert) {

				log(make(L"fastresume_rejected_alert (", widenStoC(fra_alert->msg), L")"));
				alertInfo->message = widenStoC(fra_alert->msg);
				return;
			}
		}
	}
}

