
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

// Print an error message out on the output log
void error(read r1, read r2, read r3, read r4, read r5, read r6, read r7, read r8, read r9) {

	log(make(L"error ", numerals(GetLastError()), L" ", make(r1, r2, r3, r4, r5, r6, r7, r8, r9)));
}

// Print a message out on the output log
void log(read r1, read r2, read r3, read r4, read r5, read r6, read r7, read r8, read r9) {

	CString s = make(saynow(), L" ", make(r1, r2, r3, r4, r5, r6, r7, r8, r9), L"\r\n");
	OutputDebugString(s);
	//TODO add it to the log tab
}

// Show a message to the user
void report(read r) {

	if (PROGRAM_TEST) MessageBox(Handle.window, r, PROGRAM_NAME, MB_OK);
}

// Given access to a handle, close it and make it null
void CloseHandleSafely(HANDLE *handle) {

	if (*handle && *handle != INVALID_HANDLE_VALUE) { // Only do something if it's not null or -1

		if (!CloseHandle(*handle)) error(L"closehandle safely");
		*handle = NULL; // Make it null so we don't try to close it again
	}
}

// Start a new thread to execute the given function
void BeginThread(LPVOID function) {

	// Create a new thread that runs the function
	DWORD info = 0;
	HANDLE thread = (HANDLE)_beginthreadex(
		(void *)                          NULL,     // Use default security attributes
		(unsigned)                        0,        // Default initial stack size
		(unsigned (__stdcall *) (void *)) function, // Function where the new thread will begin execution
		(void *)                          NULL,     // Pass no parameter to the thread
		(unsigned)                        0,        // Create and start thread
		(unsigned *)                      &info);   // Writes thread identifier, cannot be null for Windows 95
	if (!thread) error(L"beginthreadex");

	// Tell the system this thread is done with the new thread handle
	if (!CloseHandle(thread)) error(L"closehandle thread");
}

// Takes a dialog handle, a control identifier, and text
// Sets the text to the control
void TextDialogSet(HWND dialog, int control, read r) {

	// Get a handle to the window of the control and set the text of that window
	if (!SetDlgItemText(dialog, control, r)) error(L"setdlgitemtext");
}

// Takes a dialog handle and a dialog control identifier number
// Gets the text of the control
// Returns a string, blank if no text or any error
CString TextDialog(HWND dialog, int control) {

	// Get a handle to the window of the control and return the text of that window
	return TextWindow(GetDlgItem(dialog, control));
}

// Takes a window and text
// Sets the text to the window
void TextWindowSet(HWND window, read r) {

	// Set the text to the window
	if (!SetWindowText(window, r)) error(L"setwindowtext");
}

// Takes a window handle
// Get the text of the window, like its title or the text inside it
// Returns blank if no text or any error
CString TextWindow(HWND window) {

	// If the window handle is null, return blank
	if (!window) return L"";

	// Find the required buffer size, in bytes
	int size =
		(int)SendMessage(window, WM_GETTEXTLENGTH, 0, 0) // The number of text characters
		+ 1; // Add 1 to have space in the buffer for the null terminator

	// Open a string
	CString s;
	LPWSTR buffer = s.GetBuffer(size);

	// Write the window text into the buffer
	GetWindowText( // Writes all the text and a null terminator
		window,    // Handle to window
		buffer,    // Destination buffer
		size);     // Size of the buffer

	// Close the string and return it
	s.ReleaseBuffer();
	return s;
}

// Add text in a new line at the bottom of an edit control, and scroll to view it
void EditAppend(HWND window, read r) {

	// Add the given text to a new line at the bottom
	CString s = TextWindow(window); // Get the text that's already there, the user may have edited it
	if (is(s)) s += L"\r\n";        // If not blank, start with a newline to make sure r will be on its own line
	s += r;                         // Add the given text
	TextWindowSet(window, s);       // Put the edited text back into the control

	// Scroll down to the bottom
	EditScroll(window);
}

// Scroll to the bottom of an edit control
void EditScroll(HWND window) {

	// Find out how many lines there are, and then scroll down that many
	LRESULT lines = SendMessage(window, EM_GETLINECOUNT, 0, 0);
	SendMessage(window, EM_LINESCROLL, 0, lines);
}

// Make a new edit window
HWND WindowCreateEdit(bool scrollbars, bool capacity) {

	// Prepare the style of the edit control window
	DWORD style =
		WS_CHILD |                       // Required for child windows
		ES_LEFT |                        // Left-align text
		ES_MULTILINE;                    // Hold multiple lines of text
	if (scrollbars) style |=
		WS_VSCROLL | WS_HSCROLL |        // Scroll bars
		ES_AUTOHSCROLL | ES_AUTOVSCROLL; // Scroll when the user enters text

	// Create the edit window
	HWND window = WindowCreate(L"EDIT", NULL, style, 0, Handle.window, NULL);

	// Have it use Tahoma, not the default system font
	SendMessage( 
		(HWND)window,        // Send the message to this window
		WM_SETFONT,          // Message to send
		(WPARAM)Handle.font, // Handle to font
		0);                  // Don't tell the control to immediately redraw itself

	// Expand its text capacity
	if (capacity) SendMessage(window, EM_LIMITTEXT, 0, 0);

	// Return the handle to the edit window we made
	return window;
}

// Make a new button
HWND WindowCreateButton(read r) {

	// Prepare the style of the button window
	DWORD style =
		WS_CHILD |     // Required for child windows
		BS_PUSHBUTTON; // Have the button send the window a message when the user clicks it

	// Create the edit window
	HWND window = WindowCreate(L"BUTTON", NULL, style, 0, Handle.window, NULL);

	// Title it
	TextWindowSet(window, r);

	// Have it use Tahoma, not the default system font
	SendMessage( 
		(HWND)window,        // Send the message to this window
		WM_SETFONT,          // Message to send
		(WPARAM)Handle.font, // Handle to font
		0);                  // Don't tell the control to immediately redraw itself

	// Return the handle to the button window we made
	return window;
}

// Make a new window
HWND WindowCreate(read name, read title, DWORD style, int size, HWND parent, HMENU menu) {

	// Create the window
	HWND window = CreateWindow(
		name,                   // System or registered window class name or class
		title,                  // Text to show in the title bar, or null for no text
		style,                  // Window style
		size, size, size, size, // Window position and size
		parent,                 // Handle to parent window
		menu,                   // Menu handle or child window identification number
		Handle.instance,        // Program instance handle
		NULL);                  // No parameter
	if (!window) error(L"createwindow");
	return window;
}




void WindowSize(HWND window, int x, int y)
{
	// takes a window and pixel adjustments
	// adjusts the width and height of the window by x and y
	// returns nothing

	// NO SIZING NECESSARY
	if (!x && !y) return;

	// GET THE WINDOW'S CURRENT SIZE
	RECT r;
	if (!GetWindowRect(window, &r)) { error(L"windowsize: error getwindowrect"); return; }

	// ADJUST THE SIZE
	sizeitem size;
	size.x = r.left;
	size.y = r.top;
	size.w = r.right - r.left + x;
	size.h = r.bottom - r.top + y;

	// MOVE THE WINDOW
	WindowMove(window, size, true);
}

