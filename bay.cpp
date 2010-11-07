
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

		ColumnAdd(App.window.files, L"Column A", 100, true);
		ColumnAdd(App.window.files, L"Column B", 80, true);
		ColumnAdd(App.window.files, L"Column C", 120, true);
		ColumnAdd(App.window.files, L"Column D", 140, true);

	} else if (stage == 2) { log(L"stage 2"); stage = 3;

		ColumnIcon(App.window.files, L"Column A", App.icon.ascending);

	} else if (stage == 3) { log(L"stage 3"); stage = 4;

		ColumnIcon(App.window.files, L"Column A", App.icon.clear);

	} else if (stage == 4) { log(L"stage 4"); stage = 5;

		ColumnIcon(App.window.files, L"Column A", App.icon.descending);

	} else if (stage == 5) { log(L"stage 5"); stage = 6;

		ColumnIcon(App.window.files, L"Column A", -1);
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
