
#include "include.h" // Include headers and definitions
extern app App; // Access global object








//TODO try setting icon to -1 to remove it
//TODO confirm hitting the column works by its 11 22 33 id even if you've dragged it into a different order

// Takes a column index number and icon index number
// Shows the icon in the column header
void ColumnIcon(HWND window, int column, int icon) {

	// Set the icon in the column
	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask   = LVCF_IMAGE;
	info.iImage = icon;
	if (!ListView_SetColumn(window, column, &info)) error(L"listview_setcolumn");
}

// Takes a column index number or -1 to remove the selection from the column that has it
// Highlights the column as selected
void ColumnSelect(HWND window, int column) {

	//TODO
	/*
	ListView_SetSelectedColumn(
    HWND hwnd,
    int iCol
);
*/

	SendMessage(window, LVM_FIRST + 140, column, 0); // Send LVM_SETSELECTEDCOLUMN by offsetting by 140
	InvalidateRect(window, NULL, true);              // Invalidate the list view control and have it erased before painted
}



//add before the one with title name, or blank to add it at the end

void ColumnAdd(HWND window, int column, read title, int width, bool right) {

	//TODO never add with column 0 because then the text will be indented for the sort icon to be on the left, not the right
	//you don't have to add and then remove a column for the workaround, just add them all with index columns + 1

	int format;
	if (right) format = LVCFMT_RIGHT;
	else format = LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT;

	LVCOLUMN info;
	ZeroMemory(&info, sizeof(info));
	info.mask    = LVCF_FMT | LVCF_IMAGE | LVCF_TEXT | LVCF_WIDTH;
	info.fmt     = format;
	info.iImage  = App.icon.clear;
	info.pszText = (LPWSTR)title;
	info.cx      = width;

	int result = ListView_InsertColumn(window, column, &info);
	if (result == -1) { error(L"listview_insertcolumn"); return; }

	log(L"column ", numerals(column), L" added at ", numerals(result));

/*
	if (ListView_InsertColumn(window, column, &info) == -1) error(L"listview_insertcolumn");
	*/
}

void ColumnRemove(HWND window, read title) {

	int column = ColumnFind(window, title);
	if (column == -1) return;
	ColumnRemove(window, column);
}

void ColumnRemove(HWND window, int column) {

	if (!ListView_DeleteColumn(window, 0)) error(L"listview_deletecolumn");
}

// Find out how many columns are in the given list view control
int ColumnCount(HWND window) {

	HWND header = ListHeader(window); // Get the header
	if (!header) return -1;

	int n = Header_GetItemCount(header); // Find out how many columns there are
	if (n == -1) { error(L"header_getitemcount"); return -1; }
	return n;
}

// The title text of the column with the given index, like "Status"
CString ColumnTitle(HWND window, int column) {

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
int ColumnFind(HWND window, read title) {
	if (isblank(title)) return -1; // Make sure title has text

	int columns = ColumnCount(App.window.list);
	for (int i = 0; i < columns; i++)
		if (ColumnTitle(window, i) == CString(title))
			return i;
	return -1; // Not found
}



