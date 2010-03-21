
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

	string title = L"ltorrent some more";

	// Title

	device->Font(Handle.arial); // Find out how big the sign text will be
	device->FontColor(Handle.lightblue.color);
	device->BackgroundColor(Handle.blue.color);
	sizeitem t = SizeText(device, title);

	sizeitem s = client; // Fill to the left
	s.w = client.w - t.w - 8;
	s.h = Area.title.h;
	PaintFill(device, s, Handle.green.brush);

	s.x = client.w - 8; // Fill to the right
	s.w = 8;
	PaintFill(device, s, Handle.yellow.brush);

	s.x = client.w - t.w - 8; // Paint the text
	s.y = -7;
	s.w = t.w;
	s.h = 30;
	PaintLabel(device, title, s);
	device->Font(Handle.font);




	// Toolbar


	// Paint all the areas
	areaitem *a = Area.all;
	while (a) {
		PaintArea(device, a);
		a = a->next;
	}




/*


	// Title
	sizeitem s = client; // Field
	s.h = Area.title.h;
	PaintFill(device, s, Handle.green.brush);
	string title = L"ltorrent"; // State
	device->Font(Handle.arial);
	device->FontColor(Handle.lightblue.color);
	device->BackgroundColor(Handle.blue.color);
	sizeitem t = SizeText(device, title);
	s.x = client.w - t.w - 8;
	s.y = -7;
	s.h = 30;
	s.w = t.w;
	PaintLabel(device, title, s);

	// Toolbar

	// Bar

	// Status

	// Corner





	// PAINT THE PARTS OF THE CLIENT AREA NOT COVERED BY AREAS OR CHILD WINDOW CONTROLS
	s.Clear();

	/*
	s.w = 2 * space;
	s.h = title;
	PaintFill(device, s, Draw.color.darkblue.brush); // LEFT OF TITLE TEXT
	s.x = (2 * space) + Draw.titlesize.w;
	PaintFill(device, s, Draw.color.darkblue.brush); // BETWEEN TITLE TEXT AND OPEN LINK
	s.x = Area.help.size.Right();
	s.SetRight(client.w);
	PaintFill(device, s, Draw.color.darkblue.brush); // RIGHT OF HELP LINK
	s.x = Area.open.size.x;
	s.SetRight(Area.help.size.Right());
	s.y = Area.open.size.Bottom();
	s.SetBottom(title);
	PaintFill(device, s, Draw.color.darkblue.brush); // BENEATH LINKS

	if (State.mode == ModeDownload) {

		s = client;
		s.y = title;
		s.SetBottom(Area.pause.size.y);
		PaintFill(device, s); // FULL ROW ABOVE TOOLS
		s.y = Area.pause.size.y;
		s.w = Area.pause.size.x;
		s.h = Area.pause.size.h;
		PaintFill(device, s); // SPACE LEFT OF PAUSE
		s.x = Area.remove.size.Right();
		PaintFill(device, s); // SPACE RIGHT OF REMOVE
		s.CloseRight();
		s.w = 1;
		PaintFill(device, s, Draw.color.middle.brush); // LINE BETWEEN BUTTONS AND ADDRESS
		s.x++;
		s.SetRight(Area.address.size.x);
		s.SetBottom(Area.list.y - 1);
		PaintFill(device, s); // SPACE LEFT OF ADDRESS
		s.x = Area.address.size.Right();
		s.SetRight(Area.bar.size.x);
		PaintFill(device, s); // SPACE BETWEEN ADDRESS AND EDIT
		s.x = Area.bar.size.Right();
		s.SetRight(Area.enter.size.x);
		PaintFill(device, s); // SPACE BETWEEN EDIT AND GET
		s.x = Area.enter.size.Right();
		s.SetRight(client.w);
		PaintFill(device, s); // SPACE RIGHT OF GET
		s = Area.edit;
		s.w = 1;
		s.x--;
		s.ShiftTop(-1);
		PaintFill(device, s, Draw.color.middle.brush); // LEFT EDIT BORDER
		s.x = Area.edit.Right();
		PaintFill(device, s, Draw.color.middle.brush); // RIGHT EDIT BORDER
		s = Area.edit;
		s.y--;
		s.h = 1;
		PaintFill(device, s, Draw.color.middle.brush); // TOP EDIT BORDER
		s = Area.pause.size;
		s.SetRight(Area.remove.size.Right() + s.x + 1);
		s.SetLeft(0);
		s.y = Area.pause.size.Bottom();
		s.SetBottom(Area.list.y - 1);
		PaintFill(device, s); // BENEATH PAUSE AND REMOVE
		s = Area.address.size;
		s.CloseBottom();
		s.SetBottom(Area.list.y - 1);
		PaintFill(device, s); // BENEATH ADDRESS
		s = Area.bar.size;
		s.y = Area.button.Bottom();
		s.SetBottom(Area.list.y - 1);
		PaintFill(device, s); // BENEATH BUTTON AND COPY
		s = Area.copy.size;
		s.CloseRight();
		s.SetRight(Area.bar.size.Right());
		PaintFill(device, s); // TO THE RIGHT OF COPY
		s = Area.enter.size;
		s.CloseBottom();
		s.SetBottom(Area.list.y - 1);
		PaintFill(device, s); // BENEATH GET
		s = Area.list;
		s.Expand();
		PaintBorder(device, s, Draw.color.middle.brush); // BORDER AROUND LIST
		s.x = 0;
		s.w = 1;
		PaintFill(device, s); // LINE LEFT OF LIST
		s.x = client.w - 1;
		PaintFill(device, s); // LINE RIGHT OF LIST


	}

	// STATUS BAR
	PaintText(device, State.status, Area.status, false, true, true, true);
	*/
}

// Takes a device context and an area item
// Paints the area item in the window
void PaintArea(deviceitem *device, areaitem *a) {

	// Only paint areas that are in use and have size
	if (a->command == CommandNone || !a->size.Is()) return;

	// Define sizes as local variables
	int space = 4;
	sizeitem icon, smallicon, text, s;
	icon.w = icon.h = 24;
	smallicon.w = smallicon.h = 16;

	// Button
	if (a->command == CommandUnavailable || a->command == CommandReady || a->command == CommandSet) {

		//TODO no buttons yet

	// Link
	} else if (a->command == CommandLink) {

		// Underline link
		HFONT underline = NULL;
		if (a->display == DisplayHot) underline = Handle.underline;

		// Paint Tools
		if (a == &Area.tools) {

			device->Background(TRANSPARENT);
			PaintIcon(device, a->sizeicon, Handle.blue16, NULL);
			
			device->FontColor(Handle.white.color);
			device->Font(Handle.underline);
			PaintLabel(device, a->text, a->sizetext);

			// Paint Tools down arrow
			s = Area.tools.sizetext;
			s.CloseRight();
			s.y += (s.h / 2) + 2;
			s.x -= 7;
			s.w = 5;
			s.h = 1;
			PaintFill(device, s, Handle.white.brush);
			s.x++;
			s.y++;
			s.w -= 2;
			PaintFill(device, s, Handle.white.brush);
			s.x++;
			s.y++;
			s.w -= 2;
			PaintFill(device, s, Handle.white.brush);
		}

	// Other
	} else {

		// Bar
		if (a == &Area.bar) {

			s = Area.bar.size;
			PaintFill(device, s);

		// Corner
		} else if (a == &Area.corner) {

			s = Area.corner.size;
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



