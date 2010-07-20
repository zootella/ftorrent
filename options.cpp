
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
	page[0].pszTemplate = L"OPTIONS_DOWNLOAD";
	page[1].pszTemplate = L"OPTIONS_PASSWORDS";
	page[2].pszTemplate = L"OPTIONS_CONNECTIONS";
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
	header.pszCaption  = L"Options";                         // TEXT IN TITLE BAR
	header.nPages      = 3;                                  // NUMBER OF PAGES
	header.nStartPage  = 0;                                  // INDEX OF THE PAGE TO TAB TO BY DEFAULT
	header.ppsp        = (LPCPROPSHEETPAGE)&page;            // ADDRESS OF THE ARRAY OF PROPERTY SHEET PAGES
	header.pfnCallback = NULL;                               // NO FUNCTION CALLED WHEN THE PROPERTY SHEET IS INITIALIZED

	// CREATE AND DISPLAY THE PROPERTY SHEET
	AreaPopUp();
	PropertySheet(&header);
	AreaPopDown();
}



BOOL APIENTRY DialogOptionsDownload(HWND dialog, UINT message, UINT wparam, LPARAM lparam) {

	// THE PAGE IS ABOUT TO BE DISPLAYED FOR THE FIRST TIME
	switch (message) {
	case WM_INITDIALOG:

		/*
		// PREPARE THE FOLDER TEXT BOX
		TextDialogSet(dialog, IDC_FOLDER, State.folder);

		// FILL THE DROP DOWN LIST BOX AND SELECT THE CHOSEN ITEM
		SendDlgItemMessage(dialog, IDC_SUB, CB_ADDSTRING, 0, (LPARAM)"file");
		SendDlgItemMessage(dialog, IDC_SUB, CB_ADDSTRING, 0, (LPARAM)"site \\ file");
		SendDlgItemMessage(dialog, IDC_SUB, CB_ADDSTRING, 0, (LPARAM)"site-folder-folder \\ file");
		SendDlgItemMessage(dialog, IDC_SUB, CB_ADDSTRING, 0, (LPARAM)"site \\ folder-folder \\ file");
		SendDlgItemMessage(dialog, IDC_SUB, CB_ADDSTRING, 0, (LPARAM)"site \\ folder \\ folder \\ file");
		if (State.option.sub > 4) SendDlgItemMessage(dialog, IDC_SUB, CB_SETCURSEL, 0, 0); // FUTURE SUB VALUE
		else                      SendDlgItemMessage(dialog, IDC_SUB, CB_SETCURSEL, State.option.sub, 0);

		// PREPARE GET FILES AT ONCE
		SendMessage(GetDlgItem(dialog, IDC_GETSPIN), UDM_SETRANGE, 0, MAKELONG(9999, 1));
		SetDlgItemInt(dialog, IDC_GET, State.option.get, true);
		*/

		// LET THE SYSTEM SET THE FOCUS
		return true;

	// A BUTTON WITHIN THE DIALOG BOX WAS CLICKED
	break;
	case WM_COMMAND:

		// THE USER CLICKED BROWSE
		switch (LOWORD(wparam)) {
		case IDC_BROWSE: {

			/*
			// DISPLAY THE BROWSE FOR FOLDER DIALOG
			CString browse = FileBrowse("Choose where ftorrent should save files.");

			// IF THE USER CHOSE A PATH, WRITE IT INTO THE FOLDER EDIT BOX
			if (is(browse)) TextDialogSet(dialog, IDC_FOLDER, browse);
			*/

			// INDICATE THAT THIS MESSAGE WAS PROCESSED WITH TRUE
			return true;

		// THE GET EDIT CONTROL IS ABOUT TO REDRAW ITSELF
		break; }
		case IDC_GET:

			// THE TEXT IN THE GET EDIT CONTROL CHANGED
			switch (HIWORD(wparam)) {
			case EN_UPDATE: {

				/*
				// COMPOSE AND UPDATE THE GET TEXT IF NECESSARY
				CString gettext;
				int get = GetDlgItemInt(dialog, IDC_GET, NULL, false);
				if      (get == 1) gettext = "file at a time";
				else if (get >  1) gettext = "files at once";
				if (gettext != TextDialog(dialog, IDC_GETTEXT)) TextDialogSet(dialog, IDC_GETTEXT, gettext);
				*/

			break; }
			}

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

			/*
			// SET THE REGISTRY AND OPTION VARIABLES TO THE FOLDER TEXT IF THEY CHANGED
			OptionSetText("folder", &State.option.folder, TextDialog(dialog, IDC_FOLDER));

			// GET AND STORE THE USER'S CHOICE FROM THE DROP DOWN LIST BOX
			int sub;
			sub = (int)SendDlgItemMessage(dialog, IDC_SUB, CB_GETCURSEL, 0, 0);
			if (sub == CB_ERR) sub = 0;
			else OptionSetNumber("sub", &State.option.sub, sub);

			// SET GET FROM THE EDIT BOX
			int get;
			get = GetDlgItemInt(dialog, IDC_GET, NULL, false);
			if (get > 0) OptionSetNumber("get", &State.option.get, get);

			// MUST RETURN TRUE FROM APPLY
			SetWindowLong(dialog, DWL_MSGRESULT, true);
			*/

		break;
		}

	// THE USER PRESSED F1 OR USED CONTEXT SENSITIVE HELP
	break;
	case WM_HELP:

		// FIND THE DIALOG BOX CONTROL AND OPEN ITS HELP
		switch (((LPHELPINFO)LongToPtr(lparam))->iCtrlId) {
		case IDC_FOLDER:
		case IDC_BROWSE:

			// OPEN HELP OPTION PAGE AND SCROLL TO FOLDER
			FileRun(L"http://www.example.com/help/option.asp#folder");

		break;
		case IDC_SUB:

			// OPEN HELP OPTION PAGE AND SCROLL TO SUBFOLDER
			FileRun(L"http://www.example.com/help/option.asp#subfolder");

		break;
		case IDC_GET:

			// OPEN HELP OPTION PAGE AND SCROLL TO GET
			FileRun(L"http://www.example.com/help/option.asp#get");

		break;
		default:

			// THE USER CLICKED SOME OTHER CONTROL, OPEN HELP OPTION PAGE
			FileRun(L"http://www.example.com/help/option.asp");

		break;
		}

	break;
	}

	// IF A MESSAGE MAKES IT HERE, YOU DID NOT HANDLE IT
	return false;
}

