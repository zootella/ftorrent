
// Include libtorrent
#include "libtorrent/create_torrent.hpp"

// Include platform
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>

// Include program
#include "resource.h"
#include "program.h"
#include "object.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;

// Run a snippet of test code
void Test() {





	mytest();


/*
	wrapper_session_settings info;
	ZeroMemory(&info, sizeof(info));

	int max_upload_bandwidth;
	int max_download_bandwidth;
	int listen_start_port;
	int listen_end_port;
	float seed_ratio_limit;
    float seed_time_ratio_limit;
    int seed_time_limit;
    int active_downloads_limit;
    int active_seeds_limit;
    int active_limit;
    int alert_mask;
    char *listen_interface;




	init();
*/
	//add a row to the list




}






//TODO actually have manual batch adds and removes instead of this design

/*

// Makes rows for items that need to be listed
void ListPulse() {

	// Find out how many torrent items ned to be listed
	torrentitem *b;
	int tolist = 0;

	b = Data.torrent;
	while (b) {

		if (!b->listed) tolist++;

	b = b->next; }

	// List them
	int icon;
	string status, size, type, kind, onpage;
	if (tolist) {

		// TELL THE LIST VIEW CONTROL HOW MANY YOU ARE GOING TO ADD
		ListAddStart(tolist);

		// LOOP THROUGH NOT LISTED ITEMS
		b = Data.bot;
		while (b) { if (!b->listed) {

			// MARK LISTED, COMPOSE DISPLAY, AND ADD ROW
			b->listed = true;
			b->Compose(&icon, &status, &size);
			ListAdd(6, (LPARAM)b, icon, status, b->u.icon, b->u.name, size, b->u.type, b->u.address, b->path);

		} b = b->next; }

		ListAddDone(tolist);


		// IF ALL THE ROWS WERE JUST ADDED, SORT THE LIST, OTHERWISE SCROLL TO THE BOTTOM
		if (ListRows() == tolist) ListSort(-2);
		else ListScroll();

	}
}

// Takes -1 to remove the sort icon and column selection, -2 to default sort, or the column to sort
// Sorts the column and sets the sort icon and column selection
void ListSort(int column) {

	// REVERSE THE DIRECTION OR SET IT ASCENDING
	if (column >= 0 && column == State.list.sort) State.list.direction *= -1;
	else                                          State.list.direction = 1;

	// CLEAR THE ICON AND COLUMN SELECTION
	if (column < 0 && State.list.sort >= 0) {

		ColumnIcon(State.list.sort, Draw.icon.clear);
		ColumnSelect(-1);

	// REVERSE THE ICON
	} else if (column >= 0 && column == State.list.sort) {

		if (State.list.direction == 1) ColumnIcon(column, Draw.icon.columnascending);
		else                           ColumnIcon(column, Draw.icon.columndescending);

	// MOVE THE ICON AND SET THE COLUMN SELECTION
	} else if (column >= 0) {

		if (State.list.sort >= 0) ColumnIcon(State.list.sort, Draw.icon.clear);
		ColumnIcon(column, Draw.icon.columnascending);
		ColumnSelect(column);
	}

	// SORT
	if (column != -1) {

		State.list.sort = column;
		if (!ListView_SortItems(Handle.list, ListCompare, 0)) error("listview_sortitems");
	}
}

// Takes the parameters of a pair of list rows
// Determines which should be listed first
// Returns negative if 1 is first, 0 tie, or positive if 2 is first
int CALLBACK ListCompare(LPARAM left, LPARAM right, LPARAM lparam) {

	// MAKE ORDER NEGATIVE TO PUT LEFT FIRST, 0 FOR TIE, OR POSITIVE TO PUT RIGHT FIRST
	int n1, n2, order;
	n1 = n2 = order = 0;


	// GET THE ITEM POINTERS OF THE TWO ROWS THAT NEED TO BE COMPARED
	torrentitem *b1, *b2;
	b1 = (torrentitem*)left;
	b2 = (torrentitem*)right;

	// STATUS AND ADDRESS
	switch (State.list.sort) {
	case -2:

		order = BotCompareStatus(b1, b2);
		if (order == 0) order = compare(b1->u.address, b2->u.address);

	// NAME
	break;
	case 1: {

		string name1, name2;
		if (is(b1->u.filename)) name1 = b1->u.filename; else name1 = b1->u.p7file + b1->u.p8ext;
		if (is(b2->u.filename)) name2 = b2->u.filename; else name2 = b2->u.p7file + b2->u.p8ext;
		order = compare(name1, name2);

	} break;
	case 0: order = BotCompareStatus(b1, b2);              break; // STATUS
	case 2: order = b1->Size() - b2->Size();               break; // SIZE
	case 3: order = compare(b1->u.type, b2->u.type);       break; // TYPE
	case 4: order = compare(b1->u.address, b2->u.address); break; // ADDRESS
	case 5: order = compare(b1->path, b2->path);           break; // SAVED TO
	}


	// RETURN THE ORDER
	return(State.list.direction * order);
}

*/