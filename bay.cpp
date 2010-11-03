
#include "include.h" // Include headers and definitions
extern app App; // Access global object




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
int ListFindColumn(HWND window, read title) {
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

void ListPrint(HWND window, std::list<Cell> c) {


}


//how about just a list of Cells, and keep p and title in the cell

void ListPrint(HWND window, Cell c) {

	// Find the column and row index where the cell is currently located in the list view control
	int column = ListFindColumn(window, c.title);
	if (column == -1) return; // Column not currently on display, don't add or edit this cell
	int row = ListFind(window, c.parameter);
	bool found = row != -1; // True if we found the row and can edit it, false not found so we should add the row
	if (row == -1) row = ListRows(window); // Not found, make a new row at the end

	// Make a list view item structure to edit the cell at those coordinates
	LVITEM info;
	ZeroMemory(&info, sizeof(info));
	info.iSubItem = column;
	info.iItem = row;

	// Parameter
	info.mask |= LVIF_PARAM;
	info.lParam = c.parameter;

	// Text
	info.mask |= LVIF_TEXT;
	info.pszText = (LPWSTR)(read)c.text;

	// If specified, an icon
	if (c.icon != -1) {
		info.mask |= LVIF_IMAGE;
		info.iImage = c.icon;
	}

	// Edit or add
	if (found) {
		if (!ListView_SetItem(window, &info)) { error(L"listview_setitem"); return; } // Edit the existing row
	} else {
		if (ListView_InsertItem(window, &info) == -1) { error(L"listview_insertitem"); return; } // Add a new row
	}
}


