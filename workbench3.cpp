
// Include libtorrent
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/alert_types.hpp"

// Include platform
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>

// Include program
#include "resource.h"
#include "define.h"
#include "object.h"
#include "library.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;




//TODO add
//factor all add paths in for open, link, and restore session to branch together
//show the user messages while you can prove he's there
//get hashes as early as possible and gracefully deal with duplicates
//deal with a first add pops a message and a second add arrives



// The user clicked Tools, Open
void CommandOpen() {

	// Show the file open box to have the user choose a torrent file
	CString path = DialogOpen();
	if (isblank(path)) return; // Canceled the file open dialog

	// With a path, what remains is the same as if the user double-clicked the torrent to open it
	CommandPath(path);
}

// The user clicked to open the torrent file at path
void CommandPath(read path) {

	// Read the infohash and name from the torrent file on the disk
	libtorrent::big_number hash;
	CString name;
	if (!LookPath(path, &hash, &name)) {
		Message(MB_ICONSTOP | MB_OK, L"Unable to read the infohash. This torrent file may be corrupted. Check how you saved or downloaded it, and try again.");
		return;
	}

	// If the ask option is checked, ask the user where to download the torrent
	CString folder;
	if (Data.ask) folder = DialogBrowse(L"Choose a folder to download '" + name + L"'.");
	else          folder = Data.folder;

	// Make sure we can write in the folder
	if (!CheckFolder(folder)) {
		Message(MB_ICONSTOP | MB_OK, L"Unable to save files to the folder at '" + folder + "'. Check the path and try again.");
		return;
	}

	// What remains is the same as restoring torrents from the last session
	EnterPath(hash, path, folder); // Now we can't show messages to the user anymore
}

void EnterPath(libtorrent::big_number, read path, read folder) {

	log(L"enter ", path);

	AddTorrent(Data.folder, path, NULL, NULL, NULL, NULL);


}


//user clicks tools, add, TODO clicks magnet link

void CommandAdd() {

	Dialog(L"DIALOG_ADD", DialogAdd);
}

void EnterLink(read link) {

	log(L"enter ", link);

	AddTorrent(Data.folder, NULL, hash, name, tracker, NULL);
}








//program loads stuff from last time

void EnterSave(libtorrent::big_number hash) {


	//compose paths to

	CString meta = PathTorrentMeta(hash);
	CString stor = PathTorrentStore(hash);
	CString optn = PathTorrentOption(hash);



	libtorrent::entry d;
	if (LoadEntry(PathOption(), d)) { // Loaded




	if (true) { // Torrent file on the disk


	} else { // No torrent file


	}

	AddTorrent(folder, torrent, hash, name, tracker, store);



}






void AddTorrent(read folder, read torrent, read hash, read name, read tracker, read store) {

	//TODO look for duplicates before you get here
	//TODO don't show a error messge in the case of a duplicate, rather, select that item in the list

	// add it to libtorrent
	torrentitem t = AddTorrentLibrary(folder, torrent, hash, name, tracker, store);
	if (!t.handle.is_valid()) return;

	// copy it into the program's list
	Data.torrents.push_back(t);

	// add it to the list view
	ListAdd(
		Handle.list,
		5,
		(LPARAM)t.Hash(),
		t.ComposeStatusIcon(),
		t.ComposeStatus(),
		t.ComposeNameIcon(),
		t.ComposeName(),
		t.ComposeSize(),
		t.ComposeHash(),
		t.ComposePath(),
		L"");
}


