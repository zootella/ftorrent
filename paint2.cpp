
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

void PaintLoad()
{
	// takes nothing
	// loads drawing resources from the system, freeing any already loaded, and sizes text
	// returns nothing

	// GET THE POINT SIZE OF THE NORMAL FONT
	int points;
	points = 11; // DEFAULT 11 WHICH LOOKS LIKE 8 ON SCREEN

	// CREATE A FONT HANDLE BASED ON THE ONE THE SYSTEM USES IN MESSAGE BOXES
	DeleteObjectSafely(Draw.font.normal);
	DeleteObjectSafely(Draw.font.underline);
	NONCLIENTMETRICS info;
	ZeroMemory(&info, sizeof(info));
	DWORD size = sizeof(info) - sizeof(info.iPaddedBorderWidth); // IGNORE LAST INT FOR THIS TO WORK
	info.cbSize = size; // SIZE OF THIS STRUCTURE
	int result = SystemParametersInfo(
		SPI_GETNONCLIENTMETRICS,  // SYSTEM PARAMETER TO RETRIEVE
		size,                     // SIZE OF THE STRUCTURE
		&info,                    // STRUCTURE TO FILL WITH INFORMATION
		0);                       // NOT SETTING A SYSTEM PARAMETER
	if (result) {

		// GET THE POINT SIZE OF THE NORMAL FONT, NORMALLY 11 WHICH LOOKS LIKE 8 ON SCREEN
		points = -info.lfMenuFont.lfHeight;

		// CREATE THE NORMAL FONT
		Draw.font.normal = CreateFontIndirect(&info.lfMenuFont);
		if (!Draw.font.normal) Report("drawload: error createfontindirect");

		// CREATE THE UNDERLINED FONT
		info.lfMenuFont.lfUnderline = true;
		Draw.font.underline = CreateFontIndirect(&info.lfMenuFont);
		if (!Draw.font.underline) Report("drawload: error createfontindirect");

	} else {

		// REPORT ERROR AND SET THE FONT HANDLE TO NULL
		Report("drawload: error systemparametersinfo");
		Draw.font.normal = Draw.font.underline = NULL;
	}

	// CREATE FONTS
	DeleteObjectSafely(Draw.font.arial28);            Draw.font.arial28            = CreateFont("Arial",   28);
	DeleteObjectSafely(Draw.font.verdana13);          Draw.font.verdana13          = CreateFont("Verdana", 13);
	DeleteObjectSafely(Draw.font.verdana13underline); Draw.font.verdana13underline = CreateFont("Verdana", 13, true);

	// SET THE FONT OF THE EDIT BOX, RETURNS ERROR ON WINDOWS 9X
	SendMessage( 
		(HWND)Handle.edit,        // SEND THE MESSAGE TO THIS WINDOW
		WM_SETFONT,               // MESSAGE TO SEND
		(WPARAM)Draw.font.normal, // HANDLE TO FONT
		0);                       // TELL THE CONTROL TO NOT NOT IMMEDIATELY REDRAW ITSELF

	// SIZE TEXT
	deviceitem device;
	device.OpenCreate();
	device.Font(Draw.font.arial28);
	Draw.titlesize = SizeText(&device, Draw.title);
	device.Font(Draw.font.verdana13);
	Draw.sign.line1size = SizeText(&device, Draw.sign.line1);
	Draw.sign.line2size = SizeText(&device, Draw.sign.line2);
	Draw.sign.line3size = SizeText(&device, Draw.sign.line3);
	Draw.sign.line4size = SizeText(&device, Draw.sign.line4);

	// SIZE TEXT IN THE AREA ITEMS
	device.Font(Draw.font.normal);
	areaitem *a;
	a = Draw.area.all;
	while (a) {

		a->textsize = SizeText(&device, a->text);
		a = a->next;
	}

	// GET THE SHARED HANDLES TO SYSTEM BRUSHES
	Draw.color.face     = BrushSystem(COLOR_3DFACE);
	Draw.color.shadow   = BrushSystem(COLOR_3DSHADOW);
	Draw.color.window   = BrushSystem(COLOR_WINDOW);
	Draw.color.text     = BrushSystem(COLOR_WINDOWTEXT);
	Draw.color.selected = BrushSystem(COLOR_HIGHLIGHT);

	// MAKE THE COLOR BRUSHES
	DeleteObjectSafely(Draw.color.white.brush);
	DeleteObjectSafely(Draw.color.blue.brush);
	DeleteObjectSafely(Draw.color.darkblue.brush);
	DeleteObjectSafely(Draw.color.lightblue.brush);
	Draw.color.white     = BrushColor(RGB(255, 255, 255));
	Draw.color.blue      = BrushColor(RGB(  0,   0, 255));
	Draw.color.darkblue  = BrushColor(RGB(  0, 102, 204));
	Draw.color.lightblue = BrushColor(RGB( 51, 153, 255));

	// MIX BRUSHES
	DeleteObjectSafely(Draw.color.middle.brush);
	Draw.color.middle = BrushColor(MixColors(GetSysColor(COLOR_3DFACE), 1, GetSysColor(COLOR_3DSHADOW), 1));
}

