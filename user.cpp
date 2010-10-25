
#include "include.h"

// Access to global objects
extern handletop Handle;
extern areatop   Areas;
extern datatop   Data;
extern statetop  State;

// Set up the program image list
void StartIcon() {

	// Initialize ext with a guid so the first requested extension will never match
	Handle.icon.ext = TextGuid();

	// Create the program image list
	Handle.icon.list = ImageList_Create(
		16, 16,         // Image size
		ILC_COLOR32 |   // Windows XP style 32 bit antialiased icons
		ILC_MASK,       // Show icon transparency
		0,              // Start with no icons
		ICON_CAPACITY); // Be able to grow to hold this many more icons
	if (!Handle.icon.list) error(L"imagelist_create");

	// Load the resource icons into the program image list and get their indices there, or -1 if not loaded
	Handle.icon.clear      = IconAddResource(L"CLEAR_ICON");
	Handle.icon.ascending  = IconAddResource(L"COLUMN_ASCENDING");
	Handle.icon.descending = IconAddResource(L"COLUMN_DESCENDING");

	// Load the shell icon for a file
	CString type;
	Handle.icon.file = IconGet(L"", &type);
}

// Takes a device context to use to paint in the window
// Paints the client area of the window outside the child window controls, and resizes the child window controls
void PaintWindow(Device *device) {

	// Get the size of the client area
	Size client = SizeClient();

	// Define sizes
	int margin = 8; // Margin to the left or right of stage text
	int above = -7; // Draw the stage text this many pixels above the client area

	// Paint the area between the toolbar buttons and the stage text
	Size s1 = Areas.stage;
	s1.w = Areas.stage.w - State.stage->size.w - margin;
	if (s1.w < margin) s1.w = margin; // Narrow window, pin to left instead of right
	PaintFill(device, s1, State.stage->background.brush);

	// Paint the stage text
	Size s2 = State.stage->size; // Start with text width and height
	s2.x = s1.Right();
	s2.y = above;
	s2.SetBottom(Areas.stage.Bottom()); // Don't paint down into the list view control
	device->Font(Handle.arial);
	device->FontColor(State.stage->ink.color);
	device->BackgroundColor(State.stage->background.color);
	PaintLabel(device, State.stage->title, s2);

	// Paint the area to the right of the stage text
	Size s3 = Areas.stage;
	s3.SetLeft(s2.Right());
	PaintFill(device, s3, State.stage->background.brush);

	// Paint all the areas
	Area *a = Areas.all;
	while (a) {
		PaintArea(device, a);
		a = a->next;
	}

	// Status
	device->Font(Handle.font);
	device->FontColor(Handle.ink.color);
	device->BackgroundColor(Handle.background.color);
	Size s = Areas.status;
	s.h = 1;
	PaintFill(device, s, Handle.line.brush);
	s = Areas.status;
	s.ShiftTop(1);
	PaintText(device, State.status, s, false, true, true, true);
}

// Takes a device context and an area
// Paints the area in the window
void PaintArea(Device *device, Area *a) {

	// Only paint areas that are in use and have size
	if (a->command == CommandNone || !a->size.Is()) return;

	// Define sizes as local variables
	int space = 2;
	Size icon, s;

	// Button
	if (a->command == CommandMenu || a->command == CommandUnavailable || a->command == CommandReady || a->command == CommandSet) {

		// Place icon
		icon.x = a->size.x + space;
		icon.y = a->size.y + space;
		icon.w = a->size.w - (2 * space);
		icon.h = a->size.h - (2 * space);

		// Paint icon
		if      (a->display == DisplayGhosted) PaintIcon(device, icon, a->dim,  State.stage->background.brush);
		else if (a->display == DisplayHot)     PaintIcon(device, icon, a->hot,  State.stage->background.brush);
		else                                   PaintIcon(device, icon, a->icon, State.stage->background.brush);

		// Fill outside margins
		s = a->size;
		s.SetBottom(icon.y);
		PaintFill(device, s, State.stage->background.brush); // Row above icon
		s = icon;
		s.w = 0;
		s.SetLeft(a->size.x);
		PaintFill(device, s, State.stage->background.brush); // Left of icon
		s = icon;
		s.CloseRight();
		s.SetRight(a->size.Right());
		PaintFill(device, s, State.stage->background.brush); // Right of icon
		s = a->size;
		s.y = icon.Bottom();
		s.SetBottom(a->size.Bottom());
		PaintFill(device, s, State.stage->background.brush); // Row beneath icon

	// Link
	} else if (a->command == CommandLink) {

	// Other
	} else {

		// Bar
		if (a == &Areas.bar) {

			s = Areas.bar.size;
			s.h = 1;
			PaintFill(device, s, Handle.line.brush);
			s = Areas.bar.size;
			s.ShiftTop(1);
			PaintFill(device, s, Handle.face.brush);

		// Corner
		} else if (a == &Areas.corner) {

			s = Areas.corner.size;
			s.h = 1;
			PaintFill(device, s, Handle.line.brush);
			s = Areas.corner.size;
			s.ShiftTop(1);
			PaintFill(device, s);
			s.CloseRight();
			s.CloseBottom();
			s.w = s.h = 2;
			s.x -= 4;
			s.y -= 12;
			PaintFill(device, s, Handle.shadow.brush);
			s.x -= 4;
			s.y += 4;
			PaintFill(device, s, Handle.shadow.brush);
			s.x += 4;
			PaintFill(device, s, Handle.shadow.brush);
			s.x -= 8;
			s.y += 4;
			PaintFill(device, s, Handle.shadow.brush);
			s.x += 4;
			PaintFill(device, s, Handle.shadow.brush);
			s.x += 4;
			PaintFill(device, s, Handle.shadow.brush);
		}
	}
}

