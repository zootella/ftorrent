
#include "include.h" // Include headers and definitions
app App; // Create global object

handletop Handle;
areatop   Areas;
datatop   Data;
statetop  State;

// Start the program
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR command, int show) {

	// Save the given instance handle and command text
	App.instance = instance;
	State.command = widenPtoC(command);

	// Look for the portable marker
	State.portable = DiskIsFile(PathPortable());

	// Tell the system we're going to use the list and tree view controls
	InitializeCommonControls();

	// Set up the program image list
	StartIcon();

	// Load menus
	HMENU menus = MenuLoad(L"CONTEXT_MENU");
	App.menu.tray  = MenuClip(menus, 0);
	App.menu.tools = MenuClip(menus, 1);
	if (!PROGRAM_TEST && !DeleteMenu(App.menu.tools, ID_TOOLS_TEST, 0)) error(L"deletemenu"); // Remove the test menu item

	// Load cursors
	App.cursor.arrow      = LoadSharedCursor(IDC_ARROW);
	App.cursor.hand       = LoadSharedCursor(IDC_HAND);
	App.cursor.horizontal = LoadSharedCursor(IDC_SIZEWE);
	App.cursor.vertical   = LoadSharedCursor(IDC_SIZENS);
	App.cursor.diagonal   = LoadSharedCursor(IDC_SIZENWSE);

	// Make brushes
	App.brush.face       = BrushSystem(COLOR_3DFACE); // Shared handles to system brushes
	App.brush.shadow     = BrushSystem(COLOR_3DSHADOW);
	App.brush.background = BrushSystem(COLOR_WINDOW);
	App.brush.ink        = BrushSystem(COLOR_WINDOWTEXT);
	App.brush.select     = BrushSystem(COLOR_HIGHLIGHT);
	App.brush.line       = CreateBrush(ColorMix(GetSysColor(COLOR_3DFACE), 1, GetSysColor(COLOR_3DSHADOW), 1)); // Mix

	// Make fonts
	App.font.normal    = FontMenu(false);
	App.font.underline = FontMenu(true);
	App.font.arial     = FontName(L"Arial", 28);

	// Register the class for the main window, and create it
	CString name = PROGRAM_NAME + L"ClassName"; // Compose a unique window class name
	WNDCLASSEX info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize        = sizeof(info);               // Size of this structure
	info.style         = 0;                          // Default style
	info.lpfnWndProc   = WindowProcedure;            // Function pointer to the window procedure
	info.cbClsExtra    = 0;                          // No extra bytes
	info.cbWndExtra    = 0;
	info.hInstance     = App.instance;               // Instance handle
	info.hIcon         = NULL;                       // No icons yet
	info.hIconSm       = NULL;
	info.hCursor       = NULL;                       // Mouse cursor changes
	info.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Background brush
	info.lpszMenuName  = NULL;                       // No menu
	info.lpszClassName = name;                       // Window class name
	if (!RegisterClassEx(&info)) error(L"registerclassex");
	App.window.main = WindowCreate(name, PROGRAM_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, NULL, NULL);

	// Add Exit to the main window's system menu
	HMENU menu = GetSystemMenu(App.window.main, false); // Get the menu for editing
	if (!menu) error(L"getsystemmenu");
	if (menu && !AppendMenu(menu, MF_STRING, ID_TOOLS_EXIT, L"&Exit")) error(L"appendmenu");

	// Create the list view window
	DWORD style =
		WS_CHILD             | // Required for child windows
		LVS_REPORT           | // Specifies report view, I think this puts it in details
		LBS_EXTENDEDSEL      | // Allows multiple items to be selected
		LVS_SHOWSELALWAYS    | // Shows the selection even when the control doesn't have the focus
		LBS_NOINTEGRALHEIGHT | // Allows the size to be specified exactly without snap
		LVS_SHAREIMAGELISTS;   // Will not delete the system image list when the control is destroyed
	App.window.list = WindowCreate(WC_LISTVIEW, NULL, style, 0, App.window.main, (HMENU)WINDOW_LIST);

	// Use the program image list
	ListView_SetImageList(App.window.list, App.icon.list, LVSIL_SMALL);

	// Load extended list view styles, requires common control 4.70
	style = LVS_EX_LABELTIP  | // Unfold partially hidden labels in tooltips
		LVS_EX_FULLROWSELECT | // When an item is selected, highlight it and all its subitems
		LVS_EX_SUBITEMIMAGES;  // Let subitems have icons
	ListView_SetExtendedListViewStyle(App.window.list, style);

	// Determine how wide the columns should be
	std::vector<int> widths;
	widths.push_back(-4);  // Status
	widths.push_back(200); // Name
	widths.push_back(-5);  // Size
	widths.push_back(100); // Infohash
	widths.push_back(-10); // Location
	widths = SizeColumns(widths);

	// Add the first column, which won't be able to show its icon on the right
	ListColumnInsert(App.window.list, 0, LVCFMT_LEFT, App.icon.clear, L"", 0);

	// Add the columns
	ListColumnInsert(App.window.list, 1, LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT, App.icon.clear, L"Status",   widths[0]);
	ListColumnInsert(App.window.list, 2, LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT, App.icon.clear, L"Name",     widths[1]);
	ListColumnInsert(App.window.list, 3, LVCFMT_RIGHT,                         App.icon.clear, L"Size",     widths[2]);
	ListColumnInsert(App.window.list, 4, LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT, App.icon.clear, L"Infohash", widths[3]);
	ListColumnInsert(App.window.list, 5, LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT, App.icon.clear, L"Location", widths[4]);

	// Remove the first column so all the remaining columns can show their icons on the right
	ListColumnDelete(App.window.list, 0);

	// Create the tabs window
	style = WS_CHILD;            // Required for child windows
	App.window.tabs = WindowCreate(WC_TABCONTROL, NULL, style, 0, App.window.main, (HMENU)WINDOW_TABS);
	SendMessage(                 // Have it use Tahoma, not the default system font
		(HWND)App.window.tabs,   // Send the message to this window
		WM_SETFONT,              // Message to send
		(WPARAM)App.font.normal, // Handle to font
		0);                      // Don't tell the control to immediately redraw itself

	// Add the tabs
	AddTab(App.window.tabs, 0, L"Torrent");
	AddTab(App.window.tabs, 1, L"Trackers");
	AddTab(App.window.tabs, 2, L"Peers");
	AddTab(App.window.tabs, 3, L"Pieces");
	AddTab(App.window.tabs, 4, L"Files");
	AddTab(App.window.tabs, 5, L"Speed");
	AddTab(App.window.tabs, 6, L"Log");

	// Make child windows and menus
	App.window.edit = WindowCreateEdit(true,  true);
	WindowEdit(App.window.edit, true);

	// Create the tooltip window
	style =
		WS_POPUP |     // Popup window instead of a child window
		TTS_ALWAYSTIP; // Show the tooltip even if the window is inactive
	App.window.tip = WindowCreate(TOOLTIPS_CLASS, NULL, style, 0, App.window.main, NULL);

	// Make the tooltip topmost
	int result = SetWindowPos(
		App.window.tip,  // Handle to window
		HWND_TOPMOST,    // Place the window above others without this setting
		0, 0, 0, 0,      // Retain the current position and size
		SWP_NOMOVE |
		SWP_NOSIZE |
		SWP_NOACTIVATE); // Do not activate the window
	if (!result) error(L"setwindowpos");

	// Make the areas of the window like the buttons and sizing grips
	AreaCreate();
	AreaPulse(); // Choose the program stage and set the display state of each area

	// Lower the window
	Size size = SizeWindow(App.window.main);
	size.ShiftTop((size.h * 1) / 4);
	WindowMove(App.window.main, size, false);

	// Show the child windows and then the main window
	ShowWindow(App.window.list, SW_SHOWNORMAL);
	ShowWindow(App.window.tabs, SW_SHOWNORMAL);
	ShowWindow(App.window.edit, SW_SHOWNORMAL);
	ShowWindow(App.window.main,  SW_SHOWNORMAL); // Calling this causes a paint message right now
	PaintMessage(); // Necessary to draw child window controls

	// Make sure we can edit files next to this running exe
	if (!DiskFolder(PathRunningFolder(), true, true)) {
		Message("Cannot edit files beside '" + PathRunningFile() + L"'. Run as administrator or move the program to try again.");
		return 0; // Exit the process
	}

	// Start the pulse timer
	TimerSet(TIMER_PULSE, 300);

	OptionLoad(); // Load optn.db from last time into Data variables
	if (Data.associate) AssociateGet();
//	FirewallAdd(PathRunningFile(), PROGRAM_NAME); //TODO commenting this out may solve the hang on run bug
	log(L"library start before");
	LibraryStart(); // Start libtorrent
	log(L"library start after");

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

	OptionSave(); // Save Data varibles to optn.db for next time
	log(L"library close before");
	LibraryClose(); // Close libtorrent, this can be quick or take several seconds
	log(L"library close after");
//	FirewallRemove(PathRunningFile());

	// Return the value from the quit message
	return (int)message.wParam;
}

