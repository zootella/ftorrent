
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

/*
// Shows the options dialog box
void DialogOptions() {

	// LOAD COMMON VALUES
	PROPSHEETPAGE page[3];
	page[0].dwSize      = page[1].dwSize      = page[2].dwSize      = sizeof(PROPSHEETPAGE);
	page[0].dwFlags     = page[1].dwFlags     = page[2].dwFlags     = PSP_DEFAULT;
	page[0].hInstance   = page[1].hInstance   = page[2].hInstance   = Handle.instance;
	page[0].pfnCallback = page[1].pfnCallback = page[2].pfnCallback = (LPFNPSPCALLBACK)NULL;
	page[0].lParam      = page[1].lParam      = page[2].lParam      = 0;

	// ATTACH DIALOG TEMPLATES AND PROCEDURES
	page[0].pszTemplate = "OPTIONS_DOWNLOAD";
	page[1].pszTemplate = "OPTIONS_PASSWORDS";
	page[2].pszTemplate = "OPTIONS_CONNECTIONS";
	page[0].pfnDlgProc = (DLGPROC)DialogOptionsDownload;
	page[1].pfnDlgProc = (DLGPROC)DialogOptionsPasswords;
	page[2].pfnDlgProc = (DLGPROC)DialogOptionsConnections;

	// LOAD INFORMATION INTO THE HEADER STRUCTURE
	PROPSHEETHEADER header;
	header.dwSize      = sizeof(header);                     // SIZE OF THE HEADER
	header.dwFlags     = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW; // LEAVE OUT THE APPLY BUTTON
	header.hwndParent  = Handle.window;                      // HANDLE TO PARENT WINDOW
	header.hInstance   = Handle.instance;                    // HANDLE TO APPLICATION INSTANCE
	header.hIcon       = NULL;                               // NO ICON
	header.pszCaption  = "Options";                          // TEXT IN TITLE BAR
	header.nPages      = 3;                                  // NUMBER OF PAGES
	header.nStartPage  = 0;                                  // INDEX OF THE PAGE TO TAB TO BY DEFAULT
	header.ppsp        = (LPCPROPSHEETPAGE)&page;            // ADDRESS OF THE ARRAY OF PROPERTY SHEET PAGES
	header.pfnCallback = NULL;                               // NO FUNCTION CALLED WHEN THE PROPERTY SHEET IS INITIALIZED

	// CREATE AND DISPLAY THE PROPERTY SHEET
	AreaPopUp();
	PropertySheet(&header);
	AreaPopDown();
}
*/
