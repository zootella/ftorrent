
#include "include.h" // Include headers and definitions
extern app App; // Access global object





void LogColumnFind(HWND window, read title) {

	int column = ColumnFind(window, title);
	log(L"found ", title, L" at ", numerals(column));
}


int stage = 1;


// Run a snippet of test code
void Test() {

	if (stage == 1) { log(L"stage 1"); stage = 2;

		//the necessary workaround
		/*
		ColumnAdd(App.window.files, 0, L"Workaround", 10, false);
		ColumnRemove(App.window.files, L"Workaround");
		*/
		//no, all you need to do for the workaround is never add with index 0, always add with 1+ index
		//now all you need to see is how to add a column between other columns, to add it where the user right clicked or in the order of the ones on the menu
		//when floating over an item in the columns menu of checkboxes, have status text tell the user what that column means

		ColumnAdd(App.window.files, 1, L"Column A", 100, false);
		ColumnAdd(App.window.files, 2, L"Column B", 100, false);
		ColumnAdd(App.window.files, 3, L"Column C", 100, false);

	} else if (stage == 2) { log(L"stage 2"); stage = 3;

		//0 places it first, but with the icon problem
		//1 places it second, keeping that index and shifting other indices forward a number

		ColumnAdd(App.window.files, 2, L"Column D", 100, false);

	} else if (stage == 3) { log(L"stage 3"); stage = 4;
	} else if (stage == 4) { log(L"stage 4"); stage = 5;
	} else if (stage == 5) { log(L"stage 5"); stage = 6;
	}

	log(L"find:");
	LogColumnFind(App.window.files, L"Column A");
	LogColumnFind(App.window.files, L"Column B");
	LogColumnFind(App.window.files, L"Column C");
	LogColumnFind(App.window.files, L"Column D");
	log(L"");




}










void StorePulse() {

	// Only do this once
	if (App.cycle.restored) return;
	App.cycle.restored = true;

	// Add all the torrents from last time the program ran
	std::set<hbig> hashes;
	Find f(PathRunningFolder());
	while (f.Result()) { // Loop for each file in the folder this exe is running in
		if (!f.Folder()) {
			CString s = f.info.cFileName;
			if (length(s) == 40 + length(L".optn.db") && trails(s, L".optn.db", Matching)) { // Look for "infohash.optn.db"

				hbig hash = ParseHash(clip(s, 0, 40));
				if (!hash.is_all_zeros()) hashes.insert(hash); // Only collect unique nonzero hashes
			}
		}
	}
	for (std::set<hbig>::const_iterator i = hashes.begin(); i != hashes.end(); i++) AddStore(*i);

	// Add the torrent or magnet the system launched this program with
	CString s = App.cycle.command;
	if (starts(s, L"\"")) { // Parse the command like ["C:\Folder\file.torrent" /more /arguments]
		s = after(s, L"\"");
		if (has(s, L"\"")) {
			s = before(s, L"\"");
			if (starts(s, L"magnet:", Matching)) AddMagnet(s, false); // Look for magnet first because link text might also end torrent
			else if (trails(s, L".torrent", Matching)) AddTorrent(s, false);
		}
	}
}




void ListPulse() {



	for (int i = 0; i < (int)App.torrents.size(); i++)
		App.torrents[i].Edit();




}


// Edit the list view row to match the information in this torrent
void Torrent::Edit() {

	// Update the cells that have different text
	ListEdit(
		App.window.list,
		5,
		(LPARAM)Hash(),
		ComposeStatus().icon,
		ComposeStatus().text,
		ComposeName().icon,
		ComposeName().text,
		ComposeSize().text,
		ComposeInfohash().text,
		ComposeLocation().text,
		L"");
}



std::vector<Cell> Torrent::Compose() {

	std::vector<Cell> c;
	c.push_back(ComposeStatus());
	c.push_back(ComposeName());
	c.push_back(ComposeSize());
	c.push_back(ComposeInfohash());
	c.push_back(ComposeLocation());
	return c;
}



