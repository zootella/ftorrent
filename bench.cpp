
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Run a snippet of test code
void Test() {

	TemporaryFile file;

	file.Open();
	file.Add((BYTE *)"hello", 5);
	file.Add((BYTE *)" you", 4);




}




bool TemporaryFile::Open() {

	// Choose a new random path
	path = make(PathRunningFolder(), L"\\", TextGuid(), L".dnld.db");

	// Open the file, overwriting if necessary
	file = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!file || file == INVALID_HANDLE_VALUE) return false;

	// Record the time when this happened
	tickcreated = GetTickCount();
	return true;
}

bool TemporaryFile::Add(BYTE *b, int n) {

	// Write our data to the file
	DWORD written = 0;
	int result = WriteFile(file, b, n, &written, NULL);
	if (!result || written != n) return false;

	// Update our record of the file's total size, and when this happened
	size += n;
	tickwritten = GetTickCount();
	return true;
}




/*

//TODO update project to link to wininet, confirm this doesn't make the exe bigger

// Given a http request handle and a header identifier, reads the number value of the header as i and returns true
bool HeaderNumberRead(HINTERNET request, DWORD header, int *i) {

	DWORD d, size;
	size = sizeof(d);
	if (!HttpQueryInfo(request, header | HTTP_QUERY_FLAG_NUMBER, &d, &size, NULL)) return false;
	*i = d;
	return true;
}





#define DOWNLOAD_BAY 4 * 1024 // 4 KB download bay


// make an object that holds a temporary file that closes and deletes on exit, unless you call file.Keep()
// also, it keeps track of how large it is, how much has been written to it


bool Download(CString url, CString *path) {

	//TODO do you need to replace %20 in the url with spaces and stuff like that?

	// Initialize wininet
	if (!App.internet) App.internet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0); // Do once the first time this runs
	if (!App.internet) return false;

	// Request the URL
	HINTERNET request = InternetOpenUrl(App.internet, url, NULL, 0, INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (!request) return false;

	// Read the response HTTP headers
	int statuscode, contentlength;
	if (!HeaderNumberRead(request, HTTP_QUERY_STATUS_CODE,    &statuscode))    return false;
	if (!HeaderNumberRead(request, HTTP_QUERY_CONTENT_LENGTH, &contentlength)) return false;

	//TODO if status code is bad, or content length is 0 or too big, return false
	//TODO you need to call InternetCloseHandle(request);

	BYTE bay[DOWNLOAD_BAY];

	DownloadFile file;
	if (!file.Open()) return false;

	DWORD downloaded;

	while (true) {

		// Download the data
		downloaded = 0;
		int result = InternetReadFile(request, memory, DOWNLOAD_BAY, &downloaded);
		if (!result) return false; // Error
		if (!downloaded) break; // Done

		// Add it to the file
		if (!file.Add(bay, downloaded)) return false;

		//TODO put in time limit and size limit
		//TODO put in appplication list of downloads and cancel
	}

	file.keep = true;
	*path = file.path;
	return true;
}


*/
















