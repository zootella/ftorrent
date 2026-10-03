// The setup program a user runs: one executable with a cabinet of files and a small table appended to its end, which it unpacks into the user's own application data folder and then starts the program it brought. It descends from the 2005 Zootella Setup Creator's Setup.cpp, which did the same with a zlib stream in a resource and ANSI strings, and keeps its stance: no window, no questions, nothing to click. The only interface is a dialog box when something goes wrong, so a user who sees nothing can trust that nothing went wrong.

// Everything it needs it carries. The creator, win-setup.js, builds a cabinet with Windows' own makecab, writes a table of UTF-16 strings, and appends cabinet, table, and a trailer to the compiled stub; the trailer says where the two are and holds a SHA-256 of both, which this program checks before it writes a single file. Windows itself does the unpacking, through the cabinet file decompression interface in cabinet.dll that Windows Update uses for its own packages, so no compression code is compiled in here. Everything is per user: the folder is under local application data, found by asking the shell rather than by building a path, and nothing here ever elevates, which the manifest beside this file also declares.

// Around the unpacking, three things an installer owes the system. A copy of the program already running from the folder is asked to exit the way its own File menu would, through the named pipe it serves for a second launch, and only a copy that does not answer is ended; so an upgrade writes over the files of a program that has saved its state, and never over one that is running. One shortcut goes in the Start menu, carrying the application's identifier as its AppUserModelID, which is how Start, the taskbar, and notifications recognize the program as itself. And since the program draws its window with Microsoft Edge WebView2, which every current Windows has but a stripped one may not, the registry is read for it, and a machine without it gets Microsoft's page opened instead of a program that cannot start.

// The same program is the uninstaller. Setup writes a copy of itself without the cabinet into the folder as uninstall.exe, with the table and a trailer whose cabinet size is zero, and registers it where Add or Remove Programs and Settings look. Run, it closes a running copy, walks the list of registry instructions win-setup.toml gave it, taking back only what the program wrote and only while it is still the program's, removes the shortcut, any taskbar pin, and the jump list, and removes the folder; since a program cannot delete the file it is running from, it does that work as a copy of itself started from the temporary folder. The user's data, the downloads and the settings and session state in the data folder, is never touched.

#define _WIN32_WINNT 0x0A00 // Windows 10 and later, which declares SetDefaultDllDirectories and SHGetKnownFolderPath
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS            // the C spelling of COM calls, like IShellLinkW_SetPath(link, path)
#include <windows.h>
#include <initguid.h>         // define the GUIDs the headers below declare in this file, the CLSID of the shell link and the property key for the AppUserModelID among them, rather than looking for them in a library
#include <shlobj.h>           // SHGetKnownFolderPath and IShellLinkW, the shortcut
#include <knownfolders.h>     // FOLDERID_LocalAppData and FOLDERID_Programs, included by name because shlobj.h leaves it out once initguid.h is in play
#include <propkey.h>          // PKEY_AppUserModel_ID, the property a shortcut carries so the shell knows which application it starts
#include <shellapi.h>         // ShellExecuteW, to open a web page in the user's browser
#include <tlhelp32.h>         // the process snapshot, to find a copy still running from the install folder
#include <bcrypt.h>           // SHA-256, to check the appended data before trusting it
#pragma warning(push)
#pragma warning(disable: 4201) // fdi.h declares a nameless union, which warning level 4 flags as a nonstandard extension
#include <fdi.h>              // the cabinet file decompression interface, FDICreate and FDICopy, which hands each file in the cabinet to a callback
#pragma warning(pop)
#include <strsafe.h>          // StringCchCopyW and friends, string functions that take the buffer's size and cannot run past it

// The trailer is the last bytes of the setup executable. The program finds it by reading from the end of its own file, and from it finds the cabinet and the table that were appended ahead of it. Packed to one byte so the layout is exactly the 72 bytes win-setup.js writes, in the order it writes them.
#pragma pack(push, 1)
typedef struct {
	BYTE hash[32];         // SHA-256 of the cabinet bytes followed by the table bytes
	UINT64 cabinetOffset;  // where the cabinet starts, from the start of the file
	UINT64 cabinetSize;
	UINT64 tableOffset;    // where the table starts; the table is UTF-16 strings, each ending in a null character, in the order tableString documents
	UINT64 tableSize;
	char magic[8];         // the letters winsetup, the last eight bytes of the file, so a stub nothing was appended to can say so rather than read garbage
} Trailer;
#pragma pack(pop)
static const char magic[8] = {'w', 'i', 'n', 's', 'e', 't', 'u', 'p'};

// A stream the decompressor reads or writes through the callbacks below. The decompressor thinks in files: it asks to open the cabinet by name, reads and seeks in it, and writes each unpacked file to a handle this program gives it. The cabinet is not a file, though; it is a window of bytes inside this executable, so a stream carries a base and a size, and every read and seek stays inside them. A file being written has a base of zero and no limit.
typedef struct {
	HANDLE file;      // the open handle, to this executable for the cabinet, or to a file being created
	UINT64 base;      // where the window begins in that file
	UINT64 size;      // how long the window is
	UINT64 position;  // where the next read or write happens, relative to base
	BOOL writing;     // a file being unpacked, rather than the cabinet being read
} Stream;

