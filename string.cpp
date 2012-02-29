
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Takes text
// Copies and catcatenates the text into a string
// Returns a string
CString make(read r1, read r2, read r3, read r4, read r5, read r6, read r7, read r8, read r9) {

	CString s1 = r1, s2 = r2, s3 = r3, s4 = r4, s5 = r5, s6 = r6, s7 = r7, s8 = r8, s9 = r9;
	return s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9;
}

// Takes text
// Uppercases the characters in it
// Returns a string
CString upper(read r) {

	CString s = r;
	s.MakeUpper();
	return s;
}

// Takes text
// Lowercases the characters in it
// Returns a string
CString lower(read r) {

	CString s = r;
	s.MakeLower();
	return s;
}

// Takes text
// Reads the text as a number, handling a leading minus sign properly
// Returns the number, or 0 if given blank or alphabetic text
int number(read r) {

	return _wtoi(r);
}

// Takes text
// Determines if the text is not blank
// Returns true if it is, false if not
bool is(read r) {

	if (r[0] != '\0') return true; // The text doesn't begin with a null terminator, and isn't blank
	else              return false;
}

// Takes text
// Determines if the text is blank
// Returns true if it is, false if not
bool isblank(read r) {

	if (r[0] == '\0') return true; // The text begins with the null terminator, and is blank
	else              return false;
}

// Takes text r1 and r2, and matching
// Determines if r1 and r2 are the same
// Returns true if they are, false if they are not
bool same(read r1, read r2, matching m) {

	if (compare(r1, r2, m) == 0) return true;  // They are the same
	else                         return false;
}

// Takes text r1 and r2, and matching
// Calls lstrcmp or lstrcmpi on them
// Returns the result, which is negative if r1 is before r2, zero if they are the same, and positive if r1 is below r2
int compare(read r1, read r2, matching m) {

	if (m == Different) return lstrcmpW(r1, r2);  // Case sensitive, the default
	else                return lstrcmpiW(r1, r2); // Case insensitive, matching cases
}

// Takes text r and t, and matching
// Determins if the text starts with the tag
// Returns true if it does, false if it does not or if r or t are blank
bool starts(read r, read t, matching m) {

	// Use find to determine if the tag is at the start of the text
	if (find(r, t, Forward, m) == 0) return true;
	else                             return false;
}

// Takes text r and t, and matching
// Determins if the text ends with the tag
// Returns true if it does, false if it does not or if r or t are blank
bool trails(read r, read t, matching m) {

	// Find the last instance of the tag
	int result;
	result = find(r, t, Reverse, m);
	if (result == -1) return false; // Tag not found

	if (result == length(r) - length(t)) return true;  // Tag found on end
	else                                 return false; // Tag found elsewhere
}

// Takes text r and t, and matching
// Determins if the tag appears in the text
// Returns true if it does, false if it does not or if r or t are blank
bool has(read r, read t, matching m) {

	// Use find to determine if the tag exists in the text
	if (find(r, t, Forward, m) != -1) return true;
	else                              return false;
}

// Takes text r and t, and direction and matching
// Finds in r the first or last instance of t
// Returns the zero based index of t in r, or -1 if not found or if r or t are blank
int find(read r, read t, direction d, matching m) {

	// Get lengths
	int rlength, tlength;
	rlength = length(r);
	tlength = length(t);

	// If either is blank or r is shorter than t, return not found
	if (!rlength || !tlength || rlength < tlength) return -1;

	// Variables for loop
	bool valid;         // Valid tells if the tag is being found
	int rindex, tindex; // Scannign indices
	WCHAR rchar, tchar; // Characters

	// Scan rindex between 0 and rlength - tlength in the desired direction
	if (d == Forward) rindex = 0;
	else              rindex = rlength - tlength;
	while (1) {
		if (d == Forward) { if (rindex > rlength - tlength) break; }
		else              { if (rindex < 0)                 break; }

		// Set valid true and look for the tag at rindex, to either break false at first mismatch or finish true
		valid = true;
		for (tindex = 0; tindex <= tlength - 1; tindex++) {

			// Get the pair of characters
			rchar = r[rindex + tindex];
			tchar = t[tindex];

			// Uppercase them if matching was requested
			if (m == Matching) {

				rchar = (WCHAR)CharUpper((WCHAR *)(ULONG_PTR)MAKELONG((WORD)rchar, 0));
				tchar = (WCHAR)CharUpper((WCHAR *)(ULONG_PTR)MAKELONG((WORD)tchar, 0));
			}

			// Mismatch found, set false and break
			if (rchar != tchar) { valid = false; break; }
		}

		// The tag was found at rindex, return it, done
		if (valid) return rindex;

		if (d == Forward) rindex++;
		else              rindex--;
	}

	// Not found
	return -1;
}

