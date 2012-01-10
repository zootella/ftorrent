
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// The mouse dropped something from another program on our window
HRESULT __stdcall Target::Drop(IDataObject *data, DWORD key, POINTL point, DWORD *effect) {

	// Loop through the formats the given data object supports
	IEnumFORMATETC *formats = NULL;
	FORMATETC format;
	ZeroMemory(&format, sizeof(format));
	STGMEDIUM storage;
	WCHAR bay[MAX_PATH];
	CString name, value;
	if (data->EnumFormatEtc(DATADIR_GET, &formats) == S_OK) { // Access the formats
		while (formats->Next(1, &format, NULL) == S_OK) { // Loop for each format, getting them 1 at a time
			if (GetClipboardFormatName(format.cfFormat, bay, MAX_PATH)) { // Get the text name of the format
				name = bay;

				// The format is one we're looking for
				if (name == L"FileNameW" || name == L"UniformResourceLocatorW") {

					// Get the storage data value
					ZeroMemory(&storage, sizeof(storage));
					storage.tymed = TYMED_HGLOBAL; // For storage medium type, specify we want a handle to global memory
					if (data->GetData(&format, &storage) == S_OK) {

						// Copy out the text
						HGLOBAL g = storage.hGlobal;
						WCHAR *w = (WCHAR *)GlobalLock(g); // Lock the global memory handle
						if (w) value = w;
						else   value = L"";
						GlobalUnlock(g); // Unlock the global memory handle

						// Add the path or link
						if (is(value)) {
							if (name == L"FileNameW") {
								DropPath(value);
							} else if (name == L"UniformResourceLocatorW") {
								DropLink(value);
							}
						}
					}
				}
			}
		}
	}
	if (formats) formats->Release();

	// Specify the drop as a copy and report success
	*effect = DROPEFFECT_COPY;
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
