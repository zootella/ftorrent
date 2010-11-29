
#include "include.h" // Include headers and definitions
extern app App; // Access global object





//TODO make right click menu on headers Customize... and modeless dialog box with checkboxes for each column



//On startup
//compute places
//show defaults

//On add
//use places to figure out index to put it, don't compute or change anything, though

//On reorder, there's no event to respond to

//On remove
//compute places list for visible ones, leaving other places the same
//remove the column



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







CString ColumnSave(HWND window) {

	CString s;
	int columns = ColumnCount(window);
	for (int i = 0; i < columns; i++) {

		s += make(L"view=show,");
		s += make(L"align=", ColumnIndexRight(window, i) ? L"right," : L"left,");
		s += make(L"width=", numerals(ColumnIndexWidth(window, i)), L",");
		s += make(L"title=", ColumnIndexTitle(window, i), L";");
	}
	return s;
}

void ColumnLoad(HWND window, read r) {

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

			if      (name == L"view")  show  = (value == L"show");
			else if (name == L"align") right = (value == L"right");
			else if (name == L"width") width = number(value);
			else if (name == L"title") title = value;
		}

		if (show && width > -1 && is(title))
			ColumnAdd(window, title, width, right);
	}
}



int stage = 1;




// Run a snippet of test code
void Test() {


	CString s;
	s += L"view=show,align=left,width=110,title=Column A;";
	s += L"view=show,align=left,width=110,title=Column B;";
	s += L"view=show,align=left,width=110,title=Column C;";
	s += L"view=show,align=left,width=110,title=Column D;";
	s += L"view=show,align=left,width=110,title=Column E;";
	s += L"view=show,align=left,width=110,title=Column F;";
	s += L"view=show,align=left,width=110,title=Column G;";
	ColumnLoad(App.window.files, s);


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












