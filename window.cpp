
#include "include.h" // Include headers and definitions
app App; // Create global object

// Start the program
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR command, int show) {

	// Save the given instance handle and command text
	App.window.instance = instance;
	App.cycle.command = widenPtoC(command);

	// Load options
	App.cycle.portable = DiskIsFile(PathPortable()); // Look for the portable marker
	OptionLoad(); // Load optn.db from last time into App.option variables

	// Tell the system we're going to use COM, OLE, and the list and tree view controls
	InitializeSystem();

	// Set up the program image list
	StartIcon();

	// Load menus
	HMENU menus = MenuLoad(L"CONTEXT_MENU");
	App.menu.taskbar  = MenuClip(menus, 0);
	App.menu.tools    = MenuClip(menus, 1);
	App.menu.torrents = MenuClip(menus, 2);
	App.menu.torrent  = MenuClip(menus, 3);
	App.menu.trackers = MenuClip(menus, 4);
	App.menu.tracker  = MenuClip(menus, 5);
	App.menu.peers    = MenuClip(menus, 6);
	App.menu.peer     = MenuClip(menus, 7);
	App.menu.files    = MenuClip(menus, 8);
	App.menu.file     = MenuClip(menus, 9);
	if (!PROGRAM_TEST && !DeleteMenu(App.menu.tools, IdentifyToolsTest, 0)) error(L"deletemenu"); // Remove the test menu item

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
	info.hInstance     = App.window.instance;        // Instance handle
	info.hIcon         = NULL;                       // No icons yet
	info.hIconSm       = NULL;
	info.hCursor       = NULL;                       // Mouse cursor changes
	info.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Background brush
	info.lpszMenuName  = NULL;                       // No menu
	info.lpszClassName = name;                       // Window class name
	if (!RegisterClassEx(&info)) error(L"registerclassex");
	App.window.main = WindowCreate(name, PROGRAM_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, NULL, NULL);

	// Make the main window a drop target so the user can drag links and files into it
	if (RegisterDragDrop(App.window.main, new Target(App.window.main)) != S_OK) error(L"registerdragdrop");

	// Add Exit to the main window's system menu
	HMENU menu = GetSystemMenu(App.window.main, false); // Get the menu for editing
	if (!menu) error(L"getsystemmenu");
	if (menu && !AppendMenu(menu, MF_STRING, IdentifyToolsExit, L"&Exit")) error(L"appendmenu");

	// Create the list view windows
	App.list.torrents.window = WindowCreateList();
	App.list.trackers.window = WindowCreateList();
	App.list.peers.window    = WindowCreateList();
	App.list.pieces.window   = WindowCreateList();
	App.list.files.window    = WindowCreateList();

	// Create the list view window that lists the torrents at the top
	ColumnListToWindow(App.list.torrents.window, App.list.torrents.current);
	ColumnListToWindow(App.list.trackers.window, App.list.trackers.current);
	ColumnListToWindow(App.list.peers.window,    App.list.peers.current);
	ColumnListToWindow(App.list.pieces.window,   App.list.pieces.current);
	ColumnListToWindow(App.list.files.window,    App.list.files.current);

	// Create the tabs window
	App.window.tabs = WindowCreateTabs();
	AddTab(App.window.tabs, 0, L"Torrent"); // Add the tabs
	AddTab(App.window.tabs, 1, L"Trackers");
	AddTab(App.window.tabs, 2, L"Peers");
	AddTab(App.window.tabs, 3, L"Pieces");
	AddTab(App.window.tabs, 4, L"Files");
	AddTab(App.window.tabs, 5, L"Speed");
	AddTab(App.window.tabs, 6, L"Log");

	// Make child windows and menus
	App.window.log = WindowCreateEdit(true, true);
	WindowEdit(App.window.log, true); // Let the user type text in the box

	// Create the tooltip window
	App.window.tip = WindowCreateTip();

	// Make the areas of the window like the buttons and sizing grips
	AreaCreate();
	AreaPulse(); // Choose the program stage and set the display state of each area

	// Lower the window
	Size size = SizeWindow(App.window.main);
	size.ShiftTop((size.h * 1) / 4);
	WindowMove(App.window.main, size, false);

	// Show the child windows and then the main window
	ShowWindow(App.list.torrents.window, SW_SHOWNORMAL);
	ShowWindow(App.window.tabs,          SW_SHOWNORMAL);
	ShowWindow(App.list.files.window,    SW_SHOWNORMAL);
	ShowWindow(App.window.main,          SW_SHOWNORMAL); // Calling this causes a paint message right now
	PaintMessage(); // Necessary to draw child window controls

	// Make sure we can edit files next to this running exe
	if (!DiskFolder(PathRunningFolder(), true, true)) {
		Message("Cannot edit files beside '" + PathRunningFile() + L"'. Run as administrator or move the program to try again.");
		return 0; // Exit the process
	}

	// Start the pulse timer
	TimerSet(TIMER_PULSE, 300);

	if (App.option.associate) AssociateGet();
	FirewallAdd(PathRunningFile(), PROGRAM_NAME); // A hang on run bug may have been avoided by commenting this out
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

	OptionSave(); // Save App.option varibles to optn.db for next time
	LibraryClose(); // Close libtorrent, this can be quick or take several seconds, but shouldn't exceed exit_time_limit
	FirewallRemove(PathRunningFile());

	// Log how long it took from when the user clicked exit to winmain returning and the process exiting
	DWORD exittime = GetTickCount() - App.cycle.exit;
	CString exitmessage = make(L"exited in ", InsertCommas(numerals(exittime)), L"ms");
	if (exittime > (EXIT_TIME_LIMIT * 1000) + 500) exitmessage += CString(L", which is beyond the maximum time limit which should be possible!").MakeUpper();
	log(exitmessage);

	// Return the value from the quit message
	return (int)message.wParam;
}

