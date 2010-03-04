
// Include statements
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>
#include "resource.h"
#include "program.h"
#include "class.h"
#include "function.h"

// Global objects
extern handleitem Handle;

// Show a message to the user
void Report(read r) {

	// Display the given text in a message box
	if (PROGRAM_TEST) MessageBox(Handle.window, r, L"Report", MB_OK);
}

// Start a new thread to execute the given function
void BeginThread(LPVOID function) {

	// Create a new thread that runs the function
	DWORD info;
	HANDLE thread = (HANDLE)_beginthreadex(
		(void *)                          NULL,     // Use default security attributes
		(unsigned)                        0,        // Default initial stack size
		(unsigned (__stdcall *) (void *)) function, // Function where the new thread will begin execution
		(void *)                          NULL,     // Pass no parameter to the thread
		(unsigned)                        0,        // Create and start thread
		(unsigned *)                      &info);   // Writes thread identifier, cannot be null for Windows 95
	if (!thread) Report(L"error beginthreadex");

	// Tell the system this thread is done with the new thread handle
	if (!CloseHandle(thread)) Report(L"error closehandle");
}

// Have the given window display the given text
void WindowTextSet(HWND window, read r) {

	// Set the text to the window
	if (!SetWindowText(window, r)) Report(L"error setwindowtext");
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
	if (!window) Report(L"error createwindow");
	return window;
}

// Move and resize the given window
void WindowSize(HWND window, sizeitem size) {

	// Move the window, false to not send a paint message
	if (!MoveWindow(window, size.x(), size.y(), size.w(), size.h(), false)) Report(L"error movewindow");
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

// Takes a menu resource name and an index, like 0, to clip out the first submenu
// Loads the menu and clips out the submenu at the given index
// Returns the menu, or null on error
HMENU MenuLoad(read name, int index) {

	// Load the menu resource
	HMENU menus = LoadMenu(Handle.instance, name);
	if (!menus) { Report(L"error loadmenu"); return NULL; }

	// Clip off the submenu at the given index, and return it
	HMENU menu = GetSubMenu(menus, index);
	if (!menu) Report(L"error getsubmenu");
	return menu;
}

// Show the given context menu
// Holds here while the user makes a choice
// Returns the choice the user made, or 0 if the user clicked outside or on error
UINT MenuShow(HMENU menu, int x, int y) {

	// Show the context menu and hold execution here until the user chooses from the menu or cancels it
	return TrackPopupMenu(
		menu,            // Handle to the menu to display
		TPM_NONOTIFY |   // Return the chosen menu item without sending messages to the main window
		TPM_RETURNCMD |
		TPM_RIGHTBUTTON, // Let the user click an item with the left or right mouse button
		x,               // Desired place to show it in screen coordinates
		y,
		0,
		Handle.window,
		NULL);
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

	// Create a brush of the solid color
	brushitem brush;
	brush.color = color;
	brush.brush = CreateSolidBrush(color);
	if (!brush.brush) Report(L"error createsolidbrush");

	// Return the brush color and handle
	return brush;
}

// Takes a font face name and point size
// Creates the font
// Returns a handle to it
HFONT CreateFont(read face, int points) {

	// Create the font
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
	if (!font) Report(L"error createfontindirect");
	return font;
}

// Repaint the window right now
void PaintMessage(HWND window) {

	// Invalidate the whole client area of the given window, and make it process a paint message right now
	InvalidateRect(window, NULL, false); // false to not wipe the window with the background color
	UpdateWindow(window); // Call the window procedure directly with a paint message right now
}

// Takes a device context, size, and brush
// Fills the rectangle with the color
void PaintFill(deviceitem *device, sizeitem size, HBRUSH brush) {

	// Make sure there are pixels to paint
	if (!size.is()) return;

	// Paint the rectangle
	RECT rectangle = size.rectangle();
	FillRect(device->device, &rectangle, brush); // Returns error if the computer is locked
}

// Paint a 1-pixel wide border inside the given size
void PaintBorder(deviceitem *device, sizeitem size, HBRUSH brush) {

	// Paint the 4 edges of the border
	sizeitem edge;
	edge = size; edge.w(1); PaintFill(device, edge, brush); edge.x(size.r() - 1); PaintFill(device, edge, brush);
	edge = size; edge.h(1); PaintFill(device, edge, brush); edge.y(size.b() - 1); PaintFill(device, edge, brush);
}

// Takes a deviceitem that has a font, text, and a bounding position and size
// Paints the text there
void PaintText(deviceitem *device, read r, sizeitem size) {

	// Paint the text, if the background is opaque, this will cause a flicker
	RECT rectangle = size.rectangle();
	if (!DrawText(device->device, r, -1, &rectangle, DT_NOPREFIX)) Report(L"error drawtext");
}

// Adds the program icon to the taskbar notification area
void TaskbarIconAdd() {

	// Only do something if the icon isn't there
	if (Handle.taskbar) return;
	Handle.taskbar = true;

	// Add the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize           = sizeof(info);                     // Size of this structure
	info.hWnd             = Handle.window;                    // Handle to the window that will receive messages
	info.uID              = 0;                                // Program defined identifier
	info.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP; // Mask for message, icon and tip
	info.uCallbackMessage = MESSAGE_TASKBAR;                  // Program defined message identifier
	info.hIcon            = Handle.iconsmall;                 // Icon handle
	lstrcpy(info.szTip, PROGRAM_NAME);                        // 64 character buffer for tooltip text
	if (!Shell_NotifyIcon(NIM_ADD, &info)) Report(L"error shell_notifyicon nim_add");
}

// Removes the program icon from the taskbar notification area
void TaskbarIconRemove() {

	// Only do something if the icon is there
	if (!Handle.taskbar) return;
	Handle.taskbar = false;

	// Remove the taskbar notification icon
	NOTIFYICONDATA info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);  // Size of this structure
	info.hWnd   = Handle.window; // Handle to the window that will receive messages
	info.uID    = 0;             // Program defined identifier
	if (!Shell_NotifyIcon(NIM_DELETE, &info)) Report(L"error shell_notifyicon nim_delete");
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
	if (!cursor) Report(L"error loadimage cursor");
	return(cursor);
}