// Make the areas for the window
void AreaCreate() {

	// Load stage icons from resources
	HICON blue16   = LoadIconResource(L"0_STAGE_BLUE", 16, 16);
	HICON blue32   = LoadIconResource(L"0_STAGE_BLUE", 32, 32);
	HICON green16  = LoadIconResource(L"STAGE_GREEN",  16, 16);
	HICON green32  = LoadIconResource(L"STAGE_GREEN",  32, 32);
	HICON red16    = LoadIconResource(L"STAGE_RED",    16, 16);
	HICON red32    = LoadIconResource(L"STAGE_RED",    32, 32);
	HICON yellow16 = LoadIconResource(L"STAGE_YELLOW", 16, 16);
	HICON yellow32 = LoadIconResource(L"STAGE_YELLOW", 32, 32);

	// Make colors
	Handle.blue         = CreateBrush(RGB(  0, 102, 204));
	Handle.lightblue    = CreateBrush(RGB( 51, 153, 255));
	Handle.green        = CreateBrush(RGB(102, 204,  51));
	Handle.lightgreen   = CreateBrush(RGB(153, 255, 102));
	Handle.red          = CreateBrush(RGB(255, 102,  51));
	Handle.lightred     = CreateBrush(RGB(255, 153, 102));
	Handle.yellow       = CreateBrush(RGB(255, 204,   0));
	Handle.lightyellow  = CreateBrush(RGB(255, 255, 102));
	Handle.rednotice    = CreateBrush(RGB(135,   0,   0));
	Handle.yellownotice = CreateBrush(RGB(135, 135,   0));
	Handle.greennotice  = CreateBrush(RGB(  0, 135,   0));

	// Assemble stages
	State.start.title = PROGRAM_NAME;
	State.start.icon16 = blue16;
	State.start.icon32 = blue32;
	State.start.ink = Handle.lightblue;
	State.start.background = Handle.blue;

	State.downloading.title = L"downloading";
	State.downloading.icon16 = blue16;
	State.downloading.icon32 = blue32;
	State.downloading.ink = Handle.lightblue;
	State.downloading.background = Handle.blue;

	State.paused.title = L"paused";
	State.paused.icon16 = yellow16;
	State.paused.icon32 = yellow32;
	State.paused.ink = Handle.lightyellow;
	State.paused.background = Handle.yellow;

	State.seeding.title = L"seeding";
	State.seeding.icon16 = green16;
	State.seeding.icon32 = green32;
	State.seeding.ink = Handle.lightgreen;
	State.seeding.background = Handle.green;

	State.missing.title = L"missing";
	State.missing.icon16 = red16;
	State.missing.icon32 = red32;
	State.missing.ink = Handle.lightred;
	State.missing.background = Handle.red;

	// Text size
	Device device;
	device.OpenCreate();
	device.Font(Handle.font); // Find the height of the default font
	Areas.height = SizeText(&device, L"A").h;
	device.Font(Handle.arial); // Find the widths of the stage titles
	State.start.size       = SizeText(&device, State.start.title);
	State.downloading.size = SizeText(&device, State.downloading.title);
	State.paused.size      = SizeText(&device, State.paused.title);
	State.seeding.size     = SizeText(&device, State.seeding.title);
	State.missing.size     = SizeText(&device, State.missing.title);

	// Buttons
	Areas.tools.command  = CommandMenu;
	Areas.start.command  = CommandUnavailable;
	Areas.pause.command  = CommandUnavailable;
	Areas.stop.command   = CommandUnavailable;
	Areas.remove.command = CommandUnavailable;
	Areas.tools.tip  = L"Tools";
	Areas.start.tip  = L"Start";
	Areas.pause.tip  = L"Pause";
	Areas.stop.tip   = L"Stop";
	Areas.remove.tip = L"Remove";
	Areas.tools.dim   = NULL; // Tools is always available
	Areas.tools.hot   = LoadIconResource(L"BUTTON_TOOLS_HOT",  30, 19); // Rectangular icon
	Areas.tools.icon  = LoadIconResource(L"BUTTON_TOOLS",      30, 19);
	Areas.start.dim   = LoadIconResource(L"BUTTON_START_DIM",  19, 19); // Custom size icons
	Areas.start.hot   = LoadIconResource(L"BUTTON_START_HOT",  19, 19);
	Areas.start.icon  = LoadIconResource(L"BUTTON_START",      19, 19);
	Areas.pause.dim   = LoadIconResource(L"BUTTON_PAUSE_DIM",  19, 19);
	Areas.pause.hot   = LoadIconResource(L"BUTTON_PAUSE_HOT",  19, 19);
	Areas.pause.icon  = LoadIconResource(L"BUTTON_PAUSE",      19, 19);
	Areas.stop.dim    = LoadIconResource(L"BUTTON_STOP_DIM",   19, 19);
	Areas.stop.hot    = LoadIconResource(L"BUTTON_STOP_HOT",   19, 19);
	Areas.stop.icon   = LoadIconResource(L"BUTTON_STOP",       19, 19);
	Areas.remove.dim  = LoadIconResource(L"BUTTON_REMOVE_DIM", 19, 19);
	Areas.remove.hot  = LoadIconResource(L"BUTTON_REMOVE_HOT", 19, 19);
	Areas.remove.icon = LoadIconResource(L"BUTTON_REMOVE",     19, 19);

	// Size controls
	Areas.bar.command    = CommandSizeVertical;
	Areas.corner.command = CommandSizeDiagonal;
}

