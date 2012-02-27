
#include "include.h" // Include headers and definitions
extern app App; // Access global object


/*
// Takes a pointer to memory and the number of bytes there
// Describes the data using numbers and letters in base 16 encoding
// Returns a string
string tobase16(byte *p, DWORD bytes) {

	// Use 0-9 and A-F, 16 different characters, to describe the data
	text alphabet = _T("0123456789ABCDEF");

	// Make a string and access its buffer directly
	string s;
	int characters = bytes * 2;               // Each byte will become two characters
	TCHAR *sbuffer = s.GetBuffer(characters); // Make room for this number of bytes or wide characters, there is no null terminator
	characters = 0;                           // Now begin counting how many characters we actually copy into the string

	// Loop through the memory, encoding its bytes into pairs of letters and numbers
	for (DWORD i = 0; i < bytes; i++) { // When the index moves beyond the memory, we are done

		// Describe the 8 bits with two numerals or letters, and move i to the next byte
		sbuffer[characters++] = alphabet[*(p + i) >> 4]; // Shift right 8 bits to read just the first part 1001----
		sbuffer[characters++] = alphabet[*(p + i) & 15]; // Mask with 15 1111 to read just the second part ----1001
	}

	// Close the string of letters and numbers we generated from the data, and return it
	s.ReleaseBuffer(characters); // Tell the string we copied in this many characters, now it sets the byte or wide null terminator itself
	return s;
}

// Takes a pointer to memory, the number of bytes we can write there, and text to decode
// Decodes the numbers and letters into data using base 16 encoding
// Returns the number of bytes written, or pass null and 0 to get the size required
DWORD frombase16(byte *p, DWORD bytes, text t) {

	// Loop for each character in the text
	DWORD tlength = lstrlen(t); // How many characters there are
	TCHAR c;                    // The character we are on
	DWORD i = 0;                // The index in bytes
	byte code, b;               // Bytes to hold 4 bits and 8
	for (DWORD tindex = 0; tindex < tlength; tindex++) {

		// Get a character from the text, and convert it into its code
		c = toupper(t[tindex]);                             // Accept uppercase and lowercase letters
		if      (c >= '0' && c <= '9') code = c - '0';      // '0'  0 0000 through '9'  9 1001
		else if (c >= 'A' && c <= 'F') code = c - 'A' + 10; // 'A' 10 1010 through 'F' 15 1111
		else                           code = ~0;           // Skip this invalid character

		// The character is valid
		if (code != ~0) {

			// This is the first character in a pair
			if (tindex % 2 == 0) {

				// Shift the 4 bytes it means into the high portion of the byte, like 1000----
				b = code << 4;

			// This is the second character in a pair
			} else {

				// Copy the 4 bits from the second character in the pair into the low portion of the byte, like ----1100
				b |= code; // Use the bitwise or operator to assemble the entire byte, like 10001100

				// Copy the byte into the memory, and move to the next byte
				if (i < bytes) *(p + i) = b;
				i++;
			}
		}
	}

	// Say how many bytes the text decodes into
	return i;
}
*/






//test to see a unicode character from the disk get turned into 2 bytes, then encoded


CString PercentEncode(read r) {

	std::string s = narrowRtoS(r); // Convert r in UTF-16, where every character takes 2 bytes, to s in UTF-8, where A takes 1 byte and hiragana letter no takes 3 bytes
	const char *p = s.c_str();

	int bytes = lengthp(p);
	char bay[MAX_PATH];
	std::string done;

	for (int i = 0; i < bytes; i++) {
		unsigned char c = p[i];

		if ((c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') ||
			c == '~' || c == '!' || c == '*' || c == '(' || c == ')' || c == '\'') {

			done += c;

		} else {

			sprintf(bay, "%%%02X", c);
			done += bay;
		}
	}

	return widenStoC(done);
}


class Memory {
public:

	void *block;
	Memory() { block = NULL; }
	~Memory() { Free(); }

	bool Allocate(int bytes) {
		if (block) return false;
		block = malloc(bytes);
		if (!block) { log(L"malloc"); return false; }
		return true;
	}

	void Free() {
		if (!block) return;
		free(block);
		block = NULL;
	}
};




// returns original text on error
CString PercentDecode(read r) {

	//TODO have an option to replace plusses, also, i think you should do it beforehand, not after

	std::string s = narrowRtoS(r); // The given text should be percent encoded, so it should only contain characters that take 1 byte
	const char *p = s.c_str();
	int bytes = lengthp(p);

	Memory m;
	if (!m.Allocate(bytes + 1)) return r;

	unsigned char c1, c2, code1, code2, b;

	for (int i = 0; i < bytes; i++) {
		unsigned char c = p[i];

		if (c == '%') {
			if (i + 3 > bytes) return r;

			c1 = p[i + 1];
			c2 = p[i + 2];

			c1 = toupper(c1); // Accept uppercase and lowercase letters
			c2 = toupper(c2);


			if      (c1 >= '0' && c1 <= '9') code1 = c1 - '0';      // '0'  0 0000 through '9'  9 1001
			else if (c1 >= 'A' && c1 <= 'F') code1 = c1 - 'A' + 10; // 'A' 10 1010 through 'F' 15 1111
			else                             return r;

			if      (c2 >= '0' && c2 <= '9') code2 = c2 - '0';      // '0'  0 0000 through '9'  9 1001
			else if (c2 >= 'A' && c2 <= 'F') code2 = c2 - 'A' + 10; // 'A' 10 1010 through 'F' 15 1111
			else                             return r;


			// Shift the 4 bytes it means into the high portion of the byte, like 1000----
			b = code1 << 4;


			// Copy the 4 bits from the second character in the pair into the low portion of the byte, like ----1100
			b |= code2; // Use the bitwise or operator to assemble the entire byte, like 10001100

			// Copy the byte into the memory, and move to the next byte
//			if (i < bytes) *(p + i) = b;
			i += 2;


		} else {



		}
	}








	return L"";
}



void TestEncode(read r) {

	CString normal, encoded, decoded;

	normal = r;
	encoded = PercentEncode(normal);
	decoded = PercentDecode(encoded);

	int i = 7;
}





// Run a snippet of test code
void Test() {


	PercentDecode(L"%41");
	PercentDecode(L"%41%42");
	PercentDecode(L"%00");
	PercentDecode(L"%ff");


	/*

	TestEncode(L"hello");
	TestEncode(L"hello you");
	TestEncode(L"\r\n");
	TestEncode(L"hello+you");
	TestEncode(L"\u4e00"); // should be %E4%B8%80

	Find f(L"C:\\Documents\\test", true);
	while (f.Result()) {

		TestEncode(f.info.cFileName);
	}
	*/





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







