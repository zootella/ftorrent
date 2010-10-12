
// Program icon image list
class icontop {
public:

	// Image list
	HIMAGELIST list;                  // Handle the the program image list
	int        source[ICON_CAPACITY]; // Labels the source system index of each icon in the program list
	int        count;                 // The number of icons loaded into the program list

	// Resource icons in the program image list
	int clear, ascending, descending;
	//TODO add status icons here

	// Shell icons in the program image list
	int file; // The icon a file with no extension gets in the shell

	// The most recently requested extension and its matching program index and type text
	CString ext;
	int     index;
	CString type;

	// New
	icontop() {

		list = NULL;
		count = 0;
		clear = ascending = descending = -1; // Icon index -1 for not in list
		file = -1;
		index = -1;
	}
};

// Window and drawing resource handles
class handletop {
public:

	// Inside
	icontop icon;

	// Program instance handle
	HINSTANCE instance;

	// Windows
	HWND window, list, tabs, edit, tip;

	// Menus
	HMENU tray, menu;

	// Mouse pointers
	HCURSOR arrow, hand, horizontal, vertical, diagonal;

	// Fonts
	HFONT font, underline, arial;

	// Colors
	brushitem blue, lightblue, green, lightgreen, red, lightred, yellow, lightyellow, rednotice, greennotice; // Defined colors
	brushitem face, shadow, background, ink, select; // Shell brushes
	brushitem line; // Mixed color brush

	// Session in libtorrent
	libtorrent::session *session;

	// New
	handletop() {

		instance = NULL;
		window = list = tabs = edit = tip = NULL;
		tray = menu = NULL;
		arrow = hand = horizontal = vertical = diagonal = NULL;
		font = underline = arial = NULL;
		session = NULL;
	}
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

		all         = &tools; // Link area items into a list
		tools.next  = &start;
		start.next  = &pause;
		pause.next  = &stop;
		stop.next   = &remove;
		remove.next = &bar;
		bar.next    = &corner;
		corner.next = NULL;

		height = 0; // Not yet known
	}
};

// Program data
class datatop {
public:

	std::vector<torrentitem> torrents;

	CString command; // Command line arguments the system gave or main function


	CString folder; // Path to folder to save files
	bool ask; // True to ask where to save each torrent

	datatop() {
		ask = false;

	}

};

// Current program stage
class stageitem {
public:

	// Text and resources to describe this stage
	CString title;
	sizeitem size; // How big the text will be painted in the title font
	HICON icon16, icon32;
	brushitem ink, background;

	// New
	stageitem() {

		icon16 = icon32 = NULL;
	}
};

// List view control state
struct listtop {

	int max;       // The largest number of characters added into any list view item or subitem
	int sort;      // -1 no sort, OR 0+ column sorted
	int direction; // 1 ascending or -1 descending

	// New
	listtop() {

		max = 0;
		sort = -1;
		direction = 1;
	}
};

// Current program state
class statetop {
public:

	// Contains
	listtop list;

	// Window
	HICON taskbar; // The icon we're showing in the taskbar, NULL if no icon there now
	int pop; // How many popup boxes and menus the program has put above the window

	// Shown
	CString status; // Status bar text

	CString folder;


	DWORD exit; // The tick count when the user exited the window and the program hid it, 0 program still running
	int expect; // How many torrent resume data messages we are still waiting for, 0 got them all or not in use yet


	stageitem *stage; // Current program stage shown on the screen
	stageitem start, downloading, paused, seeding, missing; // Available program stages with painting resources

	bool restored;
	CString command;

	statetop() {

		taskbar = NULL;
		pop = 0;
		stage = &start;
		exit = 0;
		expect = 0;
		restored = false;
	}
};