// Takes the name of an icon resource and the size to load, or 0 for default
// Loads it
// Returns it, or null if any error
HICON LoadIconResource(read name, int size) {

	// Create the icon from the resource
	HICON icon = (HICON)LoadImage(
		Handle.instance,  // Load from this instance
		name,             // Resource name
		IMAGE_ICON,       // Image type
		size, size,       // Size to load, 0 to load actual resource size
		LR_DEFAULTCOLOR); // Default flag does nothing
	if (!icon) Report(L"error loadimage icon");
	return(icon);
}

// Takes a cursor to set
// Sets the cursor if it needs to be changed
void CursorSet(HCURSOR cursor) {

	// Find out what the cursor is now
	HCURSOR current = GetCursor();

	// If the cursor is different, set it
	if (cursor && current && cursor != current) {
		if (!SetCursor(cursor)) Report(L"error setcursor");
	}
}




void ContextMenuLoad(int i, HMENU *menus, HMENU *menu)
{
	// takes the 0 based index of the context menu to use
	// loads the context menus and clips the requested one
	// returns nothing, writes the menu handles or null if any error

	// LOAD THE CONTEXT MENUS
	*menus = LoadMenu(Handle.instance, "MENU_CONTEXT");
	if (!*menus) { Report("contextmenuload: error loadmenu"); *menus = NULL; *menu  = NULL; return; }

	// CLIP THE REQUESTED CONTEXT MENU
	*menu = GetSubMenu(*menus, i);
	if (!*menu) { Report("contextmenuload: error getsubmenu"); DestroyMenuSafely(*menus); *menus = NULL; *menu  = NULL; return; }
}

void ContextMenuSet(HMENU menu, UINT command, UINT state, HBITMAP bitmap)
{
	// takes a menu, command, and the state to set
	// sets the menu item
	// returns nothing

	// SET MENU ITEM INFO
	MENUITEMINFO info;
	info.cbSize = sizeof(MENUITEMINFO);
	info.fMask  = 0;
	if (bitmap) { info.fMask = info.fMask | MIIM_BITMAP; info.hbmpItem = bitmap; }
	if (state)  { info.fMask = info.fMask | MIIM_STATE;  info.fState   = state; }
	if (!SetMenuItemInfo(menu, command, false, &info)) Report("contextmenuset: error setmenuiteminfo");
}

UINT ContextMenuShow(HMENU menus, HMENU menu, bool taskbar, sizeitem *size)
{
	// takes menus and whether the context menu is being shown from the taskbar notification icon or not
	// displays the context menu and waits for the user to make a choice, then destroys the menus
	// returns the menu item identifier of the choice, or 0 if the user cancelled the menu or any error

	// IF THE MENU WAS NOT LOADED, REPORT CANCEL
	if (!menu) return(0);

	// USE THE GIVEN SIZE OR THE MOUSE POSITION
	sizeitem position;
	if (size) {

		// CONVERT THE CLIENT SIZE INTO SCREEN COORDINATES
		position = *size;
		position.Screen();

	} else {

		// GET THE MOUSE POSITION IN SCREEN COORDINATES
		position = MouseScreen();
	}

	// CONTEXT MENU FOR NOTIFICATION ICON
	if (taskbar) SetForegroundWindow(Handle.window);

	// SHOW THE CONTEXT MENU AND HOLD EXECUTION HERE UNTIL THE USER CHOOSES FROM THE MENU OR CANCELS IT
	UINT choice;
	AreaPopUp();
	choice = TrackPopupMenu(
		menu,            // HANDLE TO THE MENU TO DISPLAY
		TPM_NONOTIFY |   // RETURN THE CHOSEN MENU ITEM WITHOUT SENDING MESSAGES TO THE MAIN WINDOW
		TPM_RETURNCMD |
		TPM_RIGHTBUTTON, // LET THE USER CLICK ON AN ITEM WITH THE LEFT OR RIGHT BUTTON
		position.x,      // DESIRED MENU POSITION IN SCREEN COORDINATES
		position.y,
		0,
		Handle.window,
		NULL);
	AreaPopDown();

	// CONTEXT MENU FOR NOTIFICATION ICON
	if (taskbar) PostMessage(Handle.window, WM_NULL, 0, 0);

	// DESTROY THE CONTEXT MENUS, DESTROYING THE CLIPPED MENU AS WELL, AND RETURN THE RESULT
	DestroyMenuSafely(menus);
	return(choice);
}


