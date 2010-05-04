
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
	s = Area.status;
	s.h = 1;
	PaintFill(device, s, Handle.line.brush);
	s = Area.status;
	s.ShiftTop(1);
	PaintText(device, State.status, s, false, true, true, true);
}

// Takes a device context and an area item
// Paints the area item in the window
void PaintArea(deviceitem *device, areaitem *a) {

	// Only paint areas that are in use and have size
	if (a->command == CommandNone || !a->size.Is()) return;

	// Define sizes as local variables
	int space = 2;
	sizeitem icon, s;

	// Button
	if (a->command == CommandMenu || a->command == CommandUnavailable || a->command == CommandReady || a->command == CommandSet) {

		// Place icon
		icon.x = a->size.x + space;
		icon.y = a->size.y + space;
		icon.w = a->size.w - (2 * space);
		icon.h = a->size.h - (2 * space);

		// Paint icon
		if      (a->display == DisplayGhosted) PaintIcon(device, icon, a->dim,  Handle.blue.brush);
		else if (a->display == DisplayHot)     PaintIcon(device, icon, a->hot,  Handle.blue.brush);
		else                                   PaintIcon(device, icon, a->icon, Handle.blue.brush);

		// Fill outside margins
		s = a->size;
		s.SetBottom(icon.y);
		PaintFill(device, s, Handle.blue.brush); // Row above icon
		s = icon;
		s.w = 0;
		s.SetLeft(a->size.x);
		PaintFill(device, s, Handle.blue.brush); // Left of icon
		s = icon;
		s.CloseRight();
		s.SetRight(a->size.Right());
		PaintFill(device, s, Handle.blue.brush); // Right of icon
		s = a->size;
		s.y = icon.Bottom();
		s.SetBottom(a->size.Bottom());
		PaintFill(device, s, Handle.blue.brush); // Row beneath icon

	// Link
	} else if (a->command == CommandLink) {

	// Other
	} else {

		// Bar
		if (a == &Area.bar) {

			s = Area.bar.size;
			s.h = 1;
			PaintFill(device, s, Handle.line.brush);
			s = Area.bar.size;
			s.ShiftTop(1);
			PaintFill(device, s, Handle.face.brush);

		// Corner
		} else if (a == &Area.corner) {

			s = Area.corner.size;
			s.h = 1;
			PaintFill(device, s, Handle.line.brush);
			s = Area.corner.size;
			s.ShiftTop(1);
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