void WindowMove(HWND window, sizeitem size, bool paint)
{
	// takes a window and size item
	// moves the window
	// returns nothing

	// POSITION AND RESIZE THE WINDOW WITHOUT SENDING A PAINT MESSAGE
	if (!MoveWindow(window, size.x, size.y, size.w, size.h, paint)) error(L"windowmove: error movewindow");
}




// Make an edit window editable or read only
void WindowEdit(HWND window, boolean edit) {

	// Send the window a messge to make it read only or editable
	SendMessage( 
		(HWND)window,   // Send the message to this window
		EM_SETREADONLY, // Message to send
		(WPARAM)!edit,  // true to set read only
		0);             // Not used, must be 0
}

// Takes text to display in the dialog box
// Shows the user the browse for folder dialog box
// Returns the path the user chose, blank on cancel or error
CString DialogBrowse(read display) {

	// Setup information for the dialog box
	WCHAR name[MAX_PATH];
	BROWSEINFO info;
	ZeroMemory(&info, sizeof(info));
	info.hwndOwner      = Handle.window;        // Handle to parent window for the browse dialog
	info.pidlRoot       = NULL;                 // Browse from the desktop
	info.pszDisplayName = name;                 // Write the name of the chosen folder here
	info.lpszTitle      = display;              // Text to display in the browse dialog
	info.ulFlags        = BIF_RETURNONLYFSDIRS; // Only allow file system folders
	info.lpfn           = NULL;                 // No callback function
	info.lParam         = 0;                    // No program-defined value
	info.iImage         = 0;                    // Will be filled with the icon index in the system image list

	// Show the user the browse for folder system dialog box
	CoInitialize(NULL);
	LPITEMIDLIST list = SHBrowseForFolder(&info); // Returns memory we are responsible for freeing
	if (!list) return L""; // The user clicked Cancel or the close X

	// Get the path the user chose
	WCHAR buffer[MAX_PATH];
	SHGetPathFromIDList(list, buffer);
	CoTaskMemFree(list); // Free the COM memory the system allocated for us
	return buffer;
}

// Mix two colors in the given amounts
COLORREF ColorMix(COLORREF color1, int amount1, COLORREF color2, int amount2) {

	// Extract the individual primary color values
	int red1, red2, green1, green2, blue1, blue2;
	red1   = GetRValue(color1);
	red2   = GetRValue(color2);
	green1 = GetGValue(color1);
	green2 = GetGValue(color2);
	blue1  = GetBValue(color1);
	blue2  = GetBValue(color2);

	// Mix each color value using a weighted average, rounds fractions down
	int red, green, blue;
	red = green = blue = 0;
	if (amount1 + amount2) {

		red   = ((red1   * amount1) + (red2   * amount2)) / (amount1 + amount2);
		green = ((green1 * amount1) + (green2 * amount2)) / (amount1 + amount2);
		blue  = ((blue1  * amount1) + (blue2  * amount2)) / (amount1 + amount2);
	}

	// Return the mixed color
	return RGB(red, green, blue);
}

// Make a brushitem from the given color
// Returns a brushitem that must be deleted, or null on error
brushitem CreateBrush(COLORREF color) {

	brushitem brush;
	brush.color = color;
	brush.brush = CreateSolidBrush(color);
	if (!brush.brush) error(L"createsolidbrush");
	return brush;
}

// Repaint the window right now
void PaintMessage(HWND window) {

	// Choose window
	if (!window) window = Handle.window;

	// Mark the client area of the main window as necessary to draw
	int result = InvalidateRect(
		window, // Handle to window
		NULL,   // Invalidate the entire client area of the window
		false); // false to not wipe the window with the background color
	if (!result) error(L"invalidaterect");

	// Call the window procedure directly with a paint message right now
	// Send the window a paint message and have it process it right now
	if (!UpdateWindow(window)) error(L"updatewindow");
}

// Adds the program icon to the taskbar notification area
void TaskbarIconAdd() {

	if (State.taskbar) return;           // Icon already there, leave
	State.taskbar = State.stage->icon16; // Pick the icon for the current stage

	// Add the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize           = sizeof(info);                     // Size of this structure
	info.hWnd             = Handle.window;                    // Handle to the window that will receive messages
	info.uID              = TASKBAR_ICON;                     // Program defined identifier
	info.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP; // Mask for message, icon and tip
	info.uCallbackMessage = MESSAGE_TASKBAR;                  // Program defined message identifier
	info.hIcon            = State.taskbar;                    // Icon handle
	lstrcpy(info.szTip, PROGRAM_NAME);                        // 64 character buffer for tooltip text
	if (!Shell_NotifyIcon(NIM_ADD, &info)) error(L"shell_notifyicon nim_add");
}

// Updates the program icon in the taskbar notification area
void TaskbarIconUpdate() {

	if (!State.taskbar) return;                       // No icon to update, leave
	if (State.taskbar == State.stage->icon16) return; // Icon doesn't need to be updated, leave
	State.taskbar = State.stage->icon16;              // Record that we updated the icon

	// Add the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize           = sizeof(info);  // Size of this structure
	info.hWnd             = Handle.window; // Handle to the window that will receive messages
	info.uID              = TASKBAR_ICON;  // Program defined identifier
	info.uFlags           = NIF_ICON;      // Mask for icon only
	info.hIcon            = State.taskbar; // Icon handle
	if (!Shell_NotifyIcon(NIM_MODIFY, &info)) error(L"shell_notifyicon nim_modify");
}

// Removes the program icon from the taskbar notification area
void TaskbarIconRemove() {

	if (!State.taskbar) return; // No icon to remove, leave
	State.taskbar = false;      // Record tha we removed the icon

	// Remove the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);  // Size of this structure
	info.hWnd   = Handle.window; // Handle to the window that will receive messages
	info.uID    = TASKBAR_ICON;  // Program defined identifier
	if (!Shell_NotifyIcon(NIM_DELETE, &info)) error(L"shell_notifyicon nim_delete");
}

// Takes a system cursor identifier
// Gets the shared handle to the cursor
// Returns it, or null if any error
HCURSOR LoadSharedCursor(read name) {

	// Get the handle of the system cursor
	HCURSOR cursor = (HCURSOR)LoadImage(
		NULL,         // Load from the system
		name,         // Cursor to load
		IMAGE_CURSOR, // Image type
		0, 0,         // Use the width and height of the resource
		LR_SHARED);   // Share the system resource
	if (!cursor) error(L"loadimage cursor");
	return cursor;
}

// Takes the name of an icon resource and the size to load, or 0 for default
// Loads it
// Returns it, or null if any error
HICON LoadIconResource(read name, int w, int h) {

	// Create the icon from the resource
	HICON icon = (HICON)LoadImage(
		Handle.instance,  // Load from this instance
		name,             // Resource name
		IMAGE_ICON,       // Image type
		w, h,             // Width and height to load
		LR_DEFAULTCOLOR); // Default flag does nothing
	if (!icon) error(L"loadimage icon");
	return icon;
}

// Sets the cursor to the given cursor if it isn't that already
void CursorSet(HCURSOR cursor) {

	HCURSOR current = GetCursor(); // Find out what the cursor is now
	if (cursor && current && cursor != current) { // If we have both and they're different
		if (!SetCursor(cursor)) error(L"setcursor"); // Set the new given one
	}
}

// Takes a menu name and loads it from resources
HMENU MenuLoad(read name) {

	HMENU menus = LoadMenu(Handle.instance, name); // Load the menu resource
	if (!menus) error(L"loadmenu");
	return menus;
}

