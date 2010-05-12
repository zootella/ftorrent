
// Handles
class handletop {
public:

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
	brushitem face, shadow, background, ink, select; // Shell brushes
	brushitem line; // Mixed color brush
};

// Icon
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
	string ext;
	int    index;
	string type;
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


// STATE LIST
struct statelisttop {

	int max;       // THE LARGEST NUMBER OF CHARACTERS ADDED INTO ANY LIST VIEW ITEM OR SUBITEM
	int sort;      // -1 NO SORT, OR 0+ COLUMN SORTED
	int direction; // 1 ASCENDING OR -1 DESCENDING

	// NEW
	statelisttop() {

		max = 0;
		sort = -1;
		direction = 1;
	}
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

	// Subobjects
	statelisttop list;

	// Window
	HICON taskbar; // The icon we're showing in the taskbar, NULL if no icon there now
	int pop; // How many popup boxes and menus the program has put above the window

	// Shown
	string status; // Status bar text

	string folder;


	stageitem *stage; // Current program stage shown on the screen
	stageitem start, downloading, paused, seeding, missing; // Available program stages with painting resources

	int longest; // The number of characters not including null terminator of the longst text in any list view cell or other control
};
