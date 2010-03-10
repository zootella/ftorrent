
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

// Make painting tools once when the program starts
void PaintCreate() {

	// Load cursors
	Handle.arrow      = LoadSharedCursor(IDC_ARROW);
	Handle.horizontal = LoadSharedCursor(IDC_SIZEWE);
	Handle.vertical   = LoadSharedCursor(IDC_SIZENS);
	Handle.diagonal   = LoadSharedCursor(IDC_SIZENWSE);
	Handle.hand       = LoadSharedCursor(IDC_HAND);

	// Load icons
	Handle.iconbig   = LoadIconResource(L"APPLICATION_ICON", 32);
	Handle.iconsmall = LoadIconResource(L"APPLICATION_ICON", 16);

	// Load menus
	HMENU menus = MenuLoad(L"CONTEXT_MENU");
	Handle.menutaskbar = MenuClip(menus, 0);
	Handle.menutools   = MenuClip(menus, 1);

	// Remove the test menu item
	if (!PROGRAM_TEST) {
		if (!DeleteMenu(Handle.menutools, ID_TOOLS_TEST, 0)) Report(L"deletemenu");
	}

	// Get shared handles to system brushes
	Handle.face = BrushSystem(COLOR_3DFACE);
	Handle.shadow = BrushSystem(COLOR_3DSHADOW);
	Handle.background = BrushSystem(COLOR_WINDOW);
	Handle.ink = BrushSystem(COLOR_WINDOWTEXT);
	Handle.select = BrushSystem(COLOR_HIGHLIGHT);

	// Mix the middle gray color brush
	Handle.middle = CreateBrush(ColorMix(GetSysColor(COLOR_3DFACE), 1, GetSysColor(COLOR_3DSHADOW), 1));

	// Make a font based on what the system uses in message boxes
	NONCLIENTMETRICS info;
	ZeroMemory(&info, sizeof(info));
	DWORD size = sizeof(info) - sizeof(info.iPaddedBorderWidth); // Ignore last int for this to work
	info.cbSize = size;
	int result = SystemParametersInfo(
		SPI_GETNONCLIENTMETRICS, // System parameter to retrieve
		size,                    // Size of the structure
		&info,                   // Structure to fill with information
		0);                      // Not setting a system parameter
	if (!result) Report(L"systemparametersinfo");
	Handle.font = CreateFontIndirect(&info.lfMenuFont);
	if (!Handle.font) Report(L"createfontindirect");
}

// Paint the client area of the window and resize the child window controls
void Paint() {

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
}
