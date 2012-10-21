
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Set up the program image list
void StartIcon() {

	// Initialize ext with a guid so the first requested extension will never match
	App.icon.ext = TextGuid();

	// Create the program image list
	App.icon.list = ImageList_Create(
		16, 16,         // Image size
		ILC_COLOR32 |   // Windows XP style 32 bit antialiased icons
		ILC_MASK,       // Show icon transparency
		0,              // Start with no icons
		ICON_CAPACITY); // Be able to grow to hold this many more icons
	if (!App.icon.list) error(L"imagelist_create");

	// Load the resource icons into the program image list and get their indices there, or -1 if not loaded
	App.icon.clear      = IconAddResource(L"CLEAR_ICON");
	App.icon.ascending  = IconAddResource(L"COLUMN_ASCENDING");
	App.icon.descending = IconAddResource(L"COLUMN_DESCENDING");

	// Load the shell icon for a file
	CString type;
	App.icon.file = IconGet(L"", &type);
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
	Size s1 = App.area.stage;
	s1.w = App.area.stage.w - App.stage.show->size.w - margin;
	if (s1.w < margin) s1.w = margin; // Narrow window, pin to left instead of right
	PaintFill(device, s1, App.stage.show->background.brush);

	// Paint the stage text
	Size s2 = App.stage.show->size; // Start with text width and height
	s2.x = s1.Right();
	s2.y = above;
	s2.SetBottom(App.area.stage.Bottom()); // Don't paint down into the list view control
	device->Font(App.font.arial);
	device->FontColor(App.stage.show->ink.color);
	device->BackgroundColor(App.stage.show->background.color);
	PaintLabel(device, App.stage.show->title, s2);

	// Paint the area to the right of the stage text
	Size s3 = App.area.stage;
	s3.SetLeft(s2.Right());
	PaintFill(device, s3, App.stage.show->background.brush);

	// Paint all the areas
	Area *a = App.area.all;
	while (a) {
		PaintArea(device, a);
		a = a->next;
	}

	// Status
	device->Font(App.font.normal);
	device->FontColor(App.brush.ink.color);
	device->BackgroundColor(App.brush.background.color);
	Size s = App.area.status;
	s.h = 1;
	PaintFill(device, s, App.brush.line.brush);
	s = App.area.status;
	s.ShiftTop(1);
	PaintText(device, App.cycle.status, s, false, true, true, true);
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
		if      (a->display == DisplayGhosted) PaintIcon(device, icon, a->dim,  App.stage.show->background.brush);
		else if (a->display == DisplayHot)     PaintIcon(device, icon, a->hot,  App.stage.show->background.brush);
		else                                   PaintIcon(device, icon, a->icon, App.stage.show->background.brush);

		// Fill outside margins
		s = a->size;
		s.SetBottom(icon.y);
		PaintFill(device, s, App.stage.show->background.brush); // Row above icon
		s = icon;
		s.w = 0;
		s.SetLeft(a->size.x);
		PaintFill(device, s, App.stage.show->background.brush); // Left of icon
		s = icon;
		s.CloseRight();
		s.SetRight(a->size.Right());
		PaintFill(device, s, App.stage.show->background.brush); // Right of icon
		s = a->size;
		s.y = icon.Bottom();
		s.SetBottom(a->size.Bottom());
		PaintFill(device, s, App.stage.show->background.brush); // Row beneath icon

	// Link
	} else if (a->command == CommandLink) {

	// Other
	} else {

		// Bar
		if (a == &App.area.bar) {

			s = App.area.bar.size;
			s.h = 1;
			PaintFill(device, s, App.brush.line.brush);
			s = App.area.bar.size;
			s.ShiftTop(1);
			PaintFill(device, s, App.brush.face.brush);

		// Corner
		} else if (a == &App.area.corner) {

			s = App.area.corner.size;
			s.h = 1;
			PaintFill(device, s, App.brush.line.brush);
			s = App.area.corner.size;
			s.ShiftTop(1);
			PaintFill(device, s);
			s.CloseRight();
			s.CloseBottom();
			s.w = s.h = 2;
			s.x -= 4;
			s.y -= 12;
			PaintFill(device, s, App.brush.shadow.brush);
			s.x -= 4;
			s.y += 4;
			PaintFill(device, s, App.brush.shadow.brush);
			s.x += 4;
			PaintFill(device, s, App.brush.shadow.brush);
			s.x -= 8;
			s.y += 4;
			PaintFill(device, s, App.brush.shadow.brush);
			s.x += 4;
			PaintFill(device, s, App.brush.shadow.brush);
			s.x += 4;
			PaintFill(device, s, App.brush.shadow.brush);
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
	App.brush.blue         = CreateBrush(RGB(  0, 102, 204));
	App.brush.lightblue    = CreateBrush(RGB( 51, 153, 255));
	App.brush.green        = CreateBrush(RGB(102, 204,  51));
	App.brush.lightgreen   = CreateBrush(RGB(153, 255, 102));
	App.brush.red          = CreateBrush(RGB(255, 102,  51));
	App.brush.lightred     = CreateBrush(RGB(255, 153, 102));
	App.brush.yellow       = CreateBrush(RGB(255, 204,   0));
	App.brush.lightyellow  = CreateBrush(RGB(255, 255, 102));
	App.brush.rednotice    = CreateBrush(RGB(135,   0,   0));
	App.brush.yellownotice = CreateBrush(RGB(135, 135,   0));
	App.brush.greennotice  = CreateBrush(RGB(  0, 135,   0));

	// Assemble stages
	App.stage.start.title = PROGRAM_NAME;
	App.stage.start.icon16 = blue16;
	App.stage.start.icon32 = blue32;
	App.stage.start.ink = App.brush.lightblue;
	App.stage.start.background = App.brush.blue;

	App.stage.downloading.title = L"downloading";
	App.stage.downloading.icon16 = blue16;
	App.stage.downloading.icon32 = blue32;
	App.stage.downloading.ink = App.brush.lightblue;
	App.stage.downloading.background = App.brush.blue;

	App.stage.paused.title = L"paused";
	App.stage.paused.icon16 = yellow16;
	App.stage.paused.icon32 = yellow32;
	App.stage.paused.ink = App.brush.lightyellow;
	App.stage.paused.background = App.brush.yellow;

	App.stage.seeding.title = L"seeding";
	App.stage.seeding.icon16 = green16;
	App.stage.seeding.icon32 = green32;
	App.stage.seeding.ink = App.brush.lightgreen;
	App.stage.seeding.background = App.brush.green;

	App.stage.missing.title = L"missing";
	App.stage.missing.icon16 = red16;
	App.stage.missing.icon32 = red32;
	App.stage.missing.ink = App.brush.lightred;
	App.stage.missing.background = App.brush.red;

	// Text size
	Device device;
	device.OpenCreate();
	device.Font(App.font.normal); // Find the height of the default font
	App.font.height = SizeText(&device, L"A").h;
	device.Font(App.font.arial); // Find the widths of the stage titles
	App.stage.start.size       = SizeText(&device, App.stage.start.title);
	App.stage.downloading.size = SizeText(&device, App.stage.downloading.title);
	App.stage.paused.size      = SizeText(&device, App.stage.paused.title);
	App.stage.seeding.size     = SizeText(&device, App.stage.seeding.title);
	App.stage.missing.size     = SizeText(&device, App.stage.missing.title);

	// Commands on the menu that don't have buttons
	App.area.open.command        = CommandUnavailable;
	App.area.openfolder.command  = CommandUnavailable;
	App.area.copymagnet.command  = CommandUnavailable;
	App.area.savetorrent.command = CommandUnavailable;
	App.area.deletefiles.command = CommandUnavailable;

	// Buttons
	App.area.tools.command  = CommandMenu;
	App.area.start.command  = CommandUnavailable;
	App.area.pause.command  = CommandUnavailable;
	App.area.remove.command = CommandUnavailable;
	App.area.tools.tip  = L"Tools";
	App.area.start.tip  = L"Start";
	App.area.pause.tip  = L"Pause";
	App.area.remove.tip = L"Remove";
	App.area.tools.dim   = NULL; // Tools is always available
	App.area.tools.hot   = LoadIconResource(L"BUTTON_TOOLS_HOT",  19, 19); // Custom size icons
	App.area.tools.icon  = LoadIconResource(L"BUTTON_TOOLS",      19, 19);
	App.area.start.dim   = LoadIconResource(L"BUTTON_START_DIM",  19, 19);
	App.area.start.hot   = LoadIconResource(L"BUTTON_START_HOT",  19, 19);
	App.area.start.icon  = LoadIconResource(L"BUTTON_START",      19, 19);
	App.area.pause.dim   = LoadIconResource(L"BUTTON_PAUSE_DIM",  19, 19);
	App.area.pause.hot   = LoadIconResource(L"BUTTON_PAUSE_HOT",  19, 19);
	App.area.pause.icon  = LoadIconResource(L"BUTTON_PAUSE",      19, 19);
	App.area.remove.dim  = LoadIconResource(L"BUTTON_REMOVE_DIM", 19, 19);
	App.area.remove.hot  = LoadIconResource(L"BUTTON_REMOVE_HOT", 19, 19);
	App.area.remove.icon = LoadIconResource(L"BUTTON_REMOVE",     19, 19);

	// Size controls
	App.area.bar.command    = CommandSizeVertical;
	App.area.corner.command = CommandSizeDiagonal;
}

// Update the appearance of areas and issue commands that occur
void AreaPulse() {

	// Determine what the program stage should be right now
	Stage *stage = &App.stage.start; //TODO replace this with code that actually chooses what it should be

	// Update the display of the stage if necessary
	if (!App.stage.show || App.stage.show != stage) {
		App.stage.show = stage; // Save the new stage

		// Update the display
		SetIcon(App.window.main, App.stage.show->icon16, App.stage.show->icon32); // Window icon
		TaskbarIconUpdate(); // Taskbar notification area icon
		PaintMessage(); // Repaint the window now to show the new stage title
	}

	// Set button command states
	App.area.tools.command = CommandMenu;

	// Find out how many torrents are selected
	int selectedrows = ListSelectedRows(App.list.torrents.window);

	// Nothing selected
	if (!selectedrows) {

		// All commands unavailable
		App.area.open.command        = CommandUnavailable;
		App.area.openfolder.command  = CommandUnavailable;
		App.area.copymagnet.command  = CommandUnavailable;
		App.area.savetorrent.command = CommandUnavailable;
		App.area.start.command       = CommandUnavailable;
		App.area.pause.command       = CommandUnavailable;
		App.area.remove.command      = CommandUnavailable;
		App.area.deletefiles.command = CommandUnavailable;

	// One or more torrents are selected
	} else {

		// Commands where any selected torrent that can't do it makes the command for all of them unavailable
		App.area.open.command        = CommandReady; // Start out available
		App.area.openfolder.command  = CommandReady;
		App.area.copymagnet.command  = CommandReady;
		App.area.savetorrent.command = CommandReady;

		// Commands where any selected torrent that can do it makes the command for all of them available
		App.area.start.command       = CommandUnavailable; // Start out grayed
		App.area.pause.command       = CommandUnavailable;
		App.area.remove.command      = CommandUnavailable;
		App.area.deletefiles.command = CommandUnavailable;

		// Loop for each selected row
		int rows = ListRows(App.list.torrents.window);
		for (int row = 0; row < rows; row++) {
			if (ListSelected(App.list.torrents.window, row)) {

				// Any selected unavailable torrent disables the button
				Torrent *t = ListGetTorrent(row);
				if (t) {

					// Commands where any selected torrent that can't do it makes the command for all of them unavailable
					if (!t->CanOpen())           App.area.open.command        = CommandUnavailable; // One that can't grays the command
					if (!t->CanShowInFolder())   App.area.openfolder.command  = CommandUnavailable;
					if (!t->CanCopyMagnetLink()) App.area.copymagnet.command  = CommandUnavailable;
					if (!t->CanSaveTorrentAs())  App.area.savetorrent.command = CommandUnavailable;

					// Commands where any selected torrent that can do it makes the command for all of them available
					if (t->CanStart())                 App.area.start.command       = CommandReady; // One that can makes the command available
					if (t->CanPause())                 App.area.pause.command       = CommandReady;
					if (t->CanRemove())                App.area.remove.command      = CommandReady;
					if (t->CanDelete())                App.area.deletefiles.command = CommandReady;
				}
			}
		}
	}

	// If multiple torrents are selected, gray out commands that only make sense if a single torrent is selected
	if (selectedrows > 1) {
		App.area.open.command        = CommandUnavailable;
		App.area.openfolder.command  = CommandUnavailable;
		App.area.savetorrent.command = CommandUnavailable;
	}

	// Find what area the mouse is over, if it is inside the client area of the window, and if the primary button is up or down
	Area *over = MouseOver();
	bool inside = MouseInside();
	int pressing = GetKeyState(VK_LBUTTON) & 0x8000;

	// Set the pointer based on the area it pressed
	if (App.area.pressed) {

		if      (App.area.pressed->command == CommandReady)          CursorSet(App.cursor.arrow);
		else if (App.area.pressed->command == CommandSet)            CursorSet(App.cursor.arrow);
		else if (App.area.pressed->command == CommandMenu)           CursorSet(App.cursor.arrow);
		else if (App.area.pressed->command == CommandLink)           CursorSet(App.cursor.hand);
		else if (App.area.pressed->command == CommandSizeHorizontal) CursorSet(App.cursor.horizontal);
		else if (App.area.pressed->command == CommandSizeVertical)   CursorSet(App.cursor.vertical);
		else if (App.area.pressed->command == CommandSizeDiagonal)   CursorSet(App.cursor.diagonal);
		else                                                         CursorSet(App.cursor.arrow);

	// Set the pointer based on the area it's over
	} else if (over && !pressing) {

		if      (over->command == CommandReady)          CursorSet(App.cursor.arrow);
		else if (over->command == CommandSet)            CursorSet(App.cursor.arrow);
		else if (over->command == CommandMenu)           CursorSet(App.cursor.arrow);
		else if (over->command == CommandLink)           CursorSet(App.cursor.hand);
		else if (over->command == CommandSizeHorizontal) CursorSet(App.cursor.horizontal);
		else if (over->command == CommandSizeVertical)   CursorSet(App.cursor.vertical);
		else if (over->command == CommandSizeDiagonal)   CursorSet(App.cursor.diagonal);
		else                                             CursorSet(App.cursor.arrow);

	// Neither of those, just a regular arrow
	} else if (inside) {

		CursorSet(App.cursor.arrow);
	}

	// The tab control doesn't set the pointer, so we do it for it
	if (!App.area.pressed && App.area.tabs.Inside(MouseClient()))
		CursorSet(App.cursor.arrow);

	// Compose the display of each area and draw those that have changed
	AreaDisplay display;
	Device device;
	Area *a = App.area.all;
	while (a) {

		// Compose the display for the area
		display = DisplayNone;
		if (a->command == CommandUnavailable) {

			display = DisplayGhosted;

		} else if (a->command == CommandReady) {

			if      (a == over && a == App.area.pressed)              display = DisplayPressed;
			else if (a == over &&                          !pressing) display = DisplayHot;
			else if (a != over && a == App.area.pressed &&  pressing) display = DisplayHot; // Hot when the mouse drags away
			else                                                      display = DisplayReady;

		} else if (a->command == CommandSet) {

			display = DisplayPressed;

		} else if (a->command == CommandMenu) {

			if      (a == over && a == App.area.pressed) display = DisplayPressed;
			else if (a == over && !pressing)             display = DisplayHot;
			else                                         display = DisplayReady;

		} else if (a->command == CommandLink) {

			if      (a == over && a == App.area.pressed) display = DisplayHot;
			else if (a == over && !pressing)             display = DisplayHot;
			else                                         display = DisplayReady;
		}

		// The display of the area on the screen is different from the newly composed display
		if (a->display != display) {

			// Update the area's display
			a->display = display;

			// Get the window device context if we don't already have it and paint the area
			if (device.open == DeviceNone) {

				device.OpenGet(App.window.main);
				device.Font(App.font.normal);
				device.BackgroundColor(App.brush.background.color);
			}

			if (device.device) PaintArea(&device, a);
		}

		a = a->next;
	}

	// Adjust size
	if (App.area.pressed) {

		// Get positions in client coordinates
		Size mouse, stick, min, move;
		mouse = MouseClient();                                 // Where the mouse is
		stick.x = App.area.pressed->size.x + App.area.stick.x; // The stick is the point the mouse is dragging
		stick.y = App.area.pressed->size.y + App.area.stick.y;
		min.x = App.area.collapse.x + App.area.stick.x;        // The closest the stick can be to the client origin
		min.y = App.area.collapse.y + App.area.stick.y;
		move.x = mouse.x - stick.x;                            // From the stick to the mouse
		move.y = mouse.y - stick.y;

		// If the mouse is away from the stick, try to move the stick to it
		if (App.area.pressed->command == CommandSizeHorizontal && move.x != 0) {

			// Horizontal bar
			Layout(move.x);

		} else if (App.area.pressed->command == CommandSizeVertical && move.y != 0) {

			// Vertical bar
			Layout(move.y);

		} else if (App.area.pressed->command == CommandSizeDiagonal && (move.x != 0 || move.y != 0)) {

			// Don't try to move the stick closer to the client origin than min
			if (mouse.x < min.x) move.x = min.x - stick.x;
			if (mouse.y < min.y) move.y = min.y - stick.y;

			// Diagonal size corner
			WindowSize(App.window.main, move.x, move.y);
		}
	}

	// Compose status text
	CString s = SayNumber(ListRows(App.list.torrents.window), L"torrent");

	// The status text is different
	if (App.cycle.status != s) {
		App.cycle.status = s; // Update it

		// The status bar has size
		if (App.area.status.Is()) {

			// Get the window device context if we don't have it already
			if (device.open == DeviceNone) {

				device.OpenGet(App.window.main);
				device.Font(App.font.normal);
				device.BackgroundColor(App.brush.background.color);
			}

			// Paint the status text to the window
			PaintText(&device, App.cycle.status, App.area.status, false, true, true, true);
		}
	}
}

// Call this before launching a message box, popup menu, or dialog box that blocks and starts processing messages
// Makes the program abandon the mouse and think it's always outside the client area
void AreaPopUp() {

	// Clear record of the area the mouse pressed and release the mouse if captured
	App.area.pressed = NULL;
	MouseRelease();

	// Record there is one more pop up window
	App.cycle.pop++;

	// Pulse the area now as the peeking popup won't pulse on idle
	AreaPulse();
}

// Call this after the message box, popup menu, or dialog box that blocked and processed messages is gone
// Lets the program see the mouse position again
void AreaPopDown() {

	// Record there is one fewer pop up window
	App.cycle.pop--;
}

// Takes a number of pixels to move the bar
// Uses text sizes and client area dimensions to compute internal sizes, and moves the child window controls and areas
void Layout(int move) {

	// Remember how things are now
	Size client = SizeClient();      // Get the width and height of the client area
	if (!client.Is()) return;        // The client size is 0 when the window is minimized, don't size areas
	Size before = App.area.bar.size; // Record where the bar is before the size

	// All size constants for the program are defined here as local variables to be read in this function
	int text = App.font.height; // Text height on Windows XP is usually 13
	int icon = 16;              // Small square icons
	Size tools;                 // Tools icon
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
	App.area.tools.size  = button;
	App.area.start.size  = button;
	App.area.pause.size  = button;
	App.area.remove.size = button;
	App.area.tools.size.x  = 0 * button.w;
	App.area.start.size.x  = 1 * button.w;
	App.area.pause.size.x  = 2 * button.w;
	App.area.remove.size.x = 3 * button.w;

	// Bar
	int min = title; // Compute the minimum and maximum bar y distances
	int max = client.h - bar - status - tabs;
	if (!App.area.bar.size.y) App.area.bar.size.y = ((client.h - title - bar - tabs - status) * 3 / 7) + title; // Set the bar 3/7ths down the available area
	App.area.bar.size.y += move; // Move the bar
	if (App.area.bar.size.y > max) App.area.bar.size.y = max; // Don't let it go beyond the bounds
	if (App.area.bar.size.y < min) App.area.bar.size.y = min; // Enforce min from the top if both are in violation
	App.area.bar.size.x = 0;
	App.area.bar.size.w = client.w;
	App.area.bar.size.h = bar;

	// Stage
	App.area.stage.x = App.area.remove.size.Right();
	App.area.stage.w = client.w - App.area.remove.size.Right();
	if (App.area.stage.w < 0) App.area.stage.w = 0;
	App.area.stage.h = title;

	// List
	App.area.list.y = App.area.stage.Bottom();
	App.area.list.w = client.w;
	App.area.list.SetBottom(App.area.bar.size.y);
	App.area.list.Check(); // If the height is negative, make it 0

	// Tabs
//	App.area.tabs.x = 2; // Shift to the right to paint left margin and move right border under window edge
	App.area.tabs.y = App.area.bar.size.Bottom();
	App.area.tabs.w = client.w;
	App.area.tabs.h = tabs;

	// Info
	App.area.info.y = App.area.tabs.Bottom();
	App.area.info.w = client.w;
	App.area.info.h = client.h - title - App.area.list.h - bar - tabs - status;

	// Status and corner
	App.area.collapse.y = title + bar + tabs; // Determine where corner would be if the window were very small
	App.area.collapse.w = status;
	App.area.collapse.h = status;
	App.area.corner.size.x = Greatest(App.area.collapse.x, client.w - status); // Corner
	App.area.corner.size.y = Greatest(App.area.collapse.y, App.area.info.Bottom());
	App.area.corner.size.w = status;
	App.area.corner.size.h = status;
	App.area.status.y = App.area.corner.size.y; // Status
	App.area.status.w = Greatest(0, client.w - status);
	App.area.status.h = status;
	if (IsZoomed(App.window.main)) { // If the window is maximized, make status the entire row and hide the size corner
		App.area.status.w = client.w;
		App.area.corner.size.CloseRight();
	}

	// Position and resize child window controls without sending paint messages
	WindowMove(App.list.torrents.window, App.area.list);
	WindowMove(App.window.tabs,          App.area.tabs);
	WindowMove(App.list.files.window,    App.area.info);

	// The first time this runs, assign the tooltip regions
	if (!before.Is()) {

		TipAdd(App.area.tools.size,  App.area.tools.tip);
		TipAdd(App.area.start.size,  App.area.start.tip);
		TipAdd(App.area.pause.size,  App.area.pause.tip);
		TipAdd(App.area.remove.size, App.area.remove.tip);
	}

	// If the bar moved, paint the window
	if (before.y && before.y != App.area.bar.size.y) PaintMessage();
}

// Takes an area that has been pressed and released
// Performs its command
void AreaDoCommand(Area *area) {

	// Menus
	if (area->command == CommandMenu) {

		// Tools
		if (area == &App.area.tools) {

			// Position the menu beneath the tools link area
			Size size = App.area.tools.size;
			size.CloseBottom();
			size.w = 0;

			// Show the popup menu and wait here for the user to click on one of the menu choices
			int choice = MenuShow(App.menu.tools, false, &size); // Wait here for the user to make a choice
			if      (choice == IdentifyToolsTest)          { Test(); }
			else if (choice == IdentifyToolsOpenTorrent)   {

				CString torrent = DialogOpen(); // Let the user choose a torrent file
				if (isblank(torrent)) return; // Canceled the file open dialog
				CString message = AddTorrent(torrent, true);
				if (is(message)) Message(message); // Show any error text to the user
			}
			else if (choice == IdentifyToolsAddMagnet)     { Dialog(L"DIALOG_ADD", DialogAdd); }
			else if (choice == IdentifyToolsCreateTorrent) { Dialog(L"DIALOG_CREATE", DialogCreate); }
			else if (choice == IdentifyToolsHelp)          { FileRun(PROGRAM_HELP); }
			else if (choice == IdentifyToolsAbout)         { Dialog(L"DIALOG_ABOUT", DialogAbout); }
			else if (choice == IdentifyToolsOptions)       { DialogOptions(); }
			else if (choice == IdentifyToolsExit)          { WindowExit(); } // Hide the window and stop libtorrent
		}

	// Buttons
	} else if (area->command == CommandReady || area->command == CommandSet) {

		int rows = ListRows(App.list.torrents.window);
		if      (area == &App.area.start)  { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(App.list.torrents.window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UseStart();  } } }
		else if (area == &App.area.pause)  { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(App.list.torrents.window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UsePause();  } } }
		else if (area == &App.area.remove) { for (int i = rows - 1; i >= 0; i--) { if (ListSelected(App.list.torrents.window, i)) { Torrent *t = ListGetTorrent(i); if (t) t->UseRemove(); } } }

	// Links
	} else if (area->command == CommandLink) {

	}
}

