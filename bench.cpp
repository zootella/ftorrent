
#include "include.h" // Include headers and definitions
extern app App; // Access global object





int ColumnFindList(std::vector<Column> list, read title) {
	for (int i = 0; i < (int)list.size(); i++)
		if (list[i].title == CString(title))
			return i;
	return -1;
}



/*

void onStartup() {

	//factory default columns written in the code as text
	CString s;
	s += L"view=show,align=left,width=110,title=Column A;";//TODO change to show=true, right=true
	s += L"view=show,align=left,width=110,title=Column B;";
	s += L"view=hide,align=left,width=110,title=Column C;";
	s += L"view=show,align=right,width=110,title=Column D;";
	s += L"view=show,align=left,width=80,title=Column E;";
	s += L"view=show,align=left,width=110,title=Column F;";
	s += L"view=show,align=left,width=110,title=Column G;";

	//copy text from optn.db or factory default into back
	std::vector<Column> back = ColumnTextToList(s);

	//load back into the control
	ColumnListToWindow(App.window.files, back);
}

void onAdd(HWND window, std::vector<Column> &back, read title) {

	//find where it is in the back list
	int i = 0;
	int add = -1;
	for (; i < (int)back.size(); i++) {

		if (back[i].title == CString(title)) {
			add = i;
			break;
		}
	}
	if (add == -1) { log(L"title to add not found"); return; }

	//remember to change its visibility in the back list
	back[add].show = true;

	//move to the next one to start searching for the next visible one in the back list
	i++;

	//from where it is in the back list, find the next visible one
	int before = -1;
	for (; i < (int)back.size(); i++) {

		if (back[i].show) {
			before = i;
			break;
		}
	}

	//add it before that one in the control
	if (before == -1)
		ColumnAdd(window, back[add].title, back[add].width, back[add].right);
	else
		ColumnAddBefore(window, back[add].title, back[before].title, back[add].width, back[add].right);
}

void onRemove(HWND window, std::vector<Column> &back, read title) {

	// find the column we're going to remove in the control
	int column = ColumnFind(window, title);
	if (column == -1) { log(L"column to remove not found"); return; }

	// find the column we're going to remove in the back list
	int i = 0;
	int remove = -1;
	for (; i < (int)back.size(); i++) {

		if (back[i].title == CString(title)) {
			remove = i;
			break;
		}
	}
	if (remove == -1) { log(L"title to remove not found"); return; }

	//change our record of its visibility in the back list
	back[remove].show = false;
	back[remove].width = ColumnWidthIndex(window, column);

	//find the name of the column after the one we're going to remove in the control
	CString after; //blank if the one we're going to remove is the last one

	//TODO, wait, the column indices don't run 0 through cols-1 in order on the control, do they?
	if (int c = 0; 

	if (column + 1 < ColumnCount(window)) {



	}









	ColumnRemoveIndex(window, column);


}


//On remove
//compute places list for visible ones, leaving other places the same
//remove the column


	//remember to change its visiblity in the back list



//what does the format for persisting column data look like?
//both factory settings, and a string for bencoding in optn.db
//you need to save column order, width, show or hide
//"t,0,110,Column A;t,25,130,Column B;"

//what does the function that computes places look like?





//simple solution to the sort problem
//no numbered weights
//in the background:
//ColA,ColB,ColC,ColD,ColE,ColF,ColG
//in the foreground
//ColB,ColF,ColD
//and then the user removes F
//in the background, move it to be before D
//and when you add one from the background, put it before the next visible one
//thsi works, qed

*/




std::vector<Column> ColumnTextToList(read r) {

	std::vector<Column> list;
	std::vector<CString> columns = words(r, L";");
	for (int c = 0; c < (int)columns.size(); c++) {

		bool show = false;
		bool right = false;
		int width = -1;
		CString title;

		std::vector<CString> parameters = words(columns[c], L",");
		for (int p = 0; p < (int)parameters.size(); p++) {

			CString name, value;
			split(parameters[p], L"=", &name, &value);
			name = trim(name, L" ");

			if      (name == L"show")  show  = (value == CString(L"true"));
			else if (name == L"right") right = (value == CString(L"true"));
			else if (name == L"width") width = number(value);
			else if (name == L"title") title = value;
		}

		if (width > -1 && is(title)) {

			Column o;
			o.show = show;
			o.right = right;
			o.width = width;
			o.title = title;
			list.push_back(o);
		}
	}
	return list;
}

void ColumnListToWindow(HWND window, std::vector<Column> list) {

	for (int i = 0; i < (int)list.size(); i++) {
		Column c = list[i];

		if (c.show && is(c.title) && c.width > -1) {
			ColumnAdd(window, c.title, c.width, c.right);
		}
	}
}

std::vector<Column> ColumnWindowToList(HWND window) {

	std::vector<Column> list;
	int columns = ColumnCount(window);
	if (columns > MAX_PATH) { log(L"too many columns"); return list; }

	int bay[MAX_PATH];
	ZeroMemory(&bay, sizeof(bay)); // Size of the whole array
	if (!ListView_GetColumnOrderArray(window, columns, bay)) { error(L"listview_getcolumnorderarray"); return list; }

	for (int i = 0; i < columns; i++) {

		Column c;
		c.order = i;
		c.index = bay[i];
		c.show = true;
		c.right = ColumnRightIndex(window, c.index);
		c.width = ColumnWidthIndex(window, c.index);
		c.title = ColumnTitleIndex(window, c.index);
		list.push_back(c);
	}
	return list;
}

CString ColumnListToText(std::vector<Column> list) {

	CString s;
	for (int i = 0; i < (int)list.size(); i++) {
		Column c = list[i];

		s += make(L"show=",  c.show  ? L"true," : L"false,");
		s += make(L"right=", c.right ? L"true," : L"false,");
		s += make(L"width=", numerals(c.width), L",");
		s += make(L"title=", c.title, L";");
	}
	return s;
}








int stage = 1;


//TODO when you drag the columns into a different order, but dont' add or remove any, their indices don't change
//and when you loop through the indexes, you get them in their original order
//you need to figure out their screen order to be able to put them back the next time in their sorted order




//TODO, next, confirm that when you add one, where does it go?

//maybe expand this to include column
//order
//index
//title
//width
//right






// Run a snippet of test code
void Test() {

	if (stage == 1) {
		stage = 2;

		CString s;
		s += L"show=true,right=false,width=50,title=A;";
		s += L"show=true,right=false,width=50,title=B;";
		s += L"show=true,right=false,width=50,title=C;";
		s += L"show=true,right=false,width=50,title=D;";
		s += L"show=true,right=false,width=50,title=E;";
		s += L"show=true,right=false,width=50,title=F;";
		s += L"show=true,right=false,width=50,title=G;";
		std::vector<Column> list = ColumnTextToList(s);
		ColumnListToWindow(App.window.files, list);

	} else if (stage == 2) {
		stage = 3;

		//confirm adding 0, 1, 2, always adds in that position, regardless of how the indices beneath are sorted
		ColumnAddIndex(App.window.files, 7, L"New", 80, false);




	}

	std::vector<Column> v = ColumnWindowToList(App.window.files);
	log(L"");
	for (int i = 0; i < (int)v.size(); i++)
		log(L"order", numerals(v[i].order), L" index", numerals(v[i].index), L" ", v[i].title, L" width", numerals(v[i].width), v[i].right ? L" right" : L"");


	/*
*/
/*
	if (stage == 1) { log(L"stage 1"); stage = 2;

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
	*/





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












