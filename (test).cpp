
// Include statements
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>
#include "resource.h"
#include "program.h"
#include "class.h"
#include "function.h"

// Global objects
extern handleitem Handle;

// Run a snippet of test code
void Test() {


	string s;
	s = numerals(0);
	s = numerals(-1);
	s = numerals(2);
	s = numerals(526);



	OutputDebugString(L"hello");
}
