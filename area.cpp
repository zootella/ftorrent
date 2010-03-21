
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

// Make the areas for the window
void AreaCreate() {

	// Text
	Area.tools.text  = L"Tools   "; // Extra space for painted down arrow

	// Text size
	deviceitem device;
	device.OpenCreate();
	device.Font(Handle.font);
	Area.tools.textsize  = SizeText(&device, Area.tools.text);

	// Icons
	Area.tools.icon  = Handle.tools;

	// Link and size command states
	Area.tools.command   = CommandLink;
	Area.bar.command     = CommandSizeVertical;
	Area.corner.command  = CommandSizeDiagonal;
}

// Update the appearance of area items and issue commands that occur
void AreaPulse() {

	// Find what area the mouse is over, if it is inside the client area of the window, and if the primary button is up or down
	areaitem *over = MouseOver();
	bool inside = MouseInside();
	int pressing = GetKeyState(VK_LBUTTON) & 0x8000;

	// Set the pointer based on the area it pressed
	if (Area.pressed) {

		if      (Area.pressed->command == CommandReady)          CursorSet(Handle.hand);
		else if (Area.pressed->command == CommandSet)            CursorSet(Handle.hand);
		else if (Area.pressed->command == CommandLink)           CursorSet(Handle.hand);
		else if (Area.pressed->command == CommandSizeHorizontal) CursorSet(Handle.horizontal);
		else if (Area.pressed->command == CommandSizeVertical)   CursorSet(Handle.vertical);
		else if (Area.pressed->command == CommandSizeDiagonal)   CursorSet(Handle.diagonal);
		else                                                     CursorSet(Handle.arrow);

	// Set the pointer based on the area it's over
	} else if (over && !pressing) {

		if      (over->command == CommandReady)          CursorSet(Handle.hand);
		else if (over->command == CommandSet)            CursorSet(Handle.hand);
		else if (over->command == CommandLink)           CursorSet(Handle.hand);
		else if (over->command == CommandSizeHorizontal) CursorSet(Handle.horizontal);
		else if (over->command == CommandSizeVertical)   CursorSet(Handle.vertical);
		else if (over->command == CommandSizeDiagonal)   CursorSet(Handle.diagonal);
		else                                             CursorSet(Handle.arrow);

	// Neither of those, just a regular arrow
	} else if (inside) {

		CursorSet(Handle.arrow);
	}

	// Compose the display of each area and draw those that have changed
	areadisplay display;
	deviceitem device;
	areaitem *a = Area.all;
	while (a) {

		// Compose the display for the area
		display = DisplayNone;
		if (a->command == CommandUnavailable) {

			display = DisplayGhosted;

		} else if (a->command == CommandReady) {

			if      (a == over && a == Area.pressed) display = DisplayPressed;
			else if (a == over && !pressing)         display = DisplayHot;
			else                                     display = DisplayReady;

		} else if (a->command == CommandSet) {

			display = DisplayPressed;

		} else if (a->command == CommandLink) {

			if      (a == over && a == Area.pressed) display = DisplayHot;
			else if (a == over && !pressing)         display = DisplayHot;
			else                                     display = DisplayReady;
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
	if (Area.pressed) {

		// Get positions in client coordinates
		sizeitem mouse, stick, min, move;
		mouse = MouseClient();                         // Where the mouse is
		stick.x = Area.pressed->size.x + Area.stick.x; // The stick is the point the mouse is dragging
		stick.y = Area.pressed->size.y + Area.stick.y;
		min.x = Area.collapse.x + Area.stick.x;        // The closest the stick can be to the client origin
		min.y = Area.collapse.y + Area.stick.y;
		move.x = mouse.x - stick.x;                    // From the stick to the mouse
		move.y = mouse.y - stick.y;

		// If the mouse is away from the stick, try to move the stick to it
		if (Area.pressed->command == CommandSizeHorizontal && move.x != 0) {

			// Horizontal bar
			Size(move.x);

		} else if (Area.pressed->command == CommandSizeVertical && move.y != 0) {

			// Vertical bar
			Size(move.y);

		} else if (Area.pressed->command == CommandSizeDiagonal && (move.x != 0 || move.y != 0)) {

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

	// COUNT HOW MANY BOT ITEMS ARE PENDING
	pending = 0;
	botitem *b;
	b = Data.bot;
	while (b) {

		if (b->status == StatusPending) pending++;

	b = b->next; }
	*/

	// Compose status text
	string s = saynumber(rows, L"file");
	if (pending)  s += L"  " + insertcommas(numerals(pending))  + L" to get";
	if (selected) s += L"  " + insertcommas(numerals(selected)) + L" selected";

	// The status text is different
	if (State.status != s) {
		State.status = s; // Update it

		// The status bar has size
		if (Area.status.Is()) {

			// Get the window device context if we don't have it already
			if (device.open == DeviceNone) {

				device.OpenGet(Handle.window);
				device.Font(Handle.font);
				device.BackgroundColor(Handle.background.color);
			}

			// Paint the status text to the window
			PaintText(&device, State.status, Area.status, false, true, true, true);
		}
	}
}

// Call this before launching a message box, popup menu, or dialog box that blocks and starts processing messages
// Makes the program abandon the mouse and think it's always outside the client area
void AreaPopUp() {

	// Clear record of the area the mouse pressed and release the mouse if captured
	Area.pressed = NULL;
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

// Suggest list column widths based on the width of the client area
void SizeColumns(int *width1, int *width2, int *width3, int *width4, int *width5, int *width6) {

	// Get sizes from the system
	sizeitem client = SizeClient(); // The main window has a size even though it's not on the screen
	int scroll = GetSystemMetrics(SM_CXVSCROLL); // The width of the vertical scroll bar, usually 16 pixels

	// Set the widths of the first 4 columns
	*width1 = 130;
	*width2 = 120;
	*width3 = 110;
	*width4 = 120;

	// Calculate the width of the last 2 columns
	int last = (client.w - 4 - *width1 - *width2 - *width3 - *width4 - (scroll * 2)) / 2;
	if (last < 120) last = 120;
	*width5 = *width6 = last;
}

// Takes a number of pixels to move the bar
// Uses text sizes and client area dimensions to compute internal sizes, and moves the child window controls and areas
void Size(int move) {

	// Remember how things are now
	sizeitem client = SizeClient();  // Get the width and height of the client area
	if (!client.Is()) return;        // The client size is 0 when the window is minimized, don't size areas
	sizeitem before = Area.bar.size; // Record where the bar is before the size

	// All size constants for the program are defined here as local variables to be read in this function
	int text = Area.tools.textsize.h; // Text height on Windows XP is usually 13
	int icon = 16;                    // Small square icons

	// Heights
	int title  = 23; // Height of status title band at the top of the client area
	int bar    = 4;
	int tabs   = 30; //TODO somehow, up here figure out how tall the tabs want to be
	int status = text + 3;

	// Toolbar spacing constants
	int a = 4; // Toolbar left margin
	int b = 3; // Toolbar top margin
	int c = 2; // Space between toolbar icon and label
	int d = 1; // Labels drop this far beneath icons
	int e = 4; // Space between toolbar items

	// Toolbar items
	int margin = a;
	Area.tools.sizeicon.x = margin;
	Area.tools.sizeicon.y = b;
	Area.tools.sizeicon.w = icon;
	Area.tools.sizeicon.h = icon;
	Area.tools.sizetext.x = margin + icon + c;
	Area.tools.sizetext.y = b + d;
	Area.tools.sizetext.w = Area.tools.textsize.w;
	Area.tools.sizetext.h = Area.tools.textsize.h;
	Area.tools.size.x = margin;
	Area.tools.size.y = b;
	Area.tools.size.w = icon + c + Area.tools.textsize.w;
	Area.tools.size.h = Greatest(icon, d + text);
	margin += Area.tools.size.w;

	// Bar
	int min = title; // Compute the minimum and maximum bar y distances
	int max = client.h - bar - status;
	if (!Area.bar.size.y) Area.bar.size.y = client.h / 2; // Set the bar
	Area.bar.size.y += move; // Move the bar
	if (Area.bar.size.y > max) Area.bar.size.y = max; // Don't let it go beyond the bounds
	if (Area.bar.size.y < min) Area.bar.size.y = min; // Enforce min from the top if both are in violation
	Area.bar.size.x = 0;
	Area.bar.size.w = client.w;
	Area.bar.size.h = bar;

	// Title
	Area.title.w = client.w;
	Area.title.h = title;

	// List
	Area.list.y = Area.title.Bottom();
	Area.list.w = client.w;
	Area.list.SetBottom(Area.bar.size.y);
	Area.list.Check(); // If the height is negative, make it 0

	// Tabs
	Area.tabs.y = Area.bar.size.Bottom();
	Area.tabs.w = client.w;
	Area.tabs.h = tabs;

	// Info
	Area.info.y = Area.tabs.Bottom();
	Area.info.w = client.w;
	Area.info.h = client.h - title - Area.list.h - bar - tabs - status;

	// Status and corner
	Area.collapse.y = title + bar + tabs; // Determine where corner would be if the window were very small
	Area.collapse.w = status;
	Area.collapse.h = status;
	Area.corner.size.x = Greatest(Area.collapse.x, client.w - status); // Corner
	Area.corner.size.y = Greatest(Area.collapse.y, Area.info.Bottom());
	Area.corner.size.w = status;
	Area.corner.size.h = status;
	Area.status.y = Area.corner.size.y; // Status
	Area.status.w = Greatest(0, client.w - status);
	Area.status.h = status;
	if (IsZoomed(Handle.window)) { // If the window is maximized, make status the entire row and hide the size corner
		Area.status.w = client.w;
		Area.corner.size.CloseRight();
	}

	// Position and resize child window controls without sending paint messages
	WindowMove(Handle.list, Area.list);
	WindowMove(Handle.tabs, Area.tabs);
	WindowMove(Handle.edit, Area.info);

	// If the bar moved, paint the window
	if (before.y && before.y != Area.bar.size.y) PaintMessage();
}