void Paint()
{
	// takes nothing
	// calls invalidate rect and update window to draw the main window
	// returns nothing

	// MARK THE CLIENT AREA OF THE MAIN WINDOW AS NECESSARY TO DRAW
	int result;
	result = InvalidateRect(
		Handle.window, // HANDLE TO WINDOW
		NULL,          // INVALIDATE THE ENTIRE CLIENT AREA OF THE WINDOW
		false);        // DRAW THE WINDOW WITHOUT WIPING IT WITH THE BACKGROUND COLOR FIRST
	if (!result) Report("drawupdate: error invalidaterect");

	// SEND THE WINDOW A PAINT MESSAGE AND HAVE IT PROCESS IT RIGHT NOW
	if (!UpdateWindow(Handle.window)) Report("drawupdate: error updatewindow");
}

void PaintWindow(deviceitem *device)
{
	// takes a device context to use to paint in the window
	// paints all areas of the client window outside the child window controls
	// returns nothing

	// PAINT ALL THE AREAS
	areaitem *a;
	a = Draw.area.all;
	while (a) {

		PaintArea(device, a);
		a = a->next;
	}

	// SIZE CONSTANTS
	int space, title;
	space = 4;
	title = 23;

	// GET THE SIZE OF THE CLIENT AREA
	sizeitem client, s;
	client = SizeClient();

	// PAINT THE PARTS OF THE CLIENT AREA NOT COVERED BY AREAS OR CHILD WINDOW CONTROLS
	s.x = 2 * space;
	s.y = -7;
	s.h = 30;
	PaintText(device, Draw.title, s, false, false, false, false, 0, Draw.font.arial28, &Draw.color.lightblue, &Draw.color.darkblue);
	s.Clear();
	s.w = 2 * space;
	s.h = title;
	PaintFill(device, s, Draw.color.darkblue.brush); // LEFT OF TITLE TEXT
	s.x = (2 * space) + Draw.titlesize.w;
	PaintFill(device, s, Draw.color.darkblue.brush); // BETWEEN TITLE TEXT AND OPEN LINK
	s.x = Draw.area.help.size.Right();
	s.SetRight(client.w);
	PaintFill(device, s, Draw.color.darkblue.brush); // RIGHT OF HELP LINK
	s.x = Draw.area.open.size.x;
	s.SetRight(Draw.area.help.size.Right());
	s.y = Draw.area.open.size.Bottom();
	s.SetBottom(title);
	PaintFill(device, s, Draw.color.darkblue.brush); // BENEATH LINKS

	if (State.mode == ModeDownload) {

		s = client;
		s.y = title;
		s.SetBottom(Draw.area.pause.size.y);
		PaintFill(device, s); // FULL ROW ABOVE TOOLS
		s.y = Draw.area.pause.size.y;
		s.w = Draw.area.pause.size.x;
		s.h = Draw.area.pause.size.h;
		PaintFill(device, s); // SPACE LEFT OF PAUSE
		s.x = Draw.area.remove.size.Right();
		PaintFill(device, s); // SPACE RIGHT OF REMOVE
		s.CloseRight();
		s.w = 1;
		PaintFill(device, s, Draw.color.middle.brush); // LINE BETWEEN BUTTONS AND ADDRESS
		s.x++;
		s.SetRight(Draw.area.address.size.x);
		s.SetBottom(Draw.area.list.y - 1);
		PaintFill(device, s); // SPACE LEFT OF ADDRESS
		s.x = Draw.area.address.size.Right();
		s.SetRight(Draw.area.bar.size.x);
		PaintFill(device, s); // SPACE BETWEEN ADDRESS AND EDIT
		s.x = Draw.area.bar.size.Right();
		s.SetRight(Draw.area.enter.size.x);
		PaintFill(device, s); // SPACE BETWEEN EDIT AND GET
		s.x = Draw.area.enter.size.Right();
		s.SetRight(client.w);
		PaintFill(device, s); // SPACE RIGHT OF GET
		s = Draw.area.edit;
		s.w = 1;
		s.x--;
		s.ShiftTop(-1);
		PaintFill(device, s, Draw.color.middle.brush); // LEFT EDIT BORDER
		s.x = Draw.area.edit.Right();
		PaintFill(device, s, Draw.color.middle.brush); // RIGHT EDIT BORDER
		s = Draw.area.edit;
		s.y--;
		s.h = 1;
		PaintFill(device, s, Draw.color.middle.brush); // TOP EDIT BORDER
		s = Draw.area.pause.size;
		s.SetRight(Draw.area.remove.size.Right() + s.x + 1);
		s.SetLeft(0);
		s.y = Draw.area.pause.size.Bottom();
		s.SetBottom(Draw.area.list.y - 1);
		PaintFill(device, s); // BENEATH PAUSE AND REMOVE
		s = Draw.area.address.size;
		s.CloseBottom();
		s.SetBottom(Draw.area.list.y - 1);
		PaintFill(device, s); // BENEATH ADDRESS
		s = Draw.area.bar.size;
		s.y = Draw.area.button.Bottom();
		s.SetBottom(Draw.area.list.y - 1);
		PaintFill(device, s); // BENEATH BUTTON AND COPY
		s = Draw.area.copy.size;
		s.CloseRight();
		s.SetRight(Draw.area.bar.size.Right());
		PaintFill(device, s); // TO THE RIGHT OF COPY
		s = Draw.area.enter.size;
		s.CloseBottom();
		s.SetBottom(Draw.area.list.y - 1);
		PaintFill(device, s); // BENEATH GET
		s = Draw.area.list;
		s.Expand();
		PaintBorder(device, s, Draw.color.middle.brush); // BORDER AROUND LIST
		s.x = 0;
		s.w = 1;
		PaintFill(device, s); // LINE LEFT OF LIST
		s.x = client.w - 1;
		PaintFill(device, s); // LINE RIGHT OF LIST

	} else if (State.mode == ModeSite) {

		// FULL ROW ABOVE TOOLS
		s = client;
		s.y = title;
		s.SetBottom(Draw.area.back.size.y);
		PaintFill(device, s);

		// SPACES BETWEEN BUTTONS
		s = Draw.area.back.size;
		s.x = 0;
		s.SetRight(Draw.area.back.size.x);
		PaintFill(device, s);
		s.x = Draw.area.refresh.size.Right();
		PaintFill(device, s);
		s.CloseRight();
		s.w = 1;
		PaintFill(device, s, Draw.color.middle.brush);
		s.w = Draw.area.back.size.x;
		s.PositionRight(Draw.area.expand.size.x);
		PaintFill(device, s);
		s.x = Draw.area.expand.size.Right();
		PaintFill(device, s);
		s.CloseRight();
		s.w = 1;
		PaintFill(device, s, Draw.color.middle.brush);
		s.w = Draw.area.back.size.x;
		s.PositionRight(Draw.area.get.size.x);
		PaintFill(device, s);
		s.x = Draw.area.get.size.Right();
		s.SetRight(client.w);
		PaintFill(device, s);

		// BENEATH BUTTONS
		s = client;
		s.y = Draw.area.back.size.Bottom();
		s.h = space;
		PaintFill(device, s);
		s.y += space + 1;
		PaintFill(device, s);
		s.h = 0;
		s.ShiftTop(-1);
		s.w = space;
		PaintFill(device, s);
		s.CloseRight();
		s.SetRight(client.w - space);
		PaintFill(device, s, Draw.color.middle.brush);
		s.CloseRight();
		s.SetRight(client.w);
		PaintFill(device, s);

		// SIZE AROUND THE ADDRESS ROW
		sizeitem address;
		address = client;
		address.y = Draw.area.back.size.Bottom() + 1 + (2 * space);
		address.SetBottom(Draw.area.bar.size.y - space);

		// PAINT AROUND THE ADDRESS ROW
		s = address;
		s.SetRight(Draw.area.address.size.x);
		PaintFill(device, s);
		s = Draw.area.address.size;
		s.y = address.y;
		s.SetBottom(Draw.area.address.size.y);
		PaintFill(device, s);
		s.y = Draw.area.address.size.Bottom();
		s.SetBottom(address.Bottom());
		PaintFill(device, s);
		s = address;
		s.x = Draw.area.address.size.Right();
		s.w = space;
		PaintFill(device, s);
		s.CloseRight();
		s.SetRight(Draw.area.enter.size.x - space);
		PaintBorder(device, s, Draw.color.middle.brush);
		s.Expand(-1);
		s.SetRight(Draw.area.edit.x);
		PaintFill(device, s);
		s = Draw.area.edit;
		s.y = address.y + 1;
		s.SetBottom(Draw.area.edit.y);
		PaintFill(device, s);
		s.y = Draw.area.edit.Bottom();
		s.SetBottom(address.Bottom() - 1);
		PaintFill(device, s);
		s = address;
		s.x = Draw.area.edit.Right() + 1;
		s.SetRight(Draw.area.enter.size.x);
		PaintFill(device, s);
		s = Draw.area.enter.size;
		s.y = address.y;
		s.SetBottom(Draw.area.enter.size.y);
		PaintFill(device, s);
		s.y = Draw.area.enter.size.Bottom();
		PaintFill(device, s);
		s = address;
		s.SetLeft(Draw.area.enter.size.Right());
		PaintFill(device, s);

		// PAINT AROUND THE TREE AND LIST
		s = address;
		s.CloseBottom();
		s.SetBottom(Draw.area.bar.size.y);
		PaintFill(device, s);
		s = Draw.area.bar.size;
		s.x = 0;
		s.w = 1;
		PaintFill(device, s);
		s.x++;
		PaintFill(device, s, Draw.color.middle.brush);
		s = Draw.area.tree;
		s.y--;
		s.h = 1;
		PaintFill(device, s, Draw.color.middle.brush);
		s.y = Draw.area.tree.Bottom();
		PaintFill(device, s, Draw.color.middle.brush);
		s = Draw.area.list;
		s.y--;
		s.h = 1;
		PaintFill(device, s, Draw.color.middle.brush);
		s.y = Draw.area.list.Bottom();
		PaintFill(device, s, Draw.color.middle.brush);
		s = Draw.area.bar.size;
		s.x = Draw.area.list.Right();
		PaintFill(device, s, Draw.color.middle.brush);
		s.x++;
		PaintFill(device, s);
	}

	// STATUS BAR
	PaintText(device, State.status, Draw.area.status, false, true, true, true);
}