// Hide the window stop libtorrent
void WindowExit() {

	ShowWindow(App.window.main, SW_HIDE); // Hide the window
	TaskbarIconRemove();                  // Remove the taskbar icon if we have one

	App.cycle.exit = GetTickCount(); // Record that the user gave the exit command and when it happened
	LibraryStop();                   // Ask libtorrent to prepare save data for each torrent
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

		App.area.pressed = MouseOver();                                     // Save the area the mouse pressed
		if (App.area.pressed) App.area.stick = MouseArea(App.area.pressed); // Save where in that area it started dragging
		MouseCapture();                                                     // Have our window get all the mouse messages
		if (App.area.pressed && App.area.pressed->command == CommandMenu)   // Show a menu on the downclick
			AreaDoCommand(App.area.pressed);
		return 0;

	// The primary mouse button has clicked up in the main window, or anywhere if the mouse is captured
	break;
	case WM_LBUTTONUP:

		if (App.area.pressed && App.area.pressed == MouseOver()) // If the mouse released the same button it clicked
			AreaDoCommand(App.area.pressed);                     // Perform that command
		App.area.pressed = NULL;                                 // Clear our record of the area the mouse pressed
		MouseRelease();                                          // Stop getting all mouse messages
		return 0;

	// A new window has captured the mouse
	break;
	case WM_CAPTURECHANGED:

		// The program has lost the mouse capture, mark no area pressed
		if (App.window.main != (HWND)lparam) App.area.pressed = NULL;

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
		case IdentifyToolsExit:

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

	// Notification from a common control
	break;
	case WM_NOTIFY:

		// The user pressed a key in the control
		HWND window = ((LPNMHDR)lparam)->hwndFrom; // Get the window handle of the common control the notification is about
		switch (((LPNMLISTVIEW)lparam)->hdr.code) {
		case NM_SETFOCUS:     NotifySetFocus(window);                                      break; // The list view control window has gained the focus
		case NM_DBLCLK:       NotifyDoubleClick(window);                                   break; // The user double clicked
		case NM_RCLICK:       NotifyRightClick(window);                                    break; // The user right clicked
		case LVN_COLUMNCLICK: NotifyColumnClick(window, ((LPNMLISTVIEW)lparam)->iSubItem); break; // The user clicked a column
		case LVN_KEYDOWN:

			switch (((LPNMLVKEYDOWN)lparam)->wVKey) {
			case VK_RETURN: NotifyKeyEnter(window);  break; // The user pressed the Enter key
			case VK_DELETE: NotifyKeyDelete(window); break; // The user pressed the Delete key
			}

			if (GetKeyState(VK_CONTROL) & 0x8000) {
				switch (((LPNMLVKEYDOWN)lparam)->wVKey) {
				case 'A': NotifyKeyControlA(window); break; // The user pressed Control A to select all
				case 'C': NotifyKeyControlC(window); break; // The user pressed Control C to copy
				case 'V': NotifyKeyControlV(window); break; // The user pressed Control V to paste
				}
			}

		// A drawing operation is happening
		break;
		case NM_CUSTOMDRAW:

			LPNMLVCUSTOMDRAW draw = (LPNMLVCUSTOMDRAW)lparam; // Point to the custom draw structure
			switch (draw->nmcd.dwDrawStage) {
			case CDDS_PREPAINT:                    return CDRF_NOTIFYITEMDRAW;                     break; // Draw the control, request the next message
			case CDDS_ITEMPREPAINT:                return CDRF_NOTIFYITEMDRAW;                     break; // Draw an item, request the next message TODO what if status is in the first one
			case CDDS_SUBITEM | CDDS_ITEMPREPAINT: if (PaintCustom(draw)) return CDRF_SKIPDEFAULT; break; // Draw a subitem, do custom drawing and have the control skip default drawing
			}

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
	MenuSet(App.menu.taskbar, IdentifyTaskbarRestore, MFS_DEFAULT, HBMMENU_POPUP_RESTORE);
	int choice = MenuShow(App.menu.taskbar, true, NULL); // Wait here while the menu is up

	// Restore
	switch (choice) {
	case IdentifyTaskbarRestore:

		// Restore from taskbar
		TaskbarIconRemove();
		ShowWindow(App.window.main, SW_SHOW);
		if (IsIconic(App.window.main)) ShowWindow(App.window.main, SW_RESTORE);

	// Exit
	break;
	case IdentifyTaskbarExit:

		// Hide the window and stop libtorrent
		WindowExit();

	break;
	}
}








void NotifySetFocus(HWND window) {
	log(L"set focus");
}
void NotifyDoubleClick(HWND window) {
	log(L"double click");
}
void NotifyRightClick(HWND window) {
	int row = ListMouse(window);
	log(L"right click, row ", numerals(row));

	if (window == App.list.torrents.window && !row) {

		int choice = MenuShow(App.menu.torrents);
		if      (choice == IdentifyTorrentsAddMagnet)     { Dialog(L"DIALOG_ADD",    DialogAdd);    }
		else if (choice == IdentifyTorrentsCreateTorrent) { Dialog(L"DIALOG_CREATE", DialogCreate); }

	} else if (window == App.list.torrents.window && row) {

		// Set current area command states based on which rows are selected in the list view
		AreaPulse(); // If the right click selected a row, a regular pluse might not have done this already

		// Set menu items available or grayed based on area command states
		MenuSet(App.menu.torrent, IdentifyTorrentOpen,                 App.area.open.command        == CommandUnavailable ? MFS_DISABLED : MFS_ENABLED);
		MenuSet(App.menu.torrent, IdentifyTorrentOpenContainingFolder, App.area.openfolder.command  == CommandUnavailable ? MFS_DISABLED : MFS_DEFAULT); // Open folder is always the default
		MenuSet(App.menu.torrent, IdentifyTorrentCopyMagnetLink,       App.area.copymagnet.command  == CommandUnavailable ? MFS_DISABLED : MFS_ENABLED);
		MenuSet(App.menu.torrent, IdentifyTorrentSaveTorrentAs,        App.area.savetorrent.command == CommandUnavailable ? MFS_DISABLED : MFS_ENABLED);
		MenuSet(App.menu.torrent, IdentifyTorrentStart,                App.area.start.command       == CommandUnavailable ? MFS_DISABLED : MFS_ENABLED);
		MenuSet(App.menu.torrent, IdentifyTorrentPause,                App.area.pause.command       == CommandUnavailable ? MFS_DISABLED : MFS_ENABLED);
		MenuSet(App.menu.torrent, IdentifyTorrentRemove,               App.area.remove.command      == CommandUnavailable ? MFS_DISABLED : MFS_ENABLED);
		MenuSet(App.menu.torrent, IdentifyTorrentDelete,               App.area.deletefiles.command == CommandUnavailable ? MFS_DISABLED : MFS_ENABLED);

		// Show the context menu and get the user's choice
		int choice = MenuShow(App.menu.torrent);
		int rows = ListRows(window); // Find out how many rows there are to be able to loop through them

		// Call the corresponding method on the torrents behind each selected row, loop up from the bottom to be able to delete rows without changing upcoming indices in the loop
		if      (choice == IdentifyTorrentOpen)                 { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UseOpen();                 } } }
		else if (choice == IdentifyTorrentOpenContainingFolder) { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UseOpenContainingFolder(); } } }
		else if (choice == IdentifyTorrentSaveTorrentAs)        { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UseSaveTorrentAs();        } } }
		else if (choice == IdentifyTorrentStart)                { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UseStart();                } } }
		else if (choice == IdentifyTorrentPause)                { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UsePause();                } } }
		else if (choice == IdentifyTorrentRemove)               { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UseRemove();               } } }
		else if (choice == IdentifyTorrentDelete)               { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UseDelete();               } } }
		else if (choice == IdentifyTorrentCopyMagnetLink) {

			// Copy the selected magnet links to the clipboard
			CString copy;
			for (int i = 0; i < rows; i++) { // Loop forwards from the start so the text and row order match
				if (ListSelected(window, i)) {
					Torrent *t = ListGetTorrent(i);
					if (t) t->UseCopyMagnetLink(&copy);
				}
			}
			if (is(copy)) ClipboardCopy(copy);
		}

	} else if (window == App.list.trackers.window && !row) {

		int choice = MenuShow(App.menu.trackers);

	} else if (window == App.list.trackers.window && row) {

		int choice = MenuShow(App.menu.tracker);

	} else if (window == App.list.peers.window && !row) {

		int choice = MenuShow(App.menu.peers);

	} else if (window == App.list.peers.window && row) {

		int choice = MenuShow(App.menu.peer);

	} else if (window == App.list.files.window && !row) {

		int choice = MenuShow(App.menu.files);

	} else if (window == App.list.files.window && row) {

		int choice = MenuShow(App.menu.file);
	}



}
void NotifyColumnClick(HWND window, int column) {
	log(L"column click, number ", numerals(column));
}
void NotifyKeyEnter(HWND window) {
	log(L"enter");
}
void NotifyKeyDelete(HWND window) {
	log(L"delete");
}
void NotifyKeyControlA(HWND window) {
	log(L"control a");
}
void NotifyKeyControlC(HWND window) {
	log(L"control c");
}
void NotifyKeyControlV(HWND window) {
	log(L"control v");
}

