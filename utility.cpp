
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Print an error message out on the output log
void error(read r1, read r2, read r3, read r4, read r5, read r6, read r7, read r8, read r9) {

	if (PROGRAM_TEST) log(make(L"error ", numerals(GetLastError()), L" ", make(r1, r2, r3, r4, r5, r6, r7, r8, r9)));
}

// Print an error message out on the output log
void error(int result, read r1, read r2, read r3, read r4, read r5, read r6, read r7, read r8, read r9) {

	if (PROGRAM_TEST) log(make(L"error ", numerals(GetLastError()), L" result ", numerals(result), L" ", make(r1, r2, r3, r4, r5, r6, r7, r8, r9)));
}

// Print a message out on the output log
void log(read r1, read r2, read r3, read r4, read r5, read r6, read r7, read r8, read r9) {

	if (!PROGRAM_TEST) return;
	CString s = make(SayNow(), L" ", make(r1, r2, r3, r4, r5, r6, r7, r8, r9), L"\r\n");
	OutputDebugString(s);
	//TODO add it to the log tab
}

// Show a message to the user
void report(read r) {

	if (PROGRAM_TEST) MessageBox(App.window.main, r, PROGRAM_NAME, MB_OK);
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

// Make a list view control window
HWND WindowCreateList() {

	// Create the list view window
	DWORD style =
		WS_CHILD             | // Required for child windows
		LVS_REPORT           | // Details view with multiple columns
		LBS_EXTENDEDSEL      | // Allows multiple items to be selected
		LVS_SHOWSELALWAYS    | // Shows the selection even when the control doesn't have the focus
		LBS_NOINTEGRALHEIGHT | // Allows the size to be specified exactly without snap
		LVS_SHAREIMAGELISTS;   // Will not delete the image list when the control is destroyed
	HWND window = WindowCreate(WC_LISTVIEW, NULL, style, 0, App.window.main, (HMENU)WINDOW_LIST);

	// Use the program image list
	ListView_SetImageList(window, App.icon.list, LVSIL_SMALL);

	// Load extended list view styles, requires common control 4.70
	style =
		LVS_EX_LABELTIP      | // Unfold partially hidden labels in tooltips
		LVS_EX_FULLROWSELECT | // When an item is selected, highlight it and all its subitems
		LVS_EX_SUBITEMIMAGES | // Let subitems have icons
		LVS_EX_HEADERDRAGDROP; // Let the user drag columns into a different order
	ListView_SetExtendedListViewStyle(window, style);
	return window;
}

// Make a tabstrip window
HWND WindowCreateTabs() {

	// Create the tabs window
	DWORD style = WS_CHILD;      // Required for child windows
	HWND window = WindowCreate(WC_TABCONTROL, NULL, style, 0, App.window.main, (HMENU)WINDOW_TABS);
	SendMessage(                 // Have it use Tahoma, not the default system font
		(HWND)window,            // Send the message to this window
		WM_SETFONT,              // Message to send
		(WPARAM)App.font.normal, // Handle to font
		0);                      // Don't tell the control to immediately redraw itself
	return window;
}

// Make a tooltip window
HWND WindowCreateTip() {

	// Create the tooltip window
	DWORD style =
		WS_POPUP |     // Popup window instead of a child window
		TTS_ALWAYSTIP; // Show the tooltip even if the window is inactive
	HWND window = WindowCreate(TOOLTIPS_CLASS, NULL, style, 0, App.window.main, NULL);

	// Make the tooltip topmost
	int result = SetWindowPos(
		window,          // Handle to window
		HWND_TOPMOST,    // Place the window above others without this setting
		0, 0, 0, 0,      // Retain the current position and size
		SWP_NOMOVE |
		SWP_NOSIZE |
		SWP_NOACTIVATE); // Do not activate the window
	if (!result) error(L"setwindowpos");
	return window;
}

// Make a text edit window
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
	HWND window = WindowCreate(L"EDIT", NULL, style, 0, App.window.main, NULL);

	// Have it use Tahoma, not the default system font
	SendMessage( 
		(HWND)window,            // Send the message to this window
		WM_SETFONT,              // Message to send
		(WPARAM)App.font.normal, // Handle to font
		0);                      // Don't tell the control to immediately redraw itself

	// Expand its text capacity
	if (capacity) SendMessage(window, EM_LIMITTEXT, 0, 0);

	// Return the handle to the edit window we made
	return window;
}

// Make a button
HWND WindowCreateButton(read r) {

	// Prepare the style of the button window
	DWORD style =
		WS_CHILD |     // Required for child windows
		BS_PUSHBUTTON; // Have the button send the window a message when the user clicks it

	// Create the edit window
	HWND window = WindowCreate(L"BUTTON", NULL, style, 0, App.window.main, NULL);

	// Title it
	TextWindowSet(window, r);

	// Have it use Tahoma, not the default system font
	SendMessage( 
		(HWND)window,            // Send the message to this window
		WM_SETFONT,              // Message to send
		(WPARAM)App.font.normal, // Handle to font
		0);                      // Don't tell the control to immediately redraw itself

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
		App.window.instance,    // Program instance handle
		NULL);                  // No parameter
	if (!window) error(L"createwindow");
	return window;
}

// Adjust the width and height of the given window by x and y
void WindowSize(HWND window, int x, int y) {

	// No sizing necessary
	if (!x && !y) return;

	// Get the window's current size
	RECT r;
	if (!GetWindowRect(window, &r)) { error(L"getwindowrect"); return; }

	// Adjust the size
	Size size;
	size.x = r.left;
	size.y = r.top;
	size.w = r.right - r.left + x;
	size.h = r.bottom - r.top + y;

	// Move the window
	WindowMove(window, size, true);
}

