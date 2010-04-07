
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



	TCITEM item;
	ZeroMemory(&item, sizeof(item));
	item.mask        = TCIF_TEXT; // Parts set below
	item.dwState     = 0;         // Ignored when inserting a new tab
	item.dwStateMask = 0;         // Ignored when inserting a new tab
	item.pszText     = L"General";
	item.cchTextMax  = 0;         // Only used when the structure is receiving information
	item.iImage      = -1;        // No icon
	item.lParam      = 0;         // No extra information


	SendMessage(
		Handle.tabs,
		TCM_INSERTITEM,
		0, // Index of the new tab
		(LPARAM)&item);





}