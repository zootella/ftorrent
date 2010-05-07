
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
extern icontop   Icon;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;

// Set up the program image list
void StartIcon() {

	// Initialize ext with a guid so the first requested extension will never match
	Icon.ext = TextGuid();

	// Create the program image list
	Icon.list = ImageList_Create(
		16, 16,         // Image size
		ILC_COLOR32 |   // Windows XP style 32 bit antialiased icons
		ILC_MASK,       // Show icon transparency
		0,              // Start with no icons
		ICON_CAPACITY); // Be able to grow to hold this many more icons
	if (!Icon.list) Report(L"imagelist_create");

	// Load the resource icons into the program image list and get their indices there, or -1 if not loaded
	Icon.clear      = IconAddResource(L"CLEAR_ICON");
	Icon.ascending  = IconAddResource(L"COLUMN_ASCENDING");
	Icon.descending = IconAddResource(L"COLUMN_DESCENDING");

	// Load the shell icon for a file
	string type;
	Icon.file = IconGet(L"", &type);
}
