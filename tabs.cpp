
// Include statements
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>
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

void Tabs() {

	/*
	// Create the list view window
	style =
		WS_CHILD |             // Required for child windows
		LVS_REPORT |           // Specifies report view, I think this puts it in details
		LBS_EXTENDEDSEL |      // Allows multiple items to be selected
		LVS_SHOWSELALWAYS |    // Shows the selection even when the control doesn't have the focus
		LBS_NOINTEGRALHEIGHT | // Allows the size to be specified exactly without snap
		LVS_SHAREIMAGELISTS;   // Will not delete the system image list when the control is destroyed
	Handle.list = WindowCreate(WC_LISTVIEW, NULL, style, 0, Handle.window, (HMENU)WINDOW_LIST);

	// Create the tabs window
	style =
		WS_CHILD;              // Required for child windows
	Handle.tabs = WindowCreate(WC_TABCONTROL, NULL, style, 0, Handle.window, (HMENU)WINDOW_TABS);
	*/




}