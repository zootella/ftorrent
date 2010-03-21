
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
	device->Font(Handle.arial); // Find out how big the sign text will be
	device->FontColor(Handle.lightblue.color);
	device->BackgroundColor(Handle.blue.color);
	sizeitem t = SizeText(device, State.title);
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
	PaintLabel(device, State.title, s);
	device->Font(Handle.font);

	// Paint all the areas
	areaitem *a = Area.all;
	while (a) {
		PaintArea(device, a);
		a = a->next;
	}

	// Status
	device->Font(Handle.font);
	device->FontColor(Handle.ink.color);
	device->BackgroundColor(Handle.background.color);
	PaintText(device, State.status, Area.status, false, true, true, true);
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



