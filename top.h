
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
	HICON blue16, blue32, green16, green32, red16, red32, yellow16, yellow32; // Application state icons
	HICON ascending, descending; // List column icons
	HICON pausedim, pausehot, pause; // Button icons
	HICON removedim, removehot, remove;
	HICON startdim, starthot, start;
	HICON stopdim, stophot, stop;
	HICON toolshot, tools;

	// Colors
	brushitem blue, lightblue, green, lightgreen, red, lightred, yellow, lightyellow;
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
	sizeitem title, status; // Sizes in the client area used when painting

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

// Current state
enum stagekind {

	// Different possible stages for the whole program to be in
	StageEmpty,       // Blue "start" empty of torrents at the start
	StageDownloading, // Blue "downloading" getting data
	StagePaused,      // Yellow "paused" all paused
	StageSeeding,     // Green "seeding" uploading to peers
	StageMissing      // Red "missing" there are pieces in a torrent that no peer has 
};
class statetop {
public:

	// Window
	boolean taskbar; // True when the window is hidden and icon is in the taskbar notification area
	int pop; // How many popup boxes and menus the program has put above the window

	// Shown
	stagekind stage; // Current program stage shown on the screen
	string status; // Status bar text

	string title;
	string folder;
};
