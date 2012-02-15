
// Target the user can drag and drop links and files onto
class Target : public IDropTarget {
public:

	// Standard methods for ole objects
	HRESULT __stdcall QueryInterface(REFIID id, void **object) { // Get pointers to the supported interface on this target object
		if (id == IID_IDropTarget || id == IID_IUnknown) { // Yes, we support this interface
			AddRef();       // Count another user
			*object = this; // Point the given pointer at us
			return S_OK;    // Report success
		} else {                  // No, we don't support the requested interface
			*object = NULL;       // Null the given pointer
			return E_NOINTERFACE; // Interface not found
		}
	}
	ULONG __stdcall AddRef() { // Count that the interface of this object has one more external user
		return InterlockedIncrement(&references);
	}
	ULONG __stdcall Release() { // Count one fewer external user
		LONG count = InterlockedDecrement(&references);
		if (!count) delete this; // No external users of our interface
		return count;
	}

	// Methods specific to a drop target
	HRESULT __stdcall DragEnter(IDataObject *data, DWORD key, POINTL point, DWORD *effect);
	HRESULT __stdcall DragOver(DWORD key, POINTL point, DWORD *effect);
	HRESULT __stdcall DragLeave();
	HRESULT __stdcall Drop(IDataObject *data, DWORD key, POINTL point, DWORD *effect);

	// New and delete
	Target(HWND window) { references = 1; } // Count our own reference to this new object
	~Target() {}

	// The reference count of this ole object, 0 when nobody wants it and it should be deleted
	LONG references;
};

// Get information about a single file, or list the contents of a folder
class Find {
public:

	// Members
	HANDLE handle;        // Search handle
	WIN32_FIND_DATA info; // Information about what we found this time
	CString search;       // Query path

	// Takes a path to a file or folder, and false to get information about it, or true to list its contents
	Find(read path, bool list = true) {

		// Set values to start the search
		handle = INVALID_HANDLE_VALUE;
		ZeroMemory(&info, sizeof(info));
		search = path;
		if (list) search += L"\\*.*";

		// We're not going to use this in a loop, run the single search now
		if (!list) Result();
	}

	// Clean up contents when this object goes out of scope
	~Find() { Close(); }

	// Loop calling this method to get results until it returns false
	bool Result() {

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
		Close();
		return false;
	}

	// True if this object found
	bool Found() { return handle != INVALID_HANDLE_VALUE; } // A file or folder
	bool Folder() { return (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; } // A folder

	// Close the search we started
	void Close() {
		if (handle != INVALID_HANDLE_VALUE) {
			FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
		}
	}
};

// Wraps a registry key, taking care of closing it
class Registry {
public:

	// The handle to the registry key
	HKEY key;

	// Open a registry key and store its handle in this object
	bool Open(HKEY root, read path, bool write);
	void Close() { if (key) RegCloseKey(key); key = NULL; }

	// Make a new local registry object, and delete it when it goes out of scope
	Registry() { key = NULL; }
	~Registry() { Close(); }
};

// Wraps a BSTR, a COM string, taking care of memory allocation
class Bstr {
public:

	// The BSTR
	BSTR bstr;

	// Make a new object
	Bstr()       { bstr = NULL; }         // With no BSTR allocated
	Bstr(read r) { bstr = NULL; Set(r); } // From the given text
	~Bstr()      { Clear(); }             // It frees its memory when you delete it

	// Use AllocSysString and SysFreeString to allocate and free the BSTR
	void Set(CString s) { Clear(); bstr = s.AllocSysString(); }
	void Clear() { if (bstr) { SysFreeString(bstr); bstr = NULL; } }
};

// Add an application to the Windows Firewall exceptions list
class Firewall {
public:

	// COM interfaces
	INetFwMgr                    *manager;
	INetFwPolicy                 *policy;
	INetFwProfile                *profile;
	INetFwAuthorizedApplications *list;
	INetFwAuthorizedApplication  *program;

	// Make a new object
	Firewall() {

		// Set the COM interface pointers to NULL so we'll know if we've initialized them
		manager = NULL;
		policy  = NULL;
		profile = NULL;
		list    = NULL;
		program = NULL;
	}

	// Delete the object
	~Firewall() {

		// Release the COM interfaces that we got access to
		if (program) { program->Release(); program = NULL; } // Release them in reverse order
		if (list)    { list->Release();    list    = NULL; }
		if (profile) { profile->Release(); profile = NULL; }
		if (policy)  { policy->Release();  policy  = NULL; }
		if (manager) { manager->Release(); manager = NULL; }
	}

