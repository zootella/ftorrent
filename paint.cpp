
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

// Make painting tools once when the program starts
void PaintCreate() {
}

// Paint the client area of the window and resize the child window controls
void Paint() {

	/*
	// Sizes used in the layout
	int margin = 16;
	int labelwidth = 70;
	int buttonwidth = 80;
	int taskheight = 122;
	int buttonheight = 25;
	int statusheight = 54;

	// Rectangular sizes in the client area
	sizeitem client;
	sizeitem label1, label2, label3;
	sizeitem border1, border2, border3;
	sizeitem tasks, status, errors;
	sizeitem clear, task, start, stop, reset;

	// Find the width and height of the client area
	RECT rectangle;
	GetClientRect(Handle.window, &rectangle);
	client.set(rectangle); // 0 when the window is minimized

	// Tasks label
	label1.set(margin, margin, labelwidth, taskheight);

	// Tasks box
	border1 = label1;
	border1.x(label1.r() + margin);
	border1.r(client.w() - margin); // Measure from right, and keep it wide enough
	if (border1.w() < buttonwidth) border1.w(buttonwidth);

	// Clear button
	clear = border1;
	clear.y(border1.b() + margin);
	clear.w(buttonwidth);
	clear.h(buttonheight);

	// Buttons in that row
	task = clear;
	task.addx(buttonwidth + margin);
	start = task;
	start.addx(buttonwidth + margin);
	stop = start;
	stop.addx(buttonwidth + margin);
	reset = stop;
	reset.addx(buttonwidth + margin);

	// Labels on the left
	label2 = label1;
	label2.y(clear.b() + margin);
	label2.h(statusheight);
	label3 = label2;
	label3.y(label2.b() + margin);
	label3.b(client.b() - margin); // Measure from bottom, and keep it tall enough
	if (label3.h() < buttonheight) label3.h(buttonheight);

	// Controls on the right
	border2 = label2;
	border2.x(border1.x());
	border2.w(border1.w());
	border3 = label3;
	border3.x(border1.x());
	border3.w(border1.w());

	// Controls in borders;
	tasks  = border1; tasks.inside();
	status = border2; status.inside();
	errors = border3; errors.inside();

	// Move labels inside to line up with controls
	label1.Inside();
	label2.Inside();
	label3.Inside();

	// Pick colors for the background banner message
	brushitem *field, *ink, *label;
	field = &Handle.green;
	ink   = &Handle.lightgreen;
	label = &Handle.black;

	// Position and size child window controls
	WindowSize(Handle.tasks, tasks);
	WindowSize(Handle.status, status);
	WindowSize(Handle.errors, errors);
	WindowSize(Handle.clear, clear);
	WindowSize(Handle.task, task);
	WindowSize(Handle.start, start);
	WindowSize(Handle.stop, stop);
	WindowSize(Handle.reset, reset);

	// Paint the window
	deviceitem device;
	device.OpenPaint(Handle.window);

	// Paint the background color
	PaintFill(&device, client, field->brush);

	// Paint the text lables on the left
	device.Font(Handle.font);
	device.FontColor(label->color);
	device.Background(TRANSPARENT);
	PaintText(&device, L"Tasks",  label1);
	PaintText(&device, L"Status", label2);
	PaintText(&device, L"Errors", label3);

	// Paint the borders
	PaintBorder(&device, border1, Handle.middle.brush);
	PaintBorder(&device, border2, Handle.middle.brush);
	PaintBorder(&device, border3, Handle.middle.brush);
	*/
}












// Loads drawing resources from the system, freeing any already loaded, and sizes text
void PaintLoad() {

/*
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
	*/
}

void PaintWindow(deviceitem *device)
{
	// takes a device context to use to paint in the window
	// paints all areas of the client window outside the child window controls
	// returns nothing

	/*

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

	}

	// STATUS BAR
	PaintText(device, State.status, Draw.area.status, false, true, true, true);
	*/
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

		/*
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

		*/

	// PAINT LINK
	} else if (a->command == CommandLink) {

		/*

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

		*/

	// PAINT BAR
	} else {

		/*

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

		*/
	}
}