// Update the appearance of areas and issue commands that occur
void AreaPulse() {

	// Determine what the program stage should be right now
	stageitem *stage = &State.start; //TODO replace this with code that actually chooses what it should be

	// Update the display of the stage if necessary
	if (!State.stage || State.stage != stage) {
		State.stage = stage; // Save the new stage

		// Update the display
		SetIcon(Handle.window, State.stage->icon16, State.stage->icon32); // Window icon
		TaskbarIconUpdate(); // Taskbar notification area icon
		PaintMessage(); // Repaint the window now to show the new stage title
	}

	// Set button command states
	Areas.tools.command  = CommandMenu;
	Areas.start.command  = CommandReady; //TODO base these on what's selected, if anything
	Areas.pause.command  = CommandReady;
	Areas.stop.command   = CommandReady;
	Areas.remove.command = CommandUnavailable;

	// Find what area the mouse is over, if it is inside the client area of the window, and if the primary button is up or down
	Area *over = MouseOver();
	bool inside = MouseInside();
	int pressing = GetKeyState(VK_LBUTTON) & 0x8000;

	// Set the pointer based on the area it pressed
	if (Areas.pressed) {

		if      (Areas.pressed->command == CommandReady)          CursorSet(Handle.arrow);
		else if (Areas.pressed->command == CommandSet)            CursorSet(Handle.arrow);
		else if (Areas.pressed->command == CommandMenu)           CursorSet(Handle.arrow);
		else if (Areas.pressed->command == CommandLink)           CursorSet(Handle.hand);
		else if (Areas.pressed->command == CommandSizeHorizontal) CursorSet(Handle.horizontal);
		else if (Areas.pressed->command == CommandSizeVertical)   CursorSet(Handle.vertical);
		else if (Areas.pressed->command == CommandSizeDiagonal)   CursorSet(Handle.diagonal);
		else                                                     CursorSet(Handle.arrow);

	// Set the pointer based on the area it's over
	} else if (over && !pressing) {

		if      (over->command == CommandReady)          CursorSet(Handle.arrow);
		else if (over->command == CommandSet)            CursorSet(Handle.arrow);
		else if (over->command == CommandMenu)           CursorSet(Handle.arrow);
		else if (over->command == CommandLink)           CursorSet(Handle.hand);
		else if (over->command == CommandSizeHorizontal) CursorSet(Handle.horizontal);
		else if (over->command == CommandSizeVertical)   CursorSet(Handle.vertical);
		else if (over->command == CommandSizeDiagonal)   CursorSet(Handle.diagonal);
		else                                             CursorSet(Handle.arrow);

	// Neither of those, just a regular arrow
	} else if (inside) {

		CursorSet(Handle.arrow);
	}

	// The tab control doesn't set the pointer, so we do it for it
	if (!Areas.pressed && Areas.tabs.Inside(MouseClient()))
		CursorSet(Handle.arrow);

	// Compose the display of each area and draw those that have changed
	AreaDisplay display;
	Device device;
	Area *a = Areas.all;
	while (a) {

		// Compose the display for the area
		display = DisplayNone;
		if (a->command == CommandUnavailable) {

			display = DisplayGhosted;

		} else if (a->command == CommandReady) {

			if      (a == over && a == Areas.pressed)              display = DisplayPressed;
			else if (a == over &&                       !pressing) display = DisplayHot;
			else if (a != over && a == Areas.pressed &&  pressing) display = DisplayHot; // Hot when the mouse drags away
			else                                                   display = DisplayReady;

		} else if (a->command == CommandSet) {

			display = DisplayPressed;

		} else if (a->command == CommandMenu) {

			if      (a == over && a == Areas.pressed) display = DisplayPressed;
			else if (a == over && !pressing)          display = DisplayHot;
			else                                      display = DisplayReady;

		} else if (a->command == CommandLink) {

			if      (a == over && a == Areas.pressed) display = DisplayHot;
			else if (a == over && !pressing)          display = DisplayHot;
			else                                      display = DisplayReady;
		}

		// The display of the area on the screen is different from the newly composed display
		if (a->display != display) {

			// Update the area's display
			a->display = display;

			// Get the window device context if we don't already have it and paint the area
			if (device.open == DeviceNone) {

				device.OpenGet(Handle.window);
				device.Font(Handle.font);
				device.BackgroundColor(Handle.background.color);
			}

			if (device.device) PaintArea(&device, a);
		}

		a = a->next;
	}

	// Adjust size
	if (Areas.pressed) {

		// Get positions in client coordinates
		Size mouse, stick, min, move;
		mouse = MouseClient();                           // Where the mouse is
		stick.x = Areas.pressed->size.x + Areas.stick.x; // The stick is the point the mouse is dragging
		stick.y = Areas.pressed->size.y + Areas.stick.y;
		min.x = Areas.collapse.x + Areas.stick.x;        // The closest the stick can be to the client origin
		min.y = Areas.collapse.y + Areas.stick.y;
		move.x = mouse.x - stick.x;                      // From the stick to the mouse
		move.y = mouse.y - stick.y;

		// If the mouse is away from the stick, try to move the stick to it
		if (Areas.pressed->command == CommandSizeHorizontal && move.x != 0) {

			// Horizontal bar
			Layout(move.x);

		} else if (Areas.pressed->command == CommandSizeVertical && move.y != 0) {

			// Vertical bar
			Layout(move.y);

		} else if (Areas.pressed->command == CommandSizeDiagonal && (move.x != 0 || move.y != 0)) {

			// Don't try to move the stick closer to the client origin than min
			if (mouse.x < min.x) move.x = min.x - stick.x;
			if (mouse.y < min.y) move.y = min.y - stick.y;

			// Diagonal size corner
			WindowSize(Handle.window, move.x, move.y);
		}
	}

	// Find out how many rows there are, and how many are selected
	int rows, selected, pending;
	rows = selected = pending = 0;
	/*
	rows = ListRows();
	selected = ListSelectedRows();

	// COUNT HOW MANY ARE PENDING
	pending = 0;
	botitem *b;
	b = Data.bot;
	while (b) {

		if (b->status == StatusPending) pending++;

	b = b->next; }
	*/

	//TODO integrate this into what's next
//	State.title = L"ftorrent";

	// Compose status text
	CString s = SayNumber(rows, L"file");
	if (pending)  s += L"  " + InsertCommas(numerals(pending))  + L" to get";
	if (selected) s += L"  " + InsertCommas(numerals(selected)) + L" selected";

	// The status text is different
	if (State.status != s) {
		State.status = s; // Update it

		// The status bar has size
		if (Areas.status.Is()) {

			// Get the window device context if we don't have it already
			if (device.open == DeviceNone) {

				device.OpenGet(Handle.window);
				device.Font(Handle.font);
				device.BackgroundColor(Handle.background.color);
			}

			// Paint the status text to the window
			PaintText(&device, State.status, Areas.status, false, true, true, true);
		}
	}
}

