
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

// P  const char *     narrow  in
// S  std::string      narrow  in  out
// R  const wchar_t *  wide    in
// W  std::wstring     wide    in  out
// C  CString          wide        out

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

// Convert the hash value in the given text into a libtorrent big number hash value object
libtorrent::big_number StringToHash(const char *s) {

	std::stringstream stream;
	libtorrent::big_number hash;
	stream << s;
	stream >> hash;
	return hash;
}

// Convert the given hash value object into text
CString HashToString(const libtorrent::sha1_hash &hash) {

	std::stringstream stream;
	stream << hash;
	return widenStoC(stream.str());
}

// Convert the given peer ID object into text
CString PeerToString(const libtorrent::peer_id &id) {

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




bool SaveEntry(read path, ) {



}

 LoadEntry(read path, ) {


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
		if (b) Handle.session->start_dht(e); // With the file information we got
		else   Handle.session->start_dht();  // Without any information from a previous session

}
