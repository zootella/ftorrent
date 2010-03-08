
// A rectangular size in the window
class sizeitem {
private:

	// Coordinates from client area origin, and width and height dimensions
	int mx, my, mw, mh;

public:

	// Get the dimensions
	int x() { return mx; }
	int y() { return my; }
	int w() { return mw; }
	int h() { return mh; }

	// Set the dimensions
	void x(int nx) { if (nx < 0) nx = 0; mx = nx; } // Replace a negative value with 0
	void y(int ny) { if (ny < 0) ny = 0; my = ny; }
	void w(int nw) { if (nw < 0) nw = 0; mw = nw; }
	void h(int nh) { if (nh < 0) nh = 0; mh = nh; }
	void set(int nx, int ny, int nw, int nh) { x(nx); y(ny); w(nw); h(nh); } // All at once
	void clear() { x(0); y(0); w(0); h(0); } // Clear all to 0

	// Adjust the dimensions
	void addx(int nx) { x(x() + nx); }
	void addy(int ny) { y(y() + ny); }
	void addw(int nw) { w(w() + nw); }
	void addh(int nh) { h(h() + nh); }

	// Right and bottom
	int r() { return x() + w(); }
	int b() { return y() + h(); }
	void r(int nr) { w(nr - x()); }
	void b(int nb) { h(nb - y()); }

	// New and convert in and out
	sizeitem() { clear(); }
	sizeitem(POINT p) { set(p); }
	sizeitem(RECT  r) { set(r); }
	sizeitem(SIZE  s) { set(s); }
	void set(POINT p) { x(p.x);    y(p.y);   w(0);             h(0);              }
	void set(RECT  r) { x(r.left); y(r.top); w(r.right - x()); h(r.bottom - y()); }
	void set(SIZE  s) { x(0);      y(0);     w(s.cx);          h(s.cy);           }
	POINT point() { POINT p; p.x = x(); p.y = y(); return p; }
	RECT rectangle() { RECT r; r.left = x(); r.top = y(); r.right = x() + w(); r.bottom = y() + h(); return r; }
	SIZE size() { SIZE s; s.cx = w(); s.cy = h(); return s; }

	// Make the size n pixels smaller around the edges
	void inside(int n = 1) { addx(n); addy(n); addw(-(2 * n)); addh(-(2 * n)); }

	// Determine if this size holds any pixels
	bool is() { return w() > 0 && h() > 0; }

	// Convert between screen and client window coordinates
	void screen(HWND window = NULL);
	void client(HWND window = NULL);
};

// Hold a device context with information about how to put it away
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

// Area item
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

// Handles
class handletop {
public:

	// Program handles
	HINSTANCE instance; // Program instance handle
	CRITICAL_SECTION section; // Critical section for the job object

	// Windows
	HWND window; // The main window on the screen
	HWND tasks, status, errors; // Child window edit controls
	HWND clear, task, start, stop, reset; // Child window buttons

	// Menus, icons, fonts and brushes
	HMENU menutaskbar, menutools;
	HICON iconbig, iconsmall;
	HFONT font, arial;
	brushitem white, black, blue, lightblue, yellow, lightyellow, green, lightgreen, red, lightred, middle;
};

// Drawing tools
class drawtop {
public:

	// The list of area items, the area item the mouse pressed, and pointers to the area items
	areaitem *all, *pressed;
	areaitem open, help, pause, remove, back, forward, stop, refresh, expand, get, address, enter, copy, bar, corner;

	// Child window control sizes
	sizeitem edit, button, tree, list;

	// Sizes in the client area used when painting
	sizeitem status;

	// Where the size area would be if the window were very small
	sizeitem sizemin;

	// The point in the pressed area where the mouse started dragging
	sizeitem stick;

	// New
	drawtop() {

		// Default values
		all = pressed = NULL;

		// Link area items into a list
		all          = &open;
		open.next    = &help;
		help.next    = &pause;
		pause.next   = &remove;
		remove.next  = &back;
		back.next    = &forward;
		forward.next = &stop;
		stop.next    = &refresh;
		refresh.next = &expand;
		expand.next  = &get;
		get.next     = &address;
		address.next = &enter;
		enter.next   = &copy;
		copy.next    = &bar;
		bar.next     = &corner;
		corner.next  = NULL;
	}
};

// Program data
class datatop {
public:

};

// Current state
class statetop {
public:

	// State
	boolean taskbar; // true when the window is hidden and icon is in the taskbar notification area
	int pop; // How many popup boxes and menus the program has put above the window
};
