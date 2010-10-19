
// Include libtorrent
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/alert_types.hpp"

// Include platform
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>
#include <netfw.h>

// Include program
#include "resource.h"
#include "define.h"
#include "object.h"
#include "library.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Areas;
extern datatop   Data;
extern statetop  State;

// Takes a window
// Uses this size in client coordinates
// Converts the size to screen coordinates
void Size::Screen(HWND window) {

	// Choose window
	if (!window) window = Handle.window;

	// Make a point with the x and y coordinates of this size item
	POINT p = Point();

	// Convert the point
	Size size;
	if (!ClientToScreen(window, &p)) { error(L"clienttoscreen"); return; }

	// Store the converted position in this size item
	x = p.x;
	y = p.y;
}

// Takes a window
// Uses this size in screen coordinates
// Converts the size to client coordinates
void Size::Client(HWND window) {

	// Choose window
	if (!window) window = Handle.window;

	// Make a point with the x and y coordinates of this size item
	POINT p = Point();

	// Convert the point
	Size size;
	if (!ScreenToClient(window, &p)) { error(L"screentoclient"); return; }

	// Store the converted position in this size item
	x = p.x;
	y = p.y;
}

// Load the given handle to a device context into this object
void Device::OpenUse(HDC newdevice) {

	// Record how we opened the device
	if (open != DeviceNone) return;
	open = DeviceUse;

	// Use the given device context
	device = newdevice;
}

// Create a default display device context for the screen
void Device::OpenCreate() {

	// Record how we opened the device
	if (open != DeviceNone) return;
	open = DeviceCreate;

	// Create the device context
	device = CreateDC(L"DISPLAY", NULL, NULL, NULL);
	if (!device) error(L"createdc");
}

// Get the device context from the given window
void Device::OpenGet(HWND newwindow) {

	// Record how we opened the device
	if (open != DeviceNone) return;
	open = DeviceGet;

	// Get the device context
	window = newwindow;
	device = GetDC(newwindow);
	if (!device) error(L"getdc");
}

// Tell the system the program will start painting
void Device::OpenPaint(HWND newwindow) {

	// Record how we opened the device
	if (open != DeviceNone) return;
	open = DevicePaint;

	// Paint the device context
	window = newwindow;
	device = BeginPaint(window, &paint);
	if (!device) error(L"beginpaint");
}

// Restore the contents of the device context and end or delete it
Device::~Device() {

	// Put everything back into the device context
	if (font) SelectObject(device, font);
	if (replacebackground) SetBkMode(device, background);
	if (replacefontcolor) SetTextColor(device, fontcolor);
	if (replacebackgroundcolor) SetBkColor(device, backgroundcolor);

	// Close the device context
	if      (open == DeviceCreate) { if (!DeleteDC(device))          error(L"deletedc"); }
	else if (open == DeviceGet)    { if (!ReleaseDC(window, device)) error(L"releasedc"); }
	else if (open == DevicePaint)  { EndPaint(window, &paint); }
}

// Load the given font into this device
void Device::Font(HFONT newfont) {

	// Keep the first one that comes out
	HFONT outfont = (HFONT)SelectObject(device, newfont);
	if (!font) font = outfont;
}

// Load the given background mode into this device
void Device::Background(int newbackground) {

	// Keep the first one that comes out
	int outbackground = SetBkMode(device, newbackground);
	if (!replacebackground) { replacebackground = true; background = outbackground; }
}

// Load the given text color into this device
void Device::FontColor(COLORREF newcolor) {

	// Keep the first one that comes out
	COLORREF outcolor = SetTextColor(device, newcolor);
	if (!replacefontcolor) { replacefontcolor = true; fontcolor = outcolor; }
}

// Loads the given background color into this device
void Device::BackgroundColor(COLORREF newcolor) {

	// Keep the first one that comes out
	COLORREF outcolor = SetBkColor(device, newcolor);
	if (!replacebackgroundcolor) { replacebackgroundcolor = true; backgroundcolor = outcolor; }
}