#define PATH_SIZE 4096 // room for a path, in characters; the manifest declares the program long path aware, so paths are not held to MAX_PATH
static WCHAR selfPath[PATH_SIZE];   // this executable's own path, which the cabinet stream opens
static WCHAR localData[PATH_SIZE];  // the user's local application data folder, under which the program and its data folder both live
static WCHAR folder[PATH_SIZE];     // the folder the files are unpacked into, local application data joined with the product name
static WCHAR program[PATH_SIZE];    // the program inside it, which is started at the end and asked to exit at the start
static WCHAR title[256];            // the title of the dialogs, the product name and the word Setup
static Trailer trailer;
static BYTE* table;                 // the table, read whole into memory; it is a few hundred bytes
static UINT64 written;              // how many bytes the cabinet unpacked to, which is the size an uninstall entry reports

// Show what went wrong and stop. There is no other interface, so this is the one place the user sees text. The system's own description of the last error follows the sentence, when there is one, because it is usually the useful half: which file, which folder, access denied or disk full.
__declspec(noreturn) static void fail(const WCHAR* what) {
	DWORD error = GetLastError();
	WCHAR text[1024];
	WCHAR reason[512] = L"";
	if (error) FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, error, 0, reason, 512, NULL); // the system's sentence for the code, on one line
	StringCchPrintfW(text, 1024, L"%s\n\n%s", what, reason);
	MessageBoxW(NULL, text, title[0] ? title : L"Setup", MB_OK | MB_ICONERROR | MB_SETFOREGROUND); // in front, since a setup program started from a browser's download list may not be the active window
	ExitProcess(1);
}

// Read a run of bytes from this executable at an absolute offset
static void readSelf(HANDLE file, UINT64 offset, void* into, DWORD size) {
	LARGE_INTEGER where;
	where.QuadPart = (LONGLONG)offset;
	DWORD got = 0;
	if (!SetFilePointerEx(file, where, NULL, FILE_BEGIN) || !ReadFile(file, into, size, &got, NULL) || got != size) fail(L"Setup could not read its own file.");
}

// SHA-256, Windows' own through the cryptography API in bcrypt.dll: begin, add bytes, and finish into 32 bytes
static BCRYPT_ALG_HANDLE hashAlgorithm;
static BCRYPT_HASH_HANDLE hashBegin(void) {
	BCRYPT_HASH_HANDLE hash = NULL;
	if ((!hashAlgorithm && BCryptOpenAlgorithmProvider(&hashAlgorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) != 0) || BCryptCreateHash(hashAlgorithm, &hash, NULL, 0, NULL, 0, 0) != 0) fail(L"Setup could not start a hash.");
	return hash;
}
static void hashAdd(BCRYPT_HASH_HANDLE hash, const void* bytes, DWORD size) {
	if (BCryptHashData(hash, (PUCHAR)bytes, size, 0) != 0) fail(L"Setup could not hash.");
}
static void hashEnd(BCRYPT_HASH_HANDLE hash, BYTE digest[32]) {
	if (BCryptFinishHash(hash, digest, 32, 0) != 0) fail(L"Setup could not finish a hash.");
	BCryptDestroyHash(hash);
}

// Check the SHA-256 in the trailer against the cabinet and table as they sit in the file, so a download cut short or a file altered after it was built stops here, before anything is written
static void checkHash(HANDLE file) {
	BCRYPT_HASH_HANDLE hash = hashBegin();
	BYTE* buffer = HeapAlloc(GetProcessHeap(), 0, 1 << 20); // a megabyte at a time, since the cabinet is tens of megabytes
	if (!buffer) fail(L"Setup ran out of memory.");
	UINT64 runs[2][2] = {{trailer.cabinetOffset, trailer.cabinetSize}, {trailer.tableOffset, trailer.tableSize}}; // the cabinet, then the table, in that order, as the creator hashed them
	for (int run = 0; run < 2; run++) {
		UINT64 offset = runs[run][0], left = runs[run][1];
		while (left) {
			DWORD piece = left > (1 << 20) ? (1 << 20) : (DWORD)left;
			readSelf(file, offset, buffer, piece);
			hashAdd(hash, buffer, piece);
			offset += piece;
			left -= piece;
		}
	}
	BYTE digest[32];
	hashEnd(hash, digest);
	HeapFree(GetProcessHeap(), 0, buffer);
	if (memcmp(digest, trailer.hash, 32) != 0) { SetLastError(0); fail(L"This copy of Setup is damaged or incomplete. Download it again."); }
}

// The strings in the table, by position. The creator writes them in this order, and a later addition goes on the end, so an older stub given a newer table still finds what it knows.
// 0  the product name, which names the folder under local application data and appears in the dialogs' titles
// 1  the program to start when the files are in place, as a path relative to that folder, like ftorrent.exe
// 2  the application's identifier, like com.ftorrent.ftorrent, which names its data folder and is the AppUserModelID its shortcut carries
// 3  the version, like 0.1.0
// 4  the publisher
// 5  the home page
// 6 on  the uninstall instructions, each one string of tab-separated fields: verb, key, name, equals, at; win-setup.js says what each verb does
static const WCHAR* tableStringOrNull(int index) {
	const WCHAR* at = (const WCHAR*)table;
	const WCHAR* end = (const WCHAR*)(table + trailer.tableSize);
	for (int i = 0; at < end; i++) {
		const WCHAR* start = at;
		while (at < end && *at) at++; // to this string's null
		if (at >= end) break;         // a string that never ended: the table is malformed
		if (i == index) return start;
		at++;                         // past the null, to the next string
	}
	return NULL;
}
static const WCHAR* tableString(int index) {
	const WCHAR* found = tableStringOrNull(index);
	if (!found) { SetLastError(0); fail(L"Setup's table is missing a value it needs."); }
	return found;
}