// Takes text to parse, opening and closing tags, and matching
// Gets the text between the tags
// Returns a string
CString parse(read r, read t1, read t2, matching m) {

	// Clip from after the first tag and then before the second or blank if not found
	CString s = after(r, t1, Forward, m);
	if (has(s, t2, m)) s = before(s, t2, Forward, m);
	else               s = L"";
	return s;
}

// Takes text and tag, and direction and matching
// Splits the text before the tag
// Returns a string, the text from r if not found in either direction
CString before(read r, read t, direction d, matching m) {

	// Use split
	CString b, a;
	split(r, t, &b, &a, d, m);
	return b;
}

// Takes text and tag, and direction and matching
// Splits the text after the tag
// Returns a string, blank if not found in either direction
CString after(read r, read t, direction d, matching m) {

	// Use split
	CString b, a;
	split(r, t, &b, &a, d, m);
	return a;
}

// Takes text and tag, strings for before and after, and direction and matching
// Splits the text around the tag, writing text in before and after
// Returns nothing, puts all text in before and none in after if not found in either direction
void split(read r, read t, CString *b, CString *a, direction d, matching m) {

	// Find the tag in the text using the direction and matching passed to this function
	int i;
	i = find(r, t, d, m);
	if (i == -1) {

		// Not found, all text is before and none is after, done
		*b = r;
		*a = L"";
		return;
	}

	// Get lengths
	int rlength, tlength;
	rlength = length(r);
	tlength = length(t);

	// Clip out before and after form a copy of r so that r and *b being the same won't mangle *a
	CString source = r;
	*b = clip(source, 0, i);
	*a = clip(source, i + tlength, rlength - tlength - i);
}

// Takes text, find and replace tags, and matching
// Makes a single pass down the text, replacing whole instances of the find text with the replacement text
// Returns a string
CString replace(read r, read t1, read t2, matching m) {

	// If the text or the find text is blank, or if the find text is not found, return the text unchanged
	CString top, left, bottom;
	top = r;
	if (isblank(r) || isblank(t1) || !has(r, t1, m)) return top;

	// Loop while top has find
	while (has(top, t1, m)) {

		// f is in top
		split(top, t1, &left, &top, Forward, m);
		bottom += left + t2;
	}

	// f is not in top
	bottom += top;

	// Return bottom text
	return bottom;
}

// Takes text, a starting index, and a number of characters to copy or -1 for all
// Clips out that text, not reading outside of r
// Returns a string
CString clip(read r, int startindex, int characters) {

	// Get the length and eliminate special cases
	CString s;
	int n = length(r);
	if (n == 0 || characters == 0) { return s; }            // No characters to clip or none requested
	if (startindex < 0 || startindex > n - 1) { return s; } // Start index outside of r

	// Adjust local copy of characters
	if (characters < 0 || characters > n - startindex) characters = n - startindex;

	// Copy the text into the string, crop it, and return it
	s = r;
	s = s.Mid(startindex, characters);
	return s;
}

// Takes text and tag, and direction and matching
// Confirms the text starts or ends with the tag, inserting it if necessary
// Returns a string
CString on(read r, read t, direction d, matching m) {

	CString s = r;
	if (d == Forward) { if (!starts(s, t, m)) s = t + s; } // Confirm the text starts with the tag
	else              { if (!trails(s, t, m)) s = s + t; } // Confirm the text ends with the tag
	return s;
}

