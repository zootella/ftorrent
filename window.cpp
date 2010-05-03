
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

// Global objects
handletop Handle; // Window handles
areatop   Area;   // Button and drag areas
datatop   Data;   // Linked data items
statetop  State;  // State variables

// Start the program
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR command, int show) {

	// Save the given instance handle
	Handle.instance = instance;

	// Tell the system we're going to use the list and tree view controls
	InitializeCommonControls();

	// Load menus
	HMENU menus = MenuLoad(L"CONTEXT_MENU");
	Handle.tray = MenuClip(menus, 0);
	Handle.menu = MenuClip(menus, 1);
	if (!PROGRAM_TEST && !DeleteMenu(Handle.menu, ID_TOOLS_TEST, 0)) Report(L"deletemenu"); // Remove the test menu item

	// Load icons
	Handle.clear      = LoadIconResource(L"CLEAR_ICON",        16, 16);
	Handle.blue16     = LoadIconResource(L"0_STATE_BLUE",      16, 16);
	Handle.blue32     = LoadIconResource(L"0_STATE_BLUE",      32, 32);
	Handle.green16    = LoadIconResource(L"STATE_GREEN",       16, 16);
	Handle.green32    = LoadIconResource(L"STATE_GREEN",       32, 32);
	Handle.red16      = LoadIconResource(L"STATE_RED",         16, 16);
	Handle.red32      = LoadIconResource(L"STATE_RED",         32, 32);
	Handle.yellow16   = LoadIconResource(L"STATE_YELLOW",      16, 16);
	Handle.yellow32   = LoadIconResource(L"STATE_YELLOW",      32, 32);
	Handle.ascending  = LoadIconResource(L"COLUMN_ASCENDING",  16, 16); // Buttons
	Handle.descending = LoadIconResource(L"COLUMN_DESCENDING", 16, 16);
	Handle.pausedim   = LoadIconResource(L"BUTTON_PAUSE_DIM",  19, 19);
	Handle.pausehot   = LoadIconResource(L"BUTTON_PAUSE_HOT",  19, 19);
	Handle.pause      = LoadIconResource(L"BUTTON_PAUSE",      19, 19);
	Handle.removedim  = LoadIconResource(L"BUTTON_REMOVE_DIM", 19, 19);
	Handle.removehot  = LoadIconResource(L"BUTTON_REMOVE_HOT", 19, 19);
	Handle.remove     = LoadIconResource(L"BUTTON_REMOVE",     19, 19);
	Handle.startdim   = LoadIconResource(L"BUTTON_START_DIM",  19, 19);
	Handle.starthot   = LoadIconResource(L"BUTTON_START_HOT",  19, 19);
	Handle.start      = LoadIconResource(L"BUTTON_START",      19, 19);
	Handle.stopdim    = LoadIconResource(L"BUTTON_STOP_DIM",   19, 19);
	Handle.stophot    = LoadIconResource(L"BUTTON_STOP_HOT",   19, 19);
	Handle.stop       = LoadIconResource(L"BUTTON_STOP",       19, 19);
	Handle.toolshot   = LoadIconResource(L"BUTTON_TOOLS_HOT",  19, 30); // Rectangular size
	Handle.tools      = LoadIconResource(L"BUTTON_TOOLS",      19, 30);

	// Load cursors
	Handle.arrow      = LoadSharedCursor(IDC_ARROW);
	Handle.hand       = LoadSharedCursor(IDC_HAND);
	Handle.horizontal = LoadSharedCursor(IDC_SIZEWE);
	Handle.vertical   = LoadSharedCursor(IDC_SIZENS);
	Handle.diagonal   = LoadSharedCursor(IDC_SIZENWSE);

	// Make brushes
	Handle.face        = BrushSystem(COLOR_3DFACE); // Shared handles to system brushes
	Handle.shadow      = BrushSystem(COLOR_3DSHADOW);
	Handle.background  = BrushSystem(COLOR_WINDOW);
	Handle.ink         = BrushSystem(COLOR_WINDOWTEXT);
	Handle.select      = BrushSystem(COLOR_HIGHLIGHT);
	Handle.blue        = CreateBrush(RGB(  0, 102, 204)); // Colors
	Handle.lightblue   = CreateBrush(RGB( 51, 153, 255));
	Handle.green       = CreateBrush(RGB(102, 204,  51));
	Handle.lightgreen  = CreateBrush(RGB(153, 255, 102));
	Handle.red         = CreateBrush(RGB(255, 102,  51));
	Handle.lightred    = CreateBrush(RGB(255, 153, 102));
	Handle.yellow      = CreateBrush(RGB(255, 204,   0));
	Handle.lightyellow = CreateBrush(RGB(255, 255, 102));
	Handle.line        = CreateBrush(ColorMix(GetSysColor(COLOR_3DFACE), 1, GetSysColor(COLOR_3DSHADOW), 1)); // Mix

	// Make fonts
	Handle.font      = FontMenu(false);
	Handle.underline = FontMenu(true);
	Handle.arial     = FontName(L"Arial", 28);

	// Register the class for the main window, and create it
	string name = PROGRAM_NAME + L"ClassName"; // Compose a unique window class name
	WNDCLASSEX info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize        = sizeof(info);               // Size of this structure
	info.style         = 0;                          // Default style
	info.lpfnWndProc   = WindowProcedure;            // Function pointer to the window procedure
	info.cbClsExtra    = 0;                          // No extra bytes
	info.cbWndExtra    = 0;
	info.hInstance     = Handle.instance;            // Instance handle
	info.hIcon         = Handle.blue32;              // Large and small icons
	info.hIconSm       = Handle.blue16;
	info.hCursor       = NULL;                       // Mouse cursor changes
	info.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Background brush
	info.lpszMenuName  = NULL;                       // No menu
	info.lpszClassName = name;                       // Window class name
	if (!RegisterClassEx(&info)) Report(L"registerclassex");
	Handle.window = WindowCreate(name, PROGRAM_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, NULL, NULL);

	// Add Exit to the main window's system menu
	HMENU menu = GetSystemMenu(Handle.window, false); // Get the menu for editing
	if (!menu) Report(L"getsystemmenu");
	if (menu && !AppendMenu(menu, MF_STRING, ID_TOOLS_EXIT, L"&Exit")) Report(L"appendmenu");

	// Create the list view window
	DWORD style =
		WS_CHILD |             // Required for child windows
		LVS_REPORT |           // Specifies report view, I think this puts it in details
		LBS_EXTENDEDSEL |      // Allows multiple items to be selected
		LVS_SHOWSELALWAYS |    // Shows the selection even when the control doesn't have the focus
		LBS_NOINTEGRALHEIGHT | // Allows the size to be specified exactly without snap
		LVS_SHAREIMAGELISTS;   // Will not delete the system image list when the control is destroyed
	Handle.list = WindowCreate(WC_LISTVIEW, NULL, style, 0, Handle.window, (HMENU)WINDOW_LIST);

	// Create the tabs window
	style = WS_CHILD;        // Required for child windows
	Handle.tabs = WindowCreate(WC_TABCONTROL, NULL, style, 0, Handle.window, (HMENU)WINDOW_TABS);
	SendMessage(             // Have it use Tahoma, not the default system font
		(HWND)Handle.tabs,   // Send the message to this window
		WM_SETFONT,          // Message to send
		(WPARAM)Handle.font, // Handle to font
		0);                  // Don't tell the control to immediately redraw itself

	// Add the tabs
	AddTab(Handle.tabs, 0, L"General");
	AddTab(Handle.tabs, 1, L"Trackers");
	AddTab(Handle.tabs, 2, L"Peers");
	AddTab(Handle.tabs, 3, L"Pieces");
	AddTab(Handle.tabs, 4, L"Files");

	// Make child windows and menus
	Handle.edit = WindowCreateEdit(true,  true);
	WindowEdit(Handle.edit, true);

	// Make the areas of the window like the buttons and sizing grips
	AreaCreate();
	AreaPulse(); // Set the display state of each area

	// Lower the window
	sizeitem size = SizeWindow(Handle.window);
	size.ShiftTop((size.h * 1) / 4);
	WindowMove(Handle.window, size, false);

	// Show the child windows and then the main window
	ShowWindow(Handle.list,   SW_SHOWNORMAL);
	ShowWindow(Handle.tabs,   SW_SHOWNORMAL);
	ShowWindow(Handle.edit,   SW_SHOWNORMAL);
	ShowWindow(Handle.window, SW_SHOWNORMAL); // Calling this causes a paint message right now
	PaintMessage(); // Necessary to draw child window controls

	// Start the pulse timer
	TimerSet(TIMER_PULSE, 300);

	// Message loop
	MSG message;
	while (true) {

		// Peek for a message without removing it to perform idle tasks if there are no messages
		if (!PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE)) WindowPulse();

		// Wait for the next message and get it
		if (GetMessage(&message, NULL, 0, 0)) {

			// Process the message
			TranslateMessage(&message);
			DispatchMessage(&message);

		} else {

			// Get message got quit and returned 0, exit the message loop
			break;
		}
	}

	// Remove the tray icon
	if (State.taskbar) TaskbarIconRemove();

	// Return the value from the quit message
	return (int)message.wParam;
}

