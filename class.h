
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

// The program's global handle object
class handleitem {
public:

	// Program handles
	HINSTANCE instance; // Program instance handle
	CRITICAL_SECTION section; // Critical section for the job object

	// Windows and menus
	HWND window; // The main window on the screen
	HWND tasks, status, errors; // Child window edit controls
	HWND clear, task, start, stop, reset; // Child window buttons
	HMENU menu; // The Tools menu

	// Painting tools
	brushitem white, black, blue, lightblue, yellow, lightyellow, green, lightgreen, red, lightred, middle;
	HFONT font, arial;

	// State
	boolean taskbar; // true when the window is hidden and icon is in the taskbar notification area
};
