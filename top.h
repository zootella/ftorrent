
// Handles
class handletop {
public:

	// Program instance handle
	HINSTANCE instance;

	// Windows
	HWND window; // Main window
	HWND list, tabs, edit; // Child window controls

	// Menus
	HMENU tray, menu; // Menus

	// Mouse pointers
	HCURSOR arrow, hand, horizontal, vertical, diagonal;

	// Fonts
	HFONT font, underline, arial;

	// Icons
	HICON red16, red32, yellow16, yellow32, green16, green32, blue16, blue32; // Application state icons
	HICON file; // Torrent file icon
	HICON tools, pause, remove, removegray; // Button icons

	// Colors
	brushitem white, black, blue, lightblue, yellow, lightyellow, green, lightgreen, red, lightred;
	brushitem face, shadow, background, ink, select; // Shell brushes
	brushitem line; // Mixed color brush
};

// Areas and sizes in the main window client area
class areatop {
public:

	// The list of area items, the area item the mouse pressed, and pointers to the area items
	areaitem *all, *pressed;
	areaitem tools, bar, corner;

	// Sizes in the client area
	sizeitem list, tabs, info; // Child window control sizes
	sizeitem title, status; // Sizes in the client area used when painting

	// Sizing positions
	sizeitem collapse; // Where the corner area would be if the window were very small
	sizeitem stick; // The point in the pressed area where the mouse started dragging

	// New
	areatop() {

		// Link area items into a list
		all         = &tools;
		tools.next  = &bar;
		bar.next    = &corner;
		corner.next = NULL;
	}
};

// Program data
class datatop {
public:

};

// Current state
class statetop {
public:

	boolean taskbar; // True when the window is hidden and icon is in the taskbar notification area
	int pop; // How many popup boxes and menus the program has put above the window
	boolean pause; // True when the Pause button is pressed
	string title; // State title at top of window
	string status; // Status bar text
};