// Make a folder and every folder above it that does not exist yet, walking the path one backslash at a time
static void makeFolders(WCHAR* path) {
	for (WCHAR* p = path + 3; *p; p++) { // past the drive letter, colon, and backslash
		if (*p == L'\\') {
			*p = 0;
			CreateDirectoryW(path, NULL); // exists already is fine, which the final check below covers
			*p = L'\\';
		}
	}
	if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) fail(L"Setup could not create a folder.");
}

// Whether the program is running: Windows will not open a running executable for writing, so the sharing violation that answers is the sign. A file that opens, or is not there yet, is not running
static BOOL programRunning(void) {
	HANDLE h = CreateFileW(program, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (h != INVALID_HANDLE_VALUE) { CloseHandle(h); return FALSE; }
	return GetLastError() == ERROR_SHARING_VIOLATION;
}

// Ask the running copy to exit, the way a second launch hands it a link: one line of JSON written into the named pipe it serves. The pipe's name is the product and a hash of the lock file's path, computed here exactly as instance.rs computes it, FNV-1a over the UTF-8 bytes of the path in lowercase; the lock file is in the data folder, local application data joined with the identifier. Answers whether the request was delivered
static BOOL askToExit(const WCHAR* product, const WCHAR* identifier) {
	WCHAR lock[PATH_SIZE];
	StringCchPrintfW(lock, PATH_SIZE, L"%s\\%s\\%s.lock", localData, identifier, product);
	CharLowerW(lock); // windows paths ignore case, and so does the hash
	char utf8[PATH_SIZE * 3];
	int bytes = WideCharToMultiByte(CP_UTF8, 0, lock, -1, utf8, sizeof(utf8), NULL, NULL) - 1; // without the null
	if (bytes <= 0) return FALSE;
	UINT64 hash = 0xcbf29ce484222325;
	for (int i = 0; i < bytes; i++) { hash ^= (BYTE)utf8[i]; hash *= 0x100000001b3; }
	WCHAR pipe[PATH_SIZE];
	StringCchPrintfW(pipe, PATH_SIZE, L"\\\\.\\pipe\\%s-%016llx", product, hash);
	HANDLE h = CreateFileW(pipe, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL); // opening a pipe by name is an ordinary file open
	if (h == INVALID_HANDLE_VALUE) return FALSE;
	const char* line = "{\"args\":[\"--exit\"]}\n";
	DWORD put = 0;
	BOOL sent = WriteFile(h, line, (DWORD)strlen(line), &put, NULL);
	CloseHandle(h);
	return sent;
}

// Wait up to the given time for the program to stop running, answering whether it did
static BOOL waitForExit(DWORD milliseconds) {
	for (DWORD waited = 0; waited < milliseconds; waited += 100) {
		if (!programRunning()) return TRUE;
		Sleep(100);
	}
	return !programRunning();
}

// End every process running from the install folder, the program and the engine it starts, which this user's account can see and end; the last resort after asking
static void endProcessesIn(void) {
	WCHAR prefix[PATH_SIZE];
	StringCchPrintfW(prefix, PATH_SIZE, L"%s\\", folder);
	size_t length = wcslen(prefix);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) return;
	PROCESSENTRY32W entry = {sizeof(entry)};
	for (BOOL more = Process32FirstW(snapshot, &entry); more; more = Process32NextW(snapshot, &entry)) {
		HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, entry.th32ProcessID);
		if (!process) continue; // another account's, or the system's
		WCHAR path[PATH_SIZE];
		DWORD size = PATH_SIZE;
		if (QueryFullProcessImageNameW(process, 0, path, &size) && _wcsnicmp(path, prefix, length) == 0) {
			TerminateProcess(process, 1);
			WaitForSingleObject(process, 3000);
		}
		CloseHandle(process);
	}
	CloseHandle(snapshot);
}

// A copy running from the install folder has to go before its files are replaced. Ask first and give it time to save and stop; a copy that does not answer, or has no pipe to answer on, is ended. A copy that will not end even then stops setup with a sentence rather than a half-written folder
static void closeRunning(const WCHAR* product, const WCHAR* identifier) {
	if (!programRunning()) return;
	if (askToExit(product, identifier) && waitForExit(15000)) return;
	endProcessesIn();
	if (waitForExit(5000)) return;
	WCHAR what[512];
	StringCchPrintfW(what, 512, L"%s is running and Setup could not close it. Exit %s, then run Setup again.", product, product);
	SetLastError(0);
	fail(what);
}

// Whether Microsoft Edge WebView2 is installed, read where its updater records the installed version: the machine's entry for the Evergreen runtime every Windows 11 and updated Windows 10 has, or a per-user one. A version of 0.0.0.0 is the updater's word for not installed
static BOOL webView2At(HKEY root, const WCHAR* key) {
	WCHAR version[64];
	DWORD size = sizeof(version);
	return RegGetValueW(root, key, L"pv", RRF_RT_REG_SZ, NULL, version, &size) == ERROR_SUCCESS && version[0] && wcscmp(version, L"0.0.0.0") != 0;
}
static BOOL webView2Present(void) {
	return webView2At(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}")
		|| webView2At(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}");
}