// Takes a menu index, like 0 for the first one
// Clips out the submenu at that index
HMENU MenuClip(HMENU menus, int index) {

	HMENU menu = GetSubMenu(menus, index); // Clip off the submenu at the given index
	if (!menu) error(L"getsubmenu");
	return menu;
}

// Takes a menu, command, and the state to set
// Sets the menu item
void MenuSet(HMENU menu, UINT command, UINT state, HBITMAP bitmap) {

	MENUITEMINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(MENUITEMINFO);
	info.fMask  = 0;
	if (bitmap) { info.fMask = info.fMask | MIIM_BITMAP; info.hbmpItem = bitmap; } // Set menu item info
	if (state)  { info.fMask = info.fMask | MIIM_STATE;  info.fState   = state; }
	if (!SetMenuItemInfo(menu, command, false, &info)) error(L"setmenuiteminfo");
}

// Takes menus and whether the context menu is being shown from the taskbar notification icon or not
// Takes size null to put the menu at the mouse pointer, or a size item in client coordinates
// Displays the context menu and waits for the user to make a choice
// Returns the menu item identifier of the choice, or 0 if the user cancelled the menu or any error
UINT MenuShow(HMENU menu, bool taskbar, sizeitem *size) {

	// Use the given size or mouse position
	sizeitem position;
	if (size) {
		position = *size;
		position.Screen(); // Convert the given client size into screen coordinates
	} else {
		position = MouseScreen(); // Get the mouse position in screen coordinates
	}

	// Context menu for notification icon
	if (taskbar) SetForegroundWindow(Handle.window);

	// Show the context menu and hold execution here until the user chooses from the menu or cancels it
	AreaPopUp();
	UINT choice = TrackPopupMenu(
		menu,            // Handle to the menu to display
		TPM_NONOTIFY |   // Return the chosen menu item without sending messages to the main window
		TPM_RETURNCMD |
		TPM_RIGHTBUTTON, // Let the user click on an item with the left or right button
		position.x,      // Desired menu position in screen coordinates
		position.y,
		0,
		Handle.window,
		NULL);
	AreaPopDown();

	// Context menu for notification icon
	if (taskbar) PostMessage(Handle.window, WM_NULL, 0, 0);

	// Return the user's choice, 0 they clicked outside the menu
	return choice;
}

// Takes the size in the client area where the tool will be shown, and the text to show
// Assigns the tooltip window to this area and sets its text
void TipAdd(sizeitem size, read r) {

	// Attach the tooltip to a rectangle in the main window
	TOOLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize      = sizeof(info);     // Size of this structure
	info.uFlags      = TTF_SUBCLASS;     // Have the tooltip control get messages from the tool window
	info.hwnd        = Handle.window;    // Handle to the window that contains the tool region
	info.uId         = 0;                // Tool identifying number
	info.rect        = size.Rectangle(); // Rectangle in the window of the tool
	info.hinst       = NULL;             // Only used when text is loaded from a resource
	info.lpszText    = (LPWSTR)r;        // Text
	info.lParam      = 0;                // No additional value assigned to tool
	if (!SendMessage(Handle.tip, TTM_ADDTOOL, 0, (LPARAM)&info)) error(L"sendmessage ttm_addtool");
}

// Have window capture the mouse if it doesn't have it already, null to use the main window
void MouseCapture(HWND window) {

	if (!window) window = Handle.window; // Use the main window if the caller didn't specify one
	if (GetCapture() != window) SetCapture(window); // If that window doesn't already have the mouse, capture it
}

// Have window release its capture of the mouse if it has it, null to use the main window
void MouseRelease(HWND window) {

	if (!window) window = Handle.window; // Pick the main window if the caller didn't specify one
	if (GetCapture() == window) ReleaseCapture(); // Only release it if window has it
}

// True if the mouse is in the client area but not over a child window control
bool MouseInside() {

	// Get the mouse cursor's position in client window coordinates, and the size of the client window
	sizeitem mouse = MouseClient();
	sizeitem client = SizeClient();

	// The mouse is inside if it is inside the client area and outside all the child window controls
	return
		client.Inside(mouse)     &&
		!Area.list.Inside(mouse) &&
		!Area.tabs.Inside(mouse) &&
		!Area.info.Inside(mouse);
}

// Find the area, if any, the mouse is currently positioned over, null if none
areaitem *MouseOver() {

	// Get the mouse cursor's position in client window coordinates
	sizeitem mouse = MouseClient();
	sizeitem client = SizeClient();
	if (!client.Inside(mouse)) return NULL; // Make sure the mouse is inside the client area

	// Move down each area item to find the one the mouse is over
	areaitem *a = Area.all;
	while (a) {
		if (a->size.Inside(mouse)) return a; // Found it
		a = a->next;
	}
	return NULL; // Not found, the mouse is away from them all
}

// Get the mouse position in x and y coordinates inside the given area item
sizeitem MouseArea(areaitem *a) {

	sizeitem s = MouseClient(); // Get the mouse position in client window coordinates
	s.x -= a->size.x; // Convert to area coordinates
	s.y -= a->size.y;
	return s;
}

// Takes a window or null to use the main one
// Gets the mouse position in client coordinates
// Returns x and y coordinates in a size item
sizeitem MouseClient(HWND window) {

	if (!window) window = Handle.window; // Choose window
	sizeitem s = MouseScreen(); // Get the mouse pointer position in screen coordinates
	s.Client(window); // Convert the position to the client coordinates of the given window
	return s;
}

// Gets the mouse position in screen coordinates
// Returns x and y coordinates in a size item
sizeitem MouseScreen() {

	// If we have a popup window or menu open, report that the mouse is off the screen
	sizeitem s;
	s.x = -1;
	s.y = -1;
	if (State.pop) return s;

	// Get the mouse position in screen coordinates
	POINT p;
	if (!GetCursorPos(&p)) return s; // Will return error if Windows is locked
	s.Set(p);
	return s;
}

// Takes a system color index
// Gets the system brush for that color
// Returns a brush that should not be deleted, or null if any error
brushitem BrushSystem(int color) {

	// Get the system brush for the given color index, as well as the color itself
	brushitem brush;
	brush.color = GetSysColor(color);
	brush.brush = GetSysColorBrush(color);
	if (!brush.brush) error(L"getsyscolorbrush");
	return brush;
}

// Takes a color
// Creates a brush of that color
// Returns a brush that must be deleted, or null if any error
brushitem BrushColor(COLORREF color) {

	// Create a new brush of the given solid color
	brushitem brush;
	brush.color = color;
	brush.brush = CreateSolidBrush(color);
	if (!brush.brush) error(L"createsolidbrush");
	return brush;
}

// Takes two colors and amounts
// Mixes the colors in those porportions
// Returns the mixed color
COLORREF MixColors(COLORREF color1, int amount1, COLORREF color2, int amount2) {

	// Extract the individual color values
	int red1, red2, green1, green2, blue1, blue2;
	red1   = GetRValue(color1);
	red2   = GetRValue(color2);
	green1 = GetGValue(color1);
	green2 = GetGValue(color2);
	blue1  = GetBValue(color1);
	blue2  = GetBValue(color2);

	// Mix each color value using a weighted average, rounds fractions down
	int red, green, blue;
	red = green = blue = 0;
	if (amount1 + amount2) {
		
		red   = ((red1   * amount1) + (red2   * amount2)) / (amount1 + amount2);
		green = ((green1 * amount1) + (green2 * amount2)) / (amount1 + amount2);
		blue  = ((blue1  * amount1) + (blue2  * amount2)) / (amount1 + amount2);
	}

	// Return the mixed color
	return RGB(red, green, blue);
}

// Measure the width and height of the client area of the given window
sizeitem SizeClient(HWND window) {

	// Pick main window if none given
	if (!window) window = Handle.window;

	// Find the width and height of the client area
	sizeitem size;
	RECT rectangle;
	if (!GetClientRect(window, &rectangle)) { error(L"getclientrect"); return size; }
	size.Set(rectangle);
	return size;
}

// Find the size of the given window on the screen
sizeitem SizeWindow(HWND window) {

	// Pick main window if none given
	if (!window) window = Handle.window;

	// Find the width and height of the client area
	sizeitem size;
	RECT rectangle;
	if (!GetWindowRect(window, &rectangle)) { error(L"getwindowrect"); return size; }
	size.Set(rectangle);
	return size;
}

// Takes a device context with a font loaded inside, and text
// Determines how wide and high in pixels the text painted will be
// Returns the size width and height, or zeroes if any error
sizeitem SizeText(deviceitem *device, read r) {

	// Get the pixel dimensions of text written in the loaded font
	SIZE size;
	if (!GetTextExtentPoint32(device->device, r, length(r), &size)) {

		error(L"gettextextentpoint32");
		size.cx = size.cy = 0;
	}

	// Return the size, will be all 0 for blank text
	sizeitem s(size);
	return s;
}

// Takes a deviceitem that has a font, text, and a bounding position and size
// Paints the text there
void PaintLabel(deviceitem *device, read r, sizeitem size) {

	// Paint the text, if the background is opaque, this will cause a flicker
	RECT rectangle = size.Rectangle();
	if (!DrawText(device->device, r, -1, &rectangle, DT_NOPREFIX)) error(L"drawtext");
}

// Takes a device context that has a font loaded into it, text, position and bounding size, and formatting options
// Fills the size and paints the text with an ellipsis
void PaintText(deviceitem *device, read r, sizeitem size, bool horizontal, bool vertical, bool left, bool right, int adjust, HFONT font, brushitem *color, brushitem *background) {

	// Prepare the device context
	if (font)       device->Font(font);
	if (color)      device->FontColor(color->color);
	if (background) device->BackgroundColor(background->color);

	// Find out how big the text will be when painted
	sizeitem text;
	text = SizeText(device, r);

	// If only a position was provided, put in the necessary size
	if (size.w <= 0) size.w = text.w;
	if (size.h <= 0) size.h = text.h;

	// Define space once in a local variable
	int space = 4;

	// Add margins
	sizeitem bound = size;
	if (left) bound.ShiftLeft(space);
	if (right) bound.w -= space;
	bound.Check();

	// Make text small enough so it will fit within bound
	if (text.w > bound.w) text.w = bound.w;
	if (text.h > bound.h) text.h = bound.h;

	// Position the text within bound
	text.x = bound.x;
	text.y = bound.y;
	if (horizontal && text.w < bound.w) text.x += ((bound.w - text.w) / 2);
	if (vertical   && text.h < bound.h) text.y += ((bound.h - text.h) / 2);

	// Adjust the text if doing so doesn't place it outside the bound
	if (text.x + adjust >= bound.x && text.Right() + adjust <= bound.Right()) text.x += adjust;

	// Fill the background, this won't make draw text's flicker worse
	if (background) PaintFill(device, size, background->brush);
	else            PaintFill(device, size);

	// Paint the text
	RECT rectangle;
	if (text.Is()) {

		// Draw text paints background beneath the text and then text over it, creating a flicker
		rectangle = text.Rectangle();
		if (!DrawText(device->device, r, -1, &rectangle, DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE)) error(L"drawtext");
	}

	// Put back the device context
	if (background) device->BackgroundColor(device->backgroundcolor);
	if (color)      device->FontColor(device->fontcolor);
	if (font)       device->Font(Handle.font);
}

// Use the device to paint size with brush
void PaintFill(deviceitem *device, sizeitem size, HBRUSH brush) {

	// Make sure there is a size to fill
	if (!size.Is()) return;

	// Choose brush
	if (!brush) brush = Handle.background.brush;

	// Paint the rectangle
	RECT rectangle = size.Rectangle();
	FillRect(device->device, &rectangle, brush); // Will return error if Windows is locked
}

// Takes a device context, size, and brushes for the upper left and lower right corners
// Paints a 1 pixel wide border inside the size
void PaintBorder(deviceitem *device, sizeitem size, HBRUSH brush1, HBRUSH brush2) {

	// Use the same brush for both corners
	if (!brush2) brush2 = brush1;

	// Paint the 4 edges of the border
	sizeitem edge;
	edge = size;                       edge.w--;                         edge.h = 1;  PaintFill(device, edge, brush1);
	edge = size;                       edge.w = 1; edge.y++;             edge.h -= 2; PaintFill(device, edge, brush1);
	edge = size; edge.x += edge.w - 1; edge.w = 1;                       edge.h--;    PaintFill(device, edge, brush2);
	edge = size;                                   edge.y += edge.h - 1; edge.h = 1;  PaintFill(device, edge, brush2);
}

// Takes a device context, position, and icon
// Provide a background brush for flicker free drawing, or NULL for a transparent background
// Paints the icon
void PaintIcon(deviceitem *device, sizeitem position, HICON icon, HBRUSH background) {

	int result = DrawIconEx(
		device->device,          // Handle to device context
		position.x, position.y,  // Position to paint the icon
		icon,                    // Handle to icon to paint
		0, 0,                    // Use the width and height of the icon resource
		0,                       // Not an animated icon
		background,              // Paint into an offscreen bitmap over this brush first to not flicker on the screen
		DI_IMAGE | DI_MASK);     // Use the image and mask to draw alpha icons correctly
	if (!result) error(L"drawiconex");
}

// Make a font based on what the system uses in menus, true to underline it
HFONT FontMenu(boolean underline) {

	NONCLIENTMETRICS info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info); // Must define _WIN32_WINNT=0x0501 for sizeof(info) to return the size SPI_GETNONCLIENTMETRICS expects
	int result = SystemParametersInfo(
		SPI_GETNONCLIENTMETRICS, // System parameter to retrieve
		sizeof(info),            // Size of the structure
		&info,                   // Structure to fill with information
		0);                      // Not setting a system parameter
	if (!result) { error(L"systemparametersinfo getnonclientmetrics"); return NULL; }
	if (underline) info.lfMenuFont.lfUnderline = true; // Underline if requested
	HFONT font = CreateFontIndirect(&info.lfMenuFont);
	if (!font) error(L"createfontindirect systemparametersinfo");
	return font;
}