// Move window to size, true to repaint
void WindowMove(HWND window, Size size, bool paint) {

	// Position and resize the window and also optionally send it a paint message 
	if (!MoveWindow(window, size.x, size.y, size.w, size.h, paint)) error(L"movewindow");
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

// Make a Brush from the given color
// Returns a Brush that must be deleted, or null on error
Brush CreateBrush(COLORREF color) {

	Brush brush;
	brush.color = color;
	brush.brush = CreateSolidBrush(color);
	if (!brush.brush) error(L"createsolidbrush");
	return brush;
}

// Repaint the window right now
void PaintMessage(HWND window) {

	// Choose window
	if (!window) window = App.window.main;

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

	if (App.cycle.taskbar) return;              // Icon already there, leave
	App.cycle.taskbar = App.stage.show->icon16; // Pick the icon for the current stage

	// Add the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize           = sizeof(info);                     // Size of this structure
	info.hWnd             = App.window.main;                  // Handle to the window that will receive messages
	info.uID              = TASKBAR_ICON;                     // Program defined identifier
	info.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP; // Mask for message, icon and tip
	info.uCallbackMessage = MESSAGE_TASKBAR;                  // Program defined message identifier
	info.hIcon            = App.cycle.taskbar;                // Icon handle
	lstrcpy(info.szTip, PROGRAM_NAME);                        // 64 character buffer for tooltip text
	if (!Shell_NotifyIcon(NIM_ADD, &info)) error(L"shell_notifyicon nim_add");
}

// Updates the program icon in the taskbar notification area
void TaskbarIconUpdate() {

	if (!App.cycle.taskbar) return;                          // No icon to update, leave
	if (App.cycle.taskbar == App.stage.show->icon16) return; // Icon doesn't need to be updated, leave
	App.cycle.taskbar = App.stage.show->icon16;              // Record that we updated the icon

	// Add the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize           = sizeof(info);      // Size of this structure
	info.hWnd             = App.window.main;   // Handle to the window that will receive messages
	info.uID              = TASKBAR_ICON;      // Program defined identifier
	info.uFlags           = NIF_ICON;          // Mask for icon only
	info.hIcon            = App.cycle.taskbar; // Icon handle
	if (!Shell_NotifyIcon(NIM_MODIFY, &info)) error(L"shell_notifyicon nim_modify");
}

// Removes the program icon from the taskbar notification area
void TaskbarIconRemove() {

	if (!App.cycle.taskbar) return; // No icon to remove, leave
	App.cycle.taskbar = NULL;       // Record tha we removed the icon

	// Remove the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);    // Size of this structure
	info.hWnd   = App.window.main; // Handle to the window that will receive messages
	info.uID    = TASKBAR_ICON;    // Program defined identifier
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
		App.window.instance, // Load from this instance
		name,                // Resource name
		IMAGE_ICON,          // Image type
		w, h,                // Width and height to load
		LR_DEFAULTCOLOR);    // Default flag does nothing
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

	HMENU menus = LoadMenu(App.window.instance, name); // Load the menu resource
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
// Takes size null to put the menu at the mouse pointer, or a size in client coordinates
// Displays the context menu and waits for the user to make a choice
// Returns the menu item identifier of the choice, or 0 if the user cancelled the menu or any error
UINT MenuShow(HMENU menu, bool taskbar, Size *size) {

	// Use the given size or mouse position
	Size position;
	if (size) {
		position = *size;
		position.Screen(); // Convert the given client size into screen coordinates
	} else {
		position = MouseScreen(); // Get the mouse position in screen coordinates
	}

	// Context menu for notification icon
	if (taskbar) SetForegroundWindow(App.window.main);

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
		App.window.main,
		NULL);
	AreaPopDown();

	// Context menu for notification icon
	if (taskbar) PostMessage(App.window.main, WM_NULL, 0, 0);

	// Return the user's choice, 0 they clicked outside the menu
	return choice;
}

// Takes the size in the client area where the tool will be shown, and the text to show
// Assigns the tooltip window to this area and sets its text
void TipAdd(Size size, read r) {

	// Attach the tooltip to a rectangle in the main window
	TOOLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize      = sizeof(info);     // Size of this structure
	info.uFlags      = TTF_SUBCLASS;     // Have the tooltip control get messages from the tool window
	info.hwnd        = App.window.main;  // Handle to the window that contains the tool region
	info.uId         = 0;                // Tool identifying number
	info.rect        = size.Rectangle(); // Rectangle in the window of the tool
	info.hinst       = NULL;             // Only used when text is loaded from a resource
	info.lpszText    = (LPWSTR)r;        // Text
	info.lParam      = 0;                // No additional value assigned to tool
	if (!SendMessage(App.window.tip, TTM_ADDTOOL, 0, (LPARAM)&info)) error(L"sendmessage ttm_addtool");
}

// Have window capture the mouse if it doesn't have it already, null to use the main window
void MouseCapture(HWND window) {

	if (!window) window = App.window.main; // Use the main window if the caller didn't specify one
	if (GetCapture() != window) SetCapture(window); // If that window doesn't already have the mouse, capture it
}

// Have window release its capture of the mouse if it has it, null to use the main window
void MouseRelease(HWND window) {

	if (!window) window = App.window.main; // Pick the main window if the caller didn't specify one
	if (GetCapture() == window) ReleaseCapture(); // Only release it if window has it
}

// True if the mouse is in the client area but not over a child window control
bool MouseInside() {

	// Get the mouse cursor's position in client window coordinates, and the size of the client window
	Size mouse = MouseClient();
	Size client = SizeClient();

	// The mouse is inside if it is inside the client area and outside all the child window controls
	return
		client.Inside(mouse)         &&
		!App.area.list.Inside(mouse) &&
		!App.area.tabs.Inside(mouse) &&
		!App.area.info.Inside(mouse);
}

// Find the area, if any, the mouse is currently positioned over, null if none
Area *MouseOver() {

	// Get the mouse cursor's position in client window coordinates
	Size mouse = MouseClient();
	Size client = SizeClient();
	if (!client.Inside(mouse)) return NULL; // Make sure the mouse is inside the client area

	// Move down each area to find the one the mouse is over
	Area *a = App.area.all;
	while (a) {
		if (a->size.Inside(mouse)) return a; // Found it
		a = a->next;
	}
	return NULL; // Not found, the mouse is away from them all
}

// Get the mouse position in x and y coordinates inside the given area
Size MouseArea(Area *a) {

	Size s = MouseClient(); // Get the mouse position in client window coordinates
	s.x -= a->size.x; // Convert to area coordinates
	s.y -= a->size.y;
	return s;
}

// Takes a window or null to use the main one
// Gets the mouse position in client coordinates
// Returns x and y coordinates in a size
Size MouseClient(HWND window) {

	if (!window) window = App.window.main; // Choose window
	Size s = MouseScreen(); // Get the mouse pointer position in screen coordinates
	s.Client(window); // Convert the position to the client coordinates of the given window
	return s;
}

// Gets the mouse position in screen coordinates
// Returns x and y coordinates in a size
Size MouseScreen() {

	// If we have a popup window or menu open, report that the mouse is off the screen
	Size s;
	s.x = -1;
	s.y = -1;
	if (App.cycle.pop) return s;

	// Get the mouse position in screen coordinates
	POINT p;
	if (!GetCursorPos(&p)) return s; // Will return error if Windows is locked
	s.Set(p);
	return s;
}

// Takes a system color index
// Gets the system brush for that color
// Returns a brush that should not be deleted, or null if any error
Brush BrushSystem(int color) {

	// Get the system brush for the given color index, as well as the color itself
	Brush brush;
	brush.color = GetSysColor(color);
	brush.brush = GetSysColorBrush(color);
	if (!brush.brush) error(L"getsyscolorbrush");
	return brush;
}