// The one shortcut, in the user's Start menu, pointing at the program and carrying the application's identifier as its AppUserModelID. The program sets the same identifier on its window when it runs, so a pinned shortcut and the running window are one taskbar button rather than two, and a notification is credited to the shortcut rather than to a path. The shortcut is written through the shell's own interfaces, a shell link for the target and a property store for the identifier, which is the same way Explorer writes one
static void makeShortcut(const WCHAR* product, const WCHAR* identifier) {
	PWSTR programs = NULL;
	if (SHGetKnownFolderPath(&FOLDERID_Programs, KF_FLAG_CREATE, NULL, &programs) != S_OK) fail(L"Windows did not say where the Start menu is.");
	WCHAR path[PATH_SIZE];
	StringCchPrintfW(path, PATH_SIZE, L"%s\\%s.lnk", programs, product);
	CoTaskMemFree(programs);

	IShellLinkW* link = NULL;
	IPropertyStore* store = NULL;
	IPersistFile* file = NULL;
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(hr)) hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void**)&link);
	if (SUCCEEDED(hr)) hr = IShellLinkW_SetPath(link, program);
	if (SUCCEEDED(hr)) hr = IShellLinkW_SetWorkingDirectory(link, folder);
	if (SUCCEEDED(hr)) hr = IShellLinkW_QueryInterface(link, &IID_IPropertyStore, (void**)&store);
	if (SUCCEEDED(hr)) {
		PROPVARIANT value;
		PropVariantInit(&value);
		value.vt = VT_LPWSTR;
		value.pwszVal = (PWSTR)identifier; // the store copies it; this program never frees it
		hr = IPropertyStore_SetValue(store, &PKEY_AppUserModel_ID, &value);
		if (SUCCEEDED(hr)) hr = IPropertyStore_Commit(store);
	}
	if (SUCCEEDED(hr)) hr = IShellLinkW_QueryInterface(link, &IID_IPersistFile, (void**)&file);
	if (SUCCEEDED(hr)) hr = IPersistFile_Save(file, path, TRUE);
	if (file) IPersistFile_Release(file);
	if (store) IPropertyStore_Release(store);
	if (link) IShellLinkW_Release(link);
	if (FAILED(hr)) { SetLastError((DWORD)hr); fail(L"Setup could not write the Start menu shortcut."); }
}

// The callbacks the decompressor calls for memory and for files. It was designed around the C runtime's open, read, write, close, and seek, which is why the handles are integers and the open takes a name and flags; here each handle is a pointer to a Stream.
static FNALLOC(memoryAllocate) { return HeapAlloc(GetProcessHeap(), 0, cb); }
static FNFREE(memoryFree) { HeapFree(GetProcessHeap(), 0, pv); }

// Open the cabinet. The decompressor opens by the name FDICopy was given, which is the one word cabinet, and the stream it gets is a window onto this executable, from the cabinet's offset for its size. Files being unpacked are not opened here; the copy notification below creates them.
static FNOPEN(streamOpen) {
	UNREFERENCED_PARAMETER(oflag); UNREFERENCED_PARAMETER(pmode);
	if (lstrcmpA(pszFile, "cabinet") != 0) return -1;
	Stream* s = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Stream));
	if (!s) return -1;
	s->file = CreateFileW(selfPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (s->file == INVALID_HANDLE_VALUE) { HeapFree(GetProcessHeap(), 0, s); return -1; }
	s->base = trailer.cabinetOffset;
	s->size = trailer.cabinetSize;
	return (INT_PTR)s;
}

// Read from a stream, never past the end of its window
static FNREAD(streamRead) {
	Stream* s = (Stream*)hf;
	if (s->position >= s->size) return 0;
	if (cb > s->size - s->position) cb = (UINT)(s->size - s->position);
	LARGE_INTEGER where;
	where.QuadPart = (LONGLONG)(s->base + s->position);
	DWORD got = 0;
	if (!SetFilePointerEx(s->file, where, NULL, FILE_BEGIN) || !ReadFile(s->file, pv, cb, &got, NULL)) return (UINT)-1;
	s->position += got;
	return got;
}

// Write to a file being unpacked, and count the bytes
static FNWRITE(streamWrite) {
	Stream* s = (Stream*)hf;
	DWORD put = 0;
	if (!WriteFile(s->file, pv, cb, &put, NULL)) return (UINT)-1;
	s->position += put;
	written += put;
	return put;
}

static FNCLOSE(streamClose) {
	Stream* s = (Stream*)hf;
	CloseHandle(s->file);
	HeapFree(GetProcessHeap(), 0, s);
	return 0;
}

// Move within a stream's window, from its start, from here, or from its end, as the C runtime's seek does; the decompressor seeks only in the cabinet
static FNSEEK(streamSeek) {
	Stream* s = (Stream*)hf;
	LONGLONG from = seektype == FILE_CURRENT ? (LONGLONG)s->position : seektype == FILE_END ? (LONGLONG)s->size : 0;
	LONGLONG to = from + dist;
	if (to < 0 || (UINT64)to > s->size) return -1;
	s->position = (UINT64)to;
	if (s->writing) { // a file being written: move the real file pointer too
		LARGE_INTEGER where;
		where.QuadPart = to;
		SetFilePointerEx(s->file, where, NULL, FILE_BEGIN);
	}
	return (long)to;
}

