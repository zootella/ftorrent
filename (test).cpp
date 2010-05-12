
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

// Run a snippet of test code
void Test() {

	string s = TextGuid();



}



// Generates a guid
// Returns a string with the 32 lowercase hexidecimal characters of the guid, or blank if any error
string TextGuid() {

	// Get a new unique GUID from the system
	GUID guid;
	HRESULT result = CoCreateGuid(&guid);
	if (result != S_OK) { Report(L"cocreateguid"); return L""; }

	// Convert the GUID into an OLE wide character string
	WCHAR bay[MAX_PATH];
	int characters = StringFromGUID2(
		guid,          // GUID to convert
		(LPOLESTR)bay, // Write text here
		MAX_PATH);     // Size of bay
	if (!characters) { Report(L"stringfromguid2"); return L""; }

	// Convert the OLE wide character string into a text string
	COLE2T text(bay);
	string s;
	s = text;

	s = lower(clip(s, 1, 8) + clip(s, 10, 4) + clip(s, 15, 4) + clip(s, 20, 4) + clip(s, 25, 12)); // Clip out the number parts of the GUID string and lowercase it
	if (length(s) != 32) { Report(L"guid length not 32 characters"); return L""; }                 // Make sure the GUID string is 32 characters
	return s;                                                                                      // Return the string
}




