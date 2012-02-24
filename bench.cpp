
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Run a snippet of test code
void Test() {

	CString before = L"http://www.amazon.co.jp/%E3%83%95%E3%82%A1%E3%82%A4%E3%83%8A%E3%83%AB%E3%83%95%E3%82%A1%E3%83%B3%E3%82%BF%E3%82%B8%E3%83%BCX-%E3%83%A1%E3%83%A2%E3%83%AA%E3%82%A2%E3%83%AB%E3%82%A2%E3%83%AB%E3%83%90%E3%83%A0/dp/4887870280/ref=sr_1_1?ie=UTF8&qid=1329936759&sr=8-1";
	CString after = UrlDecode(before);
	log(L"Decoded ", before, L" to ", after);
}




CString UrlEncode(read r) { return WebCanonicalizeUrl(r, true); }
CString UrlDecode(read r) { return WebCanonicalizeUrl(r, false); }

CString WebCanonicalizeUrl(read r, bool encode) {

	// seems to be only good for decoding
	// encoding is easy anyways
	// also, you need to decode + to space, but encode space to %20

	// Assemble flags for url encoding or decoding
	DWORD flags;
	if (encode) flags = 0;
	else        flags = ICU_DECODE | ICU_NO_ENCODE;

	// Try with a static bay
	WCHAR bay[MAX_PATH];
	DWORD characters = MAX_PATH;
	int result = InternetCanonicalizeUrl(r, bay, &characters, flags);
	int e = GetLastError();
	if (result) return bay; // The static buffer was large enough
	if (e != ERROR_INSUFFICIENT_BUFFER) { error(L"internetcanonicalizeurl bay"); return L""; }

	// Try again with a larger dynamic buffer
	CString s;
	WCHAR *buffer = s.GetBuffer(characters);
	if (!InternetCanonicalizeUrl(r, buffer, &characters, flags)) { error(L"internetcanonicalizeurl buffer"); return L""; }
	s.ReleaseBuffer();
	return s;
}