//TODO right click is appearing when it's in the column, but how do you get it only when it's in the column
















// Called after each group of messages and every .3 seconds
// Updates the program's data and display
void WindowPulse() {

	// Pulse the program from data to display
	LibraryPulse();
	StorePulse();
	ListPulse();
	AreaPulse();
}





void StorePulse() {

	// Only do this once
	if (App.cycle.restored) return;
	App.cycle.restored = true;

	// Add all the torrents from last time the program ran
	std::set<hbig> hashes;
	Find f(PathRunningFolder(), true);
	while (f.Result()) { // Loop for each file in the folder this exe is running in
		if (!f.Folder()) {
			CString s = f.info.cFileName;
			if (length(s) == 40 + length(L".optn.db") && trails(s, L".optn.db", Matching)) { // Look for "infohash.optn.db"

				hbig hash = ParseHash(clip(s, 0, 40));
				if (!hash.is_all_zeros()) hashes.insert(hash); // Only collect unique nonzero hashes
			}
		}
	}
	for (std::set<hbig>::const_iterator i = hashes.begin(); i != hashes.end(); i++) AddStore(*i);

	// Add the torrent or magnet the system launched this program with
	CString s = App.cycle.command;
	if (starts(s, L"\"")) { // Parse the command like ["C:\Folder\file.torrent" /more /arguments]
		s = after(s, L"\"");
		if (has(s, L"\"")) {
			s = before(s, L"\"");
			if (starts(s, L"magnet:", Matching)) AddMagnet(s, false); // Look for magnet first because link text might also end torrent
			else if (trails(s, L".torrent", Matching)) AddTorrent(s, false);
		}
	}
}




void ListPulse() {



	for (int i = 0; i < (int)App.torrents.size(); i++)
		App.torrents[i].Edit();


	App.list.refresh = false; // We filled any blank cells caused by newly added columns


}