// What the decompressor tells us as it works through the cabinet. Two notifications matter: a file is about to be copied, where this creates the destination and hands back its stream, and a file is finished, where this sets its date and closes it. The cabinet names files with backslashes in paths relative to the cabinet's root, which is the install folder here, and the names are UTF-8 when the attribute says so and the system's 8-bit code page otherwise; makecab writes plain ASCII for ours.
static FNFDINOTIFY(notify) {
	switch (fdint) {
	case fdintCOPY_FILE: {
		WCHAR name[PATH_SIZE], path[PATH_SIZE];
		if (!MultiByteToWideChar((pfdin->attribs & _A_NAME_IS_UTF) ? CP_UTF8 : CP_ACP, 0, pfdin->psz1, -1, name, PATH_SIZE)) return -1;
		StringCchPrintfW(path, PATH_SIZE, L"%s\\%s", folder, name);
		WCHAR* slash = wcsrchr(path, L'\\'); // the file's own folder, which may be new
		if (slash) { *slash = 0; makeFolders(path); *slash = L'\\'; }
		Stream* s = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Stream));
		if (!s) return -1;
		s->file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); // over whatever was there, which is how an upgrade replaces the previous version's files
		if (s->file == INVALID_HANDLE_VALUE) { HeapFree(GetProcessHeap(), 0, s); fail(L"Setup could not create a file."); }
		s->size = (UINT64)-1;
		s->writing = TRUE;
		return (INT_PTR)s;
	}
	case fdintCLOSE_FILE_INFO: {
		Stream* s = (Stream*)pfdin->hf;
		FILETIME local, utc;
		if (DosDateTimeToFileTime(pfdin->date, pfdin->time, &local) && LocalFileTimeToFileTime(&local, &utc)) SetFileTime(s->file, NULL, NULL, &utc); // the date modified the file had when it was packed
		streamClose((INT_PTR)s);
		return TRUE;
	}
	case fdintNEXT_CABINET:
		return -1; // the files are in one cabinet; a cabinet asking for a second is not ours
	default:
		return 0;  // cabinet information, partial files, and enumeration, none of which need anything
	}
}

// Replace every occurrence of a placeholder in text, writing the result into out
static void substitute(WCHAR* out, size_t size, const WCHAR* text, const WCHAR* placeholder, const WCHAR* with) {
	size_t length = wcslen(placeholder);
	out[0] = 0;
	for (const WCHAR* at = text; *at; ) {
		const WCHAR* found = wcsstr(at, placeholder);
		if (!found) { StringCchCatW(out, size, at); return; }
		StringCchCatNW(out, size, at, (size_t)(found - at));
		StringCchCatW(out, size, with);
		at = found + length;
	}
}

// Whether a value under HKCU still says what the instruction expects; a value that is missing, or of another type, does not
static BOOL valueEquals(const WCHAR* key, const WCHAR* name, const WCHAR* expected) {
	WCHAR value[2048];
	DWORD size = sizeof(value);
	if (RegGetValueW(HKEY_CURRENT_USER, key, name, RRF_RT_REG_SZ, NULL, value, &size) != ERROR_SUCCESS) return FALSE;
	return wcscmp(value, expected) == 0;
}

// Whether a key under HKCU exists and holds nothing, no values and no subkeys
static BOOL keyEmpty(const WCHAR* key) {
	HKEY h = NULL;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ, &h) != ERROR_SUCCESS) return FALSE;
	DWORD subkeys = 0, values = 0;
	RegQueryInfoKeyW(h, NULL, NULL, NULL, &subkeys, NULL, NULL, &values, NULL, NULL, NULL, NULL);
	RegCloseKey(h);
	return subkeys == 0 && values == 0;
}

// One instruction from the uninstall list: a copy of the table's string, since splitting writes into it. The fields are verb, key, name, equals, and at, separated by tabs; the key begins HKCU\ and is used without that prefix; {program} in equals becomes the installed program's path. Anything already gone is simply gone, so an uninstall run twice does no harm, and a verb this build does not know is skipped, so an older uninstaller given a newer list still does what it can
static void applyInstruction(WCHAR* line) {
	const WCHAR* fields[5] = {L"", L"", L"", L"", L""};
	int count = 0;
	for (WCHAR* at = line; count < 5; count++) {
		fields[count] = at;
		WCHAR* tab = wcschr(at, L'\t');
		if (!tab) break;
		*tab = 0;
		at = tab + 1;
	}
	const WCHAR* verb = fields[0];
	const WCHAR* key = fields[1];
	const WCHAR* name = fields[2];
	const WCHAR* at = fields[4];
	if (_wcsnicmp(key, L"HKCU\\", 5) != 0) return; // only the user's own hive, which win-setup.js already enforced
	key += 5;
	WCHAR expected[2048];
	substitute(expected, 2048, fields[3], L"{program}", program);

	if (wcscmp(verb, L"delete-key") == 0) {
		RegDeleteTreeW(HKEY_CURRENT_USER, key);
	} else if (wcscmp(verb, L"delete-value") == 0 || (wcscmp(verb, L"delete-value-if") == 0 && valueEquals(key, name, expected))) {
		HKEY h = NULL;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_SET_VALUE, &h) == ERROR_SUCCESS) { RegDeleteValueW(h, name); RegCloseKey(h); }
	} else if (wcscmp(verb, L"delete-key-if-empty") == 0) {
		if (keyEmpty(key)) RegDeleteKeyW(HKEY_CURRENT_USER, key);
	} else if (wcscmp(verb, L"delete-key-if") == 0) {
		WCHAR check[PATH_SIZE];
		if (at[0]) StringCchPrintfW(check, PATH_SIZE, L"%s\\%s", key, at); else StringCchCopyW(check, PATH_SIZE, key);
		if (valueEquals(check, name, expected)) RegDeleteTreeW(HKEY_CURRENT_USER, key);
	}
}

