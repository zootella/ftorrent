
#include "include.h" // Include headers and definitions
extern app App; // Access global object



bool PaintCustom(LPNMLVCUSTOMDRAW draw) {
	return false; // TODO Don't do any custom drawing yet
}







bool Torrent::CanOpen() { return false; }
void Torrent::UseOpen() {
	if (!CanOpen()) { log(L"cant open"); return; }
}

bool Torrent::CanOpenContainingFolder() { return false; }
void Torrent::UseOpenContainingFolder() {
	if (!CanOpenContainingFolder()) { log(L"cant open containing folder"); return; }
}

bool Torrent::CanCopyMagnetLink() { return false; }
void Torrent::UseCopyMagnetLink() {
	if (!CanCopyMagnetLink()) { log(L"cant copy magnet link"); return; }
}

bool Torrent::CanSaveTorrentAs() { return false; }
void Torrent::UseSaveTorrentAs() {
	if (!CanSaveTorrentAs()) { log(L"cant save torrent as"); return; }
}






// Start this torrent so it downloads and seeds
bool Torrent::CanStart() { return paused; } // Use our flag rather than anything in the libtorrent handle
void Torrent::UseStart() {
	if (!CanStart()) { log(L"cant start"); return; }

	paused = false;            // Set this torrent object's flag
	handle.auto_managed(true); // Let libtorrent start and stop this torrent given the others it has
	handle.resume();           // Let libtorrent take the torrent online
	Save();                    // Save the option file for this torrent, including the updated paused flag
}

// Pause this torrent so it doesn't use the internet or change
bool Torrent::CanPause() { return !CanStart(); } // Opposite of start
void Torrent::UsePause() {
	if (!CanPause()) { log(L"cant pause"); return; }

	paused = true;              // Our record of whether we're paused or not
	handle.auto_managed(false); // Prevent libtorrent from automatically starting the torrent
	handle.pause();             // Have libtorrent take the torrent offline
	Save();                     // Save the option file with the changed paused flag
}

// Remove this torrent from the program
bool Torrent::CanRemove() { return true; } // A torrent can always be removed or deleted
void Torrent::UseRemove() {
	if (!CanRemove()) { log(L"cant remove"); return; }

	RemoveTorrent(this, false);
}

// Remove this torrent from the program, and delete any files we downloaded for it
bool Torrent::CanDelete() { return true; }
void Torrent::UseDelete() {
	if (!CanDelete()) { log(L"cant delete"); return; }

	RemoveTorrent(this, true);
}








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
	cells.push_back(Cell(HashParameter(), title));
	return cells[n]; // Size before is index of the last one we just added
}








// Define factory default columns in list view controls
void DefaultColumns() {

	App.list.trackers.factory += L"";
	App.list.peers.factory    += L"";
	App.list.pieces.factory   += L"";
	App.list.files.factory    += L"";


	App.list.torrents.factory += L"show=true,right=false,width=100,title=state;";

	App.list.torrents.factory += L"show=true,right=false,width=150,title=Status;";
	App.list.torrents.factory += L"show=true,right=false,width=170,title=Name;";
	App.list.torrents.factory += L"show=true,right=true, width=200,title=Size;";
	App.list.torrents.factory += L"show=true,right=false,width=220,title=Infohash;";
	App.list.torrents.factory += L"show=true,right=false,width=150,title=Location;";

	App.list.torrents.factory += L"show=true,right=false,width=110,title=object paused;";
	App.list.torrents.factory += L"show=true,right=false,width=110,title=handle paused;";
	App.list.torrents.factory += L"show=true,right=false,width=110,title=is auto managed;";

}



void Torrent::Compose() {
	ComposeStatus();
	ComposeName();
	ComposeSize();
	ComposeInfohash();
	ComposeLocation();


	GetCell(L"object paused").text = paused ? L"true" : L"false";
	GetCell(L"handle paused").text = handle.is_paused() ? L"true" : L"false";
	GetCell(L"is auto managed").text = handle.is_auto_managed() ? L"true" : L"false";

	Cell &state = GetCell(L"state");

	libtorrent::torrent_status::state_t s = handle.status().state;
	if      (s == libtorrent::torrent_status::queued_for_checking)  GetCell(L"state").text = L"queued for checking";
	else if (s == libtorrent::torrent_status::checking_files)       GetCell(L"state").text = L"checking files";
	else if (s == libtorrent::torrent_status::downloading_metadata) GetCell(L"state").text = L"downloading metadata";
	else if (s == libtorrent::torrent_status::downloading)          GetCell(L"state").text = L"downloading";
	else if (s == libtorrent::torrent_status::finished)             GetCell(L"state").text = L"finished";
	else if (s == libtorrent::torrent_status::seeding)              GetCell(L"state").text = L"seeding";
	else if (s == libtorrent::torrent_status::allocating)           GetCell(L"state").text = L"allocating";
	else if (s == libtorrent::torrent_status::checking_resume_data) GetCell(L"state").text = L"checking resume data";



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
LPARAM Torrent::HashParameter() {
	return ClipHash(handle.info_hash());
}





// Save the folder, name, and trackers in this torrent to a file like "infohash.optn.db" next to the running exe
void Torrent::Save() {

	// Copy the torrent's set of trackers into a libtorrent bencoded list
	libtorrent::entry::list_type l;
	for (std::set<CString>::const_iterator i = trackers.begin(); i != trackers.end(); i++)
		l.push_back(narrowRtoS(*i));

	// Compose text for the torrent's paused state
	CString p = paused ? L"t" : L"f";

	// Make and fill the bencoded dictionary
	libtorrent::entry::dictionary_type d;
	d[narrowRtoS(L"paused")]   = narrowRtoS(p);
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

	// Load the paused state, folder path, and name into this torrent
	paused = widenStoC(d[narrowRtoS(L"paused")].string()) == CString(L"t");
	folder = widenStoC(d[narrowRtoS(L"folder")].string()); // Not found returns blank
	name = widenStoC(d[narrowRtoS(L"name")].string());

	// Load in the list of trackers
	trackers.clear();
	libtorrent::entry::list_type l = d[narrowRtoS(L"trackers")].list();
	for (libtorrent::entry::list_type::const_iterator i = l.begin(); i != l.end(); i++)
		trackers.insert(widenStoC(i->string()));
	return true;
}