// Takes a color
// Creates a brush of that color
// Returns a brush that must be deleted, or null if any error
Brush BrushColor(COLORREF color) {

	// Create a new brush of the given solid color
	Brush brush;
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
Size SizeClient(HWND window) {

	// Pick main window if none given
	if (!window) window = App.window.main;

	// Find the width and height of the client area
	Size size;
	RECT rectangle;
	if (!GetClientRect(window, &rectangle)) { error(L"getclientrect"); return size; }
	size.Set(rectangle);
	return size;
}

// Find the size of the given window on the screen
Size SizeWindow(HWND window) {

	// Pick main window if none given
	if (!window) window = App.window.main;

	// Find the width and height of the client area
	Size size;
	RECT rectangle;
	if (!GetWindowRect(window, &rectangle)) { error(L"getwindowrect"); return size; }
	size.Set(rectangle);
	return size;
}

// Takes a device context with a font loaded inside, and text
// Determines how wide and high in pixels the text painted will be
// Returns the size width and height, or zeroes if any error
Size SizeText(Device *device, read r) {

	// Get the pixel dimensions of text written in the loaded font
	SIZE size;
	if (!GetTextExtentPoint32(device->device, r, length(r), &size)) {

		error(L"gettextextentpoint32");
		size.cx = size.cy = 0;
	}

	// Return the size, will be all 0 for blank text
	Size s(size);
	return s;
}

// Takes a Device that has a font, text, and a bounding position and size
// Paints the text there
void PaintLabel(Device *device, read r, Size size) {

	// Paint the text, if the background is opaque, this will cause a flicker
	RECT rectangle = size.Rectangle();
	if (!DrawText(device->device, r, -1, &rectangle, DT_NOPREFIX)) error(L"drawtext");
}

// Takes a device context that has a font loaded into it, text, position and bounding size, and formatting options
// Fills the size and paints the text with an ellipsis
void PaintText(Device *device, read r, Size size, bool horizontal, bool vertical, bool left, bool right, int adjust, HFONT font, Brush *color, Brush *background) {

	// Prepare the device context
	if (font)       device->Font(font);
	if (color)      device->FontColor(color->color);
	if (background) device->BackgroundColor(background->color);

	// Find out how big the text will be when painted
	Size text;
	text = SizeText(device, r);

	// If only a position was provided, put in the necessary size
	if (size.w <= 0) size.w = text.w;
	if (size.h <= 0) size.h = text.h;

	// Define space once in a local variable
	int space = 4;

	// Add margins
	Size bound = size;
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
	if (font)       device->Font(App.font.normal);
}

// Use the device to paint size with brush
void PaintFill(Device *device, Size size, HBRUSH brush) {

	// Make sure there is a size to fill
	if (!size.Is()) return;

	// Choose brush
	if (!brush) brush = App.brush.background.brush;

	// Paint the rectangle
	RECT rectangle = size.Rectangle();
	FillRect(device->device, &rectangle, brush); // Will return error if Windows is locked
}

// Takes a device context, size, and brushes for the upper left and lower right corners
// Paints a 1 pixel wide border inside the size
void PaintBorder(Device *device, Size size, HBRUSH brush1, HBRUSH brush2) {

	// Use the same brush for both corners
	if (!brush2) brush2 = brush1;

	// Paint the 4 edges of the border
	Size edge;
	edge = size;                       edge.w--;                         edge.h = 1;  PaintFill(device, edge, brush1);
	edge = size;                       edge.w = 1; edge.y++;             edge.h -= 2; PaintFill(device, edge, brush1);
	edge = size; edge.x += edge.w - 1; edge.w = 1;                       edge.h--;    PaintFill(device, edge, brush2);
	edge = size;                                   edge.y += edge.h - 1; edge.h = 1;  PaintFill(device, edge, brush2);
}

// Takes a device context, position, and icon
// Provide a background brush for flicker free drawing, or NULL for a transparent background
// Paints the icon
void PaintIcon(Device *device, Size position, HICON icon, HBRUSH background) {

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

// Return the largest positive number amongst the given numbers, 0 if none
int Greatest(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8) {

	// Find the biggest positive number and return it
	int i = 0;
	if (i < i1) i = i1;
	if (i < i2) i = i2;
	if (i < i3) i = i3;
	if (i < i4) i = i4;
	if (i < i5) i = i5;
	if (i < i6) i = i6;
	if (i < i7) i = i7;
	if (i < i8) i = i8;
	return i;
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

	if (!window) window = App.window.main; // Choose window
	if (!KillTimer(window, timer)) error(L"killtimer"); // Kill the timer
}

// Takes a timer identifier and the number of milliseconds after which the timer should expire
// Sets the timer
void TimerSet(UINT_PTR timer, UINT time, HWND window) {

	if (!window) window = App.window.main; // Choose window
	if (!SetTimer(window, timer, time, NULL)) error(L"settimer"); // Set the timer
}

// Takes a path to a file and parameters text or blank
// Shell executes it
void FileRun(read path, read parameters) {

	// Shell execute the given text
	ShellExecute(
		App.window.main, // Handle to a window to get message boxes from this operation
		NULL,            // Default run
		path,            // File to run
		parameters,      // Parameters ot pass to program
		L"C:\\",         // Starting directory
		SW_SHOWNORMAL);  // Default show
}


// Takes a dialog box resource name and procedure function
// Shows the dialog box
// Returns the result from the dialog box
int Dialog(read resource, DLGPROC procedure, LPARAM lparam) {

	// Choose procedure
	if (!procedure) procedure = DialogProcedure;

	// Show the dialog box, sending messages to its procedure and returning here when it is closed
	int result;
	AreaPopUp();
	if (lparam) result = (int)DialogBoxParam(App.window.instance, resource, App.window.main, procedure, lparam);
    else result = (int)DialogBox(App.window.instance, resource, App.window.main, procedure);
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

		// The user clicked OK, Cancel, Yes, or No
		switch (LOWORD(wparam)) {
		case IDOK:
		case IDCANCEL: // Cancel or the corner X
		case IDYES:
		case IDNO:

			// Close the dialog
			EndDialog(dialog, LOWORD(wparam)); // Have DialogBox() return what button the user clicked
			return true; // We processed the message

		break;
		}

	break;
	}

	return false; // We didn't process the message
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
	if (icon == -1) icon = App.icon.file; // If the icon could not be found or added in the program list, use the index of the default shell file icon in the list
	return icon;
}

// Takes a file extension like ".zip" which can be blank
// Gets shell information about the extension and its index in the program image list
// Returns the program image list icon index and writes the type text, or -1 and blanks if any error
int IconGet(read ext, CString *type) {

	// If the requested extension is the last one loaded, fill the request quickly
	if (same(ext, App.icon.ext, Matching)) {

		*type = App.icon.type;
		return App.icon.index;
	}

	// Get the system index and type text for the extension
	int systemindex;
	CString typetext;
	if (!ShellInfo(ext, &systemindex, &typetext)) { *type = L""; return -1; }

	// Find the system index in the program image list or make program index -1
	int programindex;
	for (programindex = App.icon.count - 1; programindex >= 0; programindex--) {

		if (App.icon.source[programindex] == systemindex) break;
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
	App.icon.ext   = ext;
	App.icon.type  = *type;
	App.icon.index = programindex;

	// Return the index of the icon in the program image list
	return programindex;
}

// Takes the text resource name of an icon, like "ICON_NAME"
// Loads the icon into the program image list
// Returns the index of the icon in the program image list, or -1 if any error
int IconAddResource(read resource) {

	// Load the icon from the resource
	HICON icon = (HICON)LoadImage(
		App.window.instance, // Handle to instance
		resource,            // Resource name text
		IMAGE_ICON,          // This resource is an icon
		16, 16,              // Size
		LR_DEFAULTCOLOR);    // No special options
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
	if (App.icon.count >= ICON_CAPACITY) return -1;

	// Add the icon to the end of the program image list
	int programindex = ImageList_ReplaceIcon(App.icon.list, -1, icon);
	if (programindex == -1) { error(L"imagelist_replaceicon"); return -1; }

	// Write the index in the array, up the count, and return the program index of the added icon
	App.icon.source[App.icon.count] = systemindex;
	App.icon.count++;
	return programindex;
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

// Make a shortcut at path that runs target
bool FileLink(read path, read target, read description) {

	OleInitialize(NULL); // Use OLE
	IShellLink *i1;
	HRESULT result = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&i1);
	if (SUCCEEDED(result)) {

		i1->SetPath(target);
		i1->SetDescription(description);
		IPersistFile *i2;
		result = i1->QueryInterface(IID_IPersistFile, (LPVOID *)&i2);

		if (SUCCEEDED(result)) {

			result = i2->Save(path, true);
			i2->Release();
		}
		i1->Release();
	}
	return SUCCEEDED(result);
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
	lstrcpy(bay, L"");
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

/*
Paths next to this running exe

PathTorrentIcon    "torrent.ico"       The torrent icon

PathPortable       "port.db"           The portable marker
PathStore          "stor.db"           libtorrent session state
PathOption         "optn.db"           Program options the user edits

PathTorrentMeta    "infohash.meta.db"  Copy of the torrent file
PathTorrentStore   "infohash.stor.db"  libtorrent resume data for the torrent
PathTorrentOption  "infohash.optn.db"  Torrent options the user edits
*/
CString PathTorrentIcon() { return PathRunningFolder() + L"\\torrent.ico"; }

CString PathPortable() { return PathRunningFolder() + L"\\port.db"; }
CString PathStore()    { return PathRunningFolder() + L"\\stor.db"; }
CString PathOption()   { return PathRunningFolder() + L"\\optn.db"; }

CString PathTorrentMeta(hbig hash)   { return PathRunningFolder() + L"\\" + base16(hash) + L".meta.db"; }
CString PathTorrentStore(hbig hash)  { return PathRunningFolder() + L"\\" + base16(hash) + L".stor.db"; }
CString PathTorrentOption(hbig hash) { return PathRunningFolder() + L"\\" + base16(hash) + L".optn.db"; }

/*
Shell paths

PathDocuments        "C:\Documents and Settings\user\My Documents"                                                        "C:\Users\user\Documents"
PathTorrents         "C:\Documents and Settings\user\My Documents\Torrents"                                               "C:\Users\user\Documents\Torrents"

PathApplicationData  "C:\Documents and Settings\user\Application Data"                                                    "C:\Users\user\AppData\Roaming"
PathFolder           "C:\Documents and Settings\user\Application Data\name"                                               "C:\Users\user\AppData\Roaming\name"
PathLaunch           "C:\Documents and Settings\user\Application Data\name\name.exe"                                      "C:\Users\user\AppData\Roaming\name.exe"

PathLinkDesktop      "C:\Documents and Settings\user\Desktop\name.lnk"                                                    "C:\Users\user\Desktop\name.lnk"
PathLinkStart        "C:\Documents and Settings\user\Start Menu\Programs\name.lnk"                                        "C:\Users\user\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\name.lnk"
PathLinkQuick        "C:\Documents and Settings\user\Application Data\Microsoft\Internet Explorer\Quick Launch\name.lnk"  "C:\Users\user\AppData\Roaming\Microsoft\Internet Explorer\Quick Launch\name.lnk"
*/
CString PathDocuments()       { return PathShell(CSIDL_MYDOCUMENTS); }
CString PathTorrents()        { return PathShell(CSIDL_MYDOCUMENTS) + L"\\Torrents"; }

CString PathApplicationData() { return PathShell(CSIDL_APPDATA); }
CString PathFolder()          { return PathShell(CSIDL_APPDATA)          + L"\\" + PROGRAM_NAME; }
CString PathLaunch()          { return PathShell(CSIDL_APPDATA)          + L"\\" + PROGRAM_NAME + L"\\" + PROGRAM_NAME + L".exe"; }

CString PathLinkDesktop()     { return PathShell(CSIDL_DESKTOPDIRECTORY) + L"\\" + PROGRAM_NAME + L".lnk"; }
CString PathLinkStart()       { return PathShell(CSIDL_PROGRAMS)         + L"\\" + PROGRAM_NAME + L".lnk"; }
CString PathLinkQuick()       { return PathShell(CSIDL_APPDATA) + L"\\Microsoft\\Internet Explorer\\Quick Launch\\" + PROGRAM_NAME + L".lnk"; }

// The path to the folder this running exe is in, like "C:", "C:\folder", or "\\computer\share\folder", no trailing backslash, blank on error
CString PathRunningFolder() {

	return before(PathRunningFile(), L"\\", Reverse);
}

// The path to this running exe file like "C:\file.exe", "C:\folder\file.exe", or "\\computer\share\folder\file.exe", no trailing backslash, blank on error
CString PathRunningFile() {

	WCHAR bay[MAX_PATH];
	lstrcpy(bay, L"");
	if (!GetModuleFileName(NULL, bay, MAX_PATH)) { error(L"getmodulefilename"); return L""; }
	return bay;
}

// Get the path to the special folder with the given id, or blank on error
CString PathShell(int id) {

	LPMALLOC memory;
	LPITEMIDLIST list;
	WCHAR bay[MAX_PATH]; // Text buffer to write in
	lstrcpy(bay, L"");
	CString path;        // String to return with path or blank on error

	if (!SHGetMalloc(&memory)) {
		if (!SHGetSpecialFolderLocation(App.window.main, id, &list)) {
			if (SHGetPathFromIDList(list, bay)) {

				path = bay; // Everything worked, copy the bay into a string

			} else { error(L"shgetpathfromidlist"); }
			memory->Free(list);
		} else { error(L"shgetspecialfolderlocation"); }
		memory->Release();
	} else { error(L"shgetmalloc"); }

	return path; // Return the path we found, or blank on error
}

// Takes a text message to show the user in the dialog box
// Displays the browse for folder dialog box
// Returns the path the user chose, or blank if cancel or error
CString DialogBrowse(read message) {

	// Show the box
	OleInitialize(NULL); // If not already
	WCHAR name[MAX_PATH];
	lstrcpy(name, L"");
	BROWSEINFO info;
	ZeroMemory(&info, sizeof(info));
	info.hwndOwner      = App.window.main;          // Parent window for the dialog
	info.pidlRoot       = NULL;                     // Browse from the desktop
	info.pszDisplayName = name;                     // Destination buffer
	info.lpszTitle      = message;                  // Text to show the user in the box
	info.ulFlags        = BIF_RETURNONLYFSDIRS |    // Only allow file system folders
	                      BIF_NEWDIALOGSTYLE;       // Use the new dialog that sizes makes folders
	info.lpfn           = NULL;                     // No callback function
	info.lParam         = 0;                        // No lparam
	info.iImage         = 0;                        // Filled with the icon index of the selected item
	LPITEMIDLIST result = SHBrowseForFolder(&info); // May be really slow in Visual Studio, but double clicking the exe is fine
	if (!result) return L""; // The user cancelled the box

	// Get the path of the folder the user chose
	WCHAR bay[MAX_PATH];
	lstrcpy(bay, L"");
	SHGetPathFromIDList(result, bay);
	return bay;
}

// Show the user the file open dialog to get them to pick a torrent file
// Returns the path to the torrent file, or blank on cancel
CString DialogOpen() {

	// Filter and extension text for torrents
	read filter = L"Torrents (*.torrent)\0*.torrent\0\0"; // List ends with two wide terminators
	read extension = L"torrent";
	WCHAR path[MAX_PATH];
	lstrcpy(path, L""); // Blank the buffer because the system reads initalization information there

	// Show the user the dialog box
	OPENFILENAME info;
	ZeroMemory(&info, sizeof(info));
	info.lStructSize = sizeof(info);    // Size of this structure to tell it what version we're using
	info.hwndOwner   = App.window.main; // Main window handle to show the box above it
	info.hInstance   = NULL;            // Not using a template handle
	info.lpstrFilter = filter;          // File type text pairs terminated by two wide nulls
	info.lpstrFile   = path;            // Destination buffer and size in characters
	info.nMaxFile    = MAX_PATH;
	info.Flags       =
		OFN_DONTADDTORECENT |           // Don't add this document to the system's recently opened documents menu
		OFN_FILEMUSTEXIST   |           // Only let the user open files that exist, not type a new path
		OFN_PATHMUSTEXIST   |
		OFN_HIDEREADONLY;               // Hide the read only checkbox
	info.lpstrDefExt = extension;       // File extension to add if the user doesn't type one
	int result = GetOpenFileName(&info);
	if (!result) return L""; // The user canceled the box
	return path;
}

// Takes a suggested file name, like "Suggested Name" without the extension
// Shows the user the file save dialog to get them to save a torrent file
// Returns the path to save the torrent file with the extension, or blank on cancel
CString DialogSave(read suggest) {

	// Filter and extension text for torrents
	read filter = L"Torrents (*.torrent)\0*.torrent\0\0"; // List ends with two wide terminators
	read extension = L"torrent";
	WCHAR path[MAX_PATH];
	lstrcpy(path, suggest);

	// Show the user the dialog box
	OPENFILENAME info;
	ZeroMemory(&info, sizeof(info));
	info.lStructSize = sizeof(info);        // Size of this structure to tell it what version we're using
	info.hwndOwner   = App.window.main;     // Main window handle to show the box above it
	info.lpstrFilter = filter;              // File type text pairs terminated by two wide nulls
	info.lpstrFile   = path;                // Destination buffer and size in characters
	info.nMaxFile    = MAX_PATH;
	info.Flags       = OFN_OVERWRITEPROMPT; // Warn the user if there's already a file at the path they pick
	info.lpstrDefExt = extension;           // File extension to add if the user doesn't type one
	int result = GetSaveFileName(&info);
	if (!result) return L""; // The user canceled the box
	return path;
}

// True if path is to a folder, drive root, or network share
// Set create to true to make folders if necessary
// Set write to true to also confirm we can write to it
bool DiskFolder(read path, bool create, bool write) {

	// A trailing backslash is not allowed, give this "C:", not "C:\"
	if (trails(path, L"\\")) return false;

	// Strings to split
	CString s = path;
	CString b, build;

	// Drive path, like "C:" or "C:\folder\subfolder"
	if (clip(s, 1, 1) == L":") {

		// Loop making build like "C:", "C:\folder", "C:\folder\subfolder"
		while (is(s)) {
			split(s, L"\\", &b, &s);
			if (is(build)) build += L"\\";
			build += b;
			if (!DiskFolderCheck(build, create)) return false; // Make it a folder
		}

	// Network path, like "\\computer\share" or "\\computer\share\folder\subfolder"
	} else if (starts(s, L"\\\\")) {

		// Make build like "\\computer\share"
		s = clip(s, 2);
		CString computer, share, build;
		split(s, L"\\", &computer, &s);
		split(s, L"\\", &share, &s);
		build = L"\\\\" + computer + L"\\" + share;
		if (!DiskFolderCheck(build, create)) return false; // Make it a folder

		// Loop making build like "\\computer\share\folder", "\\computer\share\folder\subfolder"
		while (is(s)) {
			split(s, L"\\", &b, &s);
			build += L"\\" + b;
			if (!DiskFolderCheck(build, create)) return false; // Make it a folder
		}

	// Invalid path
	} else {
		return false;
	}

	// Also confirm writing there works
	if (write) {
		CString tick = make(path, L"\\", numerals(GetTickCount())); // Try making and deleting a subfolder
		if (!DiskMakeFolder(tick)) return false;
		if (!DiskDeleteFolder(tick)) return false;
	}

	// Everything worked
	return true;
}

// True if path is to a folder on the disk
// Also returns true for drive roots and network shares like "C:" and "\\computer\share"
bool DiskIsFolder(read path) {

	// Only return true if we can get the file attributes and they include the directory flag
	DWORD d = GetFileAttributes(path);
	return d != INVALID_FILE_ATTRIBUTES && (d & FILE_ATTRIBUTE_DIRECTORY);
}

// True if path is to a file on the disk
bool DiskIsFile(read path) {

	// Only return true if we can get the file attributes and they don't include the directory flag
	DWORD d = GetFileAttributes(path);
	return d != INVALID_FILE_ATTRIBUTES && !(d & FILE_ATTRIBUTE_DIRECTORY);
}

// Look for a folder at path, set create true to try to make one there if necessary
// True if there's a folder at path
bool DiskFolderCheck(read path, bool create) {

	// Check or make one folder
	if (DiskIsFolder(path)) return true; // Already a folder, return true
	if (create) return DiskMakeFolder(path); // Try to make the folder, return false on error
	else return false; // Can't make the folder and no folder there
}

// Make a new folder at path, or confirm one is already there
bool DiskMakeFolder(read path) {

	// See or make
	if (DiskIsFolder(path)) return true; // Already there
	return CreateDirectory(path, NULL) != 0; // NULL to use default security attributes
}

// Delete the empty folder at path
bool DiskDeleteFolder(read path) {

	// Remove a read-only attribute that would prevent delete from working
	SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);

	// Delete the folder and check the result
	int result = RemoveDirectory(path);
	DWORD error = GetLastError();
	return result != 0 || error == ERROR_FILE_NOT_FOUND;
}

// Takes a root key handle name, a key path, and a registry variable name or blank for default
// Gets the information from the registry
// Returns true if it works, and writes the value
bool RegistryRead(HKEY root, read path, read name, CString *value) {

	// Open the key
	Registry r;
	if (!r.Open(root, path, false)) return false;

	// Get the size required
	DWORD size;
	int result = RegQueryValueEx(
		r.key,  // Handle to an open key
		name,   // Name of the value to read
		0,
		NULL,
		NULL,   // No data buffer, we're requesting the size
		&size); // Required size in bytes including the null terminator
	if (result != ERROR_SUCCESS) { error(result, L"regqueryvalueex text size"); return false; }

	// Open a string
	CString s;
	LPWSTR buffer = s.GetBuffer(size / sizeof(WCHAR)); // How many characters we'll write, including the null terminator

	// Read the binary data
	result = RegQueryValueEx(
		r.key,          // Handle to an open key
		name,           // Name of the value to read
		0,
		NULL,
		(LPBYTE)buffer, // Data buffer, writes the null terminator
		&size);         // Size of data buffer in bytes
	s.ReleaseBuffer();
	if (result != ERROR_SUCCESS) { error(result, L"regqueryvalueex text"); return false; }

	// Write the string
	*value = s;
	return true;
}

// Takes a root key handle name, a key path, a registry variable name or blank for default, and value text
// Stores the information in the registry
// Returns false on error
bool RegistryWrite(HKEY root, read path, read name, read value) {

	// Open the key
	Registry r;
	if (!r.Open(root, path, true)) return false;

	// Set or make and set the text value
	int result = RegSetValueEx(
		r.key,                                 // Handle to an open key
		name,                                  // Name of the value to set or make and set
		0,
		REG_SZ,                                // Variable type is a null-terminated string
		(const BYTE *)value,                   // Address of the value data to load
		(lstrlen(value) + 1) * sizeof(WCHAR)); // Size of the value data in bytes, add 1 to write the null terminator
	if (result != ERROR_SUCCESS) { error(result, L"regsetvalueex text"); return false; }
	return true;
}

// Takes a root key handle name or open base key, and the path to a key beneath it
// Deletes the key from the registry, including its subkeys
// Returns false on error
bool RegistryDelete(HKEY base, read path) {

	// Open the key
	Registry r;
	if (!r.Open(base, path, true)) return false;

	// Loop for each subkey, deleting them all
	DWORD size;
	WCHAR subkey[MAX_PATH];
	lstrcpy(subkey, L"");
	int result;
	while (true) {

		// Get the name of the first subkey
		size = MAX_PATH;
		result = RegEnumKeyEx(r.key, 0, subkey, &size, NULL, NULL, NULL, NULL);
		if (result == ERROR_NO_MORE_ITEMS) break; // There are no subkeys
		else if (result != ERROR_SUCCESS) { error(result, L"regenumkeyex"); return false; } // RegEnumKeyEx returned an error

		// Delete it, making the next subkey the new first one
		if (!RegistryDelete(r.key, subkey)) return false;
	}

	// We've cleared this key of subkeys, close it and delete it
	r.Close();
	result = RegDeleteKey(base, path);
	if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) { error(result, L"regdeletekey"); return false; }
	return true;
}

// Takes a root key handle name, a key path, and true to make keys and get write access
// Opens or creates and opens the key and returns true
bool Registry::Open(HKEY root, read path, bool write) {

	// Make sure we were given a key and path
	if (!root || isblank(path)) return false;

	// Variables for opening the key
	HKEY k;
	DWORD info;
	int result;

	// If the caller wants write access, create the key if it isn't there
	if (write) {

		// Open or create and open the key
		result = RegCreateKeyEx(
			root,                    // Handle to open root key
			path,                    // Subkey name
			0,
			L"",
			REG_OPTION_NON_VOLATILE, // Save information in the registry file
			KEY_ALL_ACCESS,          // Get access to read and write values in the key we're making and opening
			NULL,
			&k,                      // The opened or created key handle is put here
			&info);                  // Tells if the key was opened or created and opened
		if (result != ERROR_SUCCESS) { error(result, L"regcreatekeyex"); return false; }

	// If the caller only wants read access, don't create the key when trying to open it
	} else {

		// Open the key
		result = RegOpenKeyEx(
			root,     // Handle to open root key
			path,     // Subkey name
			0,
			KEY_READ, // We only need to read the key we're opening
			&k);      // The opened key handle is put here
		if (result != ERROR_SUCCESS) { error(result, L"regopenkeyex"); return false; }
	}

	// Save the open key in this object
	key = k;
	return true;
}

// Takes a path like "C:\Folder\Program.exe" and a name like "Program Name"
// Adds the program's listing in Windows Firewall to make sure it is listed and checked
// Returns false on error
bool FirewallAdd(read path, read name) {

	// Make a firewall and have it access the COM interfaces of Windows Firewall
	Firewall f;
	if (!f.Access()) return false;

	// Add the program's listing
	if (!f.Add(path, name)) return false;
	return true;
}

// Takes a path and file name like "C:\Folder\Program.exe"
// Removes the program's listing from the Windows Firewall exceptions list
// Returns false on error
bool FirewallRemove(read path) {

	// Make a firewall and have it access the COM interfaces of Windows Firewall
	Firewall f;
	if (!f.Access()) return false;

	// Remove the program's listing
	if (!f.Remove(path)) return false;
	return true;
}

// Get access to the COM objects
// Returns true if it works, false if there was an error
bool Firewall::Access() {

	// Initialize COM itself so this thread can use it
	HRESULT result = CoInitialize(NULL); // Must be NULL
	if (FAILED(result)) { error(result, L"coinitialize"); return false; }

	// Create an instance of the firewall settings manager
	result = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void **)&manager);
	if (FAILED(result) || !manager) { error(result, L"cocreateinstance netfwmgr"); return false; }

	// Retrieve the local firewall policy
	result = manager->get_LocalPolicy(&policy);
	if (FAILED(result) || !policy) { error(result, L"get_localpolicy"); return false; }

	// Retrieve the firewall profile currently in effect
	result = policy->get_CurrentProfile(&profile);
	if (FAILED(result) || !profile) { error(result, L"get_currentprofile"); return false; }

	// Retrieve the authorized application collection
	result = profile->get_AuthorizedApplications(&list);
	if (FAILED(result) || !list) { error(result, L"get_authorizedapplications"); return false; }

	// Everything worked
	return true;
}

