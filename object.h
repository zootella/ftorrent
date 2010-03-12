
// A device context with information about how to put it away
enum deviceopen {

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
class brushitem {
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
	sizeitem    size;       // Position and size of area in the main client window
	areacommand command;    // The state of the command this area item represents
	areadisplay display;    // How this area is currently drawn in the window
	string      text, tip;  // Text painted in the area item and any for a tooltip
	int         adjust;     // Pixels to nudge the text horizontally
	sizeitem    textsize;   // How big the text is when painted
	HICON       icon, gray; // Icons for available and unavailable appearance

	// New
	areaitem() {

		command = CommandNone;
		display = DisplayNone;
		adjust = 1; // By default, adjust text to the right 1 pixel
		icon = gray = NULL;
	}
};
