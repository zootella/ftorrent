
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

// Show an error message to the user
void Report(read r) {

	if (PROGRAM_TEST) MessageBox(
		Handle.window,
		make(L"error ", numerals(GetLastError()), L" ", r),
		L"Report",
		MB_OK);
}

// Given access to a handle, close it and make it null
void CloseHandleSafely(HANDLE *handle) {

	if (*handle && *handle != INVALID_HANDLE_VALUE) { // Only do something if it's not null or -1

		if (!CloseHandle(*handle)) Report(L"closehandle safely");
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
	if (!thread) Report(L"beginthreadex");

	// Tell the system this thread is done with the new thread handle
	if (!CloseHandle(thread)) Report(L"closehandle thread");
}

// Have the given window display the given text
void WindowTextSet(HWND window, read r) {

	if (!SetWindowText(window, r)) Report(L"setwindowtext"); // Set the text to the window
}

// Get the text a window is displaying
// Returns blank if no text or any error
string WindowTextGet(HWND window) {

	// If the window handle is null, return blank
	if (!window) return L"";

	// Find the required buffer size, in bytes
	int size =
		(int)SendMessage(window, WM_GETTEXTLENGTH, 0, 0) // The number of text characters
		+ 1; // Add 1 to have space in the buffer for the null terminator

	// Open a string
	string s;
	write buffer = s.GetBuffer(size);

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
	string s = WindowTextGet(window); // Get the text that's already there, the user may have edited it
	if (is(s)) s += L"\r\n";          // If not blank, start with a newline to make sure r will be on its own line
	s += r;                           // Add the given text
	WindowTextSet(window, s);         // Put the edited text back into the control

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
	WindowTextSet(window, r);

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
	if (!window) Report(L"createwindow");
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
	if (!GetWindowRect(window, &r)) { Report(L"windowsize: error getwindowrect"); return; }

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
	if (!MoveWindow(window, size.x, size.y, size.w, size.h, paint)) Report(L"windowmove: error movewindow");
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
string DialogBrowse(read display) {

	// Setup information for the dialog box
	character name[MAX_PATH];
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
	character buffer[MAX_PATH];
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
	if (!brush.brush) Report(L"createsolidbrush");
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
	if (!result) Report(L"invalidaterect");

	// Call the window procedure directly with a paint message right now
	// Send the window a paint message and have it process it right now
	if (!UpdateWindow(window)) Report(L"updatewindow");
}

// Adds the program icon to the taskbar notification area
void TaskbarIconAdd() {

	// Only do something if the icon isn't there
	if (State.taskbar) return;
	State.taskbar = true;

	// Add the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize           = sizeof(info);                     // Size of this structure
	info.hWnd             = Handle.window;                    // Handle to the window that will receive messages
	info.uID              = 0;                                // Program defined identifier
	info.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP; // Mask for message, icon and tip
	info.uCallbackMessage = MESSAGE_TASKBAR;                  // Program defined message identifier
	info.hIcon            = Handle.blue16;                    // Icon handle
	lstrcpy(info.szTip, PROGRAM_NAME);                        // 64 character buffer for tooltip text
	if (!Shell_NotifyIcon(NIM_ADD, &info)) Report(L"shell_notifyicon nim_add");
}

// Removes the program icon from the taskbar notification area
void TaskbarIconRemove() {

	// Only do something if the icon is there
	if (!State.taskbar) return;
	State.taskbar = false;

	// Remove the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);  // Size of this structure
	info.hWnd   = Handle.window; // Handle to the window that will receive messages
	info.uID    = 0;             // Program defined identifier
	if (!Shell_NotifyIcon(NIM_DELETE, &info)) Report(L"shell_notifyicon nim_delete");
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
	if (!cursor) Report(L"loadimage cursor");
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
	if (!icon) Report(L"loadimage icon");
	return icon;
}

// Sets the cursor to the given cursor if it isn't that already
void CursorSet(HCURSOR cursor) {

	HCURSOR current = GetCursor(); // Find out what the cursor is now
	if (cursor && current && cursor != current) { // If we have both and they're different
		if (!SetCursor(cursor)) Report(L"setcursor"); // Set the new given one
	}
}

// Takes a menu name and loads it from resources
HMENU MenuLoad(read name) {

	HMENU menus = LoadMenu(Handle.instance, name); // Load the menu resource
	if (!menus) Report(L"loadmenu");
	return menus;
}

// Takes a menu index, like 0 for the first one
// Clips out the submenu at that index
HMENU MenuClip(HMENU menus, int index) {

	HMENU menu = GetSubMenu(menus, index); // Clip off the submenu at the given index
	if (!menu) Report(L"getsubmenu");
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
	if (!SetMenuItemInfo(menu, command, false, &info)) Report(L"setmenuiteminfo");
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

	/*
	// The mouse is inside if it is inside the client area and outside all the child window controls
	return
		client.Inside(mouse)            &&
		!Draw.area.edit.Inside(mouse)   &&
		!Draw.area.button.Inside(mouse) &&
		!Draw.area.tree.Inside(mouse)   &&
		!Draw.area.list.Inside(mouse);
		*/
	return false;
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
	if (!brush.brush) Report(L"getsyscolorbrush");
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
	if (!brush.brush) Report(L"createsolidbrush");
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
	if (!GetClientRect(window, &rectangle)) { Report(L"getclientrect"); return size; }
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
	if (!GetWindowRect(window, &rectangle)) { Report(L"getwindowrect"); return size; }
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

		Report(L"gettextextentpoint32");
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
	if (!DrawText(device->device, r, -1, &rectangle, DT_NOPREFIX)) Report(L"drawtext");
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
		if (!DrawText(device->device, r, -1, &rectangle, DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE)) Report(L"drawtext");
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
	if (!result) Report(L"drawiconex");
}

// Make a font based on what the system uses in menus, true to underline it
HFONT FontMenu(boolean underline) {

	NONCLIENTMETRICS info;
	ZeroMemory(&info, sizeof(info));
	DWORD size = sizeof(info) - sizeof(info.iPaddedBorderWidth); // Ignore last int for this to work
	info.cbSize = size;
	int result = SystemParametersInfo(
		SPI_GETNONCLIENTMETRICS, // System parameter to retrieve
		size,                    // Size of the structure
		&info,                   // Structure to fill with information
		0);                      // Not setting a system parameter
	if (!result) { Report(L"systemparametersinfo getnonclientmetrics"); return NULL; }
	if (underline) info.lfMenuFont.lfUnderline = true; // Underline if requested
	HFONT font = CreateFontIndirect(&info.lfMenuFont);
	if (!font) Report(L"createfontindirect systemparametersinfo");
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
	if (!font) Report(L"createfontindirect logfont");
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
	if (!InitCommonControlsEx(&info)) Report(L"initcommoncontrolsex");
}

// Takes a window handle and timer identifier
// Kills the timer
void KillTimerSafely(UINT_PTR timer, HWND window) {

	if (!window) window = Handle.window; // Choose window
	if (!KillTimer(window, timer)) Report(L"killtimer"); // Kill the timer
}

// Takes a timer identifier and the number of milliseconds after which the timer should expire
// Sets the timer
void TimerSet(UINT_PTR timer, UINT time, HWND window) {

	if (!window) window = Handle.window; // Choose window
	if (!SetTimer(window, timer, time, NULL)) Report(L"settimer"); // Set the timer
}