// Takes a path and file name like "C:\Folder\Program.exe" and a name like "Program Name"
// Lists and checks the program on Windows Firewall, so now it can listed on a socket without a warning popping up
// Returns false on error
bool Firewall::Add(read path, read name) {

	// Create an instance of an authorized application, we'll use this to add our new application
	if (program) { program->Release(); program = NULL; }
	HRESULT result = CoCreateInstance(__uuidof(NetFwAuthorizedApplication), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwAuthorizedApplication), (void **)&program);
	if (FAILED(result)) { error(result, L"cocreateinstance netfwauthorizedapplication"); return false; };

	// Set the text
	Bstr p(path);                                       // Express the text as BSTRs
	result = program->put_ProcessImageFileName(p.bstr); // Set the process image file name
	if (FAILED(result)) { error(result, L"put_processimagefilename"); return false; };
	Bstr n(name);
	result = program->put_Name(n.bstr);                 // Set the program name
	if (FAILED(result)) { error(result, L"put_name"); return false; };

	// Get the program on the Windows Firewall exceptions list
	result = list->Add(program); // Add the application to the collection
	if (FAILED(result)) { error(result, L"firewall add"); return false; };
	return true;
}

// Takes a path like "C:\Folder\Program.exe"
// Removes the program from Windows Firewall
// Returns false on error
bool Firewall::Remove(read path) {

	// Remove the program from the Windows Firewall exceptions list
	Bstr p(path); // Express the text as a BSTR
	HRESULT result = list->Remove(p.bstr);
	if (FAILED(result)) { error(result, L"firewall remove"); return false; };
	return true;
}

