
#include "include.h" // Include headers and definitions
extern app App; // Access global object








// The next visible column in list after title, blank if last or none more visible
CString ColumnNextVisible(std::vector<Column> list, read title) {
	CString found1, found2;
	for (int i = 0; i < (int)list.size(); i++) {
		if (isblank(found1)) { // looking for first column
			if (list[i].title == CString(title)) {
				found1 = list[i].title;
			}
		} else if (isblank(found2)) { // looking for second column
			if (list[i].show) {
				found2 = list[i].title;
			}
		}
	}
	return found2;
}







void onStartup() {

	//factory default columns written in the code as text
	CString s;
	s += L"show=true,right=false,width=50,title=A;";
	s += L"show=true,right=false,width=50,title=B;";
	s += L"show=true,right=false,width=50,title=C;";
	s += L"show=true,right=false,width=50,title=D;";
	s += L"show=true,right=false,width=50,title=E;";
	s += L"show=true,right=false,width=50,title=F;";
	s += L"show=true,right=false,width=50,title=G;";

	//copy text from optn.db or factory default into back
	std::vector<Column> back = ColumnTextToList(s);

	//load back into the control
	ColumnListToWindow(App.window.files, back);
}

void onAdd(HWND window, std::vector<Column> &list, read title) {

	//find where it is in the back list
	int a = ColumnFindList(list, title);
	if (a == -1) { log(L"title to add not found"); return; }

	//remember to change its visibility in the back list
	list[a].show = true;
	if (list[a].width < 32) list[a].width = 32; // adding a column with no width would be very confusing

	//from where it is in the back list, find the next visible one
	CString anchor;
	for (int i = a + 1; i < (int)back.size(); i++) {
		if (list[i].show) {
			anchor = list[i].title;
			break;
		}
	}
	int place = -1;
	if (is(anchor)) place = ColumnFindPlace(window, anchor); //and where the next visible one is in the window

	//add it
	if (place != -1)
		ColumnAddIndex(window, place, list[a].title, list[a].width, list[a].right);//before the next visible one in the back list
	else
		ColumnAdd(window, list[a].title, list[a].width, list[a].right);//no next visible, so just add it at the end
}

void onRemove(HWND window, std::vector<Column> &list, read title) {

	//simple remove

	//find the column in the background list
	int i = ColumnFindList(list, title);
	//set its show false
	if (i != -1) list[i].show = false;
	//remove it from the window
	ColumnRemove(window, title);








	// find the column we're going to remove
	std::vector<Column> show = ColumnWindowToList(window); //get the current columns in the window
	int place = ColumnFindList(show, title); //where the column to remove is in the window
	int index = ColumnFindList(list, title); //where the column is in the background list
	if (index == -1 || place == -1) { log(L"column to remove not found"); return; }

	// next visible column
	if (place + 1 < show.size()) {//if the one we're removing isn't last

		CString anchor = show[place + 1].title; //anchor is the next visible one
		int anchorplace = ColumnFindList(show, anchor);
		int anchorindex = ColumnFindList(list, anchor);

		//if the next visible one in the back list isn't the same as the next visible one in the window
		





	}




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









	ColumnRemoveIndex(window, place);

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


	} else if (stage == 2) {
		stage = 3;

		//confirm adding 0, 1, 2, always adds in that position, regardless of how the indices beneath are sorted
		ColumnAddIndex(App.window.files, 7, L"New", 80, false);




	}

	std::vector<Column> v = ColumnWindowToList(App.window.files);
	log(L"");
	for (int i = 0; i < (int)v.size(); i++)
		log(L"place", numerals(v[i].place), L" index", numerals(v[i].index), L" ", v[i].title, L" width", numerals(v[i].width), v[i].right ? L" right" : L"");


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












