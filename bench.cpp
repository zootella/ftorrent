
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Run a snippet of test code
void Test() {


	CString s;
	s = ClipboardPasteAsciiText();
	s = ClipboardPasteUnicodeText();

}





// Close our access to the clipboard when this object goes out of scope
class Clipboard {
public:

	bool open; // True when this program has the clipboard open, and needs to close it
	Clipboard() { open = false; }
	~Clipboard() { Close(); }

	bool Open() {
		if (open) return true; // Already open
		if (!OpenClipboard(App.window.main)) { error(L"closeclipboard"); return false; }
		open = true; // Record we have the clipboard open
		return true; // Success
	}

	bool Close() {
		if (!open) return true; // Already closed
		if (!CloseClipboard()) { error(L"closeclipboard"); return false; }
		open = false; // Record we closed the clipboard
		return true; // Success
	}
};




void ClipboardCopy(read r) {

	//TODO
}
void ClipboardCopyAsciiText(read r) {

	std::string s = narrowRtoS(r);
	const char *c = s.c_str();

	int characters = lengthp(c) + 1;
	int bytes = characters * sizeof(char);

	HANDLE h = GlobalAlloc(GHND, bytes);
	char *p = (char *)GlobalLock(h);
	copyp(p, characters, c);// COPY THE BYTES OF THE TEXT INTO THE MEMORY BLOCK, THIS DOES INCLUDE AND DOES COPY THE NULL TERMINATOR AT THE END OF R
	GlobalUnlock(h);

	Clipboard clipboard;
	if (!clipboard.Open()) return;

	EmptyClipboard();
	SetClipboardData(CF_TEXT, h);// COPY THE DATA ONTO THE CLIPBOARD, THE MEMORY IS NO LONGER this program'S TO FREE
}
void ClipboardCopyUnicodeText(read r) {

	int characters = lengthr(r) + 1;
	int bytes = characters * sizeof(WCHAR);

	HANDLE h = GlobalAlloc(GHND, bytes);
	WCHAR *p = (WCHAR *)GlobalLock(h);
	copyr(p, characters, r);// COPY THE BYTES OF THE TEXT INTO THE MEMORY BLOCK, THIS DOES INCLUDE AND DOES COPY THE NULL TERMINATOR AT THE END OF R
	GlobalUnlock(h);

	Clipboard clipboard;
	if (!clipboard.Open()) return;

	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, h);// COPY THE DATA ONTO THE CLIPBOARD, THE MEMORY IS NO LONGER this program'S TO FREE
}

CString ClipboardPaste() {

	return L"";//TODO
}
CString ClipboardPasteAsciiText() {

	if (!IsClipboardFormatAvailable(CF_TEXT)) return L""; // Only do something if there is text on the clipboard

	Clipboard clipboard;
	if (!clipboard.Open()) return L"";

	HANDLE h = GetClipboardData(CF_TEXT);
	if (!h) return L"";

	char *p = (char *)GlobalLock(h); // Lock and unlock
	CString s = widenPtoC(p);
	GlobalUnlock(h);

	return s;
}
CString ClipboardPasteUnicodeText() {

	if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return L""; // Only do something if there is unicode text on the clipboard

	Clipboard clipboard;
	if (!clipboard.Open()) return L""; // Open the clipboard

	HANDLE h = GetClipboardData(CF_UNICODETEXT); // Get a handle to the unicode text data on the clipboard
	if (!h) return L"";

	WCHAR *w = (WCHAR *)GlobalLock(h); // Lock and unlock
	CString s = w;
	GlobalUnlock(h);

	return s;
}












// Gets the ascii text on the clipboard, blank if no text or any error
// Gets the unicode text on the clipboard, blank if no text or any error

// Copy the given unicode text to the clipboard


