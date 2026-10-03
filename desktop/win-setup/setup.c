// The setup program a user runs: one executable with a cabinet of files and a small table appended to its end, which it unpacks into the user's own application data folder and then starts the program it brought. It descends from the 2005 Zootella Setup Creator's Setup.cpp, which did the same with a zlib stream in a resource and ANSI strings, and keeps its stance: no window, no questions, nothing to click. The only interface is a dialog box when something goes wrong, so a user who sees nothing can trust that nothing went wrong.

// Everything it needs it carries. The creator, win-setup.js, builds a cabinet with Windows' own makecab, writes a table of UTF-16 strings, and appends cabinet, table, and a trailer to the compiled stub; the trailer says where the two are and holds a SHA-256 of both, which this program checks before it writes a single file. Windows itself does the unpacking, through the cabinet file decompression interface in cabinet.dll that Windows Update uses for its own packages, so no compression code is compiled in here. Everything is per user: the folder is under local application data, found by asking the shell rather than by building a path, and nothing here ever elevates, which the manifest beside this file also declares.

#define _WIN32_WINNT 0x0A00 // Windows 10 and later, which declares SetDefaultDllDirectories and SHGetKnownFolderPath
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>   // SHGetKnownFolderPath and FOLDERID_LocalAppData, the per-user folder where the program is installed
#include <bcrypt.h>   // SHA-256, to check the appended data before trusting it
#pragma warning(push)
#pragma warning(disable: 4201) // fdi.h declares a nameless union, which warning level 4 flags as a nonstandard extension
#include <fdi.h>      // the cabinet file decompression interface, FDICreate and FDICopy, which hands each file in the cabinet to a callback
#pragma warning(pop)
#include <strsafe.h>  // StringCchCopyW and friends, string functions that take the buffer's size and cannot run past it

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
static WCHAR folder[PATH_SIZE];     // the folder the files are unpacked into, local application data joined with the product name
static WCHAR title[256];            // the title of the failure dialog, the product name and the word Setup
static Trailer trailer;
static BYTE* table;                 // the table, read whole into memory; it is a few hundred bytes

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

// Check the SHA-256 in the trailer against the cabinet and table as they sit in the file, so a download cut short or a file altered after it was built stops here, before anything is written. The hash is Windows' own, through the cryptography API in bcrypt.dll.
static void checkHash(HANDLE file) {
	BCRYPT_ALG_HANDLE algorithm = NULL;
	BCRYPT_HASH_HANDLE hash = NULL;
	if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) != 0 || BCryptCreateHash(algorithm, &hash, NULL, 0, NULL, 0, 0) != 0) fail(L"Setup could not start the hash check.");
	BYTE* buffer = HeapAlloc(GetProcessHeap(), 0, 1 << 20); // a megabyte at a time, since the cabinet is tens of megabytes
	if (!buffer) fail(L"Setup ran out of memory.");
	UINT64 runs[2][2] = {{trailer.cabinetOffset, trailer.cabinetSize}, {trailer.tableOffset, trailer.tableSize}}; // the cabinet, then the table, in that order, as the creator hashed them
	for (int run = 0; run < 2; run++) {
		UINT64 offset = runs[run][0], left = runs[run][1];
		while (left) {
			DWORD piece = left > (1 << 20) ? (1 << 20) : (DWORD)left;
			readSelf(file, offset, buffer, piece);
			if (BCryptHashData(hash, buffer, piece, 0) != 0) fail(L"Setup could not hash its own file.");
			offset += piece;
			left -= piece;
		}
	}
	BYTE digest[32];
	if (BCryptFinishHash(hash, digest, 32, 0) != 0) fail(L"Setup could not finish the hash check.");
	BCryptDestroyHash(hash);
	BCryptCloseAlgorithmProvider(algorithm, 0);
	HeapFree(GetProcessHeap(), 0, buffer);
	if (memcmp(digest, trailer.hash, 32) != 0) { SetLastError(0); fail(L"This copy of Setup is damaged or incomplete. Download it again."); }
}

// The strings in the table, by position. The creator writes them in this order, and a later addition goes on the end, so an older stub given a newer table still finds what it knows.
// 0  the product name, which names the folder under local application data and appears in the failure dialog's title
// 1  the program to start when the files are in place, as a path relative to that folder, like ftorrent.exe
static const WCHAR* tableString(int index) {
	const WCHAR* at = (const WCHAR*)table;
	const WCHAR* end = (const WCHAR*)(table + trailer.tableSize);
	for (int i = 0; at < end; i++) {
		const WCHAR* start = at;
		while (at < end && *at) at++; // to this string's null
		if (at >= end) break;         // a string that never ended: the table is malformed
		if (i == index) return start;
		at++;                         // past the null, to the next string
	}
	SetLastError(0);
	fail(L"Setup's table is missing a value it needs.");
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

// Write to a file being unpacked
static FNWRITE(streamWrite) {
	Stream* s = (Stream*)hf;
	DWORD put = 0;
	if (!WriteFile(s->file, pv, cb, &put, NULL)) return (UINT)-1;
	s->position += put;
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

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR arguments, int show) {
	UNREFERENCED_PARAMETER(instance); UNREFERENCED_PARAMETER(previous); UNREFERENCED_PARAMETER(arguments); UNREFERENCED_PARAMETER(show);

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
	StringCchPrintfW(title, 256, L"%s Setup", product);

	// The install folder: local application data, which is per user and on this machine rather than roaming with a domain profile, joined with the product name, like C:\Users\name\AppData\Local\ftorrent
	PWSTR local = NULL;
	if (SHGetKnownFolderPath(&FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &local) != S_OK) fail(L"Windows did not say where local application data is.");
	StringCchPrintfW(folder, PATH_SIZE, L"%s\\%s", local, product);
	CoTaskMemFree(local);
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

	// Start the program from its folder, and leave; this process has no reason to wait for it
	WCHAR program[PATH_SIZE];
	StringCchPrintfW(program, PATH_SIZE, L"%s\\%s", folder, run);
	STARTUPINFOW startup = {sizeof(startup)};
	PROCESS_INFORMATION process;
	if (!CreateProcessW(program, NULL, NULL, NULL, FALSE, 0, NULL, folder, &startup, &process)) fail(L"Setup unpacked the files but could not start the program.");
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return 0;
}
