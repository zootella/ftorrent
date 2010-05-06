
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
	HICON clear;
	HICON ascending, descending; // List column icons

	// Colors
	brushitem face, shadow, background, ink, select; // Shell brushes
	brushitem line; // Mixed color brush
};

// Areas and sizes in the main window client area
class areatop {
public:

	// The list of area items, the area item the mouse pressed, and pointers to the area items
	areaitem *all, *pressed;
	areaitem tools, start, pause, stop, remove;
	areaitem bar, corner;

	// Sizes in the client area
	sizeitem list, tabs, info; // Child window control sizes
	sizeitem stage, status; // Sizes in the client area used when painting

	// Sizing positions
	sizeitem collapse; // Where the corner area would be if the window were very small
	sizeitem stick; // The point in the pressed area where the mouse started dragging

	// Sizes
	int height; // Pixel height of the default font

	// New
	areatop() {

		// Link area items into a list
		all         = &tools;
		tools.next  = &start;
		start.next  = &pause;
		pause.next  = &stop;
		stop.next   = &remove;
		remove.next = &bar;
		bar.next    = &corner;
		corner.next = NULL;
	}
};

// Program data
class datatop {
public:

};

// Current program stage
class stageitem {
public:

	// Text and resources to describe this stage
	string title;
	sizeitem size; // How big the text will be painted in the title font
	HICON icon16, icon32;
	brushitem ink, background;
};
class statetop {
public:

	// Window
	HICON taskbar; // The icon we're showing in the taskbar, NULL if no icon there now
	int pop; // How many popup boxes and menus the program has put above the window

	// Shown
	string status; // Status bar text

	string folder;


	stageitem *stage; // Current program stage shown on the screen
	stageitem start, downloading, paused, seeding, missing; // Available program stages with painting resources
};
