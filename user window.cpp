
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
handleitem Handle; // No extern, this is where the global objects are made

// Start the program
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR command, int show) {

	// Save and initialize handles
	Handle.instance = instance;
	InitializeCriticalSection(&Handle.section);

	// Creating painting tools
	PaintCreate();

	// Register the class for the main window, and create it
	WNDCLASSEX info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize        = sizeof(info);
	info.style         = 0;
	info.lpfnWndProc   = MainWinProc;
	info.cbClsExtra    = 0;
	info.cbWndExtra    = 0;
	info.hInstance     = Handle.instance;
	info.hIcon         = Handle.iconbig;
	info.hIconSm       = Handle.iconsmall;
	info.hCursor       = LoadCursor(NULL, IDC_ARROW);
	info.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	info.lpszMenuName  = NULL;
	info.lpszClassName = L"ltorrentClassName";
	ATOM result = RegisterClassEx(&info);
	if (!result) Report(L"error registerclassex");
	Handle.window = WindowCreate(L"ltorrentClassName", PROGRAM_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, NULL, NULL);

	// Add Exit to the main window's system menu
	HMENU m = GetSystemMenu(Handle.window, false); // Get the menu for editing
	if (!m) Report(L"error getsystemmenu");
	if (m && !AppendMenu(m, MF_STRING, ID_TOOLS_EXIT, L"&Exit")) Report(L"error appendmenu");

	// Make child windows and menus
	Handle.tasks  = WindowCreateEdit(true,  false); // Edit controls
	Handle.status = WindowCreateEdit(false, false);
	Handle.errors = WindowCreateEdit(true,  true);
	WindowEdit(Handle.tasks,  false); // Start out edit controls read-only
	WindowEdit(Handle.status, false);
	WindowEdit(Handle.errors, false);
	Handle.clear = WindowCreateButton(L"Clear"); // Buttons
	Handle.task  = WindowCreateButton(L"Task");
	Handle.start = WindowCreateButton(L"Start");
	Handle.stop  = WindowCreateButton(L"Stop");
	Handle.reset = WindowCreateButton(L"Reset");

	// Prepare the window to show current information, and make the correct controls available
	//TODO // Tries to paint now, but doesn't because the window isn't on the screen yet

	// Show the child windows and then the main window
	ShowWindow(Handle.tasks,  SW_SHOWNORMAL);
	ShowWindow(Handle.status, SW_SHOWNORMAL);
	ShowWindow(Handle.errors, SW_SHOWNORMAL);
	ShowWindow(Handle.clear,  SW_SHOWNORMAL);
	ShowWindow(Handle.task,   SW_SHOWNORMAL);
	ShowWindow(Handle.start,  SW_SHOWNORMAL);
	ShowWindow(Handle.stop,   SW_SHOWNORMAL);
	ShowWindow(Handle.reset,  SW_SHOWNORMAL);
	ShowWindow(Handle.window, SW_SHOWNORMAL); // Calling this causes a paint message right now
	PaintMessage(Handle.window); // Necessary to draw child window controls

	// Start the pulse timer
	if (!SetTimer(Handle.window, TIMER_PULSE, PULSE, NULL)) Report(L"error settimer");

	// Run the message loop until the user closes the program
	MSG message;
	while (GetMessage(&message, NULL, 0, 0)) { // Returns false on the final message
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	return (int)message.wParam; // Return the exit code in the final message
}

// Process a message from the system
LRESULT CALLBACK MainWinProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {

	// The user sized the window, the first size message happens before the first paint
	switch (message) {
	case WM_SIZE:

		// Repaint the window right now
		PaintMessage(Handle.window);
		return 0;

	// The parts of the client area not covered by child window controls need to be painted
	break;
	case WM_PAINT:

		// Paint the client area
		Paint();
		return 0;

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

			// The user clicked the Test menu item
			if      ((HWND)lparam == Handle.clear) {  } // Button child window controls
			else if ((HWND)lparam == Handle.start) {  }
			else if ((HWND)lparam == Handle.stop)  {  }
			else if ((HWND)lparam == Handle.reset) {  }
			else if ((HWND)lparam == Handle.task)  { // The user clicked the Task button

				// Show the context menu beneath the task button
				RECT rectangle;
				if (!GetWindowRect(Handle.task, &rectangle)) Report(L"error getwindowrect");
				sizeitem size(rectangle);
				size.addy(size.h());
				size.client();
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
	MenuSet(Handle.menutaskbar, ID_TASKBAR_RESTORE, MFS_DEFAULT, HBMMENU_POPUP_RESTORE);
	UINT choice = MenuShow(Handle.menutaskbar, true, NULL); // Wait here while the menu is up

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
