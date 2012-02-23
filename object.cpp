
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// The mouse dragged something into our window
HRESULT __stdcall Target::DragEnter(IDataObject *data, DWORD key, POINTL point, DWORD *effect) {
	*effect = App.cycle.drop = CanDrop(data) ? DROPEFFECT_COPY : DROPEFFECT_NONE; // Determine if we could take what the user has dragged over us
	return S_OK;
}

// The mouse moved while holding down something over our window
HRESULT __stdcall Target::DragOver(DWORD key, POINTL point, DWORD *effect) {
	*effect = App.cycle.drop; // Set the effect we picked when we looked at the data object in drag enter
	return S_OK;
}

// The mouse left after dragging something over our window
HRESULT __stdcall Target::DragLeave() {
	return S_OK;
}

// The mouse dropped something from another program on our window
HRESULT __stdcall Target::Drop(IDataObject *data, DWORD key, POINTL point, DWORD *effect) {

	// Determine if we could take what the user is holding over us
	CString linkmagnet, linkother, disktorrent, diskother;
	if (CanDrop(data, &linkmagnet, &linkother, &disktorrent, &diskother)) { // We can take this

		// Add it
		if      (is(linkmagnet))  { AddMagnet(linkmagnet,      false); } // Drag in magnet link on the web
		else if (is(linkother))   { DownloadTorrent(linkother, false); } // Download torrent file or skin from the web (TODO)
		else if (is(disktorrent)) { AddTorrent(disktorrent,    false); } // Drag in torrent file on the disk
		else if (is(diskother))   { CreateTorrent(diskother,   false); } // Create torrent from file or folder on the disk, or put on skin (TODO)

		// Show the mouse pointer with a plus icon
		*effect = DROPEFFECT_COPY;

	} else { // We can't accept what the user is about to drop on us

		// Turn the mouse pointer into a negative icon
		*effect = DROPEFFECT_NONE;
	}
	return S_OK;
}

// Takes a window
// Uses this size in client coordinates
// Converts the size to screen coordinates
void Size::Screen(HWND window) {

	// Choose window
	if (!window) window = App.window.main;

	// Make a point with the x and y coordinates of this size
	POINT p = Point();

	// Convert the point
	Size size;
	if (!ClientToScreen(window, &p)) { error(L"clienttoscreen"); return; }

	// Store the converted position in this size
	x = p.x;
	y = p.y;
}

// Takes a window
// Uses this size in screen coordinates
// Converts the size to client coordinates
void Size::Client(HWND window) {

	// Choose window
	if (!window) window = App.window.main;

	// Make a point with the x and y coordinates of this size
	POINT p = Point();

	// Convert the point
	Size size;
	if (!ScreenToClient(window, &p)) { error(L"screentoclient"); return; }

	// Store the converted position in this size
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