BOOL APIENTRY DialogOptionsConnections(HWND dialog, UINT message, UINT wparam, LPARAM lparam) {

	// THE PAGE IS ABOUT TO BE DISPLAYED FOR THE FIRST TIME
	switch (message) {
	case WM_INITDIALOG:

		/*
		// PREPARE CONNECTIONS
		if (State.option.dialup == 0) {

			// OPTION 0 SKIP
			EnableWindow(GetDlgItem(dialog, IDC_CONNECTDIAL), false);
			EnableWindow(GetDlgItem(dialog, IDC_CONNECTWAIT), false);

		} else if (State.option.dialup == 1) {

			// OPTION 1 WAIT
			CheckDlgButton(dialog, IDC_CONNECTUSE, BST_CHECKED);
			CheckRadioButton(dialog, IDC_CONNECTDIAL, IDC_CONNECTWAIT, IDC_CONNECTWAIT);

		} else {

			// OPTION 2 DIAL
			CheckDlgButton(dialog, IDC_CONNECTUSE, BST_CHECKED);
			CheckRadioButton(dialog, IDC_CONNECTDIAL, IDC_CONNECTWAIT, IDC_CONNECTDIAL);
		}
		*/

		// LET THE SYSTEM SET THE FOCUS
		return true;

	// A BUTTON WITHIN THE DIALOG BOX WAS CLICKED
	break;
	case WM_COMMAND:

		// THE USER CLICKED INTERNET OPTIONS
		switch (LOWORD(wparam)) {
		case IDC_INTERNET:

			/*
			// OPEN THE INTERNET OPTIONS CONTROL PANEL ITEM
			FileRun(L"control.exe", L"inetcpl.cpl");
			*/

			// INDICATE THAT THIS MESSAGE WAS PROCESSED WITH TRUE
			return true;

		// THE USER CHECKED THE USE CHECK BOX
		break;
		case IDC_CONNECTUSE:

			// THE USER CHECKED THE USE BOX
			if (IsDlgButtonChecked(dialog, IDC_CONNECTUSE)) {

				// ENABLE THE RADIO OPTIONS AND SELECT DIAL
				EnableWindow(GetDlgItem(dialog, IDC_CONNECTDIAL), true);
				EnableWindow(GetDlgItem(dialog, IDC_CONNECTWAIT), true);
				CheckRadioButton(dialog, IDC_CONNECTDIAL, IDC_CONNECTWAIT, IDC_CONNECTDIAL);

			// THE USER UNCHECKED THE USE BOX
			} else {

				// DISABLE AND CLEAR BOTH RADIO OPTIONS
				EnableWindow(GetDlgItem(dialog, IDC_CONNECTDIAL), false);
				EnableWindow(GetDlgItem(dialog, IDC_CONNECTWAIT), false);
				CheckDlgButton(dialog, IDC_CONNECTDIAL, BST_UNCHECKED);
				CheckDlgButton(dialog, IDC_CONNECTWAIT, BST_UNCHECKED);
			}

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

			/*
			// SET CONNECTIONS FROM THE CHECK BOX AND RADIO CONTROLS
			if      (!IsDlgButtonChecked(dialog, IDC_CONNECTUSE)) OptionSetNumber("dialup", &State.option.dialup, 0); // OPTION 0 SKIP
			else if (IsDlgButtonChecked(dialog, IDC_CONNECTWAIT)) OptionSetNumber("dialup", &State.option.dialup, 1); // OPTION 1 WAIT
			else                                                  OptionSetNumber("dialup", &State.option.dialup, 2); // OPTION 2 DIAL
			*/

			// MUST RETURN TRUE FROM APPLY
			SetWindowLong(dialog, DWL_MSGRESULT, true);

		break;
		}

	// THE USER PRESSED F1 OR USED CONTEXT SENSITIVE HELP
	break;
	case WM_HELP:

		// OPEN HELP CONNECT PAGE
		FileRun(L"http://www.example.com/help/connect.asp");

	break;
	}

	// IF A MESSAGE MAKES IT HERE, YOU DID NOT HANDLE IT
	return false;
}