// Takes text and tag, and direction and matching
// Confirms the text does not start or end with the tag, removing multiple instances of it if necessary
// Returns a string
CString off(read r, read t, direction d, matching m) {

	CString s = r;
	if (d == Forward) { while(starts(s, t, m)) s = clip(s, length(t), -1); }            // Remove the tag from the start of the string
	else              { while(trails(s, t, m)) s = clip(s, 0, length(s) - length(t)); } // Remove the tag from the end of the string
	return s;
}

// Takes text and tags
// Removes the tags from the start and end of the text
// Returns a string
CString trim(read r, read t1, read t2, read t3) {

	// Copy the text into a string
	CString s = r;

	// Remove the tags from the start of the string until gone
	while (true) {

		if      (starts(s, t1)) s = clip(s, length(t1), -1);
		else if (starts(s, t2)) s = clip(s, length(t2), -1);
		else if (starts(s, t3)) s = clip(s, length(t3), -1);
		else                    break;
	}

	// Remove the tags from the end of the string until gone
	while (true) {

		if      (trails(s, t1)) s = clip(s, 0, length(s) - length(t1));
		else if (trails(s, t2)) s = clip(s, 0, length(s) - length(t2));
		else if (trails(s, t3)) s = clip(s, 0, length(s) - length(t3));
		else                    break;
	}

	// Return the string
	return s;
}

// Split r into a list of words separated by a tag
std::vector<CString> words(read r, read t) {

	CString raw = r;        // The end of the given text that still needs to be processed
	CString word;           // An individual word the loop has found
	std::vector<CString> v; // The list of words we build up and return
	while (has(raw, t)) {   // There's a tag

		split(raw, t, &word, &raw); // Split off the word before it
		v.push_back(word);          // Add the word to our list
	}

	v.push_back(raw); // Everything after the last tag is the last word
	return v;         // Return the list we built up
}

// Takes a number and a name
// Composes text like "14 apples"
// Returns a string
CString SayNumber(int number, read name) {

	if      (number == 0) return make(L"no ", name, L"s");                               // Zero yields "no [name]s"
	else if (number == 1) return make(L"1 ", name);                                      // One yields "1 [name]"
	else                  return make(InsertCommas(numerals(number)), L" ", name, L"s"); // Greater yields "[number] [name]s"
}

// Takes text
// Inserts commas between groups of three characters
// Returns a string
CString InsertCommas(read r) {

	// Make strings
	CString s, left, bottom;
	s = r;

	// Move down commas and groups of 3 characters
	while (length(s) > 3) {

		left = clip(s, length(s) - 3, 3);
		s = clip(s, 0, length(s) - 3);
		bottom = L"," + left + bottom;
	}

	// Move down the leading gorup of up to 3 characters and return the string
	bottom = s + bottom;
	return bottom;
}

// Takes a number of milliseconds
// Composes text to describe how long that is
// Returns a string
CString SayTime(DWORD time) {

	// Return explination for less than a second
	if (time < 1000) return L"less than a second";

	// Calculate the hour, minute, and second numbers
	int hour, minute, second;
	hour = time / 3600000;
	minute = (time / 60000) - (hour * 60);
	second = (time / 1000) - (hour * 3600) - (minute * 60);

	// Compose the text to display and return it
	CString s;
	if (hour) s += SayNumber(hour, L"hour");
	if (hour || minute) s += L" " + SayNumber(minute, L"minute");
	s += L" " + SayNumber(second, L"second");
	return trim(s, L" ");
}