// Call this before launching a message box, popup menu, or dialog box that blocks and starts processing messages
// Makes the program abandon the mouse and think it's always outside the client area
void AreaPopUp() {

	// Clear record of the area the mouse pressed and release the mouse if captured
	Areas.pressed = NULL;
	MouseRelease();

	// Record there is one more pop up window
	State.pop++;

	// Pulse the area now as the peeking popup won't pulse on idle
	AreaPulse();
}

// Call this after the message box, popup menu, or dialog box that blocked and processed messages is gone
// Lets the program see the mouse position again
void AreaPopDown() {

	// Record there is one fewer pop up window
	State.pop--;
}

// Given a list of ints like -5, -4, 120, 200
// Turns negative weights into positive pixel widths based on how wide the window is on the screen
// Returns a list with all positive pixel widths
std::vector<int> SizeColumns(std::vector<int> w) {

	// Total given weight and pixel widths
	int totalweight = 0;
	int assignedwidth = 0;
	for (int i = 0; i < (int)w.size(); i++) {
		if (w[i] < 0) {
			totalweight += -w[i];  // Negative, a weight
		} else {
			assignedwidth += w[i]; // Positive, a width already assigned
		}
	}

	// Measure total pixel width, making the colums fill the client width leaving a margin on the right 2 scrollbar widths wide
	int totalwidth = SizeClient().w            // The main window has a size even before it's on the screen
		- (2 * GetSystemMetrics(SM_CXVSCROLL)) // 2 widths of the vertical scroll bar like 16 pixels
		- assignedwidth;                       // Already assigned column widths

	// Calculate pixel width for each weight unit
	int e = 0;
	if (totalweight > 0)
		e = totalwidth / totalweight;

	// Turn weights into pixel widths
	for (int i = 0; i < (int)w.size(); i++) {
		if (w[i] < 0) { // Negative, a weight
			w[i] = -(w[i] * e); // Turn it into a pixel width
			if (w[i] < 32) { // Enforce a minimum width
				w[i] = 32;
			}
		}
	}
	return w;
}