	// Methods to adjust the settings of Windows Firewall
	bool Access();
	bool Add(read path, read name);
	bool Remove(read path);
};

// A device context with information about how to put it away
enum DeviceOpen {

	DeviceNone,   // We haven't obtained the device context yet
	DeviceUse,    // The system provided a device context for the program to use
	DeviceCreate, // The program created a default display device context
	DeviceGet,    // The program asked the system for the window's device context
	DevicePaint,  // The program got the device context in response to a paint message
};
class Device {
public:
~Device();

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
	DeviceOpen open;
	HDC device;
	HWND window;
	PAINTSTRUCT paint;

	// Painting tools to put back
	HFONT font;
	bool replacebackground, replacefontcolor, replacebackgroundcolor;
	COLORREF fontcolor, backgroundcolor;
	int background;

	// New
	Device() {

		open = DeviceNone;
		device = NULL;
		window = NULL;
		font = NULL;
		replacebackground = replacefontcolor = replacebackgroundcolor = false;
	}
};

// Hold a color and brush
class Brush {
public:

	// The color and brush
	COLORREF color;
	HBRUSH brush;

	// New
	Brush() {
		color = 0;
		brush = NULL;
	}
};

// A rectangular size in the window
class Size {
public:

	// Coordinates from client area origin and width and height dimensions
	int x, y, w, h;

	// New
	Size() { Clear(); }
	Size(POINT p) { Set(p); }
	Size(RECT  r) { Set(r); }
	Size(SIZE  s) { Set(s); }

	// Clear
	void Clear() { x = y = w = h = 0; }

	// Set
	void Set(POINT p) { x = p.x;    y = p.y;   w = 0;           h = 0;            }
	void Set(RECT  r) { x = r.left; y = r.top; w = r.right - x; h = r.bottom - y; }
	void Set(SIZE  s) { x = 0;      y = 0;     w = s.cx;        h = s.cy;         }

	// Convert
	POINT Point() { POINT p; p.x = x; p.y = y; return p; }
	RECT Rectangle() { RECT r; r.left = x; r.top = y; r.right = x + w; r.bottom = y + h; return r; }
	SIZE ToSize() { SIZE s; s.cx = w; s.cy = h; return s; }

	// Take negative width or height to 0
	void Check() { if (w < 0) w = 0; if (h < 0) h = 0; }

	// Determine if the size holds any pixels, and if a point is inside the size
	bool Is() { return w > 0 && h > 0; }
	bool Inside(Size s) { return s.x >= x && s.x < x + w && s.y >= y && s.y < y + h; }

	// Read like a rectangle
	int Right() { return x + w; }
	int Bottom() { return y + h; }

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
enum AreaCommand {

	// The program changes an area's command condition when teh availability or state of its command changes
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
enum AreaDisplay {

	// These are all the ways an area can be displayed on the screen
	DisplayNone,    // Bar or uninitialized button
	DisplayGhosted, // The button is gray and unresponsive
	DisplayReady,   // The button looks ready to be clicked
	DisplayHot,     // The mouse has activated the button
	DisplayPressed, // The button is pressed, set, set hot, or set pressed, which all look the same
};
class Area {
public:

	// Pointers
	Area *next;

	// Data
	Size        size;           // Position and size of area in the main client window
	AreaCommand command;        // The state of the command this area represents
	AreaDisplay display;        // How this area is currently drawn in the window
	CString     text, tip;      // Text painted in the area and any for a tooltip
	int         adjust;         // Pixels to nudge the text horizontally
	Size        textsize;       // How big the text is when painted
	HICON       icon, hot, dim; // Icons for normal, hot and dim appearances

	// New
	Area() {

		command = CommandNone;
		display = DisplayNone;
		adjust = 1; // By default, adjust text to the right 1 pixel
		icon = hot = dim = NULL;
	}
};

/*
// A block of memory that can save itself to disk, and frees and closes when the object goes out of scope
class Memory {
public:

	DWORD size;   // Size in bytes of the memory block and disk file
	LPBYTE block; // Pointer to allocated memory, null before memory allocation
	HANDLE file;  // Handle to open disk file, null before created

	Memory() {

		size = 0;
		block = NULL;
		file = NULL;
	}

	// Allocate s bytes of memory
	bool Allocate(DWORD s) {

		size = s;
		block = (LPBYTE)malloc(size);
		if (!block) return false;
		return true;
	}

	// Save our memory to a new file at path on the disk
	bool Save(read path) {

		// Open the file, overwriting if necessary
		file = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!file || file == INVALID_HANDLE_VALUE) return false;

		// Write our data to the file
		DWORD written = 0;
		int result = WriteFile(file, block, size, &written, NULL);
		if (!result || written != size) return false;
		return true;
	}

	~Memory() {

		if (block) { free(block); block = NULL; }
		if (file) { CloseHandle(file); file = NULL; }
	}
};
*/

// A disk file for a web download that closes and deletes itself when the object goes out of scope
class WebFile {
public:

