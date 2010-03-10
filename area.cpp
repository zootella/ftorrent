
// Include statements
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>
#include "resource.h"
#include "program.h"
#include "class.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern drawtop   Draw;
extern datatop   Data;
extern statetop  State;

void AreaCreate()
{
	// takes nothing
	// creates the areas for the window
	// returns nothing

	// ICONS
	/*
	Draw.pause.icon  = Draw.icon.buttonpause;
	Draw.remove.icon = Draw.icon.buttonremove;
	Draw.remove.gray = Draw.icon.grayremove;
	Draw.enter.icon  = Draw.icon.linkget;
	*/

	// TEXT
	Draw.open.text    = "Open   ";
	Draw.help.text    = "Help...";
	Draw.pause.text   = "Pause";
	Draw.remove.text  = "Remove";
	Draw.address.text = "Addresses";
	Draw.address.tip  = "Click to Paste and Get";
	Draw.enter.text   = "Get";
	Draw.copy.text    = "Get addresses copied in other programs (Right-click links in Internet Explorer and press the T key)";

	// LINK AND SIZE COMMAND STATES
	Draw.open.command    = CommandLink;
	Draw.help.command    = CommandLink;
	Draw.address.command = CommandLink;
	Draw.enter.command   = CommandLink;
	Draw.copy.command    = CommandLink;
	Draw.bar.command     = CommandSizeVertical;
	Draw.corner.command  = CommandSizeDiagonal;

	// SET THE AREA DISPLAY STATES
	AreaPulse();
}