// True if this running exe is registered to open torrent files and magnet links, false it's not or we can't tell
bool AssociateIs() {

	CString value;
	if (!RegistryRead(HKEY_CLASSES_ROOT, L".torrent",                              L"", &value) || value != PROGRAM_NAME)                            return false;
	if (!RegistryRead(HKEY_CLASSES_ROOT, PROGRAM_NAME + L"\\shell\\open\\command", L"", &value) || value != L"\"" + PathRunningFile() + "\" \"%1\"") return false;
	if (!RegistryRead(HKEY_CLASSES_ROOT, L"Magnet\\shell\\open\\command",          L"", &value) || value != L"\"" + PathRunningFile() + "\" \"%1\"") return false;
	return true;
}

// Register this running exe to open torrent files and magnet links
void AssociateGet() {

	RegistryDelete(HKEY_CLASSES_ROOT, L".torrent");
	RegistryWrite(HKEY_CLASSES_ROOT, L".torrent",                              L"",             PROGRAM_NAME);

	RegistryDelete(HKEY_CLASSES_ROOT, PROGRAM_NAME);
	RegistryWrite(HKEY_CLASSES_ROOT, PROGRAM_NAME,                             L"",             L"Torrent");
	RegistryWrite(HKEY_CLASSES_ROOT, PROGRAM_NAME + L"\\DefaultIcon",          L"",             PathTorrentIcon());
	RegistryWrite(HKEY_CLASSES_ROOT, PROGRAM_NAME + L"\\shell\\open\\command", L"",             L"\"" + PathRunningFile() + "\" \"%1\"");

	RegistryDelete(HKEY_CLASSES_ROOT, L"Magnet");
	RegistryWrite(HKEY_CLASSES_ROOT, L"Magnet",                                L"",             L"Magnet URI");
	RegistryWrite(HKEY_CLASSES_ROOT, L"Magnet",                                L"URL Protocol", L"");
	RegistryWrite(HKEY_CLASSES_ROOT, L"Magnet\\DefaultIcon",                   L"",             PathTorrentIcon());
	RegistryWrite(HKEY_CLASSES_ROOT, L"Magnet\\shell\\open\\command",          L"",             L"\"" + PathRunningFile() + "\" \"%1\"");
}

