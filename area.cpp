
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
	Area.tools.text  = L"Tools";
	Area.pause.text  = L"Pause";
	Area.remove.text = L"Remove";

	// Text size
	deviceitem device;
	device.Font(Handle.font);
	Area.tools.textsize  = SizeText(&device, Area.tools.text);
	Area.pause.textsize  = SizeText(&device, Area.pause.text);
	Area.remove.textsize = SizeText(&device, Area.remove.text);

	// Icons
	Area.tools.icon  = Handle.toolsicon;
	Area.pause.icon  = Handle.pauseicon;
	Area.remove.icon = Handle.removeicongray;

	// Link and size command states
	Area.tools.command    = CommandLink;
	Area.bar.command     = CommandSizeVertical;
	Area.corner.command  = CommandSizeDiagonal;
}

// Update the appearance of area items and issues commands that occur
void AreaPulse() {

	// Set button command states
	if (State.pause) Area.pause.command = CommandSet;
	else             Area.pause.command = CommandReady;
	if (false) Area.remove.command = CommandReady; //TODO change back to ListSelectedRows()
	else       Area.remove.command = CommandUnavailable;

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
		min.x = Area.sizemin.x + Area.stick.x;         // The closest the stick can be to the client origin
		min.y = Area.sizemin.y + Area.stick.y;
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

void Size(int move)
{
	// takes a number of pixels to move the bar
	// uses text sizes and client area dimensions to compute internal sizes, and moves the child window controls and areas

	/*

	// GET THE WIDTH AND HEIGHT OF THE CLIENT AREA
	sizeitem client;
	client = SizeClient();
	if (!client.Is()) return; // THE CLIENT SIZE IS 0 WHEN THE WINDOW IS MINIMIZED, DON'T SIZE AREAS

	// RECORD WHERE THE ADDRESS AND BAR ARE BEFORE THE SIZE
	sizeitem address, bar;
	address = Area.address.size;
	bar     = Area.bar.size;

	// ALL SIZE CONSTANTS FOR THE PROGRAM ARE DEFINED HERE AS LOCAL VARIABLES READ IN THIS FUNCTION
	int text, row, space, icon, big, tool, title;
	text  = Area.open.textsize.h;     // TEXT HEIGHT IS USUALLY 13
	row   = Area.open.textsize.h + 3; // ROW HEIGHT IS 16, 1 PIXEL ABOVE AND 2 BELOW
	space = 4;                             // SPACING
	icon  = 16;                            // SMALL SQUARE ICONS
	big   = 24;                            // LARGE SQUARE ICONS
	tool  = 56;                            // MINIMUM BUTTON WIDTH
	title = 23;                            // HEIGHT OF TITLE BAR ON TOP OF CLIENT AREA

	// FIND THE WIDEST TEXT THAT HAS TO FIT IN A BUTTON
	int longest;
	longest = Greatest(
		Area.pause.textsize.w,
		Area.remove.textsize.w,
		Area.back.textsize.w,
		Area.forward.textsize.w,
		Area.stop.textsize.w,
		Area.refresh.textsize.w,
		Area.expand.textsize.w,
		Area.get.textsize.w);

	// BUTTON HOLDS THE BUTTON WIDTH AND HEIGHT
	sizeitem button;
	button.w = Greatest(longest + 2 + (2 * space), tool);
	button.h = big + text + 7;

	// SIZE AND POSITION THE OPEN AND HELP LINKS
	Area.open.size.x = State.titlesize.w + (4 * space);
	Area.open.size.w = Area.open.textsize.w + (2 * space);
	Area.help.size.x = Area.open.size.Right();
	Area.help.size.w = Area.help.textsize.w + (2 * space);
	if (row < title) Area.open.size.h = Area.help.size.h = row;
	else             Area.open.size.h = Area.help.size.h = title; // IN CASE THE TEXT IS HIGHER THAN 23

	int wide;

	// SIZE AND POSITION THE PAUSE AND REMOVE BUTTONS
	Area.pause.size = Area.remove.size = button;
	Area.pause.size.x = space;
	Area.remove.size.x = Area.pause.size.Right();
	Area.pause.size.y = Area.remove.size.y = title + space;

	// MAKE THE HEIGHT OF ADDRESS AND GET BE ROW OR ICON, WHICHEVER IS BIGGER
	int link;
	if (row > icon) link = row;
	else            link = icon;

	// SIZE ADDRESS AND GET
	Area.address.size.w = Area.address.textsize.w;
	Area.enter.size.w = icon + space + Area.enter.textsize.w;
	Area.address.size.h = Area.enter.size.h = link;

	// CALCULATE HOW WIDE TO DRAW THIS PART
	wide = 1 + (8 * space) + (2 * button.w) + text + Area.address.size.w + Area.enter.size.w;
	wide = Greatest(wide, client.w);

	// POSITION ADDRESS AND GET
	Area.address.size.x = Area.remove.size.Right() + 1 + (3 * space);
	Area.enter.size.PositionRight(wide - (2 * space));
	Area.address.size.y = Area.enter.size.y = title + space;

	// COMPUTE THE MINIMUM AND MAXIMUM BAR Y DISTANCES
	int min, max;
	min = Area.pause.size.Bottom() - text - space - 1;
	max = client.h - row - 2 - space - text - space - 1;

	// MOVE THE BAR, NOT LETTING IT GO BEYOND THE BOUNDS
	if (!Area.bar.size.y) Area.bar.size.y = Area.pause.size.Bottom() - 1; // SET THE BAR
	Area.bar.size.y += move;
	if (Area.bar.size.y > max) Area.bar.size.y = max;
	if (Area.bar.size.y < min) Area.bar.size.y = min; // ENFORCE MIN FROM THE TOP IF BOTH ARE IN VIOLATION

	// BAR
	Area.bar.size.x = Area.address.size.Right() + space;
	Area.bar.size.w = wide - Area.address.size.Right() - Area.enter.size.w - (4 * space);
	Area.bar.size.h = space + 1;

	// EDIT
	Area.edit.x = Area.bar.size.x + 1;
	Area.edit.w = Area.bar.size.w - 2;
	Area.edit.y = title + space + 1;
	Area.edit.SetBottom(Area.bar.size.y);

	// BUTTON AND COPY
	Area.button.x = Area.bar.size.x;
	Area.button.w = text;
	Area.copy.size.x = Area.button.Right();
	Area.copy.size.w = space + Area.copy.textsize.w;
	if (Area.copy.size.Right() > Area.bar.size.Right()) Area.copy.size.SetRight(Area.bar.size.Right());
	Area.button.y = Area.copy.size.y = Area.bar.size.Bottom();
	Area.button.h = Area.copy.size.h = text;

	// LIST
	Area.list.x = 2;
	Area.list.w = client.w - 4;
	Area.list.y = Area.copy.size.Bottom() + space + 1;
	Area.list.SetBottom(client.h - row - 1);
	Area.list.Check(); // IF THE HEIGHT IS NEGATIVE, MAKE IT 0

	// DETERMINE WHERE THE SIZE CORNER WOULD BE IF THE WINDOW WERE VERY SMALL
	Area.sizemin.w = Area.sizemin.h = row;
	Area.sizemin.y = 2 + title + (2 * space) + button.h;

	// STATUS AND SIZE
	Area.status.y = Area.corner.size.y = Area.list.Bottom() + 1; // NEVER SIZE UP FROM THE BOTTOM OF THE WINDOW
	Area.status.h = Area.corner.size.h = row;
	Area.status.w = client.w - row;
	Area.corner.size.x = Area.status.Right();
	Area.corner.size.w = row;

	// IF THE WINDOW IS MAXIMIZED, MAKE STATUS THE ENTIRE ROW AND HIDE THE SIZE CORNER
	if (IsZoomed(Handle.window)) {

		Area.status.w = client.w;
		Area.corner.size.CloseRight();
	}

	// POSITION AND RESIZE CHILD WINDOW CONTROLS WITHOUT SENDING PAINT MESSAGES
	WindowMove(Handle.edit,   Area.edit);
	WindowMove(Handle.button, Area.button);
	WindowMove(Handle.tree,   Area.tree);
	WindowMove(Handle.list,   Area.list);

	// IF ADDRESS WAS GIVEN SIZE FOR THE FIRST TIME, ASSIGN THE TOOLTIP TO IT
	if (!address.Is()) TipAdd(Area.address.size, Area.address.tip);

	// IF THE BAR MOVED, PAINT THE WINDOW
	if (bar.y && bar.y != Area.bar.size.y) Paint();

	*/
}