// Takes a number of pixels to move the bar
// Uses text sizes and client area dimensions to compute internal sizes, and moves the child window controls and areas
void Layout(int move) {

	// Remember how things are now
	Size client = SizeClient();   // Get the width and height of the client area
	if (!client.Is()) return;     // The client size is 0 when the window is minimized, don't size areas
	Size before = Areas.bar.size; // Record where the bar is before the size

	// All size constants for the program are defined here as local variables to be read in this function
	int text = Areas.height; // Text height on Windows XP is usually 13
	int icon = 16;           // Small square icons
	Size tools;              // Tools icon
	tools.w = 30;
	tools.h = 19;

	// Heights
	int title  = 23; // Height of status title band at the top of the client area
	int bar    = 4;
	int tabs   = text + 12;
	int status = text + 4;

	// Toolbar buttons
	Size button;
	button.w = button.h = title;
	Areas.tools.size  = button;
	Areas.start.size  = button;
	Areas.pause.size  = button;
	Areas.stop.size   = button;
	Areas.remove.size = button;
	Areas.tools.size.w  = tools.w + 4;
	Areas.start.size.x  = Areas.tools.size.w + (0 * button.w);
	Areas.pause.size.x  = Areas.tools.size.w + (1 * button.w);
	Areas.stop.size.x   = Areas.tools.size.w + (2 * button.w);
	Areas.remove.size.x = Areas.tools.size.w + (3 * button.w);

	// Bar
	int min = title; // Compute the minimum and maximum bar y distances
	int max = client.h - bar - status - tabs;
	if (!Areas.bar.size.y) Areas.bar.size.y = ((client.h - title - bar - tabs - status) * 3 / 7) + title; // Set the bar 3/7ths down the available area
	Areas.bar.size.y += move; // Move the bar
	if (Areas.bar.size.y > max) Areas.bar.size.y = max; // Don't let it go beyond the bounds
	if (Areas.bar.size.y < min) Areas.bar.size.y = min; // Enforce min from the top if both are in violation
	Areas.bar.size.x = 0;
	Areas.bar.size.w = client.w;
	Areas.bar.size.h = bar;

	// Stage
	Areas.stage.x = Areas.remove.size.Right();
	Areas.stage.w = client.w - Areas.remove.size.Right();
	if (Areas.stage.w < 0) Areas.stage.w = 0;
	Areas.stage.h = title;

	// List
	Areas.list.y = Areas.stage.Bottom();
	Areas.list.w = client.w;
	Areas.list.SetBottom(Areas.bar.size.y);
	Areas.list.Check(); // If the height is negative, make it 0

	// Tabs
//	Areas.tabs.x = 2; // Shift to the right to paint left margin and move right border under window edge
	Areas.tabs.y = Areas.bar.size.Bottom();
	Areas.tabs.w = client.w;
	Areas.tabs.h = tabs;

	// Info
	Areas.info.y = Areas.tabs.Bottom();
	Areas.info.w = client.w;
	Areas.info.h = client.h - title - Areas.list.h - bar - tabs - status;

	// Status and corner
	Areas.collapse.y = title + bar + tabs; // Determine where corner would be if the window were very small
	Areas.collapse.w = status;
	Areas.collapse.h = status;
	Areas.corner.size.x = Greatest(Areas.collapse.x, client.w - status); // Corner
	Areas.corner.size.y = Greatest(Areas.collapse.y, Areas.info.Bottom());
	Areas.corner.size.w = status;
	Areas.corner.size.h = status;
	Areas.status.y = Areas.corner.size.y; // Status
	Areas.status.w = Greatest(0, client.w - status);
	Areas.status.h = status;
	if (IsZoomed(Handle.window)) { // If the window is maximized, make status the entire row and hide the size corner
		Areas.status.w = client.w;
		Areas.corner.size.CloseRight();
	}

	// Position and resize child window controls without sending paint messages
	WindowMove(Handle.list, Areas.list);
	WindowMove(Handle.tabs, Areas.tabs);
	WindowMove(Handle.edit, Areas.info);

	// The first time this runs, assign the tooltip regions
	if (!before.Is()) {

		TipAdd(Areas.tools.size,  Areas.tools.tip);
		TipAdd(Areas.start.size,  Areas.start.tip);
		TipAdd(Areas.pause.size,  Areas.pause.tip);
		TipAdd(Areas.stop.size,   Areas.stop.tip);
		TipAdd(Areas.remove.size, Areas.remove.tip);
	}

	// If the bar moved, paint the window
	if (before.y && before.y != Areas.bar.size.y) PaintMessage();
}

