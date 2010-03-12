
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

// Load the given handle to a device context into this object
void deviceitem::OpenUse(HDC newdevice) {

	// Record how we opened the device
	if (open != DeviceNone) return;
	open = DeviceUse;

	// Use the given device context
	device = newdevice;
}

// Create a default display device context for the screen
void deviceitem::OpenCreate() {

	// Record how we opened the device
	if (open != DeviceNone) return;
	open = DeviceCreate;

	// Create the device context
	device = CreateDC(L"DISPLAY", NULL, NULL, NULL);
	if (!device) Report(L"createdc");
}

// Get the device context from the given window
void deviceitem::OpenGet(HWND newwindow) {

	// Record how we opened the device
	if (open != DeviceNone) return;
	open = DeviceGet;

	// Get the device context
	window = newwindow;
	device = GetDC(newwindow);
	if (!device) Report(L"getdc");
}

// Tell the system the program will start painting
void deviceitem::OpenPaint(HWND newwindow) {

	// Record how we opened the device
	if (open != DeviceNone) return;
	open = DevicePaint;

	// Paint the device context
	window = newwindow;
	device = BeginPaint(window, &paint);
	if (!device) Report(L"beginpaint");
}

// Restore the contents of the device context and end or delete it
deviceitem::~deviceitem() {

	// Put everything back into the device context
	if (font) SelectObject(device, font);
	if (replacebackground) SetBkMode(device, background);
	if (replacefontcolor) SetTextColor(device, fontcolor);
	if (replacebackgroundcolor) SetBkColor(device, backgroundcolor);

	// Close the device context
	if      (open == DeviceCreate) { if (!DeleteDC(device))          Report(L"deletedc"); }
	else if (open == DeviceGet)    { if (!ReleaseDC(window, device)) Report(L"releasedc"); }
	else if (open == DevicePaint)  { EndPaint(window, &paint); }
}

// Load the given font into this device
void deviceitem::Font(HFONT newfont) {

	// Keep the first one that comes out
	HFONT outfont;
	outfont = (HFONT)SelectObject(device, newfont);
	if (!font) font = outfont;
}

// Load the given background mode into this device
void deviceitem::Background(int newbackground) {

	// Keep the first one that comes out
	int outbackground;
	outbackground = SetBkMode(device, newbackground);
	if (!replacebackground) { replacebackground = true; background = outbackground; }
}

// Load the given text color into this device
void deviceitem::FontColor(COLORREF newcolor) {

	// Keep the first one that comes out
	COLORREF outcolor;
	outcolor = SetTextColor(device, newcolor);
	if (!replacefontcolor) { replacefontcolor = true; fontcolor = outcolor; }
}

// Loads the given background color into this device
void deviceitem::BackgroundColor(COLORREF newcolor) {

	// Keep the first one that comes out
	COLORREF outcolor = SetBkColor(device, newcolor);
	if (!replacebackgroundcolor) { replacebackgroundcolor = true; backgroundcolor = outcolor; }
}





void sizeitem::Screen(HWND window)
{
	// takes a window
	// uses this size in client coordinates
	// converts the size to screen coordinates

	// CHOOSE WINDOW
	if (!window) window = Handle.window;

	// MAKE A POINT WITH THE X AND Y COORDINATES OF THIS SIZE ITEM
	POINT p;
	p = Point();

	// CONVERT THE POINT
	sizeitem size;
	if (!ClientToScreen(window, &p)) { Report(L"error clienttoscreen"); return; }

	// STORE THE CONVERTED POSITION IN THIS SIZE ITEM
	x = p.x;
	y = p.y;
}

void sizeitem::Client(HWND window)
{
	// takes a window
	// uses this size in screen coordinates
	// converts the size to client coordinates

	// CHOOSE WINDOW
	if (!window) window = Handle.window;

	// MAKE A POINT WITH THE X AND Y COORDINATES OF THIS SIZE ITEM
	POINT p;
	p = Point();

	// CONVERT THE POINT
	sizeitem size;
	if (!ScreenToClient(window, &p)) { Report(L"error screentoclient"); return; }

	// STORE THE CONVERTED POSITION IN THIS SIZE ITEM
	x = p.x;
	y = p.y;
}


