// Call when the program starts up
// Reads the optn.db file next to this running exe, and loads values in option
void OptionLoad() {

	CString torrents, trackers, peers, pieces, files;
	libtorrent::entry d;
	if (LoadEntry(PathOption(), d)) { // Loaded

		App.option.folder = widenStoC(d[narrowRtoS(L"folder")].string()); // Path to download folder
		App.option.associate = same(widenStoC(d[narrowRtoS(L"associate")].string()), L"t"); // True to associate magnet and torrent

		/*
		//TODO persist column customizations
		torrents = widenStoC(d[narrowRtoS(L"listtorrents")].string());
		trackers = widenStoC(d[narrowRtoS(L"listtrackers")].string());
		peers    = widenStoC(d[narrowRtoS(L"listpeers")].string());
		pieces   = widenStoC(d[narrowRtoS(L"listpieces")].string());
		files    = widenStoC(d[narrowRtoS(L"listfiles")].string());
		*/
	}

	// Replace blank or invalid with factory defaults
	if (isblank(App.option.folder)) App.option.folder = PathTorrents();

	// List view columns
	DefaultColumns();
	App.list.torrents.current = ColumnTextToList(is(torrents) ? torrents : App.list.torrents.factory);
	App.list.trackers.current = ColumnTextToList(is(trackers) ? trackers : App.list.trackers.factory);
	App.list.peers.current    = ColumnTextToList(is(peers)    ? peers    : App.list.peers.factory);
	App.list.pieces.current   = ColumnTextToList(is(pieces)   ? pieces   : App.list.pieces.factory);
	App.list.files.current    = ColumnTextToList(is(files)    ? files    : App.list.files.factory);
}