// List this running exe in Add or Remove Programs
void SetupAdd() {

	RegistryWrite(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + PROGRAM_NAME, L"DisplayName", PROGRAM_NAME);
	RegistryWrite(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + PROGRAM_NAME, L"UninstallString", PathRunningFile() + L" /addremove");
}

// Remove us from the registry
void SetupRemove() {

	CString value;
	if (RegistryRead(HKEY_CLASSES_ROOT, L".torrent",                     L"", &value) && value == PROGRAM_NAME)                            RegistryDelete(HKEY_CLASSES_ROOT, L".torrent");
	if (RegistryRead(HKEY_CLASSES_ROOT, L"Magnet\\shell\\open\\command", L"", &value) && value == L"\"" + PathRunningFile() + "\" \"%1\"") RegistryDelete(HKEY_CLASSES_ROOT, L"Magnet");
	RegistryDelete(HKEY_CLASSES_ROOT, PROGRAM_NAME);

	RegistryDelete(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + PROGRAM_NAME);
}

// Make a new column on the right of any already there
void ColumnAdd(HWND window, read title, int width, bool right) {
	ColumnAddIndex(window, ColumnCount(window), title, width, right); // 1 column already there has an index of 0, so make a new one at index 1
}

// Make a new column in the list view window at 0+ column place, title text, pixel width, and true to align right
void ColumnAddIndex(HWND window, int column, read title, int width, bool right) {

	// Have the next pulse edit all the cells
	App.list.refresh = true; // There will be blank cells in the column we added, and if we add the first column, the second column will also be blank

	// Never add column 0 because the text will be indented for the sort icon to be on the left, not the right
	if (column == 0) {
		ColumnAddIndexDo(window, 0, L"", 0, false); // Add a placeholder column with index 0
		ColumnAddIndexDo(window, 1, title, width, right); // Add the real column with index 1
		ColumnRemoveIndex(window, 0); // Remove the placeholder column at 0

	// No workaround required
	} else {
		ColumnAddIndexDo(window, column, title, width, right);
	}
}
void ColumnAddIndexDo(HWND window, int column, read title, int width, bool right) {

	// Compose format for left or right
	int format;
	if (right) format = LVCFMT_RIGHT;
	else format = LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT;

	// Add the column
	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask    = LVCF_FMT | LVCF_IMAGE | LVCF_TEXT | LVCF_WIDTH;
	info.fmt     = format;
	info.iImage  = App.icon.clear;
	info.pszText = (LPWSTR)title;
	info.cx      = width;
	if (ListView_InsertColumn(window, column, &info) == -1) error(L"listview_insertcolumn");
}