// Called after each group of messages and every .3 seconds
// Updates the program's data and display
void WindowPulse() {

	// Pulse the program from data to display
	AreaPulse();
}

// Process a message from the system
LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {

	// The user sized the window, the first size message happens before the first paint
	switch (message) {
	case WM_SIZE:

		// Size the window and send it a paint message
		Size();
		PaintMessage();
		return 0;

	// The parts of the client area not covered by child window controls need to be painted
	break;
	case WM_PAINT:

		// Paint the window
		{
			deviceitem device;
			device.OpenPaint(window);
			device.Font(Handle.font); // Replace the Windows 3.1 font with something more modern
			device.BackgroundColor(Handle.background.color);
			PaintWindow(&device);
			return 0;
		}

	// A timer expired
	break;
	case WM_TIMER:

		// The pulse timer expired
		switch (wparam) {
		case TIMER_PULSE:

			// Kill the expired timer, pulse the window, and set it again
			KillTimerSafely(TIMER_PULSE);
			WindowPulse();
			TimerSet(TIMER_PULSE, 300);

		break;
		}

	// The message is a command
	break;
	case WM_COMMAND:

	// The primary mouse button has clicked down in the main window
	break;
	case WM_LBUTTONDOWN:

		// Record the area where the mosue pressed, and capture the mouse to get all mouse messages
		Area.pressed = MouseOver();
		if (Area.pressed) Area.stick = MouseArea(Area.pressed);
		MouseCapture();
		return 0;

	// The primary mouse button has clicked up in the main window, or anywhere if the mouse is captured
	break;
	case WM_LBUTTONUP:

		// Record the area where the mouse released, and release the captured mouse to stop getting all mouse messages
		if (Area.pressed && Area.pressed == MouseOver()) {
			if (AreaCommand(Area.pressed)) { // Perform the command
				PostQuitMessage(0); // The user clicked Tools, Exit, close the program
				return 0;
			}
		}
		Area.pressed = NULL;
		MouseRelease();
		return 0;

	// A new window has captured the mouse
	break;
	case WM_CAPTURECHANGED:

		// The program has lost the mouse capture, mark no area pressed
		if (Handle.window != (HWND)lparam) Area.pressed = NULL;

	// The system has removed the window from the screen
	break;
	case WM_DESTROY:

		// Close the program
		PostQuitMessage(0);
		return 0;

	// The user clicked a window button or menu item
	break;
	case WM_SYSCOMMAND:

		// The user clicked the close X or window menu item
		switch (LOWORD(wparam)) {
		case SC_CLOSE:

			// Hide the window and add the taskbar notification icon
			ShowWindow(Handle.window, SW_HIDE);
			TaskbarIconAdd();

			// Prevent the program from closing
			return 0;

		// The user clicked the Exit window menu item that we added
		break;
		case ID_TOOLS_EXIT:

			// Close the program
			PostQuitMessage(0);
			return 0;

		break;
		}

	// Message from our icon in the taskbar notification area
	break;
	case MESSAGE_TASKBAR:

		// The primary mouse button has clicked up on our icon
		switch (lparam) {
		case WM_LBUTTONUP:

			// Restore from the taskbar
			TaskbarIconRemove();
			ShowWindow(Handle.window, SW_SHOW);
			if (IsIconic(Handle.window)) ShowWindow(Handle.window, SW_RESTORE);

		// The secondary mouse button has clicked up on our icon
		break;
		case WM_RBUTTONUP:

			// Show the taskbar context menu
			MenuTaskbar();

		break;
		}

	break;
	}

	// Have the system process the message
	return DefWindowProc(window, message, wparam, lparam);
}

// Displays the taskbar notification icon context menu and performs the result
void MenuTaskbar() {

	// Highlight and show the menu to the user
	MenuSet(Handle.tray, ID_TASKBAR_RESTORE, MFS_DEFAULT, HBMMENU_POPUP_RESTORE);
	UINT choice = MenuShow(Handle.tray, true, NULL); // Wait here while the menu is up

	// Restore
	switch (choice) {
	case ID_TASKBAR_RESTORE:

		// Restore from taskbar
		TaskbarIconRemove();
		ShowWindow(Handle.window, SW_SHOW);
		if (IsIconic(Handle.window)) ShowWindow(Handle.window, SW_RESTORE);

	// Exit
	break;
	case ID_TASKBAR_EXIT:

		// Remove the icon and exit the mesage loop
		TaskbarIconRemove();
		PostQuitMessage(0);

	break;
	}
}
