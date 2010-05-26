
/*
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
*/

#include "libtorrent/create_torrent.hpp"

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


std::string makenarrow(std::wstring w) {

	std::string s;
	libtorrent::wchar_utf8(w, s);
	return s;
}

std::wstring makewide(std::string s) {

	std::wstring w;
	libtorrent::utf8_wchar(s, w);
	return w;
}

