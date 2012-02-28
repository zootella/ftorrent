
#include "include.h" // Include headers and definitions
extern app App; // Access global object








// A block of memory that frees itself when the object goes out of scope
class Memory {
public:

	void *block;
	Memory() { block = NULL; } // No memory block yet
	~Memory() { Free(); }      // Free any memory this object contains when it goes out of scope

	bool Allocate(int bytes) {   // Returns false on error
		if (block) return false; // Already have a block of memory
		block = malloc(bytes);
		if (!block) { log(L"malloc"); return false; }
		return true;
	}

	void Free() {
		if (!block) return; // No memory block to free
		free(block);
		block = NULL;       // Record that we don't have a memory block any more
	}
};

// URI encode the given text, decoding percent sequences like "%20" into the characters they represent
// Correctly decodes international text, the three UTF-8 bytes "%E4%B8%80" become L"\u4e00" the chinese character for one
// Decodes both "%20" and "+" into " " in case spaces got encoded into plusses, ok because "+" would have gotten encoded into "%2B"
// On error, returns the given text unchanged
CString PercentDecode(read r) {

	std::string estring = narrowRtoS(r); // Since r is percent encoded, it should only contain characters that take 1 byte and narrowing it won't change anything
	const char *e = estring.c_str();
	int esize = lengthp(e);              // Number of encoded characters
	int eindex = 0;                      // The index of the encoded character we're on

	Memory memory;
	if (!memory.Allocate(esize + 1)) return r; // Space for every character even if no "%xx" pairs get smaller, and a null terminator
	byte *d = (byte *)memory.block;            // Pointer to write decoded bytes in the memory block

	while (eindex < esize) { // Loop for each encoded character

		if (e[eindex] == '%') { // We're on a percent character, the start of something like "%20"

			if (eindex + 3 > esize) return r;    // Make sure the given text is long enough to have the whole "%20"
			byte pair1 = toupper(e[eindex + 1]); // Accept uppercase and lowercase letters
			byte pair2 = toupper(e[eindex + 2]);
			byte nibble1, nibble2, dbyte;

			if      (pair1 >= '0' && pair1 <= '9') nibble1 = pair1 - '0';      // '0'  0 0000 through '9'  9 1001
			else if (pair1 >= 'A' && pair1 <= 'F') nibble1 = pair1 - 'A' + 10; // 'A' 10 1010 through 'F' 15 1111
			else                                   return r;

			if      (pair2 >= '0' && pair2 <= '9') nibble2 = pair2 - '0';
			else if (pair2 >= 'A' && pair2 <= 'F') nibble2 = pair2 - 'A' + 10;
			else                                   return r;

			dbyte = nibble1 << 4; // Shift the 4 bytes it means into the high portion of the byte, like 1000----
			dbyte |= nibble2;     // Copy the 4 bits from the second character in the pair into the low portion of the byte, like ----1100, and use the bitwise or operator to assemble the entire byte, like 10001100
			
			*d = dbyte;  // Copy the decoded byte to the decoded memory
			d++;         // Move forward in the decoded memory past the 1 byte we just wrote
			eindex += 3; // Move forward in the encoded text past the 3 characters we just encoded

		} else if (e[eindex] == '+') { // We're on a plus

			*d = ' '; // It was a space that got encoded into a plus
			d++;
			eindex++;

		} else { // We're on an unreserved character like A or 7

			*d = e[eindex]; // Just copy it over
			d++;
			eindex++;
		}
	}

	*d = '\0';                              // Write a null terminator so we can look at the decoded memory as text
	return widenPtoC((char *)memory.block); // Convert the UTF-8 memory we composed into UTF-16, sets of 3 bytes will become 1 character
}

// URI encode the given text, replacing reserved characters with percent codes
// Encodes every UTF-8 byte of the text except A-Z a-z 0-9 and ~!*()' to work like encodeURIComponent() in javascript
// Encodes international characters, the chinese character for one L"\u4e00" becomes the three bytes it is in UTF-8 encoded "%E4%B8%80"
// Optionally encodes " " to "+" instead of "%20", which is ok when encoding a part of the URI after the "?"
CString PercentEncode(read r, bool plus) {

	std::string dstring = narrowRtoS(r); // Convert r in UTF-16, where every character takes 2 bytes, to dstring in UTF-8, where A takes 1 byte and hiragana letter no takes 3 bytes
	const char *d = dstring.c_str();
	int dbytes = lengthp(d);             // Number of bytes of UTF-8 text we have to encode
	char bay[MAX_PATH];                  // Bay to compose the text of one encoded byte like "%20"
	std::string e;                       // String for encoded text

	for (int dindex = 0; dindex < dbytes; dindex++) { // Loop for each byte in the decoded text

		if ((d[dindex] >= 'A' && d[dindex] <= 'Z') || // Unreserved character we don't have to encode, just copy it over
			(d[dindex] >= 'a' && d[dindex] <= 'z') ||
			(d[dindex] >= '0' && d[dindex] <= '9') ||
			d[dindex] == '~' || d[dindex] == '!' || d[dindex] == '*' || d[dindex] == '(' || d[dindex] == ')' || d[dindex] == '\'') {

			e += d[dindex];

		} else if (plus && d[dindex] == ' ') { // Space, with the option to encode " " into "+", otherwise else below will turn it into "%20"

			e += "+";

		} else { // Reserved character or other byte, percent encode it

			sprintf(bay, "%%%02X", (unsigned char)(d[dindex]));
			e += bay;
		}
	}

	return widenStoC(e);
}







void TestEncode(read r) {

	CString normal, encoded, decoded;

	normal = r;
	encoded = PercentEncode(normal, true);
	decoded = PercentDecode(encoded);

	int i = 7;
}





// Run a snippet of test code
void Test() {




	TestEncode(L"hello");
	TestEncode(L"+ +");
	TestEncode(L"hello you");
	TestEncode(L"\r\n");
	TestEncode(L"hello+you");
	TestEncode(L"\u4e00"); // should be %E4%B8%80
	TestEncode(L"a:b");

	Find f(L"C:\\Documents\\test", true);
	while (f.Result()) {

		TestEncode(f.info.cFileName);
	}





	/*
	CString before = L"http://www.amazon.co.jp/%E3%83%95%E3%82%A1%E3%82%A4%E3%83%8A%E3%83%AB%E3%83%95%E3%82%A1%E3%83%B3%E3%82%BF%E3%82%B8%E3%83%BCX-%E3%83%A1%E3%83%A2%E3%83%AA%E3%82%A2%E3%83%AB%E3%82%A2%E3%83%AB%E3%83%90%E3%83%A0/dp/4887870280/ref=sr_1_1?ie=UTF8&qid=1329936759&sr=8-1";
	CString after = UrlDecode(before);
	log(L"Decoded ", before, L" to ", after);
	*/
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