// Remove the column named title
void ColumnRemove(HWND window, read title) {
	int column = ColumnFind(window, title);
	if (column == -1) return;
	ColumnRemoveIndex(window, column);
}

// Remove the column at the given index
void ColumnRemoveIndex(HWND window, int column) {
	if (!ListView_DeleteColumn(window, column)) error(L"listview_deletecolumn");
}

// Set the given icon index in the column header named title
void ColumnIcon(HWND window, read title, int icon) {
	int column = ColumnFind(window, title);
	if (column == -1) return;

	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask   = LVCF_IMAGE;
	info.iImage = icon;
	if (!ListView_SetColumn(window, column, &info)) error(L"listview_setcolumn");
}

// Highlight the column named title
void ColumnSelect(HWND window, read title) {
	int column = ColumnFind(window, title);
	if (column == -1) return;

	ListView_SetSelectedColumn(window, column);
	InvalidateRect(window, NULL, true); // Invalidate the list view control and have it erased before painted
}

// Find the current 0+ index of the column with the given title in the window list view control
// This is the column index, not the order the column appears in the window, use ColumnWindowToList for that
// Also, these indices can change, so it's not necessarily the index you added the column with
int ColumnFind(HWND window, read title) {
	if (isblank(title)) return -1; // Make sure title has text

	int columns = ColumnCount(window);
	for (int i = 0; i < columns; i++)
		if (ColumnTitleIndex(window, i) == CString(title))
			return i;
	return -1; // Not found
}

// The title text of the column with the given index, like "Status"
CString ColumnTitleIndex(HWND window, int column) {
	WCHAR bay[MAX_PATH]; // Destination buffer
	lstrcpy(bay, L"");

	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask       = LVCF_TEXT;
	info.pszText    = bay;
	info.cchTextMax = MAX_PATH;
	if (ListView_GetColumn(window, column, &info) == -1) { error(L"listview_getcolumn title"); return L""; }
	return bay;
}

// The width in pixels of the column with the given index, or -1 error or not found
int ColumnWidthIndex(HWND window, int column) {
	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask = LVCF_WIDTH;
	if (ListView_GetColumn(window, column, &info) == -1) { error(L"listview_getcolumn width"); return -1; }
	return info.cx;
}

// True if the column with the given index is right aligned, false if aligned left
bool ColumnRightIndex(HWND window, int column) {
	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask = LVCF_FMT;
	if (ListView_GetColumn(window, column, &info) == -1) { error(L"listview_getcolumn right"); return false; }
	return (info.fmt & LVCFMT_RIGHT) != 0;
}

// Find out how many columns are in the given list view control
int ColumnCount(HWND window) {

	HWND header = ListHeader(window); // Get the header
	if (!header) return -1;

	int n = Header_GetItemCount(header); // Find out how many columns there are
	if (n == -1) { error(L"header_getitemcount"); return -1; }
	return n;
}

