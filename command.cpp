
// Include libtorrent
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/create_torrent.hpp"
#include "library.h"

// Include platform
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>

// Include program
#include "resource.h"
#include "program.h"
#include "object.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;

// Takes an area item that has been pressed and released
// Performs its command
// Returns true to exit the program, false to keep it running
bool AreaCommand(areaitem *area) {

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
			else if (choice == ID_TOOLS_OPEN)    {  }
			else if (choice == ID_TOOLS_ADD)     {  }
			else if (choice == ID_TOOLS_NEW)     {  }
			else if (choice == ID_TOOLS_HELP)    { FileRun(PROGRAM_HELP); }
			else if (choice == ID_TOOLS_ABOUT)   { Dialog(L"DIALOG_ABOUT", DialogAbout); }
			else if (choice == ID_TOOLS_OPTIONS) { DialogOptions(); }
			else if (choice == ID_TOOLS_EXIT)    { return true; } // Close the program
		}

	// Buttons
	} else if (area->command == CommandReady || area->command == CommandSet) {

	// Links
	} else if (area->command == CommandLink) {

	}

	// No request to exit the program
	return false;
}