void PaintArea(deviceitem *device, areaitem *a)
{
	// takes a device context and an area item which is a button
	// paints the area item in the window
	// returns nothing

	// ONLY PAINT AREAS THAT ARE IN USE AND HAVE SIZE
	if (a->command == CommandNone || !a->size.Is()) return;

	// DEFINE SIZES AS LOCAL VARIABLES
	sizeitem icon, smallicon, text, s;
	int space;
	space = 4;
	icon.w = icon.h = 24;
	smallicon.w = smallicon.h = 16;

	// PAINT BUTTON
	if (a->command == CommandUnavailable || a->command == CommandReady || a->command == CommandSet) {

		// SIZE ICON
		icon.x = a->size.x + (a->size.w - icon.w) / 2;
		icon.y = a->size.y + 3;

		// SIZE TEXT
		text.w = a->size.w - (2 * space) - 2;
		text.h = Draw.area.open.textsize.h;
		text.x = a->size.x + (a->size.w - text.w) / 2;
		text.y = icon.Bottom() + 1;

		// IF PRESSED, SHIFT THE ICON AND TEXT DOWN AND TO THE RIGHT ONE PIXEL
		if (a->display == DisplayPressed) { icon.x++; icon.y++; text.x++; text.y++; }

		// PAINT ICON AND TEXT
		if (a->display == DisplayGhosted) {

			PaintIcon(device, icon, a->gray);
			device->FontColor(Draw.color.shadow.color);
			PaintText(device, a->text, text, true, true, false, false, a->adjust);
			device->FontColor(device->fontcolor);

		} else {

			PaintIcon(device, icon, a->icon);
			PaintText(device, a->text, text, true, true, false, false, a->adjust);
		}

		// PAINT BORDER AND SIZE BUTTON TO BE INSIDE IT
		sizeitem button;
		button = a->size;
		if      (a->display == DisplayHot)     { PaintBorder(device, a->size, Draw.color.face.brush, Draw.color.shadow.brush); button.Expand(-1); }
		else if (a->display == DisplayPressed) { PaintBorder(device, a->size, Draw.color.shadow.brush, Draw.color.face.brush); button.Expand(-1); }

		// FILL BACKGROUND IN BETWEEN
		s = button;
		s.SetBottom(icon.y);
		PaintFill(device, s); // ROW ABOVE ICON
		s = icon;
		s.w = 0;
		s.SetLeft(button.x);
		PaintFill(device, s); // LEFT OF ICON
		s = icon;
		s.CloseRight();
		s.SetRight(button.Right());
		PaintFill(device, s); // RIGHT OF ICON
		s = button;
		s.y = icon.Bottom();
		s.SetBottom(text.y);
		PaintFill(device, s); // ROW BETWEEN ICON AND TEXT
		s = text;
		s.w = 0;
		s.SetLeft(button.x);
		PaintFill(device, s); // LEFT OF TEXT
		s = text;
		s.CloseRight();
		s.SetRight(button.Right());
		PaintFill(device, s); // RIGHT OF TEXT
		s = button;
		s.y = text.Bottom();
		s.SetBottom(button.Bottom());
		PaintFill(device, s); // ROW BENEATH TEXT

	// PAINT LINK
	} else if (a->command == CommandLink) {

		// UNDERLINE LINKS
		HFONT underline;
		underline = NULL;
		if (a->display == DisplayHot) underline = Draw.font.underline;

		// PAINT OPEN AND HELP
		if (a == &Draw.area.open || a == &Draw.area.help) PaintText(device, a->text, a->size, false, true, true, true, 0, underline, &Draw.color.white, &Draw.color.darkblue);
		if (a == &Draw.area.open) {

			// PAINT OPEN DOWN ARROW
			s = Draw.area.open.size;
			s.CloseRight();
			s.y += (s.h / 2) + 1;
			s.x -= 11;
			s.w = 5;
			s.h = 1;
			PaintFill(device, s, Draw.color.white.brush);
			s.x++;
			s.y++;
			s.w -= 2;
			PaintFill(device, s, Draw.color.white.brush);
			s.x++;
			s.y++;
			s.w -= 2;
			PaintFill(device, s, Draw.color.white.brush);
		}

		// PAINT ADDRESS AND COPY
		if      (a == &Draw.area.address) PaintText(device, a->text, a->size, false, true, false, false, 0, underline);
		else if (a == &Draw.area.copy)    PaintText(device, a->text, a->size, false, true, true,  false, 0, underline);

		// PAINT ENTER ICON AND LINK
		if (a == &Draw.area.enter) {

			// SIZE THE ICON, ANY BACKGROUND SPACE BENEATH IT, AND THE TEXT
			sizeitem entericon, enterbeneath, entertext;
			entericon = enterbeneath = entertext = a->size;
			entericon.w = entericon.h = smallicon.w;
			enterbeneath.SetTop(entericon.Bottom());
			enterbeneath.w = entericon.w;
			entertext.SetLeft(entericon.Right());

			// PAINT THE ICON, BACKGROUND SPACE, AND TEXT
			PaintIcon(device, entericon, a->icon);
			PaintFill(device, enterbeneath);
			PaintText(device, a->text, entertext, false, true, true, false, 0, underline);
		}

	// PAINT BAR
	} else {

		// PAINT BAR
		if (a == &Draw.area.bar) {

			s = Draw.area.bar.size;
			if (a->command == CommandSizeVertical) {

				s.h = 1;
				PaintFill(device, s, Draw.color.middle.brush);
				s = Draw.area.bar.size;
				s.ShiftTop(1);
				PaintFill(device, s);

			} else if (a->command == CommandSizeHorizontal) {

				s.w = 1;
				PaintFill(device, s, Draw.color.middle.brush);
				s.CloseRight();
				s.SetRight(Draw.area.bar.size.Right() - 1);
				PaintFill(device, s);
				s.CloseRight();
				s.w = 1;
				PaintFill(device, s, Draw.color.middle.brush);
			}

		// PAINT THE SIZE CORNER
		} else if (a == &Draw.area.corner) {

			s = Draw.area.corner.size;
			PaintFill(device, s);
			s.CloseRight();
			s.CloseBottom();
			s.w = s.h = 2;
			s.x -= 4;
			s.y -= 12;
			PaintFill(device, s, Draw.color.shadow.brush);
			s.x -= 4;
			s.y += 4;
			PaintFill(device, s, Draw.color.shadow.brush);
			s.x += 4;
			PaintFill(device, s, Draw.color.shadow.brush);
			s.x -= 8;
			s.y += 4;
			PaintFill(device, s, Draw.color.shadow.brush);
			s.x += 4;
			PaintFill(device, s, Draw.color.shadow.brush);
			s.x += 4;
			PaintFill(device, s, Draw.color.shadow.brush);
		}
	}
}