BOOL APIENTRY DialogOptionsPasswords(HWND sheet, UINT message, UINT wparam, LPARAM lparam) {

	// THE PAGE IS ABOUT TO BE DISPLAYED FOR THE FIRST TIME
	switch (message) {
	case WM_INITDIALOG:

		/*
		// SET PASS SHEET AND LIST
		State.pass.sheet = sheet;
		State.pass.list = GetDlgItem(sheet, IDC_LIST);

		// LOAD EXTENDED LIST VIEW STYLES, REQUIRES COMMON CONTROL 4.70
		ListView_SetExtendedListViewStyle(State.pass.list, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

		// PUT COLUMNS IN THE LIST VIEW CONTROL
		LV_COLUMN c1, c2, c3, c4;
		c1.mask = c2.mask = c3.mask = c4.mask = LVCF_WIDTH | LVCF_TEXT;
		c1.cx = 99;
		c2.cx = c3.cx = c4.cx = 64;
		c1.pszText = "Site";
		c2.pszText = "User Name";
		c3.pszText = "Password";
		c4.pszText = "Referrer";
		ListView_InsertColumn(State.pass.list, 0, &c1);
		ListView_InsertColumn(State.pass.list, 1, &c2);
		ListView_InsertColumn(State.pass.list, 2, &c3);
		ListView_InsertColumn(State.pass.list, 3, &c4);

		// LIST THE ROWS
		PassList();

		// DISABLE THE EDIT AND DELETE BUTTONS
		EnableWindow(GetDlgItem(sheet, IDC_EDIT),   false);
		EnableWindow(GetDlgItem(sheet, IDC_DELETE), false);
		*/

		// LET THE SYSTEM SET THE FOCUS
		return true;

	// A BUTTON WITHIN THE DIALOG BOX WAS CLICKED
	break;
	case WM_COMMAND:

		// THE USER CLICKED ADD
		switch (LOWORD(wparam)) {
		case IDC_ADD:

			/*
			// ADD
			State.pass.edit = "";
			Dialog("DIALOG_PASS", DialogPassword);
			*/
			return true;

		// THE USER CLICKED EDIT
		break;
		case IDC_EDIT:

			// EDIT
			/*
			State.pass.edit = PassSelected();
			Dialog("DIALOG_PASS", DialogPassword);
			*/
			return true;

		// THE USER CLICKED DELETE
		break;
		case IDC_DELETE:

			// DELETE THE SELECTED ROW
			/*
			PassDelete();
			*/

		break;
		}

	// ONE OF THE BOTTOM PROPERTY SHEET BUTTONS WAS CLICKED
	break;
	case WM_NOTIFY:

		// THE STATE OF A ROW CHANGED
		switch (((LPNMHDR)(ULONG_PTR)lparam)->code) {
		case LVN_ITEMCHANGED:

			/*
			// GET THE POINTER TO THE ITEM THAT THIS MESSAGE PERTAINS TO
			NMLISTVIEW *iteminfo;
			iteminfo = (NMLISTVIEW *)(ULONG_PTR)lparam;

			// THE STATE OF THE ROW CHANGED
			if (iteminfo->uChanged == LVIF_STATE) {

				// A ROW WAS SELECTED OR DESELECTED, SELECTION
				if (!(iteminfo->uOldState & LVIS_SELECTED) && (iteminfo->uNewState & LVIS_SELECTED)) PassSelection();
				if ((iteminfo->uOldState & LVIS_SELECTED) && !(iteminfo->uNewState & LVIS_SELECTED)) PassSelection();
			}
			*/

			return true;

		// THE USER DOUBLE CLICKED IN THE LIST VIEW CONTROL
		break;
		case NM_DBLCLK:

			/*
			// EDIT
			State.pass.edit = PassSelected();
			Dialog("DIALOG_PASS", DialogPassword);
			*/
			return true;

		// THE USER PRESSED A KEY IN THE LIST VIEW CONTROL
		break;
		case LVN_KEYDOWN:

			// DELETE
			switch (((LPNMLVKEYDOWN)lparam)->wVKey) {
			case VK_DELETE:

				// DELETE THE SELECTED ROW
				/*
				PassDelete();
				*/

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

		// OPEN HELP PASSWORD PAGE
		FileRun(L"http://www.example.com/help/password.asp");

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



