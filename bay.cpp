
#include "include.h" // Include headers and definitions
extern app App; // Access global object





HWND ListCreate() {

	// Create the list view window
	DWORD style =
		WS_CHILD             | // Required for child windows
		LVS_REPORT           | // Details view with multiple columns
		LBS_EXTENDEDSEL      | // Allows multiple items to be selected
		LVS_SHOWSELALWAYS    | // Shows the selection even when the control doesn't have the focus
		LBS_NOINTEGRALHEIGHT | // Allows the size to be specified exactly without snap
		LVS_SHAREIMAGELISTS;   // Will not delete the image list when the control is destroyed
	HWND window = WindowCreate(WC_LISTVIEW, NULL, style, 0, App.window.main, (HMENU)WINDOW_LIST);

	// Use the program image list
	ListView_SetImageList(window, App.icon.list, LVSIL_SMALL);

	// Load extended list view styles, requires common control 4.70
	style = LVS_EX_LABELTIP  | // Unfold partially hidden labels in tooltips
		LVS_EX_FULLROWSELECT | // When an item is selected, highlight it and all its subitems
		LVS_EX_SUBITEMIMAGES | // Let subitems have icons
		LVS_EX_HEADERDRAGDROP; // Let the user drag columns into a different order
	ListView_SetExtendedListViewStyle(window, style);

	// Determine how wide the columns should be
	std::vector<int> widths;
	widths.push_back(-4);  // Status
	widths.push_back(200); // Name
	widths.push_back(-5);  // Size
	widths.push_back(100); // Infohash
	widths.push_back(-10); // Location
	widths = SizeColumns(widths);

	// Add the first column, which won't be able to show its icon on the right
	ListColumnAdd(window, false, L"", 0);

	// Add the columns
	ListColumnAdd(window, false, L"Status",   50);
	ListColumnAdd(window, false, L"Name",     50);
	ListColumnAdd(window, true,  L"Size",     50);
	ListColumnAdd(window, false, L"Infohash", 50);
	ListColumnAdd(window, false, L"Location", 50);

	// Remove the first column so all the remaining columns can show their icons on the right
	ListColumnDelete(window, 0);
	return window;
}

//TODO try adding and removing icons instead of setting them to and from blank
//TODO see if you still need the add and then remove the first column workaround



void ListColumnAdd(HWND window, bool right, read title, int width) {

	int format;
	if (right) format = LVCFMT_RIGHT;
	else format = LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT;

	int column = ListColumns(window);



	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask    = LVCF_FMT | LVCF_IMAGE | LVCF_TEXT | LVCF_WIDTH;
	info.fmt     = format;
	info.iImage  = App.icon.clear;
	info.pszText = (LPWSTR)title;
	info.cx      = width;
	if (ListView_InsertColumn(window, column, &info) == -1) error(L"listview_insertcolumn");
}

void ListColumnRemove(HWND window, read title) {

	int column = ListColumnFind(window, title);
	if (column == -1) return;
	ListColumnRemove(window, column);
}

void ListColumnRemove(HWND window, int column) {

	if (!ListView_DeleteColumn(window, 0)) error(L"listview_deletecolumn");
}




/*
ListColumnAdd

ListColumnRemove

*/









//new list view design
//let the control keep its own data, don't switch to owner held data
//keep a copy of all the data in the control yourself right behind it
//use that copy to tell when a cell goes out of date, you don't need to query the control for its text at all anymore
//plan for the future feature of the user hides and drags columns
//the key for a row is its param, look this up to lead to a row index number 0+
//the key for a columns is read title, look this up to lead to a column index number 0+


/*
// Takes information for a list view column
// Adds the column to the list view control
void ListColumnInsert(HWND window, int column, Column c) {

	// Add the column to the list view control
	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask    = LVCF_FMT | LVCF_IMAGE | LVCF_TEXT | LVCF_WIDTH;
	info.fmt     = c.format;
	info.iImage  = c.icon;
	info.pszText = (LPWSTR)c.text;
	info.cx      = c.width;
	if (ListView_InsertColumn(window, column, &info) == -1) error(L"listview_insertcolumn");
}
*/


// Get the window handle of the column headers inside a list view control
HWND ListHeader(HWND window) {
	HWND header = ListView_GetHeader(window);
	if (!header) { error(L"listview_getheader"); return NULL; }
	return header;
}

// Find out how many columns are in the given list view control
int ListColumns(HWND window) {

	HWND header = ListHeader(window); // Get the header
	if (!header) return -1;

	int n = Header_GetItemCount(header); // Find out how many columns there are
	if (n == -1) { error(L"header_getitemcount"); return -1; }
	return n;
}

// The name of column 0+, like "Status"
CString ListColumnGet(HWND window, int column) {

	WCHAR bay[MAX_PATH]; // Destination buffer

	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask       = LVCF_TEXT;
	info.pszText    = bay;
	info.cchTextMax = MAX_PATH;
	if (ListView_GetColumn(window, column, &info) == -1) { error(L"listview_getcolumn"); return L""; }
	return bay;
}

// Find the 0+ index of the column with the given title in the window list view control
int ListColumnFind(HWND window, read title) {
	if (isblank(title)) return -1; // Make sure title has text

	int columns = ListColumns(App.window.list);
	for (int i = 0; i < columns; i++)
		if (ListColumnGet(window, i) == CString(title))
			return i;
	return -1; // Not found
}







// Run a snippet of test code
void Test() {












}


//todo
//have a map of CString to Cell in Torrent, use that to monitor and update what's onscreen
//then you don't need text from cell anymore, comment it out

//make teh files list in the tab below
//set it up to load Fruit items thta have different properties
//test adding and removing columns and rows this way
//right click remove fruit, test adds another group of them


void ListPrint(HWND window, Cell c, bool force) {

	int column = ListColumnFind(window, c.title); // Find the column and row index where the cell is currently located in the list view control
	if (column == -1) return; // Column not currently on display, don't add or edit this cell
	int row = ListFind(window, c.parameter);
	bool found = row != -1; // True if we found the row and can edit it, false not found so we should add the row
	if (row == -1) row = ListRows(window); // Not found, make a new row at the end

	LVITEM info; // Make a list view item structure to edit the cell at those coordinates
	ZeroMemory(&info, sizeof(info));
	info.iSubItem = column;
	info.iItem = row;

	info.mask |= LVIF_PARAM; // Parameter
	info.lParam = c.parameter;

	info.mask |= LVIF_TEXT; // Text
	info.pszText = (LPWSTR)(read)c.text;

	if (c.icon != -1) { // If specified, an icon
		info.mask |= LVIF_IMAGE;
		info.iImage = c.icon;
	}

	if (App.list.max < length(c.text)) // Keep track of the longest text ever added to a cell
		App.list.max = length(c.text);

	if (found) { // Edit
		if (force || !same(ListText(window, row, column), c.text)) { // Only do something if this is a force edit or the text would change
			if (!ListView_SetItem(window, &info)) { error(L"listview_setitem"); return; } // Edit the cell in the existing row
		}
	} else { // Add
		if (ListView_InsertItem(window, &info) == -1) { error(L"listview_insertitem"); return; } // Add the cell in a new row
	}
}



void ListPrintAll(HWND window, std::vector<Cell> c) {
	for (int i = 0; i < (int)c.size(); i++)
		ListPrint(window, c[i], false);
}
