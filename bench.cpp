
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Run a snippet of test code
void Test() {


	Memory m;
	m.Allocate(5);
	memcpy(m.block, "hello", 5);
	m.Save(L"C:\\Documents\\saved.txt");

}

/*

// Given a http request handle and a header identifier, reads the number value of the header as i and returns true
bool HeaderNumberRead(HINTERNET request, DWORD header, int *i) {

	DWORD d, size;
	size = sizeof(d);
	if (!HttpQueryInfo(request, header | HTTP_QUERY_FLAG_NUMBER, &d, &size, NULL)) return false;
	*i = d;
	return true;
}

bool Download(CString url) {

	// Initialize wininet
	if (!App.internet) App.internet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0); // Do once the first time this runs
	if (!App.internet) return false;
	//add INTERNET_FLAG_NO_CACHE_WRITE to not mess up the ie cache

	// Request the URL
	HINTERNET request = InternetOpenUrl(App.internet, url, NULL, 0, INTERNET_FLAG_EXISTING_CONNECT, 0);
	if (!request) return false;

	// Read the response HTTP headers
	int statuscode, contentlength;
	if (!HeaderNumberRead(request, HTTP_QUERY_STATUS_CODE,    &statuscode))    return false;
	if (!HeaderNumberRead(request, HTTP_QUERY_CONTENT_LENGTH, &contentlength)) return false;

	//TODO if status code is bad, or content length is 0 or too big, return false

	//TODO what happens when it's a script that's going to dynamically return something, and you don't know the content length?
	//find a gulp way to do this also, like allocate 5mb or whatever and parse whatever you get as a torrent
	//in this way though, confirm that the second call returns nothing

	// Allocate a block of memory to hold the whole file
	LPBYTE memory = (LPBYTE)HeapAlloc(Handle.heap, 0, contentlength);
	if (!memory) return false;

	// Download the data
	DWORD downloaded = 0;
	int result = InternetReadFile(request, memory, contentlength, &downloaded);
	if (!result || contentlength != downloaded) return false;

	// Confirm that was all of it
	BYTE bay[MAX_PATH];
	DWORD beyond = 0;
	result = InternetReadFile(request, bay, MAX_PATH, &beyond);





	InternetCloseHandle(request);

}






*/













