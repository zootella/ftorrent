
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
	Handle.blue16 = LoadIconResource(L"APPLICATION_ICON", 16);
	Handle.blue32 = LoadIconResource(L"APPLICATION_ICON", 32);

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
	Handle.white       = CreateBrush(RGB(255, 255, 255)); // Colors
	Handle.black       = CreateBrush(RGB(  0,   0,   0));
	Handle.blue        = CreateBrush(RGB(  0, 102, 204));
	Handle.lightblue   = CreateBrush(RGB( 51, 153, 255));
	Handle.yellow      = CreateBrush(RGB(255, 204,   0));
	Handle.lightyellow = CreateBrush(RGB(255, 255, 102));
	Handle.green       = CreateBrush(RGB(102, 204,  51));
	Handle.lightgreen  = CreateBrush(RGB(153, 255, 102));
	Handle.red         = CreateBrush(RGB(255, 102,  51));
	Handle.lightred    = CreateBrush(RGB(255, 153, 102));
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

	// Make child windows and menus
	Handle.list = WindowCreateEdit(true,  false); // Edit controls
	Handle.tabs = WindowCreateEdit(false, false);
	Handle.edit = WindowCreateEdit(true,  true);
	WindowEdit(Handle.list, true); // Start out edit controls read-only
	WindowEdit(Handle.tabs, true);
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

	// Run the message loop until the user closes the program
	MSG message;
	while (GetMessage(&message, NULL, 0, 0)) { // Returns false on the final message
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	return (int)message.wParam; // Return the exit code in the final message
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

			//TODO

		break;
		}

	// The message is a command
	break;
	case WM_COMMAND:

		// The user clicked a menu item or button
		if (HIWORD(wparam) == BN_CLICKED) {

			/*
			// The user clicked the Test menu item
			if      ((HWND)lparam == Handle.clear) {  } // Button child window controls
			else if ((HWND)lparam == Handle.start) {  }
			else if ((HWND)lparam == Handle.stop)  {  }
			else if ((HWND)lparam == Handle.reset) {  }
			else if ((HWND)lparam == Handle.task)  { // The user clicked the Task button

				// Show the context menu beneath the task button
				RECT rectangle;
				if (!GetWindowRect(Handle.task, &rectangle)) Report(L"getwindowrect");
				sizeitem size(rectangle);
				size.y += size.h;
				size.Client();
				UINT choice = MenuShow(Handle.menutools, false, &size); // Wait here for the user to make a choice
				if      (choice == ID_TOOLS_TEST)    { Test(); }
				else if (choice == ID_TOOLS_OPEN)    {  }
				else if (choice == ID_TOOLS_ADD)     {  }
				else if (choice == ID_TOOLS_NEW)     {  }
				else if (choice == ID_TOOLS_HELP)    {  }
				else if (choice == ID_TOOLS_ABOUT)   {  }
				else if (choice == ID_TOOLS_OPTIONS) {  }
				else if (choice == ID_TOOLS_EXIT)    {

					// Close the program
					PostQuitMessage(0);
					return 0;
				}
			}
			*/
		}

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
