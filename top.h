
class app_window {
public:

	HINSTANCE instance; // Program instance handle
	HWND main, tabs, tip; // Window handles
	HWND torrent, trackers, peers, pieces, speed, log;

	app_window() {
		instance = NULL;
		main = tabs = tip = NULL;
		torrent = trackers = peers = pieces = speed = log = NULL;
	}
};

class app_icon {
public:

	HIMAGELIST list; // Handle the the program image list
	int source[ICON_CAPACITY]; // Labels the source system index of each icon in the program list
	int count; // The number of icons loaded into the program list

	CString ext; // The most recently requested extension we looked up
	int index; // Its looked up index
	CString type; // Its looked up type text

	int clear, ascending, descending; // Index of resource icons in the program image list
	int file; // The icon a file with no extension gets in the shell

	app_icon() {
		list = NULL; // We haven't made our list yet
		count = 0; // It doesn't have any icons yet
		index = -1; // We haven't done our first lookup yet
		clear = ascending = descending = -1; // Icon index -1 for not in list
		file = -1;
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

	HCURSOR arrow, hand, horizontal, vertical, diagonal; // Mouse pointers

	app_cursor() {
		arrow = hand = horizontal = vertical = diagonal = NULL;
	}
};

class app_menu {
public:

	HMENU tray, tools; // Menus

	app_menu() {
		tray = tools = NULL;
	}
};

class app_font {
public:

	HFONT normal, underline, arial; // Fonts
	int height; // Pixel height of the default font

	app_font() {
		normal = underline = arial = NULL;
		height = 0; // Not yet known
	}
};

class app_skin {
public:

	app_skin() {
	}
};

class app_area {
public:

	Area *all; // The list of areas
	Area *pressed; // The area the mouse pressed

	Area tools, start, pause, stop, remove; // Buttons
	Area bar, corner; // Sizing grips

	Size list, tabs, info; // Child window control sizes
	Size stage, status; // Sizes in the client area used when painting

	Size collapse; // Where the corner area would be if the window were very small
	Size stick; // The point in the pressed area where the mouse started dragging

	app_area() {
		all         = &tools; // Link areas into a list
		tools.next  = &start;
		start.next  = &pause;
		pause.next  = &stop;
		stop.next   = &remove;
		remove.next = &bar;
		bar.next    = &corner;
		corner.next = NULL;
	}
};

class app_list {
public:

	List torrents, files;
	bool blanks; // True when a new column has been added that has blank cells

	app_list() {
		blanks = false; // No new columns with blank cells at the start
	}
};

class app_cycle {
public:

	HICON taskbar; // The icon we're showing in the taskbar, NULL if no icon there now
	int pop; // How many popup boxes and menus the program has put above the window
	CString status; // Status bar text

	DWORD exit; // The tick count when the user exited the window and the program hid it, 0 program still running
	int expect; // How many torrent resume data messages we are still waiting for, 0 got them all or not in use yet

	bool restored; // True once torrents from the previous session are restored
	CString command; // Command line arguments the program was launched with
	bool portable; // True if we're running beside the portable marker

	app_cycle() {
		taskbar = NULL; // Nothing in the taskbar to start
		pop = 0; // No popup boxes at the start
		exit = 0; // The program starts out running
		expect = 0; // No torrent resume data messages expected yet
		restored = false; // Have not yet restored torrents from the previous session
		portable = false; // Have not seen the portable marker beside us yet
	}
};

class app_stage {
public:

	Stage start, downloading, paused, seeding, missing; // Available program stages with painting resources
	Stage *show; // Current program stage shown in the window

	app_stage() {
		show = &start;
	}
};

class app_option {
public:

	CString folder; // Path to folder to save files
	bool associate; // True to associate torrent and magnet

	app_option() {
		associate = true;
	}
};

class app {
public:

	app_window window; // Window handles
	app_icon icon; // Program image list and icons
	app_brush brush; // Color brushes
	app_cursor cursor; // Mouse pointers
	app_menu menu; // Menus
	app_font font; // Fonts

	app_skin skin; // Drawing resources for the currently loaded skin
	app_area area; // Areas and sizes in the main window client area
	app_list list; // Size and sort of the main list view control

	app_cycle cycle; // Start and close of the program
	app_stage stage; // Current global download stage
	app_option option; // Program options the user sets

	libtorrent::session *session; // Session in libtorrent
	std::vector<Torrent> torrents; // Torrents the program has in its list


	std::vector<Cell> cells1;
	std::vector<Cell> cells2;

	app() {
		session = NULL;
	}
};
