
// Get information about a single file, or list the contents of a folder
class finditem { // change to Find
public:

	// Members
	HANDLE handle;        // Search handle
	WIN32_FIND_DATA info; // Information about what we found this time
	CString search;       // Query path

	// Takes a path to a file or folder, and false to get information about it, or true to list its contents
	finditem(read path, bool list = true) {

		// Set values to start the search
		handle = INVALID_HANDLE_VALUE;
		ZeroMemory(&info, sizeof(info));
		search = path;
		if (list) search += L"\\*.*";

		// We're not going to use this in a loop, run the single search now
		if (!list) result();
	}

	// Clean up contents when this object goes out of scope
	~finditem() { close(); }

	// Loop calling this method to get results until it returns false
	bool result() {

		// Start the search
		if (handle == INVALID_HANDLE_VALUE) {
			handle = FindFirstFile(search, &info);
			if (handle == INVALID_HANDLE_VALUE) return false; // Not found or other error

			// Skip over "." and ".." at the start
			if (info.cFileName != CString(L".") && info.cFileName != CString(L"..")) return true;
		}

		// Get the next file or folder in the list
		while (FindNextFile(handle, &info)) {

			// Skip over "." and ".." at the start
			if (info.cFileName != CString(L".") && info.cFileName != CString(L"..")) return true;
		}

		// Done listing the files
		close();
		return false;
	}

	// True if this object found
	bool found() { return handle != INVALID_HANDLE_VALUE; } // A file or folder
	bool folder() { return (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; } // A folder

	// Close the search we started
	void close() {
		if (handle != INVALID_HANDLE_VALUE) {
			FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
		}
	}
};

// Wraps a registry key, taking care of closing it
class registryitem { // change to Registry
public:

	// The handle to the registry key
	HKEY Key;

	// Open a registry key and store its handle in this object
	bool Open(HKEY root, read path, bool write);
	void Close() { if (Key) RegCloseKey(Key); Key = NULL; }

	// Make a new local registryitem object, and delete it when it goes out of scope
	registryitem() { Key = NULL; }
	~registryitem() { Close(); }
};

// Wraps a BSTR, a COM string, taking care of memory allocation
class bstritem { // change to Bstr
public:

	// The BSTR
	BSTR bstr;

	// Make a new bstritem object
	bstritem()       { bstr = NULL; }         // With no BSTR allocated
	bstritem(read r) { bstr = NULL; Set(r); } // From the given text
	~bstritem()      { Clear(); }             // It frees its memory when you delete it

	// Use AllocSysString and SysFreeString to allocate and free the BSTR
	void Set(CString s) { Clear(); bstr = s.AllocSysString(); }
	void Clear() { if (bstr) { SysFreeString(bstr); bstr = NULL; } }
};

// Add an application to the Windows Firewall exceptions list
class firewallitem { // change to Firewall
public:

	// COM interfaces
	INetFwMgr                    *manager;
	INetFwPolicy                 *policy;
	INetFwProfile                *profile;
	INetFwAuthorizedApplications *list;
	INetFwAuthorizedApplication  *program;

	// Make a new firewallitem object
	firewallitem() {

		// Set the COM interface pointers to NULL so we'll know if we've initialized them
		manager = NULL;
		policy  = NULL;
		profile = NULL;
		list    = NULL;
		program = NULL;
	}

	// Delete the firewallitem object
	~firewallitem() {

		// Release the COM interfaces that we got access to
		if (program) { program->Release(); program = NULL; } // Release them in reverse order
		if (list)    { list->Release();    list    = NULL; }
		if (profile) { profile->Release(); profile = NULL; }
		if (policy)  { policy->Release();  policy  = NULL; }
		if (manager) { manager->Release(); manager = NULL; }
	}

	// Methods to adjust the settings of Windows Firewall
	bool Access();
	bool AddProgram(read path, read name);
	bool RemoveProgram(read path);
};

// A device context with information about how to put it away
enum deviceopen { // change to Device

	DeviceNone,   // We haven't obtained the device context yet
	DeviceUse,    // The system provided a device context for the program to use
	DeviceCreate, // The program created a default display device context
	DeviceGet,    // The program asked the system for the window's device context
	DevicePaint,  // The program got the device context in response to a paint message
};
class deviceitem {
public:
~deviceitem();

	// Methods
	void OpenUse(HDC newdevice);
	void OpenCreate();
	void OpenGet(HWND newwindow);
	void OpenPaint(HWND newwindow);
	void Font(HFONT newfont);
	void Background(int newbackground);
	void FontColor(COLORREF newcolor);
	void BackgroundColor(COLORREF newcolor);

	// The device context
	deviceopen open;
	HDC device;
	HWND window;
	PAINTSTRUCT paint;

	// Painting tools to put back
	HFONT font;
	bool replacebackground, replacefontcolor, replacebackgroundcolor;
	COLORREF fontcolor, backgroundcolor;
	int background;

	// New
	deviceitem() {

		open = DeviceNone;
		device = NULL;
		window = NULL;
		font = NULL;
		replacebackground = replacefontcolor = replacebackgroundcolor = false;
	}
};

// Hold a color and brush
class brushitem { // change to Brush
public:

	// The color and brush
	COLORREF color;
	HBRUSH brush;

	// New
	brushitem() {
		color = 0;
		brush = NULL;
	}
};

// A rectangular size in the window
class sizeitem {
public:

	// Coordinates from client area origin and width and height dimensions
	int x, y, w, h;

	// New
	sizeitem() { Clear(); }
	sizeitem(POINT p) { Set(p); }
	sizeitem(RECT  r) { Set(r); }
	sizeitem(SIZE  s) { Set(s); }

	// Clear
	void Clear() { x = y = w = h = 0; }

	// Set
	void Set(POINT p) { x = p.x;    y = p.y;   w = 0;           h = 0;            }
	void Set(RECT  r) { x = r.left; y = r.top; w = r.right - x; h = r.bottom - y; }
	void Set(SIZE  s) { x = 0;      y = 0;     w = s.cx;        h = s.cy;         }

	// Convert
	POINT Point() { POINT p; p.x = x; p.y = y; return(p); }
	RECT Rectangle() { RECT r; r.left = x; r.top = y; r.right = x + w; r.bottom = y + h; return(r); }
	SIZE Size() { SIZE s; s.cx = w; s.cy = h; return(s); }

	// Take negative width or height to 0
	void Check() { if (w < 0) w = 0; if (h < 0) h = 0; }

	// Determine if the size holds any pixels, and if a point is inside the size
	bool Is() { return(w > 0 && h > 0); }
	bool Inside(sizeitem s) { return(s.x >= x && s.x < x + w && s.y >= y && s.y < y + h); }

	// Read like a rectangle
	int Right() { return(x + w); }
	int Bottom() { return(y + h); }

	// Set like a rectangle
	void SetLeft(int left) { w += x - left; x = left; }
	void SetTop(int top) { h += y - top; y = top; }
	void SetRight(int right) { w = right - x; }
	void SetBottom(int bottom) { h = bottom - y; }

	// Position by the right or bottom edges
	void PositionRight(int right) { x = right - w; }
	void PositionBottom(int bottom) { y = bottom - h; }

	// Shift just the left and top boundaries by a pixel amount
	void ShiftLeft(int shift) { x += shift; w -= shift; }
	void ShiftTop(int shift) { y += shift; h -= shift; }

	// Collapse the size to the right or to the bottom
	void CloseRight() { x += w; w = 0; }
	void CloseBottom() { y += h; h = 0; }

	// Expand all the edges by a pixel amount
	void Expand(int shift = 1) { x -= shift; y -= shift; w += (2 * shift); h += (2 * shift); }

	// Convert between screen and client window coordinates
	void Screen(HWND window = NULL);
	void Client(HWND window = NULL);
};

// A window area that acts as a button, link, or sizing grip
enum areacommand {

	// The program changes an area item's command condition when teh availability or state of its command changes
	CommandNone,           // Uninitialized
	CommandUnavailable,    // The button's command is currently unavailable
	CommandReady,          // The button's command is ready
	CommandSet,            // The button's command is set on
	CommandMenu,           // A button that opens a menu
	CommandLink,           // A hyperlink
	CommandSizeVertical,   // A horizontal bar that sizes vertically
	CommandSizeHorizontal, // A vertical bar that sizes horizontally
	CommandSizeDiagonal,   // An area that sizes the width and height of the window
};
enum areadisplay {

	// These are all the ways an area item can be displayed on the screen
	DisplayNone,    // Bar or uninitialized button
	DisplayGhosted, // The button is gray and unresponsive
	DisplayReady,   // The button looks ready to be clicked
	DisplayHot,     // The mouse has activated the button
	DisplayPressed, // The button is pressed, set, set hot, or set pressed, which all look the same
};
class areaitem {
public:

	// Pointers
	areaitem *next;

	// Data
	sizeitem    size;           // Position and size of area in the main client window
	areacommand command;        // The state of the command this area item represents
	areadisplay display;        // How this area is currently drawn in the window
	CString     text, tip;      // Text painted in the area item and any for a tooltip
	int         adjust;         // Pixels to nudge the text horizontally
	sizeitem    textsize;       // How big the text is when painted
	HICON       icon, hot, dim; // Icons for normal, hot and dim appearances

	// New
	areaitem() {

		command = CommandNone;
		display = DisplayNone;
		adjust = 1; // By default, adjust text to the right 1 pixel
		icon = hot = dim = NULL;
	}
};

// Torrent
class torrentitem {
public:

	CString folder; //save folder, required
	CString name; //name from the magnet link, blank if we've got the torrent file
	std::set<CString> trackers; //trackers from all sources



	libtorrent::torrent_handle handle;

	// New
	torrentitem() {

	}

	void Save();
	bool Load(hbig hash);

	void Edit();

	int ComposeStatusIcon();
	CString ComposeStatus();
	int ComposeNameIcon();
	CString ComposeName();
	CString ComposeSize();
	CString ComposeHash();
	CString ComposePath();

	DWORD Hash();



};