// Composes text like "Tue 2:07p 03.789s" with the current day and time to milliseconds
CString SayNow() {

	// Get the local time right now
	SYSTEMTIME info;
	ZeroMemory(&info, sizeof(info));
	GetLocalTime(&info);

	// Compose day text
	CString day;
	switch (info.wDayOfWeek) {
		case 0: day = "Sun"; break;
		case 1: day = "Mon"; break;
		case 2: day = "Tue"; break;
		case 3: day = "Wed"; break;
		case 4: day = "Thu"; break;
		case 5: day = "Fri"; break;
		case 6: day = "Sat"; break;
	}

	// Prepare hour and AM/PM text
	CString m = info.wHour < 12 ? L"a" : L"p";
	int h = info.wHour;
	if (!h) h = 12;
	if (h > 12) h -= 12;

	// Turn numbers into text
	CString hour        = numerals(h);
	CString minute      = numerals(info.wMinute, 10, 2);
	CString second      = numerals(info.wSecond, 10, 2); // Say "09" seconds instead of just "9" so things line up vertically
	CString millisecond = numerals(info.wMilliseconds, 10, 3);

	// Put it all together
	return day + L" " + hour + L":" + minute + m + L" " + second + L"." + millisecond + L"s";
}

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

// URI decode the given text, decoding percent sequences like "%20" into the characters they represent
// Correctly decodes international text, the three UTF-8 bytes "%E4%B8%80" become L"\u4e00" the chinese character for one
// Decodes both "%20" and "+" into " " in case spaces got encoded into plusses, ok because "+" would have gotten encoded into "%2B"
// On error, returns the given text unchanged
CString UriDecode(read r) {

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
// Works like encodeURIComponent() in javascript
// Encodes every UTF-8 byte of the text except A-Z a-z 0-9 -_.~ and ~!*()'
// Optionally encodes " " to "+" instead of "%20", which is ok when encoding a part of the URI after the "?", which is the entire magnet link
// Encodes international characters, the chinese character for one L"\u4e00" becomes the three bytes it is in UTF-8 encoded "%E4%B8%80"
CString UriEncode(read r, bool plus) {

	std::string dstring = narrowRtoS(r); // Convert r in UTF-16, where every character takes 2 bytes, to dstring in UTF-8, where A takes 1 byte and hiragana letter no takes 3 bytes
	const char *d = dstring.c_str();
	int dbytes = lengthp(d);             // Number of bytes of UTF-8 text we have to encode
	char bay[MAX_PATH];                  // Bay to compose the text of one encoded byte like "%20"
	std::string e;                       // String for encoded text

	for (int dindex = 0; dindex < dbytes; dindex++) { // Loop for each byte in the decoded text

		if ((d[dindex] >= 'A' && d[dindex] <= 'Z') || // Unreserved character we don't have to encode, just copy it over
			(d[dindex] >= 'a' && d[dindex] <= 'z') || // Letters and numbers
			(d[dindex] >= '0' && d[dindex] <= '9') ||
			d[dindex] == '-' || d[dindex] == '_' || d[dindex] == '.' || d[dindex] == '~' || // RFC 3986 unreserved characters
			d[dindex] == '~' || d[dindex] == '!' || d[dindex] == '*' || d[dindex] == '(' || d[dindex] == ')' || d[dindex] == '\'') { // Characters encodeURIComponent allows

			e += d[dindex];

		} else if (plus && d[dindex] == ' ') { // Space, with the option to encode " " into "+", otherwise the code below will turn it into "%20"

			e += "+";

		} else { // Reserved character or other byte, percent encode it

			sprintf(bay, "%%%02X", (unsigned char)(d[dindex]));
			e += bay;
		}
	}

	return widenStoC(e);
}

// Replace characters not allowed in windows file names with acceptable ones
CString SafeFileName(read r) {

	CString s = r;
	s = replace(s, L"\"", L"'"); // Turn double quotes into single ones
	s = replace(s, L"\\", L"-"); // Turn other characters that aren't allowed into hyphens
	s = replace(s,  L"/", L"-");
	s = replace(s,  L":", L"-");
	s = replace(s,  L"*", L"-");
	s = replace(s,  L"?", L"-");
	s = replace(s,  L"<", L"-");
	s = replace(s,  L">", L"-");
	s = replace(s,  L"|", L"-");
	return s;
}