bool PaintCustom(LPNMLVCUSTOMDRAW draw)
{
	// takes a pointer to a list view custom draw structure
	// does custom painting in the list view control
	// returns true to have the control skip its default painting, false to pass on the custom draw notification to windows

	// EXTRACT INFORMATION FROM THE DRAW STRUCTURE
	LPARAM parameter;
	int row, column;
	deviceitem device;
	parameter = draw->nmcd.lItemlParam;
	row = (int)draw->nmcd.dwItemSpec;
	column = draw->iSubItem;
	device.OpenUse(draw->nmcd.hdc);

	// ONLY CUSTOM DRAW THE SIZE COLUMN
	if (column != 2) return(false);

	// ONLY CUSTOM DRAW A BOT ITEM WITH A WANT ATTACHED
	botitem *b;
	b = (botitem *)parameter;
	if (!b->w) return(false);

	// ONLY CUSTOM DRAW A WANT WITH SIZE AND STRIPES TEXT
	string s, stripes;
	s = b->w->Size();
	stripes = b->w->stripes;
	if (isblank(s) || isblank(stripes)) return(false);

	// CHOOSE AND MIX COLORS
	COLORREF foreground, background, text;
	HBRUSH brush;
	if (!ListSelected(row)) {

		// NOT SELECTED, ORANGE
		foreground = RGB(255, 204, 0);
		background = MixColors(GetSysColor(COLOR_3DFACE), 1, GetSysColor(COLOR_WINDOW), 2);
		text = GetSysColor(COLOR_WINDOWTEXT);
		brush = Draw.color.window.brush;

	} else if (GetFocus() == Handle.list) {

		// SELECTED WITH THE FOCUS, BLUE
		foreground = MixColors(GetSysColor(COLOR_HIGHLIGHTTEXT), 1, GetSysColor(COLOR_HIGHLIGHT), 2);
		background = GetSysColor(COLOR_HIGHLIGHT);
		text = GetSysColor(COLOR_HIGHLIGHTTEXT);
		brush = Draw.color.selected.brush;

	} else {

		// SELECTED WITHOUT THE FOCUS, GRAY
		foreground = MixColors(GetSysColor(COLOR_WINDOWTEXT), 1, GetSysColor(COLOR_3DFACE), 4);
		background = GetSysColor(COLOR_3DFACE);
		text = GetSysColor(COLOR_WINDOWTEXT);
		brush = Draw.color.face.brush;
	}

	// GET THE RECTANGLE OF THE SUBITEM
	sizeitem cell, line, progress;
	line = progress = cell = ListCell(row, column);
	line.h = 1;   // LINE IS THE TOP LINE OF THE RECTANGLE
	progress.y++; // PROGRESS IS THE RECTANGLE BENEATH THE TOP LINE
	progress.h--; // ADJUST THE RECTANGLE FOR THE TEXT WITH THE 6 PIXEL MARGIN
	cell.x += 6;
	cell.w -= 12;

	// PAINT THE BACKGROUND
	PaintFill(&device, line, brush);
	PaintProgress(&device, progress, foreground, background, stripes);

	// SIZE THE WIDTH OF THE ITEM TEXT
	sizeitem size;
	size = SizeText(&device, s);

	// THERE IS SPACE BETWEEN THE MARGINS
	if (cell.w > 0) {

		// IF THERE IS MORE THAN ENOUGH ROOM FOR THE TEXT, RIGHT ALIGN IT
		UINT style;
		if (cell.w > size.w) style = DT_RIGHT;
		else                 style = DT_LEFT;

		// PAINT THE TEXT WITH A TRANSPARENT BACKGROUND AND AN ELLIPSIS IF NECESSARY
		device.Background(TRANSPARENT);
		device.FontColor(text);
		RECT rectangle;
		rectangle = cell.Rectangle();
		if (!DrawText(device.device, s, -1, &rectangle, style | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE)) Report("error drawtext");
	}

	// TELL THE CONTROL TO SKIP DRAWING THIS SUBITEM
	return(true);
}