// Make a font of the given face name and point size
HFONT FontName(read face, int points) {

	LOGFONT info;
	ZeroMemory(&info, sizeof(info));
	info.lfHeight         = -points;                      // Point size, minus sign required
	info.lfWidth          = 0;                            // Default width
	info.lfEscapement     = 0;                            // Not rotated
	info.lfOrientation    = 0;
	info.lfWeight         = FW_NORMAL;                    // Normal, not bold
	info.lfItalic         = (byte)false;                  // Not italic
	info.lfUnderline      = (byte)false;                  // Not underlined
	info.lfStrikeOut      = (byte)false;                  // No strikeout
	info.lfCharSet        = ANSI_CHARSET;                 // Use ANSI characters
	info.lfOutPrecision   = OUT_DEFAULT_PRECIS;           // Default size precision
	info.lfClipPrecision  = CLIP_DEFAULT_PRECIS;          // Default clipping behavior
	info.lfQuality        = DEFAULT_QUALITY;              // Don't force antialiasing
	info.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE; // Only used if the font name is unavailable
	lstrcpy(info.lfFaceName, face);                       // Font name
	HFONT font = CreateFontIndirect(&info);
	if (!font) error(L"createfontindirect logfont");
	return font;
}

int Greatest(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8)
{
	// takes numbers
	// determines the greatest amongst them
	// returns it

	// FIND THE BIGGEST POSITIVE NUMBER AND RETURN IT
	int i;
	i = 0;
	if (i < i1) i = i1;
	if (i < i2) i = i2;
	if (i < i3) i = i3;
	if (i < i4) i = i4;
	if (i < i5) i = i5;
	if (i < i6) i = i6;
	if (i < i7) i = i7;
	if (i < i8) i = i8;
	return(i);
}

// Initialize our use of the common controls
void InitializeCommonControls() {

	INITCOMMONCONTROLSEX info; // Oh yeah
	ZeroMemory(&info, sizeof(info));
	info.dwSize = sizeof(info); // Size of this structure
	info.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES; // Load list and tree view classes
	if (!InitCommonControlsEx(&info)) error(L"initcommoncontrolsex");
}

// Takes a window handle and timer identifier
// Kills the timer
void KillTimerSafely(UINT_PTR timer, HWND window) {

	if (!window) window = Handle.window; // Choose window
	if (!KillTimer(window, timer)) error(L"killtimer"); // Kill the timer
}

// Takes a timer identifier and the number of milliseconds after which the timer should expire
// Sets the timer
void TimerSet(UINT_PTR timer, UINT time, HWND window) {

	if (!window) window = Handle.window; // Choose window
	if (!SetTimer(window, timer, time, NULL)) error(L"settimer"); // Set the timer
}

// Takes a path to a file and parameters text or blank
// Shell executes it
void FileRun(read path, read parameters) {

	// Shell execute the given text
	ShellExecute(
		Handle.window,  // Handle to a window to get message boxes from this operation
		NULL,           // Default run
		path,           // File to run
		parameters,     // Parameters ot pass to program
		L"C:\\",        // Starting directory
		SW_SHOWNORMAL); // Default show
}


// Takes a dialog box procedure function and resource name
// Shows the dialog box
// Returns the result from the dialog box
int Dialog(LPCTSTR resource, DLGPROC procedure, LPARAM lparam) {

	// Choose procedure
	if (!procedure) procedure = DialogProcedure;

	// Show the dialog box, sending messages to its procedure and returning here when it is closed
	int result;
	AreaPopUp();
	if (lparam) result = (int)DialogBoxParam(Handle.instance, resource, Handle.window, procedure, lparam);
    else result = (int)DialogBox(Handle.instance, resource, Handle.window, procedure);
	AreaPopDown();
	return result;
}