	CString path; // Path of file on the disk next to this running exe
	HANDLE file;  // Handle to open disk file, null before created, null
	DWORD size;   // Size in bytes of the disk file, how much data has been written to it

	WebFile() {

		file = NULL;
		size = 0;
	}

	bool Open();
	bool Add(BYTE *b, int n);
	bool Keep();

	~WebFile() {

		// Delete the file if we didn't close it
		if (file) {
			CloseHandle(file);
			DeleteFile(path);
		}
	}
};

// Save a wininet handle here to have this object close it when it goes out of scope
class WebHandle {
public:

	HINTERNET handle;

	WebHandle() {
		handle = NULL; // No handle saved here yet
	}

	~WebHandle() {
		if (handle) InternetCloseHandle(handle); // If we've got a handle, close it
	}
};







// Current program stage
class Stage {
public:

	// Text and resources to describe this stage
	CString title;
	Size size; // How big the text will be painted in the title font
	HICON icon16, icon32;
	Brush ink, background;

	// New
	Stage() {

		icon16 = icon32 = NULL;
	}
};






// A column in a list view control
class Column {
public:

	int place;     // 0+ index where this column is currently shown in the control, changes when the user drags columns to a different order
	int index;     // 0+ index the control uses to identify this column, changes when any columns are added or removed
	bool show;     // True when this column is shown in the control
	bool right;    // Right align the column
	int width;     // Column width in pixels, changes when the user sizes the column
	CString title; // Column title text, doesn't change and uniquely identifies the column

	Column() {
		place = -1; // Indices unknown
		index = -1;
		show  = false;
		right = false;
		width = -1; // Width not defined
	}
};

// State of the columns of a list view control
class List {
public:

	HWND window; // Handle to the list view child window control
	CString factory; // Factory default columns for this list
	std::vector<Column> current; // Current record of which columns are shown
	CString sort; // blank no sort, or title of the column sorted
	int direction; // 1 ascending, -1 descending, or 0 none

	List() {
		window = NULL;
		direction = 0;
	}
};

// A single item or subitem in a list view control
class Cell {
public:

	// Location
	LPARAM parameter; // Parameter that identifies the row this cell applies to
	CString title; // Column title text that identifies the column this cell applies to
	int column, row; // Recent coordinates in the list view control

	// Display
	sbig sort; // Sort order
	int icon; // Icon index in the program image list, or -1 to not have an icon
	CString text; // Text for the cell

	int screenicon; // Icon index on the screen
	CString screentext; // Text on the screen

	Cell(LPARAM set_parameter, read set_title, sbig set_sort = 0, int set_icon = -1, read set_text = L"") { // Has defaults
		parameter = set_parameter;
		title = set_title;
		sort = set_sort;
		icon = set_icon;
		text = set_text;

		column = row = -1; // Coordinates not yet known
		Hidden(); // Not shown on screen yet
	}

	bool Different() { // True to check for blank cells in a column that was just added
		return icon != screenicon || text != screentext;
	}

	void Same() {
		screenicon = icon;
		screentext = text;
	}

	void Hidden() {
		screenicon = -2;
		screentext = L"";
	}




};











// Torrent
class Torrent {
public:


	CString Path();
	bool paused;


	CString folder; //save folder, required
	CString name; //name from the magnet link, blank if we've got the torrent file
	std::set<CString> trackers; //trackers from all sources


	std::vector<Cell> cells;



	libtorrent::torrent_handle handle;




	// New
	Torrent() {
		paused = false;
	}




	bool CanOpen();
	void UseOpen();

	bool CanOpenContainingFolder();
	void UseOpenContainingFolder();

	bool CanCopyMagnetLink();
	void UseCopyMagnetLink();

	bool CanSaveTorrentAs();
	void UseSaveTorrentAs();

	bool CanStart();
	void UseStart();

	bool CanPause();
	void UsePause();

	bool CanRemove();
	void UseRemove();

	bool CanDelete();
	void UseDelete();







	void Save();
	bool Load(hbig hash);

	void Edit();

	void Compose();

	void ComposeStatus();
	void ComposeName();
	void ComposeSize();
	void ComposeInfohash();
	void ComposeLocation();

	Cell &GetCell(read title);

	LPARAM HashParameter();



};









