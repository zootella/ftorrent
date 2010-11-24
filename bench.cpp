
#include "include.h" // Include headers and definitions
extern app App; // Access global object






//TODO try adding and removing icons instead of setting them to and from blank
//TODO see if you still need the add and then remove the first column workaround

//new list view design
//let the control keep its own data, don't switch to owner held data
//keep a copy of all the data in the control yourself right behind it
//use that copy to tell when a cell goes out of date, you don't need to query the control for its text at all anymore
//plan for the future feature of the user hides and drags columns
//the key for a row is its param, look this up to lead to a row index number 0+
//the key for a columns is read title, look this up to lead to a column index number 0+









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



int stage = 1;




// Run a snippet of test code
void Test() {

	if (stage == 1) { log(L"stage 1"); stage = 2;

		//no, all you need to do for the workaround is never add with index 0, always add with 1+ index
		//now all you need to see is how to add a column between other columns, to add it where the user right clicked or in the order of the ones on the menu
		//when floating over an item in the columns menu of checkboxes, have status text tell the user what that column means

		ColumnAdd(App.window.files, L"Column A", 100, true);
		ColumnAdd(App.window.files, L"Column B", 110, true);
		ColumnAdd(App.window.files, L"Column C", 120, true);
		ColumnAdd(App.window.files, L"Column D", 140, true);

		App.cells1.push_back(Cell(11, L"Column A", 0, -1, L"11a"));
		App.cells1.push_back(Cell(11, L"Column B", 0, -1, L"11b"));
		App.cells1.push_back(Cell(11, L"Column C", 0, -1, L"11c"));
		App.cells1.push_back(Cell(11, L"Column D", 0, -1, L"11d"));
		App.cells1.push_back(Cell(11, L"Column E", 0, -1, L"11e"));
		App.cells1.push_back(Cell(11, L"Column F", 0, -1, L"11f"));

		App.cells2.push_back(Cell(22, L"Column A", 0, -1, L"22a"));
		App.cells2.push_back(Cell(22, L"Column B", 0, -1, L"22b"));
		App.cells2.push_back(Cell(22, L"Column C", 0, -1, L"22c"));
		App.cells2.push_back(Cell(22, L"Column D", 0, -1, L"22d"));
		App.cells2.push_back(Cell(22, L"Column E", 0, -1, L"22e"));
		App.cells2.push_back(Cell(22, L"Column F", 0, -1, L"22f"));

		CellShow(App.window.files, App.cells1);
		CellShow(App.window.files, App.cells2);

	} else if (stage == 2) { log(L"stage 2"); stage = 3;

		ColumnRemove(App.window.files, L"Column A");
		ColumnRemove(App.window.files, L"Column B");
		ColumnRemove(App.window.files, L"Column C");
		ColumnRemove(App.window.files, L"Column D");

	} else if (stage == 3) { log(L"stage 3"); stage = 4;

		ColumnAdd(App.window.files, L"Column E", 110, true);

	} else if (stage == 4) { log(L"stage 4"); stage = 5;

		CellShow(App.window.files, App.cells1);
		CellShow(App.window.files, App.cells2);

	} else if (stage == 5) { log(L"stage 5"); stage = 6;

	}





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


//	App.list.refresh = false; // We filled any blank cells caused by newly added columns


}







// True if the cell in the list view control window at column and row is blank, false if it has text
bool CellBlank(HWND window, int column, int row) {
	WCHAR bay[MAX_PATH];
	lstrcpy(bay, L"");//TODO put these everywhere
	ListView_GetItemText(window, column, row, bay, MAX_PATH);

	log(L"cell c", numerals(column), L" r", numerals(row), L" has text \"", CString(bay), L"\"");

	return isblank(bay);
}

// Given row and column coordinates, get the bounding size of the list view sub item cell
Size CellSize(HWND window, int row, int column) {
	RECT rectangle;
	if (!ListView_GetSubItemRect(window, row, column, LVIR_BOUNDS, &rectangle)) { error(L"listview_getsubitemrect"); return Size(); }
	return Size(rectangle);
}






// takes a row of cells

void CellShow(HWND window, std::vector<Cell> &cells) {

	// Find each cell's current column index, picking out the one with special index 0
	Cell *primary = NULL;                         // Point primary at the cell in column 0
	std::vector<Cell *> secondary;                // Fill secondary with pointers to all the other cells
	for (int i = 0; i < (int)cells.size(); i++) { // Loop through the given row of cells
		Cell *c = &(cells[i]);

		c->column = ColumnFind(window, c->title);         // Update the cell's record of the column index it's in right now
		if      (c->column == -1) c->Hidden();            // The column is hidden, clear the cell's record of what it has on the screen
		else if (c->column ==  0) primary = c;            // This cell is in column 0, point primary at it
		else                      secondary.push_back(c); // The column is a list view submitem, add it to our list of them
	}
	if (!primary) { log(L"no primary cell found"); return; }

	// Find the current row index for the given cells
	int row = ListFind(window, primary->parameter);
	bool add = row == -1; // Not found, we'll add the primary cell in a new row at the end
	if (add) row = ListRows(window); // Choose the new last row index
	for (int i = 0; i < (int)cells.size(); i++) cells[i].row = row; // Note the row index in the cell object

	// Update the list view control
	CellShowDo(window, primary, add); // Add or edit the primary cell
	for (int i = 0; i < (int)secondary.size(); i++) // Edit the secondary cells
		CellShowDo(window, secondary[i], false);
}
void CellShowDo(HWND window, Cell *c, bool add) { // True to insert c in column 0 and a new row at the end

	// Make a list view item structure to edit the cell at those coordinates
	LVITEM info;
	ZeroMemory(&info, sizeof(info));
	info.iSubItem = c->column;
	info.iItem = c->row;

	// If column 0, parameter
	if (c->column == 0) {
		info.mask |= LVIF_PARAM;
		info.lParam = c->parameter;
	}

	// Text
	info.mask |= LVIF_TEXT;
	info.pszText = (LPWSTR)(read)(c->text);

	// If specified, an icon
	if (c->icon != -1) {
		info.mask |= LVIF_IMAGE;
		info.iImage = c->icon;
	}

	// Add this cell in a new row in column 0
	if (add) {

		if (ListView_InsertItem(window, &info) == -1) error(L"listview_insertitem");
		c->Same();

	// If necessary, edit the contents of the existing cell
	} else if (c->Different() || App.list.refresh) {

		if (!ListView_SetItem(window, &info)) error(L"listview_setitem");
		c->Same(); // Record that now display matches data
	}
}