void PaintProgress(deviceitem *device, sizeitem bound, COLORREF foreground, COLORREF background, read r)
{
	// takes a device and size to paint in, background and color, and progress text
	// paints the antialiased progress bar
	// returns nothing

	// READ THE PROGRESS INFORMATION FOR THE TOTAL SIZE AND AT MOST 16 PAIRS OF POSITIONS AND SIZES
	hyper size, pair, pairs, positions[16], sizes[16];
	string s, n;
	split(r, ",", &n, &s);
	size = number(n);
	pairs = 0;
	while (is(s) && pairs < 16) {

		split(s, ",", &n, &s);
		positions[pairs] = number(n);
		split(s, ",", &n, &s);
		sizes[pairs] = number(n);
		pairs++;
	}

	// START WITH NO BRUSH
	brushitem brush;
	COLORREF color, maskcolor;

	// START MASK TO THE LEFT OF THE FIRST VERTICAL COLUMN OF PIXELS IN THE RECTANGLE
	sizeitem mask;
	mask = bound;
	mask.w = 0;

	// LOOP FOR EACH VERTICAL COLUMN OF PIXELS IN THE PROGRESS BAR
	hyper column, columns, saturation, a1, a2, b1, b2;
	columns = bound.w;
	for (column = 0; column < columns; column++) {

		// CALCULATE THE COLUMN BOUNDARY ON THE COMMON MULTIPLIED SIZE SCALE
		a1 = column * size;
		a2 = (column + 1) * size;

		// LOOP FOR EACH PAIR TO FIND THE TOTAL SATURATION
		saturation = 0;
		for (pair = 0; pair < pairs; pair++) {

			// CALCULATE THE PROGRESS BOUNDARY ON THE SAME SCALE
			b1 = positions[pair] * columns;
			b2 = (positions[pair] + sizes[pair]) * columns;

			// CLIP B TO THE BOUNDARIES OF A
			if (b1 < a1 && b2 > a1) b1 = a1;
			if (b2 > a2 && b1 < a2) b2 = a2;

			// CALCULATE THE SATURATION ON A SCALE FROM 0 TO 255
			if (size && b1 >= a1 && b2 <= a2) saturation += (((b2 - b1) * 255) / size);
		}

		// LIMIT THE TOTAL SATURATION FOR THE COLUMN AND MIX ITS COLOR
		if (saturation > 255) saturation = 255;
		color = MixColors(foreground, (int)saturation, background, 255 - (int)saturation);

		// THIS IS THE FIRST COLUMN
		if (column == 0) {

			// SET THE MASK COLOR TO THE COLOR THIS COLUMN WILL BE PAINTED, AND CONTINUE WITH SAME CASE
			maskcolor = color;
		}

		// THIS COLUMN SHOULD BE PAINTED THE SAME AS COLUMN BEFORE IT, OR THERE IS NO COLUMN BEFORE IT TO BE DIFFERENT
		if (color == maskcolor) {

			// EXTEND THE MASK TO COVER THIS COLUMN
			mask.w++;

		// THIS COLUMN SHOULD BE PAINTED DIFFERENTLY FROM THE COLUMN BEFORE IT
		} else {

			// PAINT THE MASK
			DeleteObjectSafely(brush.brush);
			brush = BrushColor(maskcolor);
			PaintFill(device, mask, brush.brush);

			// SET THE MASK COLOR FOR THIS COLUMN AND SET THE MASK ON THIS COLUMN
			maskcolor = color;
			mask.x += mask.w;
			mask.w = 1;
		}

		// THIS IS THE LAST COLUMN
		if (column == columns - 1) {

			// PAINT THE MASK
			DeleteObjectSafely(brush.brush);
			brush = BrushColor(maskcolor);
			PaintFill(device, mask, brush.brush);
		}
	}

	// DELETE THE BRUSH
	DeleteObjectSafely(brush.brush);
}
