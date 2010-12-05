
#include "include.h" // Include headers and definitions
extern app App; // Access global object









// Run a snippet of test code
void Test() {


	Torrent t;

	Cell &a = t.GetCell(L"a");

	a.text = L"a value 1";
	log(a.text);

	Cell &a2 = t.GetCell(L"a");
	log(a2.text);
	
	a2.text = L"a value 1 edtied";
	log(a.text);
	log(a2.text);

	/*
	Cell &b = t.GetCell(L"b");

	a.text = L"a value 1";

	Cell &a2 = t.GetCell(L"a");

	log(a.text);
	log(a2.text);
	*/





}