Cell Torrent::ComposeStatus() {
	return Cell(Hash(), L"Status", 0, App.icon.ascending, L"status text");
}


Cell Torrent::ComposeName() {
	Cell c(Hash(), L"Name");
	c.text = widenStoC(handle.name());
	c.icon = App.icon.descending;
	return c;
}

Cell Torrent::ComposeSize() {
	Cell c(Hash(), L"Size");

	sbig done = handle.status().total_done; // libtorrent::size_type and sbig are both __int64
	sbig size = handle.get_torrent_info().total_size();

	c.text = make(InsertCommas(numerals(done)), L"/", InsertCommas(numerals(size)), L" bytes");
	c.sort = size;
	return c;
}

// This torrent's infohash in base 16
Cell Torrent::ComposeInfohash() {
	Cell c(Hash(), L"Infohash");
	c.text = base16(handle.info_hash());
	return c;
}

Cell Torrent::ComposeLocation() {
	Cell c(Hash(), L"Location");
	c.text = L"path text";
	return c;
}







// The first 4 bytes of the infohash in a DWORD
DWORD Torrent::Hash() {
	return HashStart(handle.info_hash());
}




// Save the folder, name, and trackers in this torrent to a file like "infohash.optn.db" next to the running exe
void Torrent::Save() {

	// Copy the torrent's set of trackers into a libtorrent bencoded list
	libtorrent::entry::list_type l;
	for (std::set<CString>::const_iterator i = trackers.begin(); i != trackers.end(); i++)
		l.push_back(narrowRtoS(*i));

	// Make and fill the bencoded dictionary
	libtorrent::entry::dictionary_type d;
	d[narrowRtoS(L"folder")] = narrowRtoS(folder);
	d[narrowRtoS(L"name")] = narrowRtoS(name);
	d[narrowRtoS(L"trackers")] = l;

	// Save it to disk
	SaveEntry(PathTorrentOption(handle.info_hash()), d);
}

// Given an infohash, load the folder, name, and trackers from "infohash.optn.db" to this torrent
bool Torrent::Load(hbig hash) {

	// Look for a file named with the given hash, and read the bencoded data inside
	libtorrent::entry d;
	if (!LoadEntry(PathTorrentOption(hash), d)) return false;

	// Load the folder path and name into this torrent
	folder = widenStoC(d[narrowRtoS(L"folder")].string()); // Not found returns blank
	name = widenStoC(d[narrowRtoS(L"name")].string());

	// Load in the list of trackers
	trackers.clear();
	libtorrent::entry::list_type l = d[narrowRtoS(L"trackers")].list();
	for (libtorrent::entry::list_type::const_iterator i = l.begin(); i != l.end(); i++)
		trackers.insert(widenStoC(i->string()));
	return true;
}









// Takes a row number
// Looks up the parameter stored in that row
// Returns the parameter
LPARAM ListGet(HWND window, int row) {

	// Read the parameter at the given row number
	LVITEM info;
	ZeroMemory(&info, sizeof(info));
	info.iItem = row;        // Look at item in the given row number
	info.mask  = LVIF_PARAM; // Get the parameter
	int result;
	result = ListView_GetItem(window, &info);
	if (!result) { error(L"listview_getitem"); return 0; }

	// Return the extracted parameter
	return info.lParam;
}

// Gets the parameter of the row that has the keyboard mark
// Returns the parameter, or 0 if there is no keyboard mark
LPARAM ListMark(HWND window) {

	// Get the row that has the mark
	int row = ListView_GetSelectionMark(window);
	if (row == -1) return 0; // No mark

	// Return the parameter of the row
	return ListGet(window, row);
}

