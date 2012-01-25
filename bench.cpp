
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Run a snippet of test code
void Test() {



	CString folderitem = L"C:\\Documents\\test\\folder item";
	CString fileitem = L"C:\\Documents\\test\\file item.txt";
	CString missingitem = L"C:\\Documents\\test\\missing item";


	//a way to find if a folder or file is there or not



	Find f1(folderitem, false);
	if (f1.Found()) log(L"folder item found");
	else            log(L"folder item not found");

	Find f2(fileitem, false);
	if (f2.Found()) log(L"file item found");
	else            log(L"file item not found");

	Find f3(missingitem, false);
	if (f3.Found()) log(L"missing item found");
	else            log(L"missing item not found");







}