// Dialog box procedure
BOOL CALLBACK DialogProcedure(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {

	// The dialog is about to be displayed
	switch (message) {
	case WM_INITDIALOG: 

		// Let the system place the focus
		return true;

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

	// Have windows process messages
	return false;
}

// Takes a tab control child window that holds tabs, an index number 0+ for a new tab, and text to title it
// Adds a new tab
void AddTab(HWND window, int index, read title) {

	// Fill out the structure and send the message
	TCITEM item;
	ZeroMemory(&item, sizeof(item));
	item.mask        = TCIF_TEXT;     // Parts set below
	item.dwState     = 0;             // Ignored when inserting a new tab
	item.dwStateMask = 0;             // Ignored when inserting a new tab
	item.pszText     = (LPWSTR)title; // Text will only be read
	item.cchTextMax  = 0;             // Only used when the structure is receiving information
	item.iImage      = -1;            // No icon
	item.lParam      = 0;             // No extra information
	SendMessage(window, TCM_INSERTITEM, index, (LPARAM)&item);
}

// Change window to show the given small and large icons
void SetIcon(HWND window, HICON icon16, HICON icon32) {

	// Send two messages
	SendMessage(window, WM_SETICON, ICON_SMALL, (LPARAM)icon16);
	SendMessage(window, WM_SETICON, ICON_BIG,   (LPARAM)icon32);
}

// Takes a file extension like ".zip" which can be blank
// Gets shell information about the extension and its index in the program image list
// Returns the program image list icon index and writes the type text, covering an error with the file icon and composed text
int Icon(read ext, CString *type) {

	int icon = IconGet(ext, type);           // If the program list is full, icon will be -1 and type will be system or composed text
	if (icon == -1) icon = Handle.icon.file; // If the icon could not be found or added in the program list, use the index of the default shell file icon in the list
	return icon;
}

// Takes a file extension like ".zip" which can be blank
// Gets shell information about the extension and its index in the program image list
// Returns the program image list icon index and writes the type text, or -1 and blanks if any error
int IconGet(read ext, CString *type) {

	// If the requested extension is the last one loaded, fill the request quickly
	if (same(ext, Handle.icon.ext, Matching)) {

		*type = Handle.icon.type;
		return Handle.icon.index;
	}

	// Get the system index and type text for the extension
	int systemindex;
	CString typetext;
	if (!ShellInfo(ext, &systemindex, &typetext)) { *type = L""; return -1; }

	// Find the system index in the program image list or make program index -1
	int programindex;
	for (programindex = Handle.icon.count - 1; programindex >= 0; programindex--) {

		if (Handle.icon.source[programindex] == systemindex) break;
	}

	// None of the icons in the program image list have the extension's system index
    if (programindex == -1) {

		// Get the icon for the extension, add it to the list, and destroy it
		HICON icon;
		if (!ShellIcon(ext, &icon)) { *type = L""; return -1; }
		programindex = IconAdd(icon, systemindex);
		DestroyIconSafely(icon);
	}

	// Copy or compose the type text, windows 9x returns blank for unknown extensions
	if      (is(typetext)) *type = typetext;
	else if (is(ext))      *type = upper(off(ext, L".")) + L" File";
	else                   *type = L"File";

	// Keep the information to answer the same question without calling the system
	Handle.icon.ext   = ext;
	Handle.icon.type  = *type;
	Handle.icon.index = programindex;

	// Return the index of the icon in the program image list
	return programindex;
}

// Takes the text resource name of an icon, like "ICON_NAME"
// Loads the icon into the program image list
// Returns the index of the icon in the program image list, or -1 if any error
int IconAddResource(read resource) {

	// Load the icon from the resource
	HICON icon = (HICON)LoadImage(
		Handle.instance,  // Handle to instance
		resource,         // Resource name text
		IMAGE_ICON,       // This resource is an icon
		16, 16,           // Size
		LR_DEFAULTCOLOR); // No special options
	if (!icon) { error(L"loadimage"); return -1; }

	int programindex = IconAdd(icon, -1); // Insert the icon into the program image list
	DestroyIconSafely(icon);              // Destroy the icon
	return programindex;                  // Return the index of the icon in the program image list, or -1 if it could not be added
}

// Takes an icon and its index in the system image list, or -1 to add it with that marker
// Adds the icon to the program image list if there is room
// Returns the index of the icon in the program image list, or -1 if any error
int IconAdd(HICON icon, int systemindex) {

	// Make sure the program image list isn't full
	if (Handle.icon.count >= ICON_CAPACITY) return -1;

	// Add the icon to the end of the program image list
	int programindex = ImageList_ReplaceIcon(Handle.icon.list, -1, icon);
	if (programindex == -1) { error(L"imagelist_replaceicon"); return -1; }

	// Write the index in the array, up the count, and return the program index of the added icon
	Handle.icon.source[Handle.icon.count] = systemindex;
	Handle.icon.count++;
	return programindex;
}

// Takes a column index number and icon index number
// Shows the icon in the column header
void ColumnIcon(HWND window, int column, int icon) {

	// Set the icon in the column
	LV_COLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask   = LVCF_IMAGE;
	info.iImage = icon;
	if (!ListView_SetColumn(window, column, &info)) error(L"listview_setcolumn");
}

// Takes a column index number or -1 to remove the selection from the column that has it
// Highlights the column as selected
void ColumnSelect(HWND window, int column) {

	SendMessage(window, LVM_FIRST + 140, column, 0); // Send LVM_SETSELECTEDCOLUMN by offsetting by 140
	InvalidateRect(window, NULL, true);              // Invalidate the list view control and have it erased before painted
}

// Takes information for a list view column
// Adds the column to the list view control
void ListColumnInsert(HWND window, int column, int format, int image, read r, int width) {

	// Add the column to the list view control
	LV_COLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask    = LVCF_FMT | LVCF_IMAGE | LVCF_TEXT | LVCF_WIDTH;
	info.fmt     = format;
	info.iImage  = image;
	info.pszText = (LPWSTR)r;
	info.cx      = width;
	if (ListView_InsertColumn(window, column, &info) == -1) error(L"listview_insertcolumn");
}

// Takes the column to delete
// Deletes it from the list view control
void ListColumnDelete(HWND window, int column) {

	// Delete the column
	if (!ListView_DeleteColumn(window, 0)) error(L"listview_deletecolumn");
}

// Takes a row number
// Looks up the parameter stored in that row
// Returns the parameter
LPARAM ListGet(HWND window, int row) {

	// Read the parameter at the given row number
	LVITEM info;
	ZeroMemory(&info, sizeof(info));
	info.iItem = row;        // Look at item in the given row number
	info.mask  = LVIF_PARAM; // Get the parameter
	int result;
	result = ListView_GetItem(window, &info);
	if (!result) { error(L"listview_getitem"); return 0; }

	// Return the extracted parameter
	return info.lParam;
}

// Gets the parameter of the row that has the keyboard mark
// Returns the parameter, or 0 if there is no keyboard mark
LPARAM ListMark(HWND window) {

	// Get the row that has the mark
	int row = ListView_GetSelectionMark(window);
	if (row == -1) return 0; // No mark

	// Return the parameter of the row
	return ListGet(window, row);
}

// Gets the parameter of the row beneath the mouse
// Returns the parameter, or 0 if the mouse is not over a row
LPARAM ListMouse(HWND window) {

	// Get the list view row beneath the client coordinate
	LVHITTESTINFO info;
	ZeroMemory(&info, sizeof(info));
    info.pt = MouseClient(window).Point();
	int row;
	row = ListView_HitTest(window, &info);
	if (row == -1) return 0; // The mouse is not over a row

	// Return the parameter of the row
	return ListGet(window, row);
}

// Takes row and column coordinates
// Gets the bounding size of the list view sub item cell
// Returns the size
sizeitem ListCell(HWND window, int row, int column) {

	// Get the rectangle of the list view sub item and return the size
	sizeitem size;
	RECT rectangle;
	if (!ListView_GetSubItemRect(window, row, column, LVIR_BOUNDS, &rectangle)) { error(L"listview_getsubitemrect"); return size; }
	size.Set(rectangle);
	return size;
}

// Counts how many rows are in the list view control
int ListRows(HWND window) {

	// Count the rows
	int rows = ListView_GetItemCount(window);
	if (rows < 0) { error(L"listview_getitemcount"); return 0; }
	return rows;
}

// Counts how many selected rows are in the list view control
int ListSelectedRows(HWND window) {

	// Count the selected rows
	int rows = ListView_GetSelectedCount(window);
	if (rows < 0) { error(L"listview_getselectedcount"); return 0; }
	return rows;
}

// Takes a row number
// Determines if that row is selected
// Returns true or false
bool ListSelected(HWND window, int row) {

	// Determine if the row is selected
	if (ListView_GetItemState(window, row, LVIS_SELECTED)) return true;
	else return false;
}

// Selects all the rows in the list view control
void ListSelectAll(HWND window) {

	// Loop down the rows, selecting each one
	int row, rows;
	rows = ListRows(window);
	for (row = 0; row < rows; row++) ListView_SetItemState(window, row, LVIS_SELECTED, LVIS_SELECTED);
}

// Scrolls to the bottom of the list view control
void ListScroll(HWND window) {

	// Count the number of rows in the list view control
	int rows = ListRows(window);
	if (rows) {

		// Scroll to make the row completely visible
		if (!ListView_EnsureVisible(window, rows - 1, false)) error(L"listview_ensurevisible");
	}
}

// Rakes a row index number
// Removes the row
void ListRemove(HWND window, int row) {

	// Remove the row from the list view control
	if (!ListView_DeleteItem(window, row)) error(L"listview_deleteitem");
}

// Remove all the rows in the list view control
void ListRemoveAll(HWND window) {

	// Remove all the rows in the list view control
	if (!ListView_DeleteAllItems(window)) error(L"listview_deleteallitems");
}

// Takes the number of rows that are going to be added
// Turns list view painting off and informs the control of the new size to hold
void ListAddStart(HWND window, int rows) {

	if (rows > 1) {                                             // Suspend drawing if more than 1 row is going to be added
		SendMessage(window, WM_SETREDRAW, false, 0);            // Prevent changes in the list view window from being drawn
		ListView_SetItemCount(window, ListRows(window) + rows); // Tell the list view control how many rows it will untimately contain
	}
}

// Takes the number of rows that were added
// Turns list view painting back on and paints the window
void ListAddDone(HWND window, int rows) {

	if (rows > 1) {                                 // Resume drawing if it was suspended
		SendMessage(window, WM_SETREDRAW, true, 0); // Allow changes in the list view window to be drawn
		InvalidateRect(window, NULL, true);         // Invalidate the list view control and have it erased before painted
	}
}

// Takes a parameter, icons and text
// Adds a new row to the list view control
void ListAdd(HWND window, int columns, LPARAM p, int icon1, read r1, int icon2, read r2, read r3, read r4, read r5, read r6) {

	// Record the maximum text length
	State.list.max = Greatest(State.list.max, length(r1), length(r2), length(r3), length(r4), length(r5), length(r6));

	// Row and column position, the number of rows is the index of the next row
	LVITEM column1, column2, column3, column4, column5, column6;
	ZeroMemory(&column1, sizeof(column1));
	ZeroMemory(&column2, sizeof(column2));
	ZeroMemory(&column3, sizeof(column3));
	ZeroMemory(&column4, sizeof(column4));
	ZeroMemory(&column5, sizeof(column5));
	ZeroMemory(&column6, sizeof(column6));
	column1.iItem = ListRows(window);
	column1.iSubItem = 0;
	column2.iSubItem = 1;
	column3.iSubItem = 2;
	column4.iSubItem = 3;
	column5.iSubItem = 4;
	column6.iSubItem = 5;

	// Masks for text and the parameter
	column1.mask = LVIF_TEXT | LVIF_PARAM;
	column2.mask = column3.mask = column4.mask = column5.mask = column6.mask = LVIF_TEXT;

	// Text
	column1.pszText = (LPWSTR)r1;
	column2.pszText = (LPWSTR)r2;
	column3.pszText = (LPWSTR)r3;
	column4.pszText = (LPWSTR)r4;
	column5.pszText = (LPWSTR)r5;
	column6.pszText = (LPWSTR)r6;

	// Icons
	if (icon1 != -1) { column1.mask = column1.mask | LVIF_IMAGE; column1.iImage = icon1; }
	if (icon2 != -1) { column2.mask = column2.mask | LVIF_IMAGE; column2.iImage = icon2; }

	// Parameter
	column1.lParam = p;

	// Insert the item and set the row
	int row = ListView_InsertItem(window, &column1);
	if (row == -1) { error(L"listview_insertitem"); return; }
	column2.iItem = column3.iItem = column4.iItem = column5.iItem = column6.iItem = row;

	// Set the subitems
	if (columns >= 2 && !ListView_SetItem(window, &column2)) { error(L"listview_setitem"); return; }
	if (columns >= 3 && !ListView_SetItem(window, &column3)) { error(L"listview_setitem"); return; }
	if (columns >= 4 && !ListView_SetItem(window, &column4)) { error(L"listview_setitem"); return; }
	if (columns >= 5 && !ListView_SetItem(window, &column5)) { error(L"listview_setitem"); return; }
	if (columns >= 6 && !ListView_SetItem(window, &column6)) { error(L"listview_setitem"); return; }
}

// Takes a parameter, icons and text
// Edits the row in the list view control
void ListEdit(HWND window, int columns, LPARAM p, int icon1, read r1, int icon2, read r2, read r3, read r4, read r5, read r6) {

	// Record the maximum text length
	State.list.max = Greatest(State.list.max, length(r1), length(r2), length(r3), length(r4), length(r5), length(r6));

	// Find the row that has the given parameter
	int row = ListFind(window, p);

	// Row and column position
	LVITEM column1, column2, column3, column4, column5, column6;
	ZeroMemory(&column1, sizeof(column1));
	ZeroMemory(&column2, sizeof(column2));
	ZeroMemory(&column3, sizeof(column3));
	ZeroMemory(&column4, sizeof(column4));
	ZeroMemory(&column5, sizeof(column5));
	ZeroMemory(&column6, sizeof(column6));
	column1.iItem = column2.iItem = column3.iItem = column4.iItem = column5.iItem = column6.iItem = row;
	column1.iSubItem = 0;
	column2.iSubItem = 1;
	column3.iSubItem = 2;
	column4.iSubItem = 3;
	column5.iSubItem = 4;
	column6.iSubItem = 5;

	// Masks for text and icons
	column1.mask = column2.mask = LVIF_TEXT | LVIF_IMAGE;
	column3.mask = column4.mask = column5.mask = column6.mask = LVIF_TEXT;

	// Text
	column1.pszText = (LPWSTR)r1;
	column2.pszText = (LPWSTR)r2;
	column3.pszText = (LPWSTR)r3;
	column4.pszText = (LPWSTR)r4;
	column5.pszText = (LPWSTR)r5;
	column6.pszText = (LPWSTR)r6;

	// Icons
	column1.iImage = icon1;
	column2.iImage = icon2;

	// Set the columns that have different text
	if (columns >= 1 && !same(ListText(window, row, 0), r1) && !ListView_SetItem(window, &column1)) error(L"listview_setitem");
	if (columns >= 2 && !same(ListText(window, row, 1), r2) && !ListView_SetItem(window, &column2)) error(L"listview_setitem");
	if (columns >= 3 && !same(ListText(window, row, 2), r3) && !ListView_SetItem(window, &column3)) error(L"listview_setitem");
	if (columns >= 4 && !same(ListText(window, row, 3), r4) && !ListView_SetItem(window, &column4)) error(L"listview_setitem");
	if (columns >= 5 && !same(ListText(window, row, 4), r5) && !ListView_SetItem(window, &column5)) error(L"listview_setitem");
	if (columns >= 6 && !same(ListText(window, row, 5), r6) && !ListView_SetItem(window, &column6)) error(L"listview_setitem");
}

// Takes a parameter
// Finds the row that has it in the list view control
// Returns the row number
int ListFind(HWND window, LPARAM p) {

	// Find the row that has p as its parameter, this may be slow
	LVFINDINFO info;
	ZeroMemory(&info, sizeof(info));
	info.flags  = LVFI_PARAM; // Search based on the parameter
	info.lParam = p;          // Here is the parameter to find
	int row;
	row = ListView_FindItem(window, -1, &info); // -1 to start from the beginning
	if (row == -1) error(L"listview_finditem");

	// Return the row number
	return row;
}

// Takes row and column numbers
// Gets the text from the list view subitem at that location
// Returns the text
CString ListText(HWND window, int row, int column) {

	return L"";
	//TODO check this in unicode before you use it
	/*
	// Make the buffer hold one more character than the longest text in the list view control to correctly read very long text
	int characters = State.list.max + 1;

	// Open a string
	CString buffer_string;
	int     buffer_characters = characters;
	LPWSTR  buffer_write      = buffer_string.GetBuffer(buffer_characters + 1 +8); //TODO make it so you don't need SAFETY 8 anymore

	// Copy in the characters
	LVITEM info;
	ZeroMemory(&info, sizeof(info));
	info.iImage     = row;
	info.iSubItem   = column;
	info.pszText    = buffer_write;
	info.cchTextMax = characters;
	int written;
	written = (int)SendMessage(window, LVM_GETITEMTEXT, row, (LPARAM)&info);
	buffer_write[written] = '\0';

	// Close the string
	buffer_write[buffer_characters] = '\0'; //TODO check memory with wide, can you do this without literal chars?
	buffer_string.ReleaseBuffer();

	// Return the string
	return buffer_string;
	*/
}

// Takes a file extension like ".zip" which can be blank
// Gets the index of its icon in the system image list and the shell type text
// Returns true if successful and writes the system index and type, or false and -1 and blank
bool ShellInfo(read ext, int *systemindex, CString *type) {

	// Use the extension to get the system image list index and type text, Windows 9x returns blank for unknown extensions
	SHFILEINFO info;
	ZeroMemory(&info, sizeof(info));
	info.hIcon = NULL; // Start out with the icon handle null and index invalid
	info.iIcon = -1;
	HIMAGELIST systemlist = (HIMAGELIST)SHGetFileInfo(
		ext,                      // File name
		FILE_ATTRIBUTE_NORMAL,    // File attributes for fictional file
		&info,                    // Put information here
		sizeof(info),
		SHGFI_USEFILEATTRIBUTES | // Use the file name and attributes as a fictional file
		SHGFI_SMALLICON |         // Use the list of small icons and return the handle to the small icon system image list
		SHGFI_SYSICONINDEX |      // Get the icon index in the system image list
		SHGFI_TYPENAME);          // Get the file type text
	if (info.hIcon) error(L"shgetfileinfo returned an icon when it should not have");
	if (!systemlist || info.iIcon < 0) {

		error(L"shgetfileinfo had an error or returned an invalid index");
		*systemindex = -1;
		*type = L"";
		return false;
	}

	// Write information and report success
	*systemindex = info.iIcon;
	*type = info.szTypeName;
	return true;
}

// Takes a file extension like ".zip" which can be blank
// Gets the small icon for the file from the system image list
// Returns true if successful and writes the icon handle, or false and null
bool ShellIcon(read ext, HICON *icon) {

	// Use the extension to get the icon
	SHFILEINFO info;
	ZeroMemory(&info, sizeof(info));
	info.hIcon = NULL; // Start out with the icon handle null and index invalid
	info.iIcon = -1;
	int result = (int)SHGetFileInfo(
		ext,                      // File name
		FILE_ATTRIBUTE_NORMAL,    // File attributes for fictional file
		&info,                    // Put information here
		sizeof(info),
		SHGFI_USEFILEATTRIBUTES | // Use the file name and attirbutes as a fictional file
		SHGFI_SMALLICON |         // Use the list of small icons and return the handle to the small icon system image list
		SHGFI_ICON);              // Get the icon itself and its index from the system image list
	if (!result || !info.hIcon || info.iIcon < 0) {

		error(L"shgetfileinfo had an error, returned no icon, or returned an invalid index");
		DestroyIconSafely(info.hIcon); // Destroy an icon if one was created
		*icon = NULL;
		return false;
	}

	// Write information and report success
	*icon = info.hIcon;
	return true;
}

// Takes a handle to an icon, or null
// Destroys the icon
void DestroyIconSafely(HICON icon) {

	// If the icon exists, destroy it
	if (icon && !DestroyIcon(icon)) error(L"destroyicon");
}

// Generates a guid
// Returns a string with the 32 lowercase hexidecimal characters of the guid, or blank if any error
CString TextGuid() {

	// Get a new unique GUID from the system
	GUID guid;
	HRESULT result = CoCreateGuid(&guid);
	if (result != S_OK) { error(L"cocreateguid"); return L""; }

	// Convert the GUID into an OLE wide character string
	WCHAR bay[MAX_PATH];
	int characters = StringFromGUID2(
		guid,          // GUID to convert
		(LPOLESTR)bay, // Write text here
		MAX_PATH);     // Size of bay
	if (!characters) { error(L"stringfromguid2"); return L""; }

	// Convert the OLE wide character string into a text string
	COLE2T text(bay);
	CString s = text;

	s = lower(clip(s, 1, 8) + clip(s, 10, 4) + clip(s, 15, 4) + clip(s, 20, 4) + clip(s, 25, 12)); // Clip out the number parts of the GUID string and lowercase it
	if (length(s) != 32) { error(L"guid length not 32 characters"); return L""; }                  // Make sure the GUID string is 32 characters
	return s;                                                                                      // Return the string
}

// The path to the user's start menu programs folder
// Like "C:\Documents and Settings\Name\Start Menu\Programs" on XP or "C:\Users\Name\AppData\Roaming\Microsoft\Windows\Start Menu\Programs" on 7
CString PathStartPrograms() { return PathId(CSIDL_PROGRAMS); }

// The path to the user's quick launch folder
// Like "C:\Documents and Settings\Name\Application Data\Microsoft\Internet Explorer\Quick Launch" on XP or "C:\Users\Name\AppData\Roaming\Microsoft\Internet Explorer\Quick Launch" on 7
CString PathQuickLaunch() { return PathId(CSIDL_APPDATA) + L"\\Microsoft\\Internet Explorer\\Quick Launch"; } //TODO check this matches what you did in november

// The path to the user's desktop folder, like "C:\Documents and Settings\Name\Desktop" on XP or "C:\Users\Name\Desktop" on 7
CString PathDesktop() { return PathId(CSIDL_DESKTOPDIRECTORY); }

// The path to the user's documents folder, like "C:\Documents and Settings\Name\My Documents" on XP or "C:\Users\Name\Documents" on 7
CString PathDocuments() { return PathId(CSIDL_MYDOCUMENTS); }

// The path to the user's application data folder, like "C:\Documents and Settings\Name\Application Data" on XP or "C:\Users\Name\AppData\Roaming" on 7
CString PathApplicationData() { return PathId(CSIDL_APPDATA); }

// Get the path to the special folder with the given id, or blank on error
CString PathId(int id) {

	LPMALLOC memory;
	LPITEMIDLIST list;
	WCHAR bay[MAX_PATH]; // Text buffer to write in
	CString path;        // String to return with path or blank on error

	if (!SHGetMalloc(&memory)) {
		if (!SHGetSpecialFolderLocation(Handle.window, id, &list)) {
			if (SHGetPathFromIDList(list, bay)) {

				path = bay; // Everything worked, copy the bay into a string

			} else { error(L"shgetpathfromidlist"); }
			memory->Free(list);
		} else { error(L"shgetspecialfolderlocation"); }
		memory->Release();
	} else { error(L"shgetmalloc"); }

	return path; // Return the path we found, or blank on error
}

// The path to the folder this running exe is in, like "C:", "C:\folder", or "\\computer\share\folder", no trailing backslash, blank on error
CString PathRunningFolder() {

	return before(PathRunning(), L"\\", Reverse);
}

// The path to this running exe like "C:\file.exe", "C:\folder\file.exe", or "\\computer\share\folder\file.exe", no trailing backslash, blank on error
CString PathRunning() {

	WCHAR bay[MAX_PATH];
	if (!GetModuleFileName(NULL, bay, MAX_PATH)) { error(L"getmodulefilename"); return L""; }
	return bay;
}

// Path to port.db alongside this running exe, the portable marker
CString PathPortable() { return PathRunningFolder() + L"\\port.db"; }

// Path to stor.db alongside this running exe, holds libtorrent saved session state
CString PathStore() { return PathRunningFolder() + L"\\stor.db"; }
// Path to optn.db alongside this running exe, holds program options the user edits
CString PathOption() { return PathRunningFolder() + L"\\optn.db"; }

// Path to meta.infohash.db alongside this running exe, the program's copy of the torrent file
CString PathTorrentMeta(libtorrent::sha1_hash hash) { return PathRunningFolder() + L"\\meta." + HashToString(hash) + L".db"; }
// Path to stor.infohash.db alongside this running exe, holds resume data for the torrent
CString PathTorrentStore(libtorrent::sha1_hash hash) { return PathRunningFolder() + L"\\stor." + HashToString(hash) + L".db"; }
// Path to optn.infohash.db alongside this running exe, holds torrent options the user edits
CString PathTorrentOption(libtorrent::sha1_hash hash) { return PathRunningFolder() + L"\\optn." + HashToString(hash) + L".db"; }


//write a function that looks at meta.*.db in the running folder, and returns a list of hash objects of everything it finds there
//then, you can use the others to get the individual files










