
// Handles
class handletop {
public:

	// Program instance handle
	HINSTANCE instance;

	// Windows
	HWND window; // Main window
	HWND list, tabs, edit; // Child window controls

	// Menus
	HMENU restoremenu, toolsmenu; // Menus

	// Mouse pointers
	HCURSOR arrow, hand, horizontal, vertical, diagonal;

	// Fonts
	HFONT font, underline, arial;

	// Icons
	HICON reddrop, reddropbig, yellowdrop, yellowdropbig, greendrop, greendropbig, bluedrop, bluedropbig; // Application icons
	HICON fileicon; // Torrent file icon
	HICON toolsicon, pauseicon, removeicon, removeicongray; // Button icons

	// Colors
	brushitem blue, lightblue, yellow, lightyellow, green, lightgreen, red, lightred;
	brushitem face, shadow, background, ink, select; // Shell brushes
	brushitem middle; // Mixed color brush
};

// Areas and sizes in the main window client area
class areatop {
public:

	// The list of area items, the area item the mouse pressed, and pointers to the area items
	areaitem *all, *pressed;
	areaitem tools, pause, remove, bar, corner;

	// Child window control sizes
	sizeitem list, tabs, edit;

	// Sizes in the client area used when painting
	sizeitem status;

	// Where the size area would be if the window were very small
	sizeitem sizemin;

	// The point in the pressed area where the mouse started dragging
	sizeitem stick;

	// New
	areatop() {

		// Default values
		all = pressed = NULL;

		// Link area items into a list
		all         = &tools;
		tools.next  = &pause;
		pause.next  = &remove;
		remove.next = &bar;
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
	string state; // State title at top of window
	string status; // Status bar text
};