// Get the window handle of the column headers inside a list view control
HWND ListHeader(HWND window) {
	HWND header = ListView_GetHeader(window);
	if (!header) { error(L"listview_getheader"); return NULL; }
	return header;
}

// Get the parameter stored at the given row number
LPARAM ListGet(HWND window, int row) {

	LVITEM info;
	ZeroMemory(&info, sizeof(info));
	info.iItem = row;        // Look at item in the given row number
	info.mask  = LVIF_PARAM; // Get the parameter
	int result;
	result = ListView_GetItem(window, &info);
	if (!result) { error(L"listview_getitem"); return 0; }
	return info.lParam;
}

// Gets the parameter of the row that has the keyboard mark, 0 if no keyboard mark
LPARAM ListMark(HWND window) {

	int row = ListView_GetSelectionMark(window); // Get the row that has the mark
	if (row == -1) return 0; // No mark
	return ListGet(window, row); // Return the parameter of the row
}

// Gets the parameter of the row beneath the mouse, 0 if the mouse is not over a row
LPARAM ListMouse(HWND window) {

	LVHITTESTINFO info; // Get the list view row beneath the client coordinate
	ZeroMemory(&info, sizeof(info));
    info.pt = MouseClient(window).Point();
	int row = ListView_HitTest(window, &info);
	if (row == -1) return 0; // The mouse is not over a row
	return ListGet(window, row); // Return the parameter of the row
}

// Counts how many rows are in the list view control
int ListRows(HWND window) {
	int rows = ListView_GetItemCount(window);
	if (rows < 0) { error(L"listview_getitemcount"); return 0; }
	return rows;
}

// Counts how many selected rows are in the list view control
int ListSelectedRows(HWND window) {
	int rows = ListView_GetSelectedCount(window);
	if (rows < 0) { error(L"listview_getselectedcount"); return 0; }
	return rows;
}

// True if the given row number is selected
bool ListSelected(HWND window, int row) {
	return ListView_GetItemState(window, row, LVIS_SELECTED) == LVIS_SELECTED;
}

// Select all the rows in the list view control
void ListSelectAll(HWND window) {
	int rows = ListRows(window);
	for (int row = 0; row < rows; row++) // Loop down the rows, selecting each one
		ListView_SetItemState(window, row, LVIS_SELECTED, LVIS_SELECTED);
}

// Scroll to the bottom of the list view control
void ListScroll(HWND window) {
	int rows = ListRows(window); // Count the number of rows in the list view control
	if (rows && !ListView_EnsureVisible(window, rows - 1, false)) error(L"listview_ensurevisible"); // Scroll to make the last row completely visible
}

// Remove the given row index
void ListRemove(HWND window, int row) {
	if (!ListView_DeleteItem(window, row)) error(L"listview_deleteitem");
}

// Remove all the rows in the list view control
void ListRemoveAll(HWND window) {
	if (!ListView_DeleteAllItems(window)) error(L"listview_deleteallitems");
}

// Given the number of rows that we will add, turn painting off and tell the control the new size to hold
void ListAddStart(HWND window, int rows) {
	if (rows > 1) {                                             // Suspend drawing if more than 1 row is going to be added
		SendMessage(window, WM_SETREDRAW, false, 0);            // Prevent changes in the list view window from being drawn
		ListView_SetItemCount(window, ListRows(window) + rows); // Tell the list view control how many rows it will have total
	}
}

// Given the number of rows that we added, turn painting back on and paints the list view control window
void ListAddDone(HWND window, int rows) {
	if (rows > 1) {                                 // Resume drawing if it was suspended
		SendMessage(window, WM_SETREDRAW, true, 0); // Allow changes in the list view window to be drawn
		InvalidateRect(window, NULL, true);         // Invalidate the list view control and have it erased before painted
	}
}

// Find the row number where the given parameter appears in the list view control, or -1 not found
int ListFind(HWND window, LPARAM p) {

	LVFINDINFO info;
	ZeroMemory(&info, sizeof(info));
	info.flags  = LVFI_PARAM;                       // Search based on the parameter
	info.lParam = p;                                // Here is the parameter to find
	int row = ListView_FindItem(window, -1, &info); // -1 to start from the beginning
	if (row == -1) error(L"listview_finditem");     // Not found
	return row;                                     // Return the row number
}

// Given row and column coordinates, get the bounding size of the list view sub item cell
Size CellSize(HWND window, int row, int column) {
	RECT rectangle;
	if (!ListView_GetSubItemRect(window, row, column, LVIR_BOUNDS, &rectangle)) { error(L"listview_getsubitemrect"); return Size(); }
	return Size(rectangle);
}

// Given cells, add or edit a row in the list view window
void CellShow(HWND window, std::vector<Cell> &cells) {

	// Find each cell's current column index, picking out the one with special index 0
	Cell *primary = NULL;                         // Point primary at the cell in column 0
	std::vector<Cell *> secondary;                // Fill secondary with pointers to all the other cells
	for (int i = 0; i < (int)cells.size(); i++) { // Loop through the given row of cells
		Cell *c = &(cells[i]);

		c->column = ColumnFind(window, c->title);         // Update the cell's record of the column index it's in right now
		if      (c->column == -1) c->Hidden();            // The column is hidden, clear the cell's record of what it has on the screen
		else if (c->column ==  0) primary = c;            // This cell is in column 0, point primary at it
		else                      secondary.push_back(c); // The column is a list view submitem, add it to our list of them
	}
	if (!primary) { log(L"no primary cell found"); return; }

	// Find the current row index for the given cells
	int row = ListFind(window, primary->parameter);
	bool add = row == -1; // Not found, we'll add the primary cell in a new row at the end
	if (add) row = ListRows(window); // Choose the new last row index
	for (int i = 0; i < (int)cells.size(); i++) cells[i].row = row; // Note the row index in the cell object

	// Update the list view control
	CellShowDo(window, primary, add); // Add or edit the primary cell
	for (int i = 0; i < (int)secondary.size(); i++) // Edit the secondary cells
		CellShowDo(window, secondary[i], false);
}
void CellShowDo(HWND window, Cell *c, bool add) { // True to insert c in column 0 and a new row at the end

	// Make a list view item structure to edit the cell at those coordinates
	LVITEM info;
	ZeroMemory(&info, sizeof(info));
	info.iSubItem = c->column;
	info.iItem = c->row;

	// If column 0, parameter
	if (c->column == 0) {
		info.mask |= LVIF_PARAM;
		info.lParam = c->parameter;
	}

	// Text
	info.mask |= LVIF_TEXT;
	info.pszText = (LPWSTR)(read)(c->text);

	// If specified, an icon
	if (c->icon != -1) {
		info.mask |= LVIF_IMAGE;
		info.iImage = c->icon;
	}

	// Add this cell in a new row in column 0
	if (add) {

		if (ListView_InsertItem(window, &info) == -1) error(L"listview_insertitem");
		c->Same();

	// If necessary, edit the contents of the existing cell
	} else if (c->Different() || App.list.refresh) {

		if (!ListView_SetItem(window, &info)) error(L"listview_setitem");
		c->Same(); // Record that now display matches data
	}
}
