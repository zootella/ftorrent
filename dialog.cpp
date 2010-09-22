
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

// Shows the options dialog box
void DialogOptions() {

	// Load common values
	PROPSHEETPAGE page[3];
	ZeroMemory(&page, sizeof(page)); // Size of the whole array
	page[0].dwSize      = page[1].dwSize      = page[2].dwSize      = sizeof(page[0]); // Size of just one page
	page[0].dwFlags     = page[1].dwFlags     = page[2].dwFlags     = PSP_DEFAULT;
	page[0].hInstance   = page[1].hInstance   = page[2].hInstance   = Handle.instance;
	page[0].pfnCallback = page[1].pfnCallback = page[2].pfnCallback = NULL;
	page[0].lParam      = page[1].lParam      = page[2].lParam      = 0;

	// Attach dialog templates and procedures
	page[0].pszTemplate = L"OPTIONS_PAGE_1";
	page[1].pszTemplate = L"OPTIONS_PAGE_2";
	page[2].pszTemplate = L"OPTIONS_PAGE_3";
	page[0].pfnDlgProc = (DLGPROC)DialogOptionsPage1;
	page[1].pfnDlgProc = (DLGPROC)DialogOptionsPage2;
	page[2].pfnDlgProc = (DLGPROC)DialogOptionsPage3;

	// Load information into the header structure
	PROPSHEETHEADER header;
	ZeroMemory(&header, sizeof(header));
	header.dwSize      = sizeof(header);          // Size of the header
	header.dwFlags     = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP; // Leave out the apply button and the help question mark
	header.hwndParent  = Handle.window;           // Handle to parent window
	header.hInstance   = Handle.instance;         // Handle to application instance
	header.hIcon       = NULL;                    // No icon
	header.pszCaption  = L"Options";              // Text in the title bar
	header.nPages      = 3;                       // Number of pages
	header.nStartPage  = 0;                       // Index of the page to tab to by default
	header.ppsp        = (LPCPROPSHEETPAGE)&page; // Address of the array of property sheet pages
	header.pfnCallback = NULL;                    // No function to call when the property sheet is initialized

	// Create and display the property sheet
	AreaPopUp();
	PropertySheet(&header);
	AreaPopDown();
}




BOOL APIENTRY DialogOptionsPage1(HWND dialog, UINT message, UINT wparam, LPARAM lparam) {

	// THE PAGE IS ABOUT TO BE DISPLAYED FOR THE FIRST TIME
	switch (message) {
	case WM_INITDIALOG:

		// LET THE SYSTEM SET THE FOCUS
		return true;

	// A BUTTON WITHIN THE DIALOG BOX WAS CLICKED
	break;
	case WM_COMMAND:

		// THE USER CLICKED BROWSE
		switch (LOWORD(wparam)) {
		case IDC_BROWSE:

			// INDICATE THAT THIS MESSAGE WAS PROCESSED WITH TRUE
			return true;

		// OTHERWISE TELL WINDOWS I DIDN'T HANDLE THE MESSAGE WITH FALSE
		break;
		default:

			return false;

		break;
		}

	// ONE OF THE BOTTOM PROPERTY SHEET BUTTONS WAS CLICKED
	break;
	case WM_NOTIFY:

		// THE USER CLICKED OK
		switch (((LPNMHDR)(ULONG_PTR)lparam)->code)
		case PSN_APPLY:

		break;

	// THE USER PRESSED F1 OR USED CONTEXT SENSITIVE HELP
	break;
	case WM_HELP:

	break;
	}

	// IF A MESSAGE MAKES IT HERE, YOU DID NOT HANDLE IT
	return false;
}

BOOL APIENTRY DialogOptionsPage2(HWND dialog, UINT message, UINT wparam, LPARAM lparam) {

	// THE PAGE IS ABOUT TO BE DISPLAYED FOR THE FIRST TIME
	switch (message) {
	case WM_INITDIALOG:

		// LET THE SYSTEM SET THE FOCUS
		return true;

	// A BUTTON WITHIN THE DIALOG BOX WAS CLICKED
	break;
	case WM_COMMAND:

		// THE USER CLICKED INTERNET OPTIONS
		switch (LOWORD(wparam)) {
		case IDC_INTERNET:

			// INDICATE THAT THIS MESSAGE WAS PROCESSED WITH TRUE
			return true;

		// THE USER CHECKED THE USE CHECK BOX
		break;
		case IDC_CONNECTUSE:

		// OTHERWISE TELL WINDOWS I DIDN'T HANDLE THE MESSAGE WITH FALSE
		break;
		default:

			return false;

		break;
		}

	// ONE OF THE BOTTOM PROPERTY SHEET BUTTONS WAS CLICKED
	break;
	case WM_NOTIFY:

		// THE USER CLICKED OK
		switch (((LPNMHDR)(ULONG_PTR)lparam)->code) {
		case PSN_APPLY:

			// MUST RETURN TRUE FROM APPLY
			SetWindowLong(dialog, DWL_MSGRESULT, true);

		break;
		}

	// THE USER PRESSED F1 OR USED CONTEXT SENSITIVE HELP
	break;
	case WM_HELP:

	break;
	}

	// IF A MESSAGE MAKES IT HERE, YOU DID NOT HANDLE IT
	return false;
}