// Gets the parameter of the row beneath the mouse
// Returns the parameter, or 0 if the mouse is not over a row
LPARAM ListMouse(HWND window) {

	// Get the list view row beneath the client coordinate
	LVHITTESTINFO info;
	ZeroMemory(&info, sizeof(info));
    info.pt = MouseClient(window).Point();
	int row;
	row = ListView_HitTest(window, &info);
	if (row == -1) return 0; // The mouse is not over a row

	// Return the parameter of the row
	return ListGet(window, row);
}

// Takes row and column coordinates
// Gets the bounding size of the list view sub item cell
// Returns the size
Size ListCell(HWND window, int row, int column) {

	// Get the rectangle of the list view sub item and return the size
	Size size;
	RECT rectangle;
	if (!ListView_GetSubItemRect(window, row, column, LVIR_BOUNDS, &rectangle)) { error(L"listview_getsubitemrect"); return size; }
	size.Set(rectangle);
	return size;
}

// Counts how many rows are in the list view control
int ListRows(HWND window) {

	// Count the rows
	int rows = ListView_GetItemCount(window);
	if (rows < 0) { error(L"listview_getitemcount"); return 0; }
	return rows;
}

// Counts how many selected rows are in the list view control
int ListSelectedRows(HWND window) {

	// Count the selected rows
	int rows = ListView_GetSelectedCount(window);
	if (rows < 0) { error(L"listview_getselectedcount"); return 0; }
	return rows;
}

// Takes a row number
// Determines if that row is selected
// Returns true or false
bool ListSelected(HWND window, int row) {

	// Determine if the row is selected
	if (ListView_GetItemState(window, row, LVIS_SELECTED)) return true;
	else return false;
}

// Selects all the rows in the list view control
void ListSelectAll(HWND window) {

	// Loop down the rows, selecting each one
	int row, rows;
	rows = ListRows(window);
	for (row = 0; row < rows; row++) ListView_SetItemState(window, row, LVIS_SELECTED, LVIS_SELECTED);
}

// Scrolls to the bottom of the list view control
void ListScroll(HWND window) {

	// Count the number of rows in the list view control
	int rows = ListRows(window);
	if (rows) {

		// Scroll to make the row completely visible
		if (!ListView_EnsureVisible(window, rows - 1, false)) error(L"listview_ensurevisible");
	}
}

// Rakes a row index number
// Removes the row
void ListRemove(HWND window, int row) {

	// Remove the row from the list view control
	if (!ListView_DeleteItem(window, row)) error(L"listview_deleteitem");
}

// Remove all the rows in the list view control
void ListRemoveAll(HWND window) {

	// Remove all the rows in the list view control
	if (!ListView_DeleteAllItems(window)) error(L"listview_deleteallitems");
}

// Takes the number of rows that are going to be added
// Turns list view painting off and informs the control of the new size to hold
void ListAddStart(HWND window, int rows) {

	if (rows > 1) {                                             // Suspend drawing if more than 1 row is going to be added
		SendMessage(window, WM_SETREDRAW, false, 0);            // Prevent changes in the list view window from being drawn
		ListView_SetItemCount(window, ListRows(window) + rows); // Tell the list view control how many rows it will untimately contain
	}
}

// Takes the number of rows that were added
// Turns list view painting back on and paints the window
void ListAddDone(HWND window, int rows) {

	if (rows > 1) {                                 // Resume drawing if it was suspended
		SendMessage(window, WM_SETREDRAW, true, 0); // Allow changes in the list view window to be drawn
		InvalidateRect(window, NULL, true);         // Invalidate the list view control and have it erased before painted
	}
}

