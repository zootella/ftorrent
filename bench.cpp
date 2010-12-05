
#include "include.h" // Include headers and definitions
extern app App; // Access global object









// Run a snippet of test code
void Test() {


	Torrent t;

	Cell a = t.GetCell(L"a");
	

	Cell b = t.GetCell(L"b");

	a.text = L"a value 1";

	Cell a2 = t.GetCell(L"a");

	log(a.text);
	log(a2.text);





}













