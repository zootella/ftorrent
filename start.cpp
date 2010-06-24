
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

// Set up the program image list
void StartIcon() {

	// Initialize ext with a guid so the first requested extension will never match
	Handle.icon.ext = TextGuid();

	// Create the program image list
	Handle.icon.list = ImageList_Create(
		16, 16,         // Image size
		ILC_COLOR32 |   // Windows XP style 32 bit antialiased icons
		ILC_MASK,       // Show icon transparency
		0,              // Start with no icons
		ICON_CAPACITY); // Be able to grow to hold this many more icons
	if (!Handle.icon.list) error(L"imagelist_create");

	// Load the resource icons into the program image list and get their indices there, or -1 if not loaded
	Handle.icon.clear      = IconAddResource(L"CLEAR_ICON");
	Handle.icon.ascending  = IconAddResource(L"COLUMN_ASCENDING");
	Handle.icon.descending = IconAddResource(L"COLUMN_DESCENDING");

	// Load the shell icon for a file
	CString type;
	Handle.icon.file = IconGet(L"", &type);
}