// Called after each group of messages and every .3 seconds
// Updates the program's data and display
void WindowPulse() {

	// Pulse the program from data to display
	LibraryPulse();
	StorePulse();
	ListPulse();
	AreaPulse();
}

// Hide the window stop libtorrent
void WindowExit() {

	ShowWindow(App.window.main, SW_HIDE); // Hide the window
	TaskbarIconRemove();                  // Remove the taskbar icon if we have one

	State.exit = GetTickCount(); // Record that the user gave the exit command and when it happened
	LibraryStop();               // Ask libtorrent to prepare save data for each torrent
}

// Process a message from the system
LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {

	// The user sized the window, the first size message happens before the first paint
	switch (message) {
	case WM_SIZE:

		// Size the window and send it a paint message
		Layout();
		PaintMessage();
		return 0;

	// The parts of the client area not covered by child window controls need to be painted
	break;
	case WM_PAINT:
	{
		// Paint the window
		Device device;
		device.OpenPaint(window);
		device.Font(App.font.normal); // Replace the Windows 3.1 font with something more modern
		device.BackgroundColor(App.brush.background.color);
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

		Areas.pressed = MouseOver();                                // Save the area the mouse pressed
		if (Areas.pressed) Areas.stick = MouseArea(Areas.pressed);  // Save where in that area it started dragging
		MouseCapture();                                             // Have our window get all the mouse messages
		if (Areas.pressed && Areas.pressed->command == CommandMenu) // Show a menu on the downclick
			AreaDoCommand(Areas.pressed);
		return 0;

	// The primary mouse button has clicked up in the main window, or anywhere if the mouse is captured
	break;
	case WM_LBUTTONUP:

		if (Areas.pressed && Areas.pressed == MouseOver()) // If the mouse released the same button it clicked
			AreaDoCommand(Areas.pressed);                  // Perform that command
		Areas.pressed = NULL;                              // Clear our record of the area the mouse pressed
		MouseRelease();                                    // Stop getting all mouse messages
		return 0;

	// A new window has captured the mouse
	break;
	case WM_CAPTURECHANGED:

		// The program has lost the mouse capture, mark no area pressed
		if (App.window.main != (HWND)lparam) Areas.pressed = NULL;

	// Another window is copying data to this one
	break;
	case WM_COPYDATA:
	{
		CString s = (read)(((PCOPYDATASTRUCT)lparam)->lpData); // Copy the null terminated wide characters into a string
		if (starts(s, L"magnet:", Matching)) AddMagnet(s, false); // Look for magnet first because link text might also end torrent
		else if (trails(s, L".torrent", Matching)) AddTorrent(s, false);
	}
	// The system has removed the window from the screen
	break;
	case WM_DESTROY:

		PostQuitMessage(0); // Post the quit message to leave the message loop
		return 0;           // We processed this message

	// The user clicked a window button or menu item
	break;
	case WM_SYSCOMMAND:

		// The user clicked the close X or window menu item
		switch (LOWORD(wparam)) {
		case SC_CLOSE:

			// Hide the window and add the taskbar notification icon
			ShowWindow(App.window.main, SW_HIDE);
			TaskbarIconAdd();

			// Prevent the program from closing
			return 0;

		// The user clicked the Exit window menu item that we added
		break;
		case ID_TOOLS_EXIT:

			// Hide the window and stop libtorrent
			WindowExit();
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
			ShowWindow(App.window.main, SW_SHOW);
			if (IsIconic(App.window.main)) ShowWindow(App.window.main, SW_RESTORE);

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
	MenuSet(App.menu.tray, ID_TASKBAR_RESTORE, MFS_DEFAULT, HBMMENU_POPUP_RESTORE);
	UINT choice = MenuShow(App.menu.tray, true, NULL); // Wait here while the menu is up

	// Restore
	switch (choice) {
	case ID_TASKBAR_RESTORE:

		// Restore from taskbar
		TaskbarIconRemove();
		ShowWindow(App.window.main, SW_SHOW);
		if (IsIconic(App.window.main)) ShowWindow(App.window.main, SW_RESTORE);

	// Exit
	break;
	case ID_TASKBAR_EXIT:

		// Hide the window and stop libtorrent
		WindowExit();

	break;
	}
}
