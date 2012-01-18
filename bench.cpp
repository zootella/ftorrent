
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Run a snippet of test code
void Test() {

}





Torrent *ListGetTorrent(int row) {

	LPARAM p = ListGet(App.list.torrents.window, row);
	if (!p) { log(L"listgettorrent parameter not found"); return NULL; }

	Torrent *t = FindTorrentParameter(p);
	if (!t) { log(L"listgettorrent torrent not found"); return NULL; }
	return t;
}
