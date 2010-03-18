
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

// Takes a device context to use to paint in the window
// Paints the client area of the window outside the child window controls, and resizes the child window controls
void PaintWindow(deviceitem *device) {

	// Get the size of the client area
	sizeitem client = SizeClient();

	// Title
	sizeitem s = client;
	s.h = 23;
	PaintFill(device, s, Handle.green.brush);

	// Toolbar

	// Bar

	// Status

	// Corner


	State.title = L"7 ltorrent start 8";
	device->Font(Handle.arial);
	device->FontColor(Handle.lightblue.color);
	device->BackgroundColor(Handle.blue.color);

	sizeitem t = SizeText(device, State.title);

	s.x = client.w - t.w - 8;
	s.y = -7;
	s.h = 30;
	s.w = t.w;



	PaintSign(device, State.title, s);



	// PAINT THE PARTS OF THE CLIENT AREA NOT COVERED BY AREAS OR CHILD WINDOW CONTROLS
	s.Clear();
	/*

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


	}

	// STATUS BAR
	PaintText(device, State.status, Draw.area.status, false, true, true, true);
	*/
}

// Takes a device context and an area item
// Paints the area item in the window
void PaintArea(deviceitem *device, areaitem *a) {
	/*

	// Only paint areas that are in use and have size
	if (a->command == CommandNone || !a->size.Is()) return;

	// Paint button
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
	*/
}



