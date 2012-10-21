
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Today's day number, assuming 366 days in every year and 31 days in every month
int DayNumber() {

	SYSTEMTIME info;
	ZeroMemory(&info, sizeof(info));
	GetSystemTime(&info); // UTC, so midnight is in England, not where the program is running
	return (366 * info.wYear) + (31 * info.wMonth) + info.wDay; // Use 366 and 31 so tomorrow the function will always return a bigger number
}

// Run a snippet of test code
void Test() {


	log(numerals(DayNumber()));




}