BOOL APIENTRY DialogOptionsPage3(HWND sheet, UINT message, UINT wparam, LPARAM lparam) {

	// THE PAGE IS ABOUT TO BE DISPLAYED FOR THE FIRST TIME
	switch (message) {
	case WM_INITDIALOG:

		// LET THE SYSTEM SET THE FOCUS
		return true;

	// A BUTTON WITHIN THE DIALOG BOX WAS CLICKED
	break;
	case WM_COMMAND:

		// THE USER CLICKED ADD
		switch (LOWORD(wparam)) {
		case IDC_ADD:

			return true;

		// THE USER CLICKED EDIT
		break;
		case IDC_EDIT:

			return true;

		// THE USER CLICKED DELETE
		break;
		case IDC_DELETE:

		break;
		}

	// ONE OF THE BOTTOM PROPERTY SHEET BUTTONS WAS CLICKED
	break;
	case WM_NOTIFY:

		// THE STATE OF A ROW CHANGED
		switch (((LPNMHDR)(ULONG_PTR)lparam)->code) {
		case LVN_ITEMCHANGED:

			return true;

		// THE USER DOUBLE CLICKED IN THE LIST VIEW CONTROL
		break;
		case NM_DBLCLK:

			return true;

		// THE USER PRESSED A KEY IN THE LIST VIEW CONTROL
		break;
		case LVN_KEYDOWN:

			// DELETE
			switch (((LPNMLVKEYDOWN)lparam)->wVKey) {
			case VK_DELETE:

			break;
			}

		// THE USER CLICKED OK
		break;
		case PSN_APPLY:

			// MUST RETURN TRUE
			SetWindowLong(sheet, DWL_MSGRESULT, true);

		break;
		}

	// THE USER PRESSED F1 OR USED CONTEXT SENSITIVE HELP
	break;
	case WM_HELP:

	break;
	}

	// IF A MESSAGE MAKES IT HERE, YOU DID NOT HANDLE IT
	return false;
}

// Dialog box procedure
BOOL CALLBACK DialogAbout(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {

	// The dialog is about to be displayed
	switch (message) {
	case WM_INITDIALOG:

		// Let the system place the focus
		return true;

	// The dialog needs to be painted
	break;
	case WM_PAINT:
		
		{
			// Do custom painting in the dialog
			deviceitem device;
			device.OpenPaint(dialog);
			device.BackgroundColor(Handle.background.color);
			device.Font(Handle.arial);

			// Compose text
			CString about = L"about " + PROGRAM_NAME;

			// Prepare rectangles
			sizeitem client = SizeClient(dialog); // Get the width of the client area of the dialog box
			sizeitem blue = client; // Blue bar at top
			blue.h = 23;
			sizeitem white = client; // White area beneath
			white.SetTop(blue.h);
			sizeitem title = SizeText(&device, about); // Title above the edge
			title.x = client.w - 8 - title.w;
			title.y = -7;
			title.SetBottom(blue.h);

			// Paint the rectangles
			PaintFill(&device, blue, State.start.background.brush);
			PaintText(&device, about, title, false, false, false, false, 0, Handle.arial, &State.start.ink, &State.start.background);
			PaintFill(&device, white, Handle.background.brush);

			// Set heights
			int text = Area.height; // Text height is usually 13
			int space = 5;

			// Size the text
			sizeitem s = white;
			s.SetLeft(89);
			s.SetTop(46);
			s.h = text;

			// Paint the text
			device.Font(Handle.font);
			PaintText(&device, PROGRAM_ABOUT1, s, false, false, false, false, 0, Handle.font, &Handle.ink, &Handle.background); s.y += text + space;
			PaintText(&device, PROGRAM_ABOUT2, s, false, false, false, false, 0, Handle.font, &Handle.ink, &Handle.background); s.y += text;
			PaintText(&device, PROGRAM_ABOUT3, s, false, false, false, false, 0, Handle.font, &Handle.ink, &Handle.background); s.y += text + space;
			PaintText(&device, PROGRAM_ABOUT4, s, false, false, false, false, 0, Handle.font, &Handle.ink, &Handle.background); s.y += text;
			return false;
		}

	// The message is a command
	break;
	case WM_COMMAND:

		// The user clicked OK or Cancel
		switch (LOWORD(wparam)) {
		case IDOK:
		case IDCANCEL:

			// Close the dialog
			EndDialog(dialog, 0);
			return true;

		break;
		}

	break;
	}

	// Have Windows process messages that make it down here
	return false;
}