// Takes a parameter, icons and text
// Adds a new row to the list view control
void ListAdd(HWND window, int columns, LPARAM p, int icon1, read r1, int icon2, read r2, read r3, read r4, read r5, read r6) {

	// Record the maximum text length
	App.list.max = Greatest(App.list.max, length(r1), length(r2), length(r3), length(r4), length(r5), length(r6));

	// Row and column position, the number of rows is the index of the next row
	LVITEM column1, column2, column3, column4, column5, column6;
	ZeroMemory(&column1, sizeof(column1));
	ZeroMemory(&column2, sizeof(column2));
	ZeroMemory(&column3, sizeof(column3));
	ZeroMemory(&column4, sizeof(column4));
	ZeroMemory(&column5, sizeof(column5));
	ZeroMemory(&column6, sizeof(column6));
	column1.iItem = ListRows(window);
	column1.iSubItem = 0;
	column2.iSubItem = 1;
	column3.iSubItem = 2;
	column4.iSubItem = 3;
	column5.iSubItem = 4;
	column6.iSubItem = 5;

	// Masks for text and the parameter
	column1.mask = LVIF_TEXT | LVIF_PARAM;
	column2.mask = column3.mask = column4.mask = column5.mask = column6.mask = LVIF_TEXT;

	// Text
	column1.pszText = (LPWSTR)r1;
	column2.pszText = (LPWSTR)r2;
	column3.pszText = (LPWSTR)r3;
	column4.pszText = (LPWSTR)r4;
	column5.pszText = (LPWSTR)r5;
	column6.pszText = (LPWSTR)r6;

	// Icons
	if (icon1 != -1) { column1.mask = column1.mask | LVIF_IMAGE; column1.iImage = icon1; }
	if (icon2 != -1) { column2.mask = column2.mask | LVIF_IMAGE; column2.iImage = icon2; }

	// Parameter
	column1.lParam = p;

	// Insert the item and set the row
	int row = ListView_InsertItem(window, &column1);
	if (row == -1) { error(L"listview_insertitem"); return; }
	column2.iItem = column3.iItem = column4.iItem = column5.iItem = column6.iItem = row;

	// Set the subitems
	if (columns >= 2 && !ListView_SetItem(window, &column2)) { error(L"listview_setitem"); return; }
	if (columns >= 3 && !ListView_SetItem(window, &column3)) { error(L"listview_setitem"); return; }
	if (columns >= 4 && !ListView_SetItem(window, &column4)) { error(L"listview_setitem"); return; }
	if (columns >= 5 && !ListView_SetItem(window, &column5)) { error(L"listview_setitem"); return; }
	if (columns >= 6 && !ListView_SetItem(window, &column6)) { error(L"listview_setitem"); return; }
}

// Takes a parameter, icons and text
// Edits the row in the list view control
void ListEdit(HWND window, int columns, LPARAM p, int icon1, read r1, int icon2, read r2, read r3, read r4, read r5, read r6) {

	// Record the maximum text length
	App.list.max = Greatest(App.list.max, length(r1), length(r2), length(r3), length(r4), length(r5), length(r6));

	// Find the row that has the given parameter
	int row = ListFind(window, p);

	// Row and column position
	LVITEM column1, column2, column3, column4, column5, column6;
	ZeroMemory(&column1, sizeof(column1));
	ZeroMemory(&column2, sizeof(column2));
	ZeroMemory(&column3, sizeof(column3));
	ZeroMemory(&column4, sizeof(column4));
	ZeroMemory(&column5, sizeof(column5));
	ZeroMemory(&column6, sizeof(column6));
	column1.iItem = column2.iItem = column3.iItem = column4.iItem = column5.iItem = column6.iItem = row;
	column1.iSubItem = 0;
	column2.iSubItem = 1;
	column3.iSubItem = 2;
	column4.iSubItem = 3;
	column5.iSubItem = 4;
	column6.iSubItem = 5;

	// Masks for text and icons
	column1.mask = column2.mask = LVIF_TEXT | LVIF_IMAGE;
	column3.mask = column4.mask = column5.mask = column6.mask = LVIF_TEXT;

	// Text
	column1.pszText = (LPWSTR)r1;
	column2.pszText = (LPWSTR)r2;
	column3.pszText = (LPWSTR)r3;
	column4.pszText = (LPWSTR)r4;
	column5.pszText = (LPWSTR)r5;
	column6.pszText = (LPWSTR)r6;

	// Icons
	column1.iImage = icon1;
	column2.iImage = icon2;

	// Set the columns that have different text
	if (columns >= 1 && !same(ListText(window, row, 0), r1) && !ListView_SetItem(window, &column1)) error(L"listview_setitem");
	if (columns >= 2 && !same(ListText(window, row, 1), r2) && !ListView_SetItem(window, &column2)) error(L"listview_setitem");
	if (columns >= 3 && !same(ListText(window, row, 2), r3) && !ListView_SetItem(window, &column3)) error(L"listview_setitem");
	if (columns >= 4 && !same(ListText(window, row, 3), r4) && !ListView_SetItem(window, &column4)) error(L"listview_setitem");
	if (columns >= 5 && !same(ListText(window, row, 4), r5) && !ListView_SetItem(window, &column5)) error(L"listview_setitem");
	if (columns >= 6 && !same(ListText(window, row, 5), r6) && !ListView_SetItem(window, &column6)) error(L"listview_setitem");
}