// Takes an area that has been pressed and released
// Performs its command
void AreaDoCommand(Area *area) {

	// Menus
	if (area->command == CommandMenu) {

		// Tools
		if (area == &Areas.tools) {

			// Position the menu beneath the tools link area
			Size size = Areas.tools.size;
			size.CloseBottom();
			size.w = 0;

			// Show the popup menu and wait here for the user to click on one of the menu choices
			UINT choice = MenuShow(Handle.menu, false, &size); // Wait here for the user to make a choice
			if      (choice == ID_TOOLS_TEST)    { Test(); }
			else if (choice == ID_TOOLS_OPEN)    {

				CString torrent = DialogOpen(); // Let the user choose a torrent file
				if (isblank(torrent)) return; // Canceled the file open dialog
				CString message = AddTorrent(torrent, true);
				if (is(message)) Message(message); // Show any error text to the user
			}
			else if (choice == ID_TOOLS_ADD)     { Dialog(L"DIALOG_ADD", DialogAdd); }
			else if (choice == ID_TOOLS_NEW)     { report(L"TODO make a new torrent"); }
			else if (choice == ID_TOOLS_HELP)    { FileRun(PROGRAM_HELP); }
			else if (choice == ID_TOOLS_ABOUT)   { Dialog(L"DIALOG_ABOUT", DialogAbout); }
			else if (choice == ID_TOOLS_OPTIONS) { DialogOptions(); }
			else if (choice == ID_TOOLS_EXIT)    { WindowExit(); } // Hide the window and stop libtorrent
		}

	// Buttons
	} else if (area->command == CommandReady || area->command == CommandSet) {

	// Links
	} else if (area->command == CommandLink) {

	}
}

// Call when the program starts up
// Reads the optn.db file next to this running exe, and loads values in Data
void OptionLoad() {

	libtorrent::entry d;
	if (LoadEntry(PathOption(), d)) { // Loaded

		Data.folder = widenStoC(d[narrowRtoS(L"folder")].string()); // Path to download folder
		Data.associate = same(widenStoC(d[narrowRtoS(L"associate")].string()), L"t"); // True to associate magnet and torrent
	}

	// Replace blank or invalid with factory defaults
	if (isblank(Data.folder)) Data.folder = PathTorrents();
}

// Call when the program is shutting down
// Saves values from Data to optn.db next to this running exe
void OptionSave() {

	libtorrent::entry::dictionary_type d;
	d[narrowRtoS(L"folder")] = narrowRtoS(Data.folder);
	d[narrowRtoS(L"associate")] = Data.associate ? narrowRtoS(L"t") : narrowRtoS(L"f");
	SaveEntry(PathOption(), d);
}

// True if the given path is to a folder we can make and write in
bool CheckFolder(read folder) {

	return DiskFolder(folder, true, true);
}

// Show a message box to the user
void Message(read r) {

	// Show the message box with the mouse away
	AreaPopUp();
	MessageBox(Handle.window, r, PROGRAM_NAME, MB_ICONWARNING | MB_OK);
	AreaPopDown();
}

