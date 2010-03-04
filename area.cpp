
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
extern handleitem Handle;

/*
void AreaCreate()
{
	// takes nothing
	// creates the areas for the window
	// returns nothing

	if (State.mode == ModeDownload) {

		// ICONS
		Draw.area.pause.icon  = Draw.icon.buttonpause;
		Draw.area.remove.icon = Draw.icon.buttonremove;
		Draw.area.remove.gray = Draw.icon.grayremove;
		Draw.area.enter.icon  = Draw.icon.linkget;

		// TEXT
		Draw.title             = "ltorrent 3 download";
		Draw.area.open.text    = "Open   ";
		Draw.area.help.text    = "Help...";
		Draw.area.pause.text   = "Pause";
		Draw.area.remove.text  = "Remove";
		Draw.area.address.text = "Addresses";
		Draw.area.address.tip  = "Click to Paste and Get";
		Draw.area.enter.text   = "Get";
		Draw.area.copy.text    = "Get addresses copied in other programs (Right-click links in Internet Explorer and press the T key)";

		// LINK AND SIZE COMMAND STATES
		Draw.area.open.command    = CommandLink;
		Draw.area.help.command    = CommandLink;
		Draw.area.address.command = CommandLink;
		Draw.area.enter.command   = CommandLink;
		Draw.area.copy.command    = CommandLink;
		Draw.area.bar.command     = CommandSizeVertical;
		Draw.area.corner.command  = CommandSizeDiagonal;

	} else if (State.mode == ModeSite) {

		// ICONS
		Draw.area.back.icon    = Draw.icon.buttonback;
		Draw.area.back.gray    = Draw.icon.grayback;
		Draw.area.forward.icon = Draw.icon.buttonforward;
		Draw.area.forward.gray = Draw.icon.grayforward;
		Draw.area.stop.icon    = Draw.icon.buttonstop;
		Draw.area.stop.gray    = Draw.icon.graystop;
		Draw.area.refresh.icon = Draw.icon.buttonrefresh;
		Draw.area.refresh.gray = Draw.icon.grayrefresh;
		Draw.area.expand.icon  = Draw.icon.buttonexpand;
		Draw.area.get.icon     = Draw.icon.buttonget;
		Draw.area.get.gray     = Draw.icon.grayget;
		Draw.area.enter.icon   = Draw.icon.linkgo;

		// TEXT
		Draw.title             = "ltorrent 3 site";
		Draw.area.open.text    = "Open   "; // EXTRA SPACE FOR PAINTED DOWN ARROW
		Draw.area.help.text    = "Help...";
		Draw.area.back.text    = "Back";
		Draw.area.forward.text = "Forward";
		Draw.area.stop.text    = "Stop";
		Draw.area.refresh.text = "Refresh";
		Draw.area.expand.text  = "Expand";
		Draw.area.get.text     = "Get";
		Draw.area.address.text = "Address";
		Draw.area.address.tip  = "Click to Paste and Go";
		Draw.area.enter.text   = "Go";

		// ADJUST TEXT
		Draw.area.back.adjust   = 0;
		Draw.area.expand.adjust = 0;

		// LINK AND SIZE COMMAND STATES
		Draw.area.open.command    = CommandLink;
		Draw.area.help.command    = CommandLink;
		Draw.area.address.command = CommandLink;
		Draw.area.enter.command   = CommandLink;
		Draw.area.bar.command     = CommandSizeHorizontal;
		Draw.area.corner.command  = CommandSizeDiagonal;
	}

	// SET THE AREA DISPLAY STATES
	AreaPulse();
}

void AreaPulse()
{
	// takes nothing
	// updates the appearance of area items and issues commands that occur
	// returns nothing

	// SET BUTTON COMMAND STATES
	if (State.mode == ModeDownload) {

		if (State.option.pause) Draw.area.pause.command  = CommandSet;   else Draw.area.pause.command  = CommandReady;
		if (ListSelectedRows()) Draw.area.remove.command = CommandReady; else Draw.area.remove.command = CommandUnavailable;

	} else if (State.mode == ModeSite) {

		if (Data.address && Data.address->back)    Draw.area.back.command    = CommandReady; else Draw.area.back.command    = CommandUnavailable;
		if (Data.address && Data.address->forward) Draw.area.forward.command = CommandReady; else Draw.area.forward.command = CommandUnavailable;
		if (!State.stop && SiteHasPending())       Draw.area.stop.command    = CommandReady; else Draw.area.stop.command    = CommandUnavailable;
		if (Data.tree)                             Draw.area.refresh.command = CommandReady; else Draw.area.refresh.command = CommandUnavailable;
		if (State.expand)                          Draw.area.expand.command  = CommandSet;   else Draw.area.expand.command  = CommandReady;
		if (ListSelectedRows())                    Draw.area.get.command     = CommandReady; else Draw.area.get.command     = CommandUnavailable;
	}

	// FIND WHAT AREA THE MOUSE IS OVER, IF IT IS INSIDE THE CLIENT AREA OF THE WINDOW, AND IF THE PRIMARY BUTTON IS UP OR DOWN
	areaitem *over;
	bool inside, pressing;
	over = MouseOver();
	inside = MouseInside();
	if (GetKeyState(VK_LBUTTON) & 0x8000) pressing = true;
	else pressing = false;

	// SET THE POINTER BASED ON THE AREA IT PRESSED
	if (Draw.area.pressed) {

		if      (Draw.area.pressed->command == CommandReady)          CursorSet(Draw.cursor.hand);
		else if (Draw.area.pressed->command == CommandSet)            CursorSet(Draw.cursor.hand);
		else if (Draw.area.pressed->command == CommandLink)           CursorSet(Draw.cursor.hand);
		else if (Draw.area.pressed->command == CommandSizeHorizontal) CursorSet(Draw.cursor.horizontal);
		else if (Draw.area.pressed->command == CommandSizeVertical)   CursorSet(Draw.cursor.vertical);
		else if (Draw.area.pressed->command == CommandSizeDiagonal)   CursorSet(Draw.cursor.diagonal);
		else                                                          CursorSet(Draw.cursor.arrow);

	// SET THE POINTER BASED ON THE AREA IT IS OVER
	} else if (over && !pressing) {

		if      (over->command == CommandReady)          CursorSet(Draw.cursor.hand);
		else if (over->command == CommandSet)            CursorSet(Draw.cursor.hand);
		else if (over->command == CommandLink)           CursorSet(Draw.cursor.hand);
		else if (over->command == CommandSizeHorizontal) CursorSet(Draw.cursor.horizontal);
		else if (over->command == CommandSizeVertical)   CursorSet(Draw.cursor.vertical);
		else if (over->command == CommandSizeDiagonal)   CursorSet(Draw.cursor.diagonal);
		else                                             CursorSet(Draw.cursor.arrow);

	// SET THE POINTER TO ARROW
	} else if (inside) {

		CursorSet(Draw.cursor.arrow);
	}

	// COMPOSE THE DISPLAY OF EACH AREA AND DRAW THOSE THAT HAVE CHANGED
	areadisplay display;
	deviceitem device;
	areaitem *a;
	a = Draw.area.all;
	while (a) {

		// COMPOSE THE DISPLAY FOR THE AREA
		display = DisplayNone;
		if (a->command == CommandUnavailable) {

			display = DisplayGhosted;

		} else if (a->command == CommandReady) {

			if      (a == over && a == Draw.area.pressed) display = DisplayPressed;
			else if (a == over && !pressing)              display = DisplayHot;
			else                                          display = DisplayReady;

		} else if (a->command == CommandSet) {

			display = DisplayPressed;

		} else if (a->command == CommandLink) {

			if      (a == over && a == Draw.area.pressed) display = DisplayHot;
			else if (a == over && !pressing)              display = DisplayHot;
			else                                          display = DisplayReady;
		}

		// THE DISPLAY OF THE AREA ON THE SCREEN IS DIFFERENT FROM THE NEWLY COMPOSED DISPLAY
		if (a->display != display) {

			// UPDATE THE AREA'S DISPLAY
			a->display = display;

			// GET THE WINDOW DEVICE CONTEXT IF IT HASN'T ALREADY BEEN OBTAINED AND PAINT THE AREA
			if (device.open == DeviceNone) {

				device.OpenGet(Handle.window);
				device.Font(Draw.font.normal);
				device.BackgroundColor(Draw.color.window.color);
			}

			if (device.device) PaintArea(&device, a);
		}

		a = a->next;
	}

	// IF THE PROGRAM IS NOT OWNED AND THE GET BUTTON IT HOT, SHOW THE SIGN
	if (!State.owned && Draw.area.get.display == DisplayHot) Sign();

	// ADJUST SIZE
	if (Draw.area.pressed) {

		// GET POSITIONS IN CLIENT COORDINATES
		sizeitem mouse, stick, min, move;
		mouse = MouseClient();                                   // WHERE THE MOUSE IS
		stick.x = Draw.area.pressed->size.x + Draw.area.stick.x; // THE STICK IS THE POINT THE MOUSE IS DRAGGING
		stick.y = Draw.area.pressed->size.y + Draw.area.stick.y;
		min.x = Draw.area.sizemin.x + Draw.area.stick.x;         // THE CLOSEST THE STICK CAN BE TO THE CLIENT ORIGIN
		min.y = Draw.area.sizemin.y + Draw.area.stick.y;
		move.x = mouse.x - stick.x;                              // FROM THE STICK TO THE MOUSE
		move.y = mouse.y - stick.y;

		// IF THE MOUSE IS AWAY FROM THE STICK, TRY TO MOVE THE STICK TO IT
		if (Draw.area.pressed->command == CommandSizeHorizontal && move.x != 0) {

			// HORIZONTAL BAR
			Size(move.x);

		} else if (Draw.area.pressed->command == CommandSizeVertical && move.y != 0) {

			// VERTICAL BAR
			Size(move.y);

		} else if (Draw.area.pressed->command == CommandSizeDiagonal && (move.x != 0 || move.y != 0)) {

			// DON'T TRY TO MOVE THE STICK CLOSER TO THE CLIENT ORIGIN THAN MIN
			if (mouse.x < min.x) move.x = min.x - stick.x;
			if (mouse.y < min.y) move.y = min.y - stick.y;

			// DIAGONAL SIZE CORNER
			WindowSize(Handle.window, move.x, move.y);
		}
	}

	// FIND OUT HOW MANY ROWS THERE ARE, AND HOW MANY ARE SELECTED
	int rows, selected, pending;
	rows = ListRows();
	selected = ListSelectedRows();

	// COUNT HOW MANY BOT ITEMS ARE PENDING
	pending = 0;
	botitem *b;
	b = Data.bot;
	while (b) {

		if (b->status == StatusPending) pending++;

	b = b->next; }

	// COMPOSE STATUS TEXT
	string s;
	s = writenumber(rows, "file");
	if (pending)  s += "  " + writecommas(numerals(pending))  + " to get";
	if (selected) s += "  " + writecommas(numerals(selected)) + " selected";

	// THE STATUS TEXT IS DIFFERENT
	if (State.status != s) {

		// UPDATE THE STATUS TEXT
		State.status = s;

		// THE STATUS BAR HAS SIZE
		if (Draw.area.status.Is()) {

			// GET THE WINDOW DEVICE CONTEXT IF IT HASN'T ALREADY BEEN OBTAINED
			if (device.open == DeviceNone) {

				device.OpenGet(Handle.window);
				device.Font(Draw.font.normal);
				device.BackgroundColor(Draw.color.window.color);
			}

			// PAINT THE STATUS TEXT TO THE WINDOW
			PaintText(&device, State.status, Draw.area.status, false, true, true, true);
		}
	}
}

void AreaPopUp()
{
	// call this before launching a message box, popup menu, or dialog box that blocks and starts processing messages
	// takes nothing
	// makes the program abandon the mouse and think it's always outside the client area
	// returns nothing

	// CLEAR RECORD OF THE AREA THE MOUSE PRESSED AND RELEASE THE MOUSE IF CAPTURED
	Draw.area.pressed = NULL;
	MouseRelease();

	// RECORD THERE IS ONE MORE POP UP WINDOW
	State.pop++;

	// PULSE THE AREA NOW AS THE PEEKING POPUP WON'T PULSE ON IDLE
	AreaPulse();
}

void AreaPopDown()
{
	// call this after the message box, popup menu, or dialog box that blocked and processed messages is gone
	// takes nothing
	// lets the program see the mouse position again
	// returns nothing

	// RECORD THERE IS ONE LESS POP UP WINDOW
	State.pop--;
}

void SizeColumns(int *width1, int *width2, int *width3, int *width4, int *width5, int *width6)
{
	// takes nothing
	// uses the client area width to set the list column widths
	// returns nothing, writes widths

	// GET SIZES FROM THE SYSTEM
	sizeitem client;
	client = SizeClient(); // THE MAIN WINDOW HAS A SIZE EVEN THOUGH IT IS NOT ON THE SCREEN
	int scroll;
	scroll = GetSystemMetrics(SM_CXVSCROLL); // THE WIDTH OF THE VERTICAL SCROLL BAR, USUALLY 16 PIXELS

	if (State.mode == ModeDownload) {

		// SET THE WIDTHS OF THE FIRST 4 COLUMNS
		*width1 = 130;
		*width2 = 120;
		*width3 = 110;
		*width4 = 120;

		// CALCULATE THE WIDTH OF THE LAST 2 COLUMNS
		int last;
		last = (client.w - 4 - *width1 - *width2 - *width3 - *width4 - (scroll * 2)) / 2;
		if (last < 120) last = 120;
		*width5 = *width6 = last;

	} else if (State.mode == ModeSite) {

		// SET THE WIDTHS OF THE FIRST 4 COLUMNS
		*width1 = 120;
		*width2 = 120;
		*width3 = 120;
		*width4 = 120;

		// CALCULATE THE WIDTH OF THE LAST COLUMN
		*width5 = client.w - ((client.w - 10) / 4) - 10 - *width1 - *width2 - *width3 - *width4 - (scroll * 2);
		if (*width5 < 120) *width5 = 120;
		*width6 = 0;
	}
}

void Size(int move)
{
	// takes a number of pixels to move the bar
	// uses text sizes and client area dimensions to compute internal sizes, and moves the child window controls and areas
	// returns nothing

	// GET THE WIDTH AND HEIGHT OF THE CLIENT AREA
	sizeitem client;
	client = SizeClient();
	if (!client.Is()) return; // THE CLIENT SIZE IS 0 WHEN THE WINDOW IS MINIMIZED, DON'T SIZE AREAS

	// RECORD WHERE THE ADDRESS AND BAR ARE BEFORE THE SIZE
	sizeitem address, bar;
	address = Draw.area.address.size;
	bar     = Draw.area.bar.size;

	// ALL SIZE CONSTANTS FOR THE PROGRAM ARE DEFINED HERE AS LOCAL VARIABLES READ IN THIS FUNCTION
	int text, row, space, icon, big, tool, title;
	text  = Draw.area.open.textsize.h;     // TEXT HEIGHT IS USUALLY 13
	row   = Draw.area.open.textsize.h + 3; // ROW HEIGHT IS 16, 1 PIXEL ABOVE AND 2 BELOW
	space = 4;                             // SPACING
	icon  = 16;                            // SMALL SQUARE ICONS
	big   = 24;                            // LARGE SQUARE ICONS
	tool  = 56;                            // MINIMUM BUTTON WIDTH
	title = 23;                            // HEIGHT OF TITLE BAR ON TOP OF CLIENT AREA

	// FIND THE WIDEST TEXT THAT HAS TO FIT IN A BUTTON
	int longest;
	longest = Greatest(
		Draw.area.pause.textsize.w,
		Draw.area.remove.textsize.w,
		Draw.area.back.textsize.w,
		Draw.area.forward.textsize.w,
		Draw.area.stop.textsize.w,
		Draw.area.refresh.textsize.w,
		Draw.area.expand.textsize.w,
		Draw.area.get.textsize.w);

	// BUTTON HOLDS THE BUTTON WIDTH AND HEIGHT
	sizeitem button;
	button.w = Greatest(longest + 2 + (2 * space), tool);
	button.h = big + text + 7;

	// SIZE AND POSITION THE OPEN AND HELP LINKS
	Draw.area.open.size.x = Draw.titlesize.w + (4 * space);
	Draw.area.open.size.w = Draw.area.open.textsize.w + (2 * space);
	Draw.area.help.size.x = Draw.area.open.size.Right();
	Draw.area.help.size.w = Draw.area.help.textsize.w + (2 * space);
	if (row < title) Draw.area.open.size.h = Draw.area.help.size.h = row;
	else             Draw.area.open.size.h = Draw.area.help.size.h = title; // IN CASE THE TEXT IS HIGHER THAN 23

	int wide;
	if (State.mode == ModeDownload) {

		// SIZE AND POSITION THE PAUSE AND REMOVE BUTTONS
		Draw.area.pause.size = Draw.area.remove.size = button;
		Draw.area.pause.size.x = space;
		Draw.area.remove.size.x = Draw.area.pause.size.Right();
		Draw.area.pause.size.y = Draw.area.remove.size.y = title + space;

		// MAKE THE HEIGHT OF ADDRESS AND GET BE ROW OR ICON, WHICHEVER IS BIGGER
		int link;
		if (row > icon) link = row;
		else            link = icon;

		// SIZE ADDRESS AND GET
		Draw.area.address.size.w = Draw.area.address.textsize.w;
		Draw.area.enter.size.w = icon + space + Draw.area.enter.textsize.w;
		Draw.area.address.size.h = Draw.area.enter.size.h = link;

		// CALCULATE HOW WIDE TO DRAW THIS PART
		wide = 1 + (8 * space) + (2 * button.w) + text + Draw.area.address.size.w + Draw.area.enter.size.w;
		wide = Greatest(wide, client.w);

		// POSITION ADDRESS AND GET
		Draw.area.address.size.x = Draw.area.remove.size.Right() + 1 + (3 * space);
		Draw.area.enter.size.PositionRight(wide - (2 * space));
		Draw.area.address.size.y = Draw.area.enter.size.y = title + space;

		// COMPUTE THE MINIMUM AND MAXIMUM BAR Y DISTANCES
		int min, max;
		min = Draw.area.pause.size.Bottom() - text - space - 1;
		max = client.h - row - 2 - space - text - space - 1;

		// MOVE THE BAR, NOT LETTING IT GO BEYOND THE BOUNDS
		if (!Draw.area.bar.size.y) Draw.area.bar.size.y = Draw.area.pause.size.Bottom() - 1; // SET THE BAR
		Draw.area.bar.size.y += move;
		if (Draw.area.bar.size.y > max) Draw.area.bar.size.y = max;
		if (Draw.area.bar.size.y < min) Draw.area.bar.size.y = min; // ENFORCE MIN FROM THE TOP IF BOTH ARE IN VIOLATION

		// BAR
		Draw.area.bar.size.x = Draw.area.address.size.Right() + space;
		Draw.area.bar.size.w = wide - Draw.area.address.size.Right() - Draw.area.enter.size.w - (4 * space);
		Draw.area.bar.size.h = space + 1;

		// EDIT
		Draw.area.edit.x = Draw.area.bar.size.x + 1;
		Draw.area.edit.w = Draw.area.bar.size.w - 2;
		Draw.area.edit.y = title + space + 1;
		Draw.area.edit.SetBottom(Draw.area.bar.size.y);

		// BUTTON AND COPY
		Draw.area.button.x = Draw.area.bar.size.x;
		Draw.area.button.w = text;
		Draw.area.copy.size.x = Draw.area.button.Right();
		Draw.area.copy.size.w = space + Draw.area.copy.textsize.w;
		if (Draw.area.copy.size.Right() > Draw.area.bar.size.Right()) Draw.area.copy.size.SetRight(Draw.area.bar.size.Right());
		Draw.area.button.y = Draw.area.copy.size.y = Draw.area.bar.size.Bottom();
		Draw.area.button.h = Draw.area.copy.size.h = text;

		// LIST
		Draw.area.list.x = 2;
		Draw.area.list.w = client.w - 4;
		Draw.area.list.y = Draw.area.copy.size.Bottom() + space + 1;
		Draw.area.list.SetBottom(client.h - row - 1);
		Draw.area.list.Check(); // IF THE HEIGHT IS NEGATIVE, MAKE IT 0

		// DETERMINE WHERE THE SIZE CORNER WOULD BE IF THE WINDOW WERE VERY SMALL
		Draw.area.sizemin.w = Draw.area.sizemin.h = row;
		Draw.area.sizemin.y = 2 + title + (2 * space) + button.h;

	} else if (State.mode == ModeSite) {

		// BUTTONS
		button.x = space;
		button.y = title + space;
		Draw.area.back.size = Draw.area.forward.size = Draw.area.stop.size = Draw.area.refresh.size = Draw.area.expand.size = Draw.area.get.size = button;
		Draw.area.forward.size.x = Draw.area.back.size.Right();
		Draw.area.stop.size.x = Draw.area.forward.size.Right();
		Draw.area.refresh.size.x = Draw.area.stop.size.Right();
		Draw.area.expand.size.x = Draw.area.refresh.size.Right() + 1 + (2 * space);
		Draw.area.get.size.x = Draw.area.expand.size.Right() + 1 + (2 * space);

		// SIZE ADDRESS AND ENTER
		Draw.area.address.size.h = Draw.area.enter.size.h = row;
		Draw.area.address.size.w = Draw.area.address.textsize.w;
		Draw.area.enter.size.w = icon + space + Draw.area.enter.textsize.w;

		// CALCULATE HOW WIDE TO DRAW THIS PART
		wide = 2 + (6 * space) + Draw.area.address.size.w + Draw.area.enter.size.w;
		wide = Greatest(wide, client.w);

		// POSITION ADDRESS AND ENTER
		Draw.area.address.size.x = 2 * space;
		Draw.area.enter.size.PositionRight(wide - (2 * space));
		Draw.area.address.size.y = Draw.area.enter.size.y = button.Bottom() + 3 + (2 * space);

		// EDIT
		Draw.area.edit = Draw.area.address.size;
		Draw.area.edit.y++;
		Draw.area.edit.h -= 3;
		Draw.area.edit.x = Draw.area.address.size.Right() + space + 2;
		Draw.area.edit.SetRight(Draw.area.enter.size.x - space - 1);
		if (Draw.area.edit.w < 0) Draw.area.edit.CloseRight(); // SINCE WIDTH IS NEGATIVE, WILL MOVE THE AREA TO THE LEFT

		// SET BAR WIDTH, HEIGHT, AND VERTICAL POSITION
		Draw.area.bar.size.w = 2 + space;
		Draw.area.bar.size.y = Draw.area.address.size.Bottom() + 2 + space;
		Draw.area.bar.size.SetBottom(client.h - row);
		Draw.area.bar.size.h = Greatest(Draw.area.bar.size.h, 2);

		// POSITION, MOVE, AND CHECK THE BAR
		if (!Draw.area.bar.size.x) Draw.area.bar.size.x = ((client.w - 10) / 4) + 2;
		Draw.area.bar.size.x += move;
		if (Draw.area.bar.size.Right() > client.w - 2) Draw.area.bar.size.PositionRight(client.w - 2);
		Draw.area.bar.size.x = Greatest(Draw.area.bar.size.x, 2); // ENFORCE LEFT IF BOTH ARE IN VIOLATION

		// TREE AND LIST
		Draw.area.tree = Draw.area.bar.size;
		Draw.area.tree.Expand(-1);
		Draw.area.tree.x = 2;
		Draw.area.tree.SetRight(Draw.area.bar.size.x);
		Draw.area.list = Draw.area.tree;
		Draw.area.list.x = Draw.area.bar.size.Right();
		Draw.area.list.SetRight(client.w - 2);
		Draw.area.list.Check();

		// DETERMINE WHERE THE SIZE CORNER WOULD BE IF THE WINDOW WERE VERY SMALL
		Draw.area.sizemin.w = Draw.area.sizemin.h = row;
		Draw.area.sizemin.y = 7 + (4 * space) + row + title + button.h;
	}

	// STATUS AND SIZE
	Draw.area.status.y = Draw.area.corner.size.y = Draw.area.list.Bottom() + 1; // NEVER SIZE UP FROM THE BOTTOM OF THE WINDOW
	Draw.area.status.h = Draw.area.corner.size.h = row;
	Draw.area.status.w = client.w - row;
	Draw.area.corner.size.x = Draw.area.status.Right();
	Draw.area.corner.size.w = row;

	// IF THE WINDOW IS MAXIMIZED, MAKE STATUS THE ENTIRE ROW AND HIDE THE SIZE CORNER
	if (IsZoomed(Handle.window)) {

		Draw.area.status.w = client.w;
		Draw.area.corner.size.CloseRight();
	}

	// POSITION AND RESIZE CHILD WINDOW CONTROLS WITHOUT SENDING PAINT MESSAGES
	WindowMove(Handle.edit,   Draw.area.edit);
	WindowMove(Handle.button, Draw.area.button);
	WindowMove(Handle.tree,   Draw.area.tree);
	WindowMove(Handle.list,   Draw.area.list);

	// IF ADDRESS WAS GIVEN SIZE FOR THE FIRST TIME, ASSIGN THE TOOLTIP TO IT
	if (!address.Is()) TipAdd(Draw.area.address.size, Draw.area.address.tip);

	// IF THE BAR MOVED, PAINT THE WINDOW
	if      (State.mode == ModeDownload) { if (bar.y && bar.y != Draw.area.bar.size.y) Paint(); }
	else if (State.mode == ModeSite)     { if (bar.x && bar.x != Draw.area.bar.size.x) Paint(); }
}
*/