
// Include libtorrent
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/alert.hpp"

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











int torrentitem::ComposeStatusIcon() {
	return Handle.icon.clear;
}

CString torrentitem::ComposeStatus() {
	return L"status text";
}

int torrentitem::ComposeNameIcon() {
	return Handle.icon.clear;
}

CString torrentitem::ComposeName() {
	return L"name text";
}

CString torrentitem::ComposeSize() {
	return L"size text";
}

CString torrentitem::ComposeHash() {
	return L"hash text";
}

CString torrentitem::ComposePath() {
	return L"path text";
}