// Takes a parameter
// Finds the row that has it in the list view control
// Returns the row number
int ListFind(HWND window, LPARAM p) {

	// Find the row that has p as its parameter, this may be slow
	LVFINDINFO info;
	ZeroMemory(&info, sizeof(info));
	info.flags  = LVFI_PARAM;                       // Search based on the parameter
	info.lParam = p;                                // Here is the parameter to find
	int row = ListView_FindItem(window, -1, &info); // -1 to start from the beginning
	if (row == -1) error(L"listview_finditem");     // Not found
	return row;                                     // Return the row number
}

// Takes row and column numbers
// Gets the text from the list view subitem at that location
// Returns the text
CString ListText(HWND window, int row, int column) {

	// Open a string
	CString s;
	int     n = App.list.max + 1; // Add 1 character to have space to write the wide null terminator
	LPWSTR  w = s.GetBuffer(n);

	//TODO do this without needing app list max
	// Copy in the characters
	ListView_GetItemText(window, row, column, w, n); // Writes null terminator
	s.ReleaseBuffer();
	return s;
}







	/*
	// Add the first column, which won't be able to show its icon on the right
	ColumnAdd(window, false, L"", 0);

	// Remove the first column so all the remaining columns can show their icons on the right
	ColumnRemove(window, 0);
	*/

//TODO try adding and removing icons instead of setting them to and from blank
//TODO see if you still need the add and then remove the first column workaround

//new list view design
//let the control keep its own data, don't switch to owner held data
//keep a copy of all the data in the control yourself right behind it
//use that copy to tell when a cell goes out of date, you don't need to query the control for its text at all anymore
//plan for the future feature of the user hides and drags columns
//the key for a row is its param, look this up to lead to a row index number 0+
//the key for a columns is read title, look this up to lead to a column index number 0+






// Get the window handle of the column headers inside a list view control
HWND ListHeader(HWND window) {
	HWND header = ListView_GetHeader(window);
	if (!header) { error(L"listview_getheader"); return NULL; }
	return header;
}




//todo
//have a map of CString to Cell in Torrent, use that to monitor and update what's onscreen
//then you don't need text from cell anymore, comment it out

//make teh files list in the tab below
//set it up to load Fruit items thta have different properties
//test adding and removing columns and rows this way
//right click remove fruit, test adds another group of them


//have this take two cells, what's there and what's current, and then update the display and the cells if they are different
//code Cell::Same() for this purpose
//have compose functions take a pointer to a cell, not return one, so data isn't copied as much

void ListPrint(HWND window, Cell c, bool force) {

	int column = ColumnFind(window, c.title); // Find the column and row index where the cell is currently located in the list view control
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
