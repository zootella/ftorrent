
#include "include.h" // Include headers and definitions
extern app App; // Access global object











// Given row and column coordinates, get the bounding size of the list view sub item cell
Size CellSize(HWND window, int row, int column) {
	RECT rectangle;
	if (!ListView_GetSubItemRect(window, row, column, LVIR_BOUNDS, &rectangle)) { error(L"listview_getsubitemrect"); return Size(); }
	return Size(rectangle);
}




//TODO confirm you can remove column 0, adn then find the paramter of the second column which became primary


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