// Write one string or one number into an open key
static void writeString(HKEY key, const WCHAR* name, const WCHAR* value) { RegSetValueExW(key, name, 0, REG_SZ, (const BYTE*)value, (DWORD)((wcslen(value) + 1) * sizeof(WCHAR))); }
static void writeNumber(HKEY key, const WCHAR* name, DWORD value) { RegSetValueExW(key, name, 0, REG_DWORD, (const BYTE*)&value, sizeof(value)); }
static void uninstallEntryPath(WCHAR* path, const WCHAR* product) { StringCchPrintfW(path, PATH_SIZE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s", product); }
// The entry Add or Remove Programs and Settings read, under the user's own Uninstall key, named for the product as NSIS named it, so an install over an NSIS-era copy takes over its entry rather than adding a second; bytes is what the whole install takes on disk
static void writeUninstallEntry(const WCHAR* product, const WCHAR* version, const WCHAR* publisher, const WCHAR* homepage, const WCHAR* uninstaller, UINT64 bytes) {
	WCHAR path[PATH_SIZE], text[PATH_SIZE];
	uninstallEntryPath(path, product);
	HKEY key = NULL;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL) != ERROR_SUCCESS) fail(L"Setup could not write the entry Add or Remove Programs reads.");
	writeString(key, L"DisplayName", product);
	StringCchPrintfW(text, PATH_SIZE, L"\"%s\"", program);
	writeString(key, L"DisplayIcon", text);
	writeString(key, L"DisplayVersion", version);
	writeString(key, L"Publisher", publisher);
	writeString(key, L"InstallLocation", folder);
	StringCchPrintfW(text, PATH_SIZE, L"\"%s\"", uninstaller);
	writeString(key, L"UninstallString", text);
	writeString(key, L"QuietUninstallString", text); // the same command: the uninstaller asks nothing either way, and this is the form a package manager looks for
	if (homepage[0]) writeString(key, L"URLInfoAbout", homepage);
	SYSTEMTIME now;
	GetLocalTime(&now);
	StringCchPrintfW(text, PATH_SIZE, L"%04d%02d%02d", now.wYear, now.wMonth, now.wDay);
	writeString(key, L"InstallDate", text);
	writeNumber(key, L"NoModify", 1); // no Change button, since there is nothing to choose
	writeNumber(key, L"NoRepair", 1);
	writeNumber(key, L"EstimatedSize", (DWORD)((bytes + 1023) / 1024)); // in kilobytes, which is what Settings shows as the size
	RegCloseKey(key);
}
static void removeUninstallEntry(const WCHAR* product) {
	WCHAR path[PATH_SIZE];
	uninstallEntryPath(path, product);
	RegDeleteTreeW(HKEY_CURRENT_USER, path);
}

// uninstall.exe: this program again, without the cabinet. The first cabinetOffset bytes of this file are the compiled stub; after them go the table, unchanged, and a trailer with a cabinet of no bytes, which is how the program knows it is the uninstaller when it runs
static void writeUninstaller(const WCHAR* path) {
	HANDLE self = CreateFileW(selfPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	HANDLE out = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (self == INVALID_HANDLE_VALUE || out == INVALID_HANDLE_VALUE) fail(L"Setup could not write the uninstaller.");
	BYTE* buffer = HeapAlloc(GetProcessHeap(), 0, 1 << 20);
	if (!buffer) fail(L"Setup ran out of memory.");
	DWORD put = 0;
	for (UINT64 offset = 0; offset < trailer.cabinetOffset; ) {
		DWORD piece = trailer.cabinetOffset - offset > (1 << 20) ? (1 << 20) : (DWORD)(trailer.cabinetOffset - offset);
		readSelf(self, offset, buffer, piece);
		if (!WriteFile(out, buffer, piece, &put, NULL) || put != piece) fail(L"Setup could not write the uninstaller.");
		offset += piece;
	}
	HeapFree(GetProcessHeap(), 0, buffer);
	CloseHandle(self);
	if (!WriteFile(out, table, (DWORD)trailer.tableSize, &put, NULL)) fail(L"Setup could not write the uninstaller.");
	Trailer own = trailer;
	own.cabinetSize = 0;
	own.tableOffset = trailer.cabinetOffset;
	BCRYPT_HASH_HANDLE hash = hashBegin(); // over no cabinet bytes and then the table, the same order as always
	hashAdd(hash, table, (DWORD)trailer.tableSize);
	hashEnd(hash, own.hash);
	if (!WriteFile(out, &own, sizeof(own), &put, NULL)) fail(L"Setup could not write the uninstaller.");
	CloseHandle(out);
}

// Take the shortcut back: unpin it from Start and the taskbar first, through the shell's pinned list, since a pin that outlives its shortcut is a dead button; delete the file; and clear the jump list Windows kept for the application's identifier
static void removeShortcut(const WCHAR* product, const WCHAR* identifier) {
	PWSTR programs = NULL;
	if (SHGetKnownFolderPath(&FOLDERID_Programs, 0, NULL, &programs) != S_OK) return;
	WCHAR path[PATH_SIZE];
	StringCchPrintfW(path, PATH_SIZE, L"%s\\%s.lnk", programs, product);
	CoTaskMemFree(programs);
	if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
		IShellItem* item = NULL;
		if (SUCCEEDED(SHCreateItemFromParsingName(path, NULL, &IID_IShellItem, (void**)&item))) {
			IStartMenuPinnedList* pinned = NULL;
			if (SUCCEEDED(CoCreateInstance(&CLSID_StartMenuPin, NULL, CLSCTX_INPROC_SERVER, &IID_IStartMenuPinnedList, (void**)&pinned))) {
				IStartMenuPinnedList_RemoveFromList(pinned, item);
				IStartMenuPinnedList_Release(pinned);
			}
			IShellItem_Release(item);
		}
		ICustomDestinationList* list = NULL;
		if (SUCCEEDED(CoCreateInstance(&CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, &IID_ICustomDestinationList, (void**)&list))) {
			ICustomDestinationList_DeleteList(list, identifier);
			ICustomDestinationList_Release(list);
		}
		IApplicationDestinations* destinations = NULL;
		if (SUCCEEDED(CoCreateInstance(&CLSID_ApplicationDestinations, NULL, CLSCTX_INPROC_SERVER, &IID_IApplicationDestinations, (void**)&destinations))) {
			if (SUCCEEDED(IApplicationDestinations_SetAppID(destinations, identifier))) IApplicationDestinations_RemoveAllDestinations(destinations);
			IApplicationDestinations_Release(destinations);
		}
	}
	DeleteFileW(path);
}

