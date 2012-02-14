
#include "include.h" // Include headers and definitions
extern app App; // Access global object



// Create and open a new temporary file to fill with data
bool TemporaryFile::Open() {

	// Choose a new random path
	path = make(PathRunningFolder(), L"\\", TextGuid(), L".temporary.db");

	// Open the file, overwriting if necessary
	file = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!file || file == INVALID_HANDLE_VALUE) return false;

	// Record the time when this happened
	tickcreated = GetTickCount();
	return true;
}

// Add n bytes at b to the end of our open file
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

// Close and rename the file
bool TemporaryFile::Keep() {


	if (file) {
		CloseHandle(file);
		file = NULL;
	}

	keep = true;

	CString source = path;
	path = make(PathRunningFolder(), L"\\", TextGuid(), L".download.db");

	if (!MoveFile(source, path)) return false;
	return true;



}





// Given a http request handle and a header identifier, reads the number value of the header as i and returns true
bool HeaderNumberRead(HINTERNET request, DWORD header, int *i) {

	DWORD d, size;
	size = sizeof(d);
	if (!HttpQueryInfo(request, header | HTTP_QUERY_FLAG_NUMBER, &d, &size, NULL)) return false;
	*i = d;
	return true;
}





#define DOWNLOAD_BAY_SIZE          1 * 1024 // 1 KB download bay, in bytes
#define DOWNLOAD_SIZE_LIMIT 4 * 1024 * 1024 // 4 MB download size limit, in bytes
#define DOWNLOAD_TIME_LIMIT        4 * 1000 // 4 second download time limit, in milliseconds




bool Download(CString url) {

	//TODO do you need to replace %20 in the url with spaces and stuff like that?

	// Initialize wininet
	if (!App.internet) App.internet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0); // Do once the first time this runs
	if (!App.internet) return false;

	// Request the URL
	InternetHandle request;
	request.handle = InternetOpenUrl(App.internet, url, NULL, 0, INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (!request.handle) return false;

	// Read the response HTTP headers
	int statuscode, contentlength;
	if (!HeaderNumberRead(request.handle, HTTP_QUERY_STATUS_CODE,    &statuscode))    return false;
	if (!HeaderNumberRead(request.handle, HTTP_QUERY_CONTENT_LENGTH, &contentlength)) return false;

	//TODO if status code is bad, or content length is 0 or too big, return false

	// Create and open a new temporary file next to this running exe
	TemporaryFile file;
	if (!file.Open()) return false;

	// Download loop
	BYTE bay[DOWNLOAD_BAY_SIZE];
	DWORD downloaded;
	while (true) {

		// Enforce the time and size limits
		if (file.tickcreated + DOWNLOAD_TIME_LIMIT < GetTickCount()) return false;
		if (file.size > DOWNLOAD_SIZE_LIMIT) return false;

		// Download the data
		downloaded = 0;
		int result = InternetReadFile(request.handle, bay, DOWNLOAD_BAY_SIZE, &downloaded);
		if (!result) return false; // Error
		if (!downloaded) break; // Done when internet read file returns without downloading anything

		// Add it to the file
		if (!file.Add(bay, downloaded)) return false;
	}

	// Enforce the time and size limits
	if (file.tickcreated + DOWNLOAD_TIME_LIMIT < GetTickCount()) return false;
	if (file.size > DOWNLOAD_SIZE_LIMIT) return false;

	// File downloaded, keep it
	file.keep = true;
	return true;
}







// Run a snippet of test code
void Test() {

	Download(L"http://getbot.com/getbot.gif");




}












