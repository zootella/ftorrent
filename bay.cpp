
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

		//no, all you need to do for the workaround is never add with index 0, always add with 1+ index
		//now all you need to see is how to add a column between other columns, to add it where the user right clicked or in the order of the ones on the menu
		//when floating over an item in the columns menu of checkboxes, have status text tell the user what that column means

		ColumnAdd(App.window.files, L"Column A", 100, false);
		ColumnAdd(App.window.files, L"Column B", 110, false);
		ColumnAdd(App.window.files, L"Column C", 120, false);
		ColumnAdd(App.window.files, L"Column D", 140, false);

		App.cells.push_back(Cell(11, L"Column A", 0, -1, L"A 11"));
		App.cells.push_back(Cell(11, L"Column B", 0, -1, L"B 11"));
		App.cells.push_back(Cell(11, L"Column C", 0, -1, L"C 11"));
		App.cells.push_back(Cell(11, L"Column D", 0, -1, L"D 11"));

		CellShow(App.window.files, App.cells);

	} else if (stage == 2) { log(L"stage 2"); stage = 3;

		log(L"2before: found param 11 at row ", numerals(ListFind(App.window.files, 11)));
		ColumnRemove(App.window.files, L"Column B");
//		CellShow(App.window.files, App.cells);
		log(L"2after:  found param 11 at row ", numerals(ListFind(App.window.files, 11)));

	} else if (stage == 3) { log(L"stage 3"); stage = 4;

		log(L"3before: found param 11 at row ", numerals(ListFind(App.window.files, 11)));
		ColumnAddBefore(App.window.files, L"Column C", L"Column B", 110, false);
		//bug, adding back a causes the contents of b to disappear, and cellshow won't do it because our record shows no change necessary
//		CellShow(App.window.files, App.cells);
		log(L"3after:  found param 11 at row ", numerals(ListFind(App.window.files, 11)));

	} else if (stage == 4) { log(L"stage 4"); stage = 5;


	} else if (stage == 5) { log(L"stage 5"); stage = 6;

	}

	/*
	LogColumnFind(App.window.files, L"Column A");
	LogColumnFind(App.window.files, L"Column B");
	LogColumnFind(App.window.files, L"Column C");
	LogColumnFind(App.window.files, L"Column D");
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




}


// Edit the list view row to match the information in this torrent
void Torrent::Edit() {

	/*
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
		*/
}






Cell *Torrent::GetCell(read title) {

	if (cells.size() == 0) {

		read r = L"Status,Name,Size,Infohash,Location";

		std::vector<CString> titles = words(r, L",");

		for (int i = 0; i < (int)titles.size(); i++)
			cells.push_back(Cell(Hash(), titles[i]));
	}

	for (int i = 0; i < (int)cells.size(); i++) {
		if (cells[i].title == CString(title))
			return &(cells[i]);
	}
	return NULL;
}


void Torrent::Compose() {
	ComposeStatus();
	ComposeName();
	ComposeSize();
	ComposeInfohash();
	ComposeLocation();
}


void Torrent::ComposeStatus() {
	Cell *c = GetCell(L"Status");
	c->text = L"status text";
}

void Torrent::ComposeName() {
	Cell *c = GetCell(L"Name");
	c->text = widenStoC(handle.name());
}

void Torrent::ComposeSize() {
	Cell *c = GetCell(L"Size");

	sbig done = handle.status().total_done; // libtorrent::size_type and sbig are both __int64
	sbig size = handle.get_torrent_info().total_size();

	c->text = make(InsertCommas(numerals(done)), L"/", InsertCommas(numerals(size)), L" bytes");
}

void Torrent::ComposeInfohash() {
	Cell *c = GetCell(L"Infohash");
	c->text = base16(handle.info_hash()); // This torrent's infohash in base 16
}

void Torrent::ComposeLocation() {
	Cell *c = GetCell(L"Location");
	c->text = L"path text";
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