// Remove a folder and everything in it. A file that will not go stops the uninstall with its name, rather than leaving a folder half gone with no word about it
static void removeTree(const WCHAR* path) {
	WCHAR pattern[PATH_SIZE];
	StringCchPrintfW(pattern, PATH_SIZE, L"%s\\*", path);
	WIN32_FIND_DATAW found;
	HANDLE find = FindFirstFileW(pattern, &found);
	if (find != INVALID_HANDLE_VALUE) {
		do {
			if (wcscmp(found.cFileName, L".") == 0 || wcscmp(found.cFileName, L"..") == 0) continue;
			WCHAR child[PATH_SIZE];
			StringCchPrintfW(child, PATH_SIZE, L"%s\\%s", path, found.cFileName);
			if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				removeTree(child);
			} else {
				SetFileAttributesW(child, FILE_ATTRIBUTE_NORMAL); // a read-only file deletes once it is not
				if (!DeleteFileW(child)) { WCHAR what[PATH_SIZE]; StringCchPrintfW(what, PATH_SIZE, L"Uninstall could not remove %s.", child); fail(what); }
			}
		} while (FindNextFileW(find, &found));
		FindClose(find);
	}
	if (!RemoveDirectoryW(path) && GetLastError() != ERROR_FILE_NOT_FOUND) { WCHAR what[PATH_SIZE]; StringCchPrintfW(what, PATH_SIZE, L"Uninstall could not remove the folder %s.", path); fail(what); }
}

// Whether this program is running from inside the install folder, which the uninstaller is when Settings or the user starts it there
static BOOL runningFromFolder(void) {
	WCHAR prefix[PATH_SIZE];
	StringCchPrintfW(prefix, PATH_SIZE, L"%s\\", folder);
	return _wcsnicmp(selfPath, prefix, wcslen(prefix)) == 0;
}

// A program cannot delete the file it is running from, so the uninstaller copies itself to the temporary folder and starts the copy, which does the work, and leaves. The copy stays in the temporary folder, a few hundred kilobytes Windows clears with the rest of it
__declspec(noreturn) static void relocate(const WCHAR* product) {
	WCHAR temp[PATH_SIZE], copy[PATH_SIZE], command[PATH_SIZE];
	if (!GetTempPathW(PATH_SIZE, temp)) fail(L"Windows did not say where the temporary folder is.");
	StringCchPrintfW(copy, PATH_SIZE, L"%s%s-uninstall-%x.exe", temp, product, GetTickCount());
	if (!CopyFileW(selfPath, copy, FALSE)) fail(L"Uninstall could not copy itself to the temporary folder.");
	StringCchPrintfW(command, PATH_SIZE, L"\"%s\" /relocated", copy);
	STARTUPINFOW startup = {sizeof(startup)};
	PROCESS_INFORMATION process;
	if (!CreateProcessW(copy, command, NULL, NULL, FALSE, 0, NULL, temp, &startup, &process)) fail(L"Uninstall could not start its copy."); // its current folder is the temporary folder, never the one it is about to remove
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	ExitProcess(0);
}

