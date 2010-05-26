
// Include libtorrent
#include "libtorrent/create_torrent.hpp"

// Include platform
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>

// Include program
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

	// Define sizes
	int margin = 8; // Margin to the left or right of stage text
	int above = -7; // Draw the stage text this many pixels above the client area

	// Paint the area between the toolbar buttons and the stage text
	sizeitem s1 = Area.stage;
	s1.w = Area.stage.w - State.stage->size.w - margin;
	if (s1.w < margin) s1.w = margin; // Narrow window, pin to left instead of right
	PaintFill(device, s1, State.stage->background.brush);

	// Paint the stage text
	sizeitem s2 = State.stage->size; // Start with text width and height
	s2.x = s1.Right();
	s2.y = above;
	s2.SetBottom(Area.stage.Bottom()); // Don't paint down into the list view control
	device->Font(Handle.arial);
	device->FontColor(State.stage->ink.color);
	device->BackgroundColor(State.stage->background.color);
	PaintLabel(device, State.stage->title, s2);

	// Paint the area to the right of the stage text
	sizeitem s3 = Area.stage;
	s3.SetLeft(s2.Right());
	PaintFill(device, s3, State.stage->background.brush);

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
	sizeitem s = Area.status;
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
		if      (a->display == DisplayGhosted) PaintIcon(device, icon, a->dim,  State.stage->background.brush);
		else if (a->display == DisplayHot)     PaintIcon(device, icon, a->hot,  State.stage->background.brush);
		else                                   PaintIcon(device, icon, a->icon, State.stage->background.brush);

		// Fill outside margins
		s = a->size;
		s.SetBottom(icon.y);
		PaintFill(device, s, State.stage->background.brush); // Row above icon
		s = icon;
		s.w = 0;
		s.SetLeft(a->size.x);
		PaintFill(device, s, State.stage->background.brush); // Left of icon
		s = icon;
		s.CloseRight();
		s.SetRight(a->size.Right());
		PaintFill(device, s, State.stage->background.brush); // Right of icon
		s = a->size;
		s.y = icon.Bottom();
		s.SetBottom(a->size.Bottom());
		PaintFill(device, s, State.stage->background.brush); // Row beneath icon

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



