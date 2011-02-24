
#include "include.h" // Include headers and definitions
extern app App; // Access global object



bool PaintCustom(LPNMLVCUSTOMDRAW draw) {
	return false; // TODO Don't do any custom drawing yet
}



Torrent *ListFindTorrent() {

	ListGet();

}






bool Torrent::CanStart() { return false; }
void Torrent::Start() {}

bool Torrent::CanStop() { return false; }
void Torrent::Stop() {}

bool Torrent::CanPause() { return false; }
void Torrent::Pause() {}

bool Torrent::CanResume() { return false; }
void Torrent::Resume() {}

bool Torrent::CanRemove() { return false; }
void Torrent::Remove() {}

bool Torrent::CanDelete() { return false; }
void Torrent::Delete() {}








// Edit the list view row to match the information in this torrent
void Torrent::Edit() {

	// Edit the row of cells in the list view control window
	Compose();
	CellShow(App.list.torrents.window, cells);
}




// Access the cell under the title column for this torrent
Cell &Torrent::GetCell(read title) {

	// Look for it
	int n = (int)cells.size(); // How many cells are in our list
	for (int i = 0; i < n; i++) {
		if (cells[i].title == CString(title))
			return cells[i]; // Found it
	}

	// Not found, make it
	cells.push_back(Cell(Hash(), title));
	return cells[n]; // Size before is index of the last one we just added
}


void Torrent::Compose() {
	ComposeStatus();
	ComposeName();
	ComposeSize();
	ComposeInfohash();
	ComposeLocation();
}

void Torrent::ComposeStatus() {
	Cell &c = GetCell(L"Status");
	c.text = L"status text";
}

void Torrent::ComposeName() {
	Cell &c = GetCell(L"Name");
	c.text = widenStoC(handle.name());
}

void Torrent::ComposeSize() {
	Cell &c = GetCell(L"Size");

	sbig done = handle.status().total_done; // libtorrent::size_type and sbig are both __int64
	sbig size = handle.get_torrent_info().total_size();

	c.text = make(InsertCommas(numerals(done)), L"/", InsertCommas(numerals(size)), L" bytes");
}

void Torrent::ComposeInfohash() {
	Cell &c = GetCell(L"Infohash");
	c.text = base16(handle.info_hash()); // This torrent's infohash in base 16
}

void Torrent::ComposeLocation() {
	Cell &c = GetCell(L"Location");
	c.text = L"path text";
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
	d[narrowRtoS(L"folder")]   = narrowRtoS(folder);
	d[narrowRtoS(L"name")]     = narrowRtoS(name);
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