// The uninstaller, which asks nothing: a user who ran it by mistake installs again. Move out of the folder; close the running copy; take back the registry entries, the entry in Add or Remove Programs, and the shortcut; remove the folder. The data folder stays, with the settings and the session state, and so do the downloads, wherever they are
static int uninstall(const WCHAR* product, const WCHAR* identifier, const WCHAR* arguments) {
	StringCchPrintfW(title, 256, L"Uninstall %s", product);
	BOOL relocated = wcsstr(arguments, L"/relocated") != NULL; // the copy in the temporary folder, started by the one in the install folder
	if (!relocated && runningFromFolder()) relocate(product);

	closeRunning(product, identifier);
	WCHAR line[4096];
	for (int i = 6; ; i++) { // the instructions, in the order win-setup.toml gives them
		const WCHAR* instruction = tableStringOrNull(i);
		if (!instruction) break;
		StringCchCopyW(line, 4096, instruction);
		applyInstruction(line);
	}
	removeUninstallEntry(product);
	removeShortcut(product, identifier);
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL); // so Explorer drops the icons and menus it cached for the types the program offered to open

	WCHAR uninstaller[PATH_SIZE];
	StringCchPrintfW(uninstaller, PATH_SIZE, L"%s\\uninstall.exe", folder);
	for (int waited = 0; waited < 10000; waited += 100) { // the uninstaller that started this copy is still exiting for a moment
		HANDLE h = CreateFileW(uninstaller, DELETE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (h != INVALID_HANDLE_VALUE) { CloseHandle(h); break; }
		if (GetLastError() == ERROR_FILE_NOT_FOUND) break;
		Sleep(100);
	}
	removeTree(folder);
	return 0;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR arguments, int show) {
	UNREFERENCED_PARAMETER(instance); UNREFERENCED_PARAMETER(previous); UNREFERENCED_PARAMETER(show);

	// First, before any library is loaded by name: only load system libraries from the system folder. A setup program runs from Downloads, where a file named like a system library could be waiting for a program that searches its own folder first; cabinet.dll and bcrypt.dll are linked to load on first use, which is after this line
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);

	// Open this executable and read the trailer from its end
	if (!GetModuleFileNameW(NULL, selfPath, PATH_SIZE)) fail(L"Setup could not find its own path.");
	HANDLE self = CreateFileW(selfPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (self == INVALID_HANDLE_VALUE) fail(L"Setup could not open its own file.");
	LARGE_INTEGER size;
	if (!GetFileSizeEx(self, &size) || size.QuadPart < (LONGLONG)sizeof(Trailer)) fail(L"Setup's file is too short to hold anything.");
	readSelf(self, (UINT64)size.QuadPart - sizeof(Trailer), &trailer, sizeof(Trailer));
	SetLastError(0);
	if (memcmp(trailer.magic, magic, 8) != 0) fail(L"This is the setup program with nothing in it. win-setup.js appends the files it installs.");
	UINT64 total = (UINT64)size.QuadPart;
	if (trailer.cabinetOffset > total || trailer.cabinetSize > total - trailer.cabinetOffset || trailer.tableOffset > total || trailer.tableSize > total - trailer.tableOffset || trailer.tableSize > (1 << 20) || trailer.tableSize % 2) fail(L"Setup's trailer describes bytes the file does not have."); // each offset within the file and each size within what follows it, checked without adding, so a trailer with numbers that wrap cannot pass

	// Check that the cabinet and table are the ones the creator hashed, then read the table
	checkHash(self);
	table = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)trailer.tableSize);
	if (!table) fail(L"Setup ran out of memory.");
	readSelf(self, trailer.tableOffset, table, (DWORD)trailer.tableSize);
	CloseHandle(self);
	const WCHAR* product = tableString(0);
	const WCHAR* run = tableString(1);
	const WCHAR* identifier = tableString(2);
	const WCHAR* version = tableString(3);
	const WCHAR* publisher = tableString(4);
	const WCHAR* homepage = tableString(5);
	StringCchPrintfW(title, 256, L"%s Setup", product);

	// The install folder: local application data, which is per user and on this machine rather than roaming with a domain profile, joined with the product name, like C:\Users\name\AppData\Local\ftorrent
	PWSTR local = NULL;
	if (SHGetKnownFolderPath(&FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &local) != S_OK) fail(L"Windows did not say where local application data is.");
	StringCchCopyW(localData, PATH_SIZE, local);
	CoTaskMemFree(local);
	StringCchPrintfW(folder, PATH_SIZE, L"%s\\%s", localData, product);
	StringCchPrintfW(program, PATH_SIZE, L"%s\\%s", folder, run);

	// A trailer with a cabinet of no bytes is the uninstaller's, written by writeUninstaller
	if (trailer.cabinetSize == 0) return uninstall(product, identifier, arguments);

	closeRunning(product, identifier); // a copy already running from the folder exits before its files are replaced
	makeFolders(folder);

	// Unpack the cabinet into it. FDICopy opens the name it is given through the open callback above, then calls notify for each file
	ERF errors = {0};
	HFDI fdi = FDICreate(memoryAllocate, memoryFree, streamOpen, streamRead, streamWrite, streamClose, streamSeek, cpuUNKNOWN, &errors);
	if (!fdi) { SetLastError(0); fail(L"Setup could not start Windows' cabinet decompressor."); }
	if (!FDICopy(fdi, "cabinet", "", 0, notify, NULL, NULL)) {
		WCHAR what[256];
		StringCchPrintfW(what, 256, L"Setup could not unpack its files. Cabinet error %d.", errors.erfOper);
		SetLastError((DWORD)errors.erfType); // the system error code the decompressor saw, when it saw one
		fail(what);
	}
	FDIDestroy(fdi);

	// The uninstaller beside the program, the entry that points at it, and the Start menu shortcut, each written whether or not it was there, so an upgrade refreshes all three
	WCHAR uninstaller[PATH_SIZE];
	StringCchPrintfW(uninstaller, PATH_SIZE, L"%s\\uninstall.exe", folder);
	writeUninstaller(uninstaller);
	writeUninstallEntry(product, version, publisher, homepage, uninstaller, written + trailer.cabinetOffset + trailer.tableSize + sizeof(Trailer)); // the unpacked files plus the uninstaller, which is the stub, the table, and a trailer
	makeShortcut(product, identifier);

	// Start the program from its folder, and leave; this process has no reason to wait for it. A Windows without WebView2 gets Microsoft's page for it instead, since the program's window cannot be drawn without it
	if (!webView2Present()) {
		WCHAR what[512];
		StringCchPrintfW(what, 512, L"%s is installed, but this Windows does not have Microsoft Edge WebView2, which %s needs to show its window. Setup will open Microsoft's page for it; install it, then start %s from the Start menu.", product, product, product);
		MessageBoxW(NULL, what, title, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
		ShellExecuteW(NULL, L"open", L"https://developer.microsoft.com/microsoft-edge/webview2/", NULL, NULL, SW_SHOWNORMAL);
		return 0;
	}
	STARTUPINFOW startup = {sizeof(startup)};
	PROCESS_INFORMATION process;
	if (!CreateProcessW(program, NULL, NULL, NULL, FALSE, 0, NULL, folder, &startup, &process)) fail(L"Setup unpacked the files but could not start the program.");
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return 0;
}