// A message from the add box
BOOL CALLBACK DialogAdd(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {

	// The dialog is about to be displayed
	switch (message) {
	case WM_INITDIALOG:

		return true; // Let the system place the focus

	// The message is a command
	break;
	case WM_COMMAND:

		// The user clicked OK
		switch (LOWORD(wparam)) {
		case IDOK:
		{
			CString magnet = TextDialog(dialog, IDC_EDIT); // Get the text the user typed
			EndDialog(dialog, 0); // Close the dialog
			CString message = AddMagnet(magnet, true);
			if (is(message)) Message(message); // Show any error text to the user
			return true;
		}
		// The user clicked Cancel
		break;
		case IDCANCEL:
			
			EndDialog(dialog, 0); // Close the dialog
			return true;
		}
	}
	return false; // We didn't process the message
}

// Shows the options dialog box
void DialogOptions() {

	// Load common values
	PROPSHEETPAGE page[3];
	ZeroMemory(&page, sizeof(page)); // Size of the whole array
	page[0].dwSize      = page[1].dwSize      = page[2].dwSize      = sizeof(page[0]); // Size of just one page
	page[0].dwFlags     = page[1].dwFlags     = page[2].dwFlags     = PSP_DEFAULT;
	page[0].hInstance   = page[1].hInstance   = page[2].hInstance   = Handle.instance;
	page[0].pfnCallback = page[1].pfnCallback = page[2].pfnCallback = NULL;
	page[0].lParam      = page[1].lParam      = page[2].lParam      = 0;

	// Attach dialog templates and procedures
	page[0].pszTemplate = L"OPTIONS_PAGE_1";
	page[1].pszTemplate = L"OPTIONS_PAGE_2";
	page[2].pszTemplate = L"OPTIONS_PAGE_3";
	page[0].pfnDlgProc = (DLGPROC)DialogOptionsPage1;
	page[1].pfnDlgProc = (DLGPROC)DialogOptionsPage2;
	page[2].pfnDlgProc = (DLGPROC)DialogOptionsPage3;

	// Load information into the header structure
	PROPSHEETHEADER header;
	ZeroMemory(&header, sizeof(header));
	header.dwSize      = sizeof(header);          // Size of the header
	header.dwFlags     = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP; // Leave out the apply button and the help question mark
	header.hwndParent  = Handle.window;           // Handle to parent window
	header.hInstance   = Handle.instance;         // Handle to application instance
	header.hIcon       = NULL;                    // No icon
	header.pszCaption  = L"Options";              // Text in the title bar
	header.nPages      = 3;                       // Number of pages
	header.nStartPage  = 0;                       // Index of the page to tab to by default
	header.ppsp        = (LPCPROPSHEETPAGE)&page; // Address of the array of property sheet pages
	header.pfnCallback = NULL;                    // No function to call when the property sheet is initialized

	// Create and display the property sheet
	AreaPopUp();
	PropertySheet(&header);
	AreaPopDown();
}

// Associate this program with magnet and torrent and show the user a color label about it
void AssociateUpdate(HWND dialog) {

	Data.associate = IsDlgButtonChecked(dialog, IDC_ASSOCIATE) ? true : false; // Copy the check to data
	if (Data.associate && !AssociateIs()) AssociateGet(); // Change the registry if necessary

	CString before = TextDialog(dialog, IDC_LABEL); // "Label" when the dialog loads before we change it to "red", "yellow", or "green"
	CString now;
	if (AssociateIs())       now = L"green";  // We have the associations
	else if (Data.associate) now = L"yellow"; // We tried to get them, but it didn't work
	else                     now = L"red";    // The user has not told the program to get them

	TextDialogSet(dialog, IDC_LABEL, now); // Set the text of the hidden label for paint to use below
	if (before != L"Label" && before != now) InvalidateRect(dialog, NULL, true); // Repaint the dialog if the label changed
}

// A message from options page 1
BOOL APIENTRY DialogOptionsPage1(HWND dialog, UINT message, UINT wparam, LPARAM lparam) {

	// The page is about to be displayed
	switch (message) {
	case WM_INITDIALOG:

		TextDialogSet(dialog, IDC_FOLDER, Data.folder); // Torrents folder path
		CheckDlgButton(dialog, IDC_ASSOCIATE, Data.associate ? BST_CHECKED : BST_UNCHECKED); // Associate checkbox
		AssociateUpdate(dialog);
		return true; // Let the system place the focus

	// The dialog lost or received the keyboard focus
	break;
	case WM_ACTIVATE:

		AssociateUpdate(dialog);

	// The dialog needs to be painted
	break;
	case WM_PAINT:
	{
		// Compose the color label
		CString code = TextDialog(dialog, IDC_LABEL);
		Brush *brush;
		CString message;
		if (code == L"red") {
			brush = &Handle.rednotice;
			message = L"You haven't made " + PROGRAM_NAME + L" your default BitTorrent client:";

		} else if (code == L"yellow") {
			brush = &Handle.yellownotice;
			message = L"Run " + PROGRAM_NAME + L" as administrator to make it your default BitTorrent client.";

		} else if (code == L"green") {
			brush = &Handle.greennotice;
			message = L"Thanks for using " + PROGRAM_NAME + L" as your default BitTorrent client.";
		}

		// Calculate where in the dialog to paint
		RECT r1, r2, r3;
		GetWindowRect(dialog, &r1);
		GetWindowRect(GetDlgItem(dialog, IDC_LABEL), &r2); // The message text dialog item has false visibility
		GetClientRect(GetDlgItem(dialog, IDC_LABEL), &r3);
		Size size;
		size.x = r2.left - r1.left;
		size.y = r2.top - r1.top;
		size.w = r3.right;
		size.h = r3.bottom;
		RECT rectangle = size.Rectangle();

		// Custom paint the message in the dialog
		Device device;
		device.OpenPaint(dialog);
		device.Font(Handle.font);
		device.FontColor(brush->color);
		if (!DrawText(device.device, message, -1, &rectangle, 0)) error(L"drawtext");
		return false;
	}
	// The user clicked a button on the page
	break;
	case WM_COMMAND:

		// Browse
		switch (LOWORD(wparam)) {
		case IDC_BROWSE:
		{
			CString browse = DialogBrowse(L"Choose the folder where " + PROGRAM_NAME + L" will download files:"); // Show the dialog
			if (is(browse)) TextDialogSet(dialog, IDC_FOLDER, browse); // If the user picked something, write it in the text field
			return true; // We handled the message
		}
		// The user checked or unchecked the associate checkbox
		break;
		case IDC_ASSOCIATE:

			AssociateUpdate(dialog);
		}

	// The user clicked one of the bottom property sheet buttons
	break;
	case WM_NOTIFY:

		// Check what the user entered on this page
		switch (((LPNMHDR)(ULONG_PTR)lparam)->code) {
		case PSN_KILLACTIVE:
		{
			CString folder = TextDialog(dialog, IDC_FOLDER);
			if (CheckFolder(folder)) {

				SetWindowLong(dialog, DWL_MSGRESULT, PSNRET_NOERROR);
				return true;

			} else {

				Message(L"Cannot save files to the folder at '" + folder + "'. Check the path and try again.");
				SetWindowLong(dialog, DWL_MSGRESULT, PSNRET_INVALID); // Keep the property sheet open so the user can fix the invalid data
				return true;
			}
		}
		// Save what the user entered on this page
		break;
		case PSN_APPLY:

			Data.folder = TextDialog(dialog, IDC_FOLDER);
			SetWindowLong(dialog, DWL_MSGRESULT, PSNRET_NOERROR);
			return true;
		}
	}
	return false; // We didn't handle the message
}

// A message from options page 2
BOOL APIENTRY DialogOptionsPage2(HWND dialog, UINT message, UINT wparam, LPARAM lparam) {

	// The page is about to be displayed
	switch (message) {
	case WM_INITDIALOG:

		return true; // Let the system place the focus

	// The user clicked a button on the page
	break;
	case WM_COMMAND:

	// The user clicked one of the bottom property sheet buttons
	break;
	case WM_NOTIFY:

		// The user clicked OK
		switch (((LPNMHDR)(ULONG_PTR)lparam)->code) {
		case PSN_KILLACTIVE:

			SetWindowLong(dialog, DWL_MSGRESULT, PSNRET_NOERROR);
			return true;

		break;
		case PSN_APPLY:

			SetWindowLong(dialog, DWL_MSGRESULT, PSNRET_NOERROR);
			return true;
		}
	}
	return false; // We didn't handle the message
}

// A message from options page 3
BOOL APIENTRY DialogOptionsPage3(HWND dialog, UINT message, UINT wparam, LPARAM lparam) {

	// The page is about to be displayed
	switch (message) {
	case WM_INITDIALOG:

		return true; // Let the system place the focus

	// The user clicked a button on the page
	break;
	case WM_COMMAND:

	// The user clicked one of the bottom property sheet buttons
	break;
	case WM_NOTIFY:

		// The user clicked OK
		switch (((LPNMHDR)(ULONG_PTR)lparam)->code) {
		break;
		case PSN_KILLACTIVE:

			SetWindowLong(dialog, DWL_MSGRESULT, PSNRET_NOERROR);
			return true;

		break;
		case PSN_APPLY:

			SetWindowLong(dialog, DWL_MSGRESULT, PSNRET_NOERROR);
			return true;
		}
	}
	return false; // We didn't handle the message
}

// A message from the about box
BOOL CALLBACK DialogAbout(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {

	// The dialog is about to be displayed
	switch (message) {
	case WM_INITDIALOG:

		// Let the system place the focus
		return true;

	// The dialog needs to be painted
	break;
	case WM_PAINT:
	{
		// Do custom painting in the dialog
		Device device;
		device.OpenPaint(dialog);
		device.BackgroundColor(Handle.background.color);
		device.Font(Handle.arial);

		// Compose text
		CString about = L"about " + PROGRAM_NAME;

		// Prepare rectangles
		Size client = SizeClient(dialog); // Get the width of the client area of the dialog box
		Size blue = client; // Blue bar at top
		blue.h = 23;
		Size white = client; // White area beneath
		white.SetTop(blue.h);
		Size title = SizeText(&device, about); // Title above the edge
		title.x = client.w - 8 - title.w;
		title.y = -7;
		title.SetBottom(blue.h);

		// Paint the rectangles
		PaintFill(&device, blue, Handle.blue.brush);
		PaintText(&device, about, title, false, false, false, false, 0, Handle.arial, &Handle.lightblue, &Handle.blue);
		PaintFill(&device, white, Handle.background.brush);

		// Set heights
		int text = Areas.height; // Text height is usually 13
		int space = 5;

		// Size the text
		Size s = white;
		s.SetLeft(89);
		s.SetTop(46);
		s.h = text;

		// Paint the text
		device.Font(Handle.font);
		PaintText(&device, PROGRAM_ABOUT1, s, false, false, false, false, 0, Handle.font, &Handle.ink, &Handle.background); s.y += text + space;
		PaintText(&device, PROGRAM_ABOUT2, s, false, false, false, false, 0, Handle.font, &Handle.ink, &Handle.background); s.y += text;
		PaintText(&device, PROGRAM_ABOUT3, s, false, false, false, false, 0, Handle.font, &Handle.ink, &Handle.background); s.y += text + space;
		PaintText(&device, PROGRAM_ABOUT4, s, false, false, false, false, 0, Handle.font, &Handle.ink, &Handle.background); s.y += text;
		return false;
	}
	// The message is a command
	break;
	case WM_COMMAND:

		// The user clicked OK or Cancel
		switch (LOWORD(wparam)) {
		case IDOK:
		case IDCANCEL:

			// Close the dialog
			EndDialog(dialog, 0);
			return true;
		}
	}
	return false; // We didn't process the message
}
