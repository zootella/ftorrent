
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

		App.cells2.push_back(Cell(22, L"Column A", 0, -1, L"22a"));
		App.cells2.push_back(Cell(22, L"Column B", 0, -1, L"22b"));
		App.cells2.push_back(Cell(22, L"Column C", 0, -1, L"22c"));
		App.cells2.push_back(Cell(22, L"Column D", 0, -1, L"22d"));

		CellShow(App.window.files, App.cells1);
		CellShow(App.window.files, App.cells2);

	} else if (stage == 2) { log(L"stage 2"); stage = 3;

		log(L"2before: found param 22 at row ", numerals(ListFind(App.window.files, 22)));
		ColumnRemove(App.window.files, L"Column B");
//		CellShow(App.window.files, App.cells);
		log(L"2after:  found param 22 at row ", numerals(ListFind(App.window.files, 22)));

	} else if (stage == 3) { log(L"stage 3"); stage = 4;

		log(L"3before: found param 11 at row ", numerals(ListFind(App.window.files, 11)));
		ColumnAddBefore(App.window.files, L"Column B", L"Column A", 110, true);
		//bug, adding back a causes the contents of b to disappear, and cellshow won't do it because our record shows no change necessary
//		CellShow(App.window.files, App.cells);
		log(L"3after:  found param 11 at row ", numerals(ListFind(App.window.files, 11)));

		//ok, now you've added back b, and it's blank
		//but it's ok, just confirm that your cell is blank function works, and then use that as part of the match

		//ok, here's what it's doing
		//when you add a column, it is of course blank
		//if you add the leftmost column, both it and the column to the right are blank
		//so, just find a quick way to look for blank cells

		log(L"22a ", CellText(App.window.files, L"Column A", 22) ? L"text" : L"blank");
		log(L"22b ", CellText(App.window.files, L"Column B", 22) ? L"text" : L"blank");
		log(L"22c ", CellText(App.window.files, L"Column C", 22) ? L"text" : L"blank");
		log(L"22d ", CellText(App.window.files, L"Column D", 22) ? L"text" : L"blank");

	} else if (stage == 4) { log(L"stage 4"); stage = 5;


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




}






// True if the cell in the list view control window in column title with row parameter p is blank
bool CellText(HWND window, read title, LPARAM p) {

	int row = ListFind(window, p);
	int column = ColumnFind(window, title);
	if (row == -1 || column == -1) return false; // Cell not found
	return CellTextIndex(window, column, row);
}

// True if the cell in the list view control window at column and row has text, false if its text is blank
bool CellTextIndex(HWND window, int column, int row) {

	WCHAR bay[MAX_PATH];
	ListView_GetItemText(window, column, row, bay, MAX_PATH);
	return is(bay);
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
	Cell *primary;
	std::vector<Cell *> secondary;

	log(L"test1: ", cells[0].text);

	Cell copy = cells[0];
	log(L"test2: ", copy.text);

	Cell *p3 = &copy;
	log(L"test3: ", p3->text);

	Cell *p4 = &(cells[0]);
	log(L"test4: ", p4->text);



	for (int i = 0; i < (int)cells.size(); i++) { // Loop through the given row of cells
		Cell *c = &(cells[i]);
		log(L"test5: ", c->text);
		int column = ColumnFind(window, c->title); // Find the column index cell c is in right now
		c->column = column;                        // Note the column index in the cell object

		if      (column == -1) c->Hidden();            // The column is hidden, clear the cell's record of what it has on the screen
		else if (column ==  0) primary = c;            // This cell is in column 0, point primary at it
		else                   secondary.push_back(c); // The column is a list view submitem, add it to our list of them
	}
	if (!primary) { log(L"no primary cell found"); return; }

	// Find the current row index for the given cells
	log(L"about to find ", numerals(primary->parameter), L" in window");
	int row = ListFind(window, primary->parameter);
	log(L"found it at row ", numerals(row));
	bool add = row == -1; // Not found, we'll add the primary cell in a new row at the end
	if (add) row = ListRows(window); // Choose the new last row index
	for (int i = 0; i < (int)cells.size(); i++) cells[i].row = row; // Note the row index in the cell object

	// Update the list view control
	CellShowDo(window, primary, add); // Add or edit the primary cell
	for (int i = 0; i < (int)secondary.size(); i++) // Edit the secondary cells
		CellShowDo(window, secondary[i], false);
}
void CellShowDo(HWND window, Cell *c, bool add) { // True to insert c in column 0 and a new row at the end

	LVITEM info; // Make a list view item structure to edit the cell at those coordinates
	ZeroMemory(&info, sizeof(info));
	info.iSubItem = c->column;
	info.iItem = c->row;

	if (c->column == 0) { //TODO, this shows that setting the subitem on parameters causes the add to fail
		                  //so then, how are you going to find a row by its parameter once you delete column 0?
		info.mask |= LVIF_PARAM; // Parameter
		info.lParam = c->parameter;

	}

	info.mask |= LVIF_TEXT; // Text
	info.pszText = (LPWSTR)(read)(c->text);

	if (c->icon != -1) { // If specified, an icon
		info.mask |= LVIF_IMAGE;
		info.iImage = c->icon;
	}

	if (add) {
		if (ListView_InsertItem(window, &info) == -1) error(L"listview_insertitem");
		c->Same(); // Record what the cell put on the screen

		log(L"added ", c->text);
	} else if (c->Different()) { // The cell's record of what's on the screen differs from current composed information
		if (!ListView_SetItem(window, &info)) error(L"listview_setitem");
		c->Same(); // Record what the cell put on the screen

		log(L"edited ", c->text);
	}
}



