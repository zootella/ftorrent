
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

// Convert a STL wide character string into a STL UTF8 byte string
std::string MakeNarrow(std::wstring w) {

	std::string s;
	libtorrent::wchar_utf8(w, s); // Use libtorrent
	return s;
}

// Convert a STL UTF8 byte string into a STL wide character string
std::wstring MakeWideString(std::string s) {

	std::wstring w;
	libtorrent::utf8_wchar(s, w);
	return w;
}

// Convert a STL UTF8 byte string into an ATL wide CString
CString StringToWideCString(std::string s) {

	return MakeWideString(s).c_str();
}






CString HashToCString(const libtorrent::sha1_hash &hash) {

	std::stringstream stream;
	stream << hash;
	return StringToWideCString(stream.str().c_str());
}

CString PeerIdToCString(const libtorrent::peer_id &id) {

	std::stringstream stream;
	stream << id;
	return StringToWideCString(stream.str().c_str());
}









// Make a new string of allocated memory you have to free from the given hash value
const char *HashToString(const libtorrent::sha1_hash &hash) {

	std::stringstream stream;
	stream << hash;
	return CopyStringFromStream(stream);
}

// Make a new string of allocated memory you have to free from the given peer ID
const char *PeerIdToString(const libtorrent::peer_id &id) {

	std::stringstream stream;
	stream << id;
	return CopyStringFromStream(stream);
}

// Make a new string of allocated memory you have to free from the text in the given stream
const char *CopyStringFromStream(const std::stringstream &stream) {

	std::string s = stream.str();
	return CopyString(s.c_str());
}

// Copy the given byte string into a new memory block you have to free later
char *CopyString(const char *s) {

	int n = std::strlen(s);           // Number of characters not including the null terminator
	char *copy = new char[n + 1];     // Allocate space for the characters and the null terminator
	strncpy_s(copy, n + 1, s, n + 1); // Copy the characters and the null terminator
	return copy;
}

// Copy the given wide string into a new memory block you have to free later
wchar_t *CopyWideString(const wchar_t *s) {

	int n = wcslen(s);                  // Number of characters not including the null terminator
	wchar_t *copy = new wchar_t[n + 1]; // Allocate space for the characters and the null terminator
	wcsncpy_s(copy, n + 1, s, n + 1);   // Copy the characters and the null terminator
	return copy;
}

// Make a boost path object from the given wide string
boost::filesystem::path WideToPath(wchar_t *w) {

	return boost::filesystem::path(MakeNarrow(w)); // Convert to UTF-8 first
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
	alertInfo->message = CopyString(alert->message().c_str());

	libtorrent::torrent_alert const *torrentAlert;

	if ((torrentAlert = dynamic_cast<libtorrent::torrent_alert const*> (alert))) {

		libtorrent::torrent_handle handle = torrentAlert->handle;

		if (handle.is_valid()) {
			alertInfo->sha1 = HashToString(handle.info_hash());

			libtorrent::save_resume_data_alert const *srd_alert = dynamic_cast<libtorrent::save_resume_data_alert const*> (alert);
			if (srd_alert) {

				ProcessSaveResumeDataAlert(handle, srd_alert, alertInfo);
				return;
			}

			libtorrent::save_resume_data_failed_alert const *srdf_alert = dynamic_cast<libtorrent::save_resume_data_failed_alert const*> (alert);
			if (srdf_alert) {

				log(make(L"save_resume_data_failed_alert (", StringToWideCString(srdf_alert->msg), L")"));
				delete[] alertInfo->message;
				alertInfo->message = CopyString(srdf_alert->msg.c_str());
				return;
			}

			libtorrent::fastresume_rejected_alert const *fra_alert = dynamic_cast<libtorrent::fastresume_rejected_alert const*> (alert);
			if (fra_alert) {

				log(make(L"fastresume_rejected_alert (", StringToWideCString(fra_alert->msg), L")"));
				delete[] alertInfo->message;
				alertInfo->message = CopyString(fra_alert->msg.c_str());
				return;
			}
		}
	}
}