void AreaPulse()
{
	// takes nothing
	// updates the appearance of area items and issues commands that occur
	// returns nothing

	// SET BUTTON COMMAND STATES
	if (State.pause) Draw.pause.command  = CommandSet;   else Draw.pause.command  = CommandReady;
	if (false /*ListSelectedRows()*/) Draw.remove.command = CommandReady; else Draw.remove.command = CommandUnavailable;

	// FIND WHAT AREA THE MOUSE IS OVER, IF IT IS INSIDE THE CLIENT AREA OF THE WINDOW, AND IF THE PRIMARY BUTTON IS UP OR DOWN
	areaitem *over;
	bool inside, pressing;
	over = MouseOver();
	inside = MouseInside();
	if (GetKeyState(VK_LBUTTON) & 0x8000) pressing = true;
	else pressing = false;

	// SET THE POINTER BASED ON THE AREA IT PRESSED
	if (Draw.pressed) {

		if      (Draw.pressed->command == CommandReady)          CursorSet(Handle.hand);
		else if (Draw.pressed->command == CommandSet)            CursorSet(Handle.hand);
		else if (Draw.pressed->command == CommandLink)           CursorSet(Handle.hand);
		else if (Draw.pressed->command == CommandSizeHorizontal) CursorSet(Handle.horizontal);
		else if (Draw.pressed->command == CommandSizeVertical)   CursorSet(Handle.vertical);
		else if (Draw.pressed->command == CommandSizeDiagonal)   CursorSet(Handle.diagonal);
		else                                                     CursorSet(Handle.arrow);

	// SET THE POINTER BASED ON THE AREA IT IS OVER
	} else if (over && !pressing) {

		if      (over->command == CommandReady)          CursorSet(Handle.hand);
		else if (over->command == CommandSet)            CursorSet(Handle.hand);
		else if (over->command == CommandLink)           CursorSet(Handle.hand);
		else if (over->command == CommandSizeHorizontal) CursorSet(Handle.horizontal);
		else if (over->command == CommandSizeVertical)   CursorSet(Handle.vertical);
		else if (over->command == CommandSizeDiagonal)   CursorSet(Handle.diagonal);
		else                                             CursorSet(Handle.arrow);

	// SET THE POINTER TO ARROW
	} else if (inside) {

		CursorSet(Handle.arrow);
	}

	// COMPOSE THE DISPLAY OF EACH AREA AND DRAW THOSE THAT HAVE CHANGED
	areadisplay display;
	deviceitem device;
	areaitem *a;
	a = Draw.all;
	while (a) {

		// COMPOSE THE DISPLAY FOR THE AREA
		display = DisplayNone;
		if (a->command == CommandUnavailable) {

			display = DisplayGhosted;

		} else if (a->command == CommandReady) {

			if      (a == over && a == Draw.pressed) display = DisplayPressed;
			else if (a == over && !pressing)         display = DisplayHot;
			else                                     display = DisplayReady;

		} else if (a->command == CommandSet) {

			display = DisplayPressed;

		} else if (a->command == CommandLink) {

			if      (a == over && a == Draw.pressed) display = DisplayHot;
			else if (a == over && !pressing)         display = DisplayHot;
			else                                     display = DisplayReady;
		}

		// THE DISPLAY OF THE AREA ON THE SCREEN IS DIFFERENT FROM THE NEWLY COMPOSED DISPLAY
		if (a->display != display) {

			// UPDATE THE AREA'S DISPLAY
			a->display = display;

			// GET THE WINDOW DEVICE CONTEXT IF IT HASN'T ALREADY BEEN OBTAINED AND PAINT THE AREA
			if (device.open == DeviceNone) {

				device.OpenGet(Handle.window);
				device.Font(Handle.font);
				device.BackgroundColor(Draw.color.window.color);
			}

			if (device.device) PaintArea(&device, a);
		}

		a = a->next;
	}

	// IF THE PROGRAM IS NOT OWNED AND THE GET BUTTON IT HOT, SHOW THE SIGN
	if (!State.owned && Draw.get.display == DisplayHot) Sign();

	// ADJUST SIZE
	if (Draw.pressed) {

		// GET POSITIONS IN CLIENT COORDINATES
		sizeitem mouse, stick, min, move;
		mouse = MouseClient();                         // WHERE THE MOUSE IS
		stick.x = Draw.pressed->size.x + Draw.stick.x; // THE STICK IS THE POINT THE MOUSE IS DRAGGING
		stick.y = Draw.pressed->size.y + Draw.stick.y;
		min.x = Draw.sizemin.x + Draw.stick.x;         // THE CLOSEST THE STICK CAN BE TO THE CLIENT ORIGIN
		min.y = Draw.sizemin.y + Draw.stick.y;
		move.x = mouse.x - stick.x;                    // FROM THE STICK TO THE MOUSE
		move.y = mouse.y - stick.y;

		// IF THE MOUSE IS AWAY FROM THE STICK, TRY TO MOVE THE STICK TO IT
		if (Draw.pressed->command == CommandSizeHorizontal && move.x != 0) {

			// HORIZONTAL BAR
			Size(move.x);

		} else if (Draw.pressed->command == CommandSizeVertical && move.y != 0) {

			// VERTICAL BAR
			Size(move.y);

		} else if (Draw.pressed->command == CommandSizeDiagonal && (move.x != 0 || move.y != 0)) {

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
		if (Draw.status.Is()) {

			// GET THE WINDOW DEVICE CONTEXT IF IT HASN'T ALREADY BEEN OBTAINED
			if (device.open == DeviceNone) {

				device.OpenGet(Handle.window);
				device.Font(Draw.font.normal);
				device.BackgroundColor(Draw.color.window.color);
			}

			// PAINT THE STATUS TEXT TO THE WINDOW
			PaintText(&device, State.status, Draw.status, false, true, true, true);
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
	Draw.pressed = NULL;
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
	address = Draw.address.size;
	bar     = Draw.bar.size;

	// ALL SIZE CONSTANTS FOR THE PROGRAM ARE DEFINED HERE AS LOCAL VARIABLES READ IN THIS FUNCTION
	int text, row, space, icon, big, tool, title;
	text  = Draw.open.textsize.h;     // TEXT HEIGHT IS USUALLY 13
	row   = Draw.open.textsize.h + 3; // ROW HEIGHT IS 16, 1 PIXEL ABOVE AND 2 BELOW
	space = 4;                             // SPACING
	icon  = 16;                            // SMALL SQUARE ICONS
	big   = 24;                            // LARGE SQUARE ICONS
	tool  = 56;                            // MINIMUM BUTTON WIDTH
	title = 23;                            // HEIGHT OF TITLE BAR ON TOP OF CLIENT AREA

	// FIND THE WIDEST TEXT THAT HAS TO FIT IN A BUTTON
	int longest;
	longest = Greatest(
		Draw.pause.textsize.w,
		Draw.remove.textsize.w,
		Draw.back.textsize.w,
		Draw.forward.textsize.w,
		Draw.stop.textsize.w,
		Draw.refresh.textsize.w,
		Draw.expand.textsize.w,
		Draw.get.textsize.w);

	// BUTTON HOLDS THE BUTTON WIDTH AND HEIGHT
	sizeitem button;
	button.w = Greatest(longest + 2 + (2 * space), tool);
	button.h = big + text + 7;

	// SIZE AND POSITION THE OPEN AND HELP LINKS
	Draw.open.size.x = Draw.titlesize.w + (4 * space);
	Draw.open.size.w = Draw.open.textsize.w + (2 * space);
	Draw.help.size.x = Draw.open.size.Right();
	Draw.help.size.w = Draw.help.textsize.w + (2 * space);
	if (row < title) Draw.open.size.h = Draw.help.size.h = row;
	else             Draw.open.size.h = Draw.help.size.h = title; // IN CASE THE TEXT IS HIGHER THAN 23

	int wide;

	// SIZE AND POSITION THE PAUSE AND REMOVE BUTTONS
	Draw.pause.size = Draw.remove.size = button;
	Draw.pause.size.x = space;
	Draw.remove.size.x = Draw.pause.size.Right();
	Draw.pause.size.y = Draw.remove.size.y = title + space;

	// MAKE THE HEIGHT OF ADDRESS AND GET BE ROW OR ICON, WHICHEVER IS BIGGER
	int link;
	if (row > icon) link = row;
	else            link = icon;

	// SIZE ADDRESS AND GET
	Draw.address.size.w = Draw.address.textsize.w;
	Draw.enter.size.w = icon + space + Draw.enter.textsize.w;
	Draw.address.size.h = Draw.enter.size.h = link;

	// CALCULATE HOW WIDE TO DRAW THIS PART
	wide = 1 + (8 * space) + (2 * button.w) + text + Draw.address.size.w + Draw.enter.size.w;
	wide = Greatest(wide, client.w);

	// POSITION ADDRESS AND GET
	Draw.address.size.x = Draw.remove.size.Right() + 1 + (3 * space);
	Draw.enter.size.PositionRight(wide - (2 * space));
	Draw.address.size.y = Draw.enter.size.y = title + space;

	// COMPUTE THE MINIMUM AND MAXIMUM BAR Y DISTANCES
	int min, max;
	min = Draw.pause.size.Bottom() - text - space - 1;
	max = client.h - row - 2 - space - text - space - 1;

	// MOVE THE BAR, NOT LETTING IT GO BEYOND THE BOUNDS
	if (!Draw.bar.size.y) Draw.bar.size.y = Draw.pause.size.Bottom() - 1; // SET THE BAR
	Draw.bar.size.y += move;
	if (Draw.bar.size.y > max) Draw.bar.size.y = max;
	if (Draw.bar.size.y < min) Draw.bar.size.y = min; // ENFORCE MIN FROM THE TOP IF BOTH ARE IN VIOLATION

	// BAR
	Draw.bar.size.x = Draw.address.size.Right() + space;
	Draw.bar.size.w = wide - Draw.address.size.Right() - Draw.enter.size.w - (4 * space);
	Draw.bar.size.h = space + 1;

	// EDIT
	Draw.edit.x = Draw.bar.size.x + 1;
	Draw.edit.w = Draw.bar.size.w - 2;
	Draw.edit.y = title + space + 1;
	Draw.edit.SetBottom(Draw.bar.size.y);

	// BUTTON AND COPY
	Draw.button.x = Draw.bar.size.x;
	Draw.button.w = text;
	Draw.copy.size.x = Draw.button.Right();
	Draw.copy.size.w = space + Draw.copy.textsize.w;
	if (Draw.copy.size.Right() > Draw.bar.size.Right()) Draw.copy.size.SetRight(Draw.bar.size.Right());
	Draw.button.y = Draw.copy.size.y = Draw.bar.size.Bottom();
	Draw.button.h = Draw.copy.size.h = text;

	// LIST
	Draw.list.x = 2;
	Draw.list.w = client.w - 4;
	Draw.list.y = Draw.copy.size.Bottom() + space + 1;
	Draw.list.SetBottom(client.h - row - 1);
	Draw.list.Check(); // IF THE HEIGHT IS NEGATIVE, MAKE IT 0

	// DETERMINE WHERE THE SIZE CORNER WOULD BE IF THE WINDOW WERE VERY SMALL
	Draw.sizemin.w = Draw.sizemin.h = row;
	Draw.sizemin.y = 2 + title + (2 * space) + button.h;

	// STATUS AND SIZE
	Draw.status.y = Draw.corner.size.y = Draw.list.Bottom() + 1; // NEVER SIZE UP FROM THE BOTTOM OF THE WINDOW
	Draw.status.h = Draw.corner.size.h = row;
	Draw.status.w = client.w - row;
	Draw.corner.size.x = Draw.status.Right();
	Draw.corner.size.w = row;

	// IF THE WINDOW IS MAXIMIZED, MAKE STATUS THE ENTIRE ROW AND HIDE THE SIZE CORNER
	if (IsZoomed(Handle.window)) {

		Draw.status.w = client.w;
		Draw.corner.size.CloseRight();
	}

	// POSITION AND RESIZE CHILD WINDOW CONTROLS WITHOUT SENDING PAINT MESSAGES
	/*
	WindowMove(Handle.edit,   Draw.edit);
	WindowMove(Handle.button, Draw.button);
	WindowMove(Handle.tree,   Draw.tree);
	WindowMove(Handle.list,   Draw.list);
	*/

	// IF ADDRESS WAS GIVEN SIZE FOR THE FIRST TIME, ASSIGN THE TOOLTIP TO IT
	if (!address.Is()) TipAdd(Draw.address.size, Draw.address.tip);

	// IF THE BAR MOVED, PAINT THE WINDOW
	if (bar.y && bar.y != Draw.bar.size.y) Paint();
}
