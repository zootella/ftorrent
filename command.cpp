
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
#include "define.h"
#include "object.h"
#include "library.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;

// Takes an area item that has been pressed and released
// Performs its command
void AreaCommand(areaitem *area) {

	// Menus
	if (area->command == CommandMenu) {

		// Tools
		if (area == &Area.tools) {

			// Position the menu beneath the tools link area
			sizeitem size = Area.tools.size;
			size.CloseBottom();
			size.w = 0;

			// Show the popup menu and wait here for the user to click on one of the menu choices
			UINT choice = MenuShow(Handle.menu, false, &size); // Wait here for the user to make a choice
			if      (choice == ID_TOOLS_TEST)    { Test(); }
			else if (choice == ID_TOOLS_OPEN)    { CommandOpen(); }
			else if (choice == ID_TOOLS_ADD)     { CommandAdd(); }
			else if (choice == ID_TOOLS_NEW)     { CommandNew(); }
			else if (choice == ID_TOOLS_HELP)    { FileRun(PROGRAM_HELP); }
			else if (choice == ID_TOOLS_ABOUT)   { Dialog(L"DIALOG_ABOUT", DialogAbout); }
			else if (choice == ID_TOOLS_OPTIONS) { DialogOptions(); }
			else if (choice == ID_TOOLS_EXIT)    { WindowExit(); } // Hide the window and stop libtorrent
		}

	// Buttons
	} else if (area->command == CommandReady || area->command == CommandSet) {

	// Links
	} else if (area->command == CommandLink) {

	}
}

// Call when the program starts up
// Reads the optn.db file next to this running exe, and loads values in Data
void OptionLoad() {

	libtorrent::entry d;
	if (LoadEntry(PathOption(), d)) { // Loaded

		Data.folder = widenStoC(d[narrowRtoS(L"folder")].string()); // Path to download folder

		CString ask = widenStoC(d[narrowRtoS(L"ask")].string()); // True to ask where to save each torrent
		Data.ask = same(ask, L"t");
	}

	// Replace blank or invalid with factory defaults
	if (isblank(Data.folder)) Data.folder = PathTorrents();
}

// Call when the program is shutting down
// Saves values from Data to optn.db next to this running exe
void OptionSave() {

	libtorrent::entry::dictionary_type d;

	d[narrowRtoS(L"folder")] = narrowRtoS(Data.folder);

	if (Data.ask) d[narrowRtoS(L"ask")] = narrowRtoS(L"t");
	else          d[narrowRtoS(L"ask")] = narrowRtoS(L"f");
	
	SaveEntry(PathOption(), d);
}









void CommandNew() {

	report(L"so you want to make a torrent");

}

