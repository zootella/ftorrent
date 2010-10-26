
class app_icon {
public:

	HIMAGELIST list;                  // Handle the the program image list
	int        source[ICON_CAPACITY]; // Labels the source system index of each icon in the program list
	int        count;                 // The number of icons loaded into the program list

	int clear, ascending, descending;// Resource icons in the program image list
	//TODO add status icons here

	int file; // The icon a file with no extension gets in the shell

	CString ext;   // The most recently requested extension we looked up
	int     index; // Its looked up index
	CString type;  // Its looked up type text

	app_icon() {

		list = NULL;
		count = 0;
		clear = ascending = descending = -1; // Icon index -1 for not in list
		file = -1;
		index = -1;
	}
};

class app_brush {
public:

	Brush blue, lightblue, green, lightgreen, red, lightred, yellow, lightyellow, rednotice, yellownotice, greennotice; // Defined colors
	Brush face, shadow, background, ink, select; // Shell brushes
	Brush line; // Mixed color brush
};

class app_cursor {
public:
	HCURSOR arrow, hand, horizontal, vertical, diagonal;// Mouse pointers

	app_cursor() {
		arrow = hand = horizontal = vertical = diagonal = NULL;
	}
};

class app_menu {
public:

	HMENU tray, tools;// Menus
	app_menu() {
		tray = tools = NULL;
	}
};

class app_font {
public:

	HFONT normal, underline, arial;// Fonts
	app_font() {
		normal = underline = arial = NULL;
	}
};

class app_window {
public:

	HWND main, list, tabs, edit, tip;// Windows
	app_window() {
		main = list = tabs = edit = tip = NULL;
	}
};

class app {
public:

	app_icon icon;
	app_brush brush;
	app_cursor cursor;
	app_menu menu;
	app_font font;
	app_window window;

	HINSTANCE instance; // Program instance handle
	libtorrent::session *session;// Session in libtorrent

	app() {
		instance = NULL;
		session = NULL;

	}
};

// Areas and sizes in the main window client area
class areatop {
public:

	// The list of areas, the area the mouse pressed, and pointers to the areas
	Area *all, *pressed;
	Area tools, start, pause, stop, remove;
	Area bar, corner;

	// Sizes in the client area
	Size list, tabs, info; // Child window control sizes
	Size stage, status; // Sizes in the client area used when painting

	// Sizing positions
	Size collapse; // Where the corner area would be if the window were very small
	Size stick; // The point in the pressed area where the mouse started dragging

	// Sizes
	int height; // Pixel height of the default font

	// New
	areatop() {

		all         = &tools; // Link areas into a list
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

	std::vector<Torrent> torrents;

	CString command; // Command line arguments the system gave or main function


	CString folder; // Path to folder to save files
	bool associate; // True to associate torrent and magnet

	datatop() {
		associate = true;

	}

};

// Current program stage
class stageitem {
public:

	// Text and resources to describe this stage
	CString title;
	Size size; // How big the text will be painted in the title font
	HICON icon16, icon32;
	Brush ink, background;

	// New
	stageitem() {

		icon16 = icon32 = NULL;
	}
};

// List view control state
struct listtop {

	int max;       // The largest number of characters added into any list view item
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
	bool portable;

	statetop() {

		taskbar = NULL;
		pop = 0;
		stage = &start;
		exit = 0;
		expect = 0;
		restored = false;
		portable = false;
	}
};

