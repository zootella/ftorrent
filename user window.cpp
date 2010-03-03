
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
	WNDCLASSEX windowclass;
	ZeroMemory(&windowclass, sizeof(windowclass));
	windowclass.cbSize        = sizeof(windowclass);
	windowclass.style         = 0;
	windowclass.lpfnWndProc   = MainWinProc;
	windowclass.cbClsExtra    = 0;
	windowclass.cbWndExtra    = 0;
	windowclass.hInstance     = Handle.instance;
	windowclass.hIcon         = (HICON)LoadImage(Handle.instance, L"APPLICATION_ICON", IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	windowclass.hIconSm       = (HICON)LoadImage(Handle.instance, L"APPLICATION_ICON", IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	windowclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	windowclass.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	windowclass.lpszMenuName  = NULL;
	windowclass.lpszClassName = L"LTorrentClassName";
	ATOM result = RegisterClassEx(&windowclass);
	if (!result) Report(L"error registerclassex");
	Handle.window = WindowCreate(L"LTorrentClassName", PROGRAMNAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, NULL, NULL);

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
	Handle.menu = MenuLoad(L"TOOLS_MENU"); // Menu

	// Prepare the window to show current information, and make the correct controls available
	WindowUpdate(); // Tries to paint now, but doesn't because the window isn't on the screen yet

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

			// Update the window with current information from the job
			WindowUpdate();

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
				UINT choice = MenuShow(Handle.menu, rectangle.left, rectangle.bottom); // Wait here for the user to make a choice
				if      (choice == ID_TASK_DELETE)        {  }
				else if (choice == ID_TASK_COPY)          {  }
				else if (choice == ID_TASK_COMPARE)       {  }
				else if (choice == ID_TASK_UPDATE)        {  }
				else if (choice == ID_TASK_UPDATECOMPARE) {  }
			}
		}

	// The user clicked the X, and the system has removed the window from the screen
	break;
	case WM_DESTROY:

		// Close the program
		PostQuitMessage(0);
		return 0;
	}

	// Have the system process the message
	return DefWindowProc(window, message, wparam, lparam);
}