// Call when the program is shutting down
// Saves values from App.option to optn.db next to this running exe
void OptionSave() {

	libtorrent::entry::dictionary_type d;

	d[narrowRtoS(L"folder")] = narrowRtoS(App.option.folder);
	d[narrowRtoS(L"associate")] = App.option.associate ? narrowRtoS(L"t") : narrowRtoS(L"f");

	/*
	//TODO update current lists to get widths before you save these
	ColumnIntegrate(&App.list.torrents.current, ColumnWindowToList(App.list.torrents.window));
	ColumnIntegrate(&App.list.trackers.current, ColumnWindowToList(App.list.trackers.window));
	ColumnIntegrate(&App.list.peers.current,    ColumnWindowToList(App.list.peers.window));
	ColumnIntegrate(&App.list.pieces.current,   ColumnWindowToList(App.list.pieces.window));
	ColumnIntegrate(&App.list.files.current,    ColumnWindowToList(App.list.files.window));

	d[narrowRtoS(L"listtorrents")] = narrowRtoS(ColumnListToText(App.list.torrents.current));
	d[narrowRtoS(L"listtrackers")] = narrowRtoS(ColumnListToText(App.list.trackers.current));
	d[narrowRtoS(L"listpeers")]    = narrowRtoS(ColumnListToText(App.list.peers.current));
	d[narrowRtoS(L"listpieces")]   = narrowRtoS(ColumnListToText(App.list.pieces.current));
	d[narrowRtoS(L"listfiles")]    = narrowRtoS(ColumnListToText(App.list.files.current));
	*/

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
	MessageBox(App.window.main, r, PROGRAM_NAME, MB_ICONWARNING | MB_OK);
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
			CString magnet = TextDialog(dialog, IdentifyEdit); // Get the text the user typed
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

// A message from the create torrent box
BOOL CALLBACK DialogCreate(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {

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
	page[0].hInstance   = page[1].hInstance   = page[2].hInstance   = App.window.instance;
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
	header.hwndParent  = App.window.main;         // Handle to parent window
	header.hInstance   = App.window.instance;     // Handle to application instance
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

	CString before = TextDialog(dialog, IdentifyLabel); // "Label" when the dialog loads before we change it to "red", "yellow", or "green"
	CString now;
	if (AssociateIs())             now = L"green";  // We have the associations
	else if (App.option.associate) now = L"yellow"; // We tried to get them, but it didn't work
	else                           now = L"red";    // The user has not told the program to get them
	EnableWindow(GetDlgItem(dialog, IdentifyAssociate), now == L"red"); // Enable the button with red text

	TextDialogSet(dialog, IdentifyLabel, now); // Set the text of the hidden label for paint to use below
	if (before != L"Label" && before != now) InvalidateRect(dialog, NULL, true); // Repaint the dialog if the label changed
}

// A message from options page 1
BOOL APIENTRY DialogOptionsPage1(HWND dialog, UINT message, UINT wparam, LPARAM lparam) {

	// The page is about to be displayed
	switch (message) {
	case WM_INITDIALOG:

		TextDialogSet(dialog, IdentifyFolder, App.option.folder); // Torrents folder path
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
		CString code = TextDialog(dialog, IdentifyLabel);
		Brush *brush;
		CString message;
		if (code == L"red") {
			brush = &App.brush.rednotice;
			message = L"You haven't made " + PROGRAM_NAME + L" your default BitTorrent client:";

		} else if (code == L"yellow") {
			brush = &App.brush.yellownotice;
			message = L"Run " + PROGRAM_NAME + L" as administrator to make it your default BitTorrent client.";

		} else if (code == L"green") {
			brush = &App.brush.greennotice;
			message = L"Thanks for using " + PROGRAM_NAME + L" as your default BitTorrent client.";
		}

		// Calculate where in the dialog to paint
		RECT r1, r2, r3;
		GetWindowRect(dialog, &r1);
		GetWindowRect(GetDlgItem(dialog, IdentifyLabel), &r2); // The message text dialog item has false visibility
		GetClientRect(GetDlgItem(dialog, IdentifyLabel), &r3);
		Size size;
		size.x = r2.left - r1.left;
		size.y = r2.top - r1.top;
		size.w = r3.right;
		size.h = r3.bottom;
		RECT rectangle = size.Rectangle();

		// Custom paint the message in the dialog
		Device device;
		device.OpenPaint(dialog);
		device.Font(App.font.normal);
		device.FontColor(brush->color);
		if (!DrawText(device.device, message, -1, &rectangle, 0)) error(L"drawtext");
		return false;
	}
	// The user clicked a button on the page
	break;
	case WM_COMMAND:

		// Browse
		switch (LOWORD(wparam)) {
		case IdentifyBrowse:
		{
			CString browse = DialogBrowse(L"Choose the folder where " + PROGRAM_NAME + L" will download files:"); // Show the dialog
			if (is(browse)) TextDialogSet(dialog, IdentifyFolder, browse); // If the user picked something, write it in the text field
			return true; // We handled the message
		}
		// The user pressed the associate button
		break;
		case IdentifyAssociate:

			AssociateGet();
			AssociateUpdate(dialog);
		}

	// The user clicked one of the bottom property sheet buttons
	break;
	case WM_NOTIFY:

		// Check what the user entered on this page
		switch (((LPNMHDR)(ULONG_PTR)lparam)->code) {
		case PSN_KILLACTIVE:
		{
			CString folder = TextDialog(dialog, IdentifyFolder);
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

			App.option.folder = TextDialog(dialog, IdentifyFolder);
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
		device.BackgroundColor(App.brush.background.color);
		device.Font(App.font.arial);

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
		PaintFill(&device, blue, App.brush.blue.brush);
		PaintText(&device, about, title, false, false, false, false, 0, App.font.arial, &App.brush.lightblue, &App.brush.blue);
		PaintFill(&device, white, App.brush.background.brush);

		// Set heights
		int text = App.font.height; // Text height is usually 13
		int space = 5;

		// Size the text
		Size s = white;
		s.SetLeft(89);
		s.SetTop(46);
		s.h = text;

		// Paint the text
		device.Font(App.font.normal);
		PaintText(&device, PROGRAM_ABOUT1, s, false, false, false, false, 0, App.font.normal, &App.brush.ink, &App.brush.background); s.y += text + space;
		PaintText(&device, PROGRAM_ABOUT2, s, false, false, false, false, 0, App.font.normal, &App.brush.ink, &App.brush.background); s.y += text;
		PaintText(&device, PROGRAM_ABOUT3, s, false, false, false, false, 0, App.font.normal, &App.brush.ink, &App.brush.background); s.y += text + space;
		PaintText(&device, PROGRAM_ABOUT4, s, false, false, false, false, 0, App.font.normal, &App.brush.ink, &App.brush.background); s.y += text;
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
