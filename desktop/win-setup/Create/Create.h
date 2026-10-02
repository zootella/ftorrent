
// Copyright (c) Zootella Development Team, 2005. This file is part of Zootella, http://www.zootella.com
// Zootella is free software; you can redistribute it and modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or, at your option, any later version.
// Zootella is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with Zootella; if not, go to http://www.gnu.org/copyleft/gpl.html or write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.

// Include Windows and shell definitions, the MFC ATL CString type, the ZLib compression library, and resource numbers
#include <windows.h>
#include <shlobj.h>
#include <atlstr.h>
#include "..\ZLib\zlib.h"
#include "Resource.h"

// Structures that define icon data formats in .ico files and program resources
#pragma pack(push) // Save the current packing alignment, so we can back to it later
#pragma pack(2)    // Tell the compiler to align the members of these structures on 2-byte boundaries

// The structure in a .ico file that describes one icon
typedef struct {

	BYTE  bWidth;        // Width of the icon in pixels
	BYTE  bHeight;       // Height of the icon in pixels
	BYTE  bColorCount;   // Number of colors in the icon
	BYTE  bReserved;     // Must be 0
	WORD  wPlanes;       // Color planes
	WORD  wBitCount;     // Bits per pixel
	DWORD dwBytesInRes;  // Resource size in bytes
	DWORD dwImageOffset; // Location in the file where this icon starts, cast to DWORD_PTR to use

} IconFile;

// The structure at the start of a .ico file that describes all the icons there
typedef struct {

	WORD     Reserved; // Must be 0
	WORD     ResType;  // Resource type, either icon 1 or cursor 2
	WORD     ResCount; // Number of icons
	IconFile entry[1]; // After that, there is a structure for each icon

} IconFileCount;

// The structure in a RT_GROUP_ICON resource that describes one icon
typedef struct {

	BYTE  bWidth;       // Width of the icon in pixels
	BYTE  bHeight;      // Height of the icon in pixels
	BYTE  bColorCount;  // Number of colors in the icon
	BYTE  bReserved;    // Must be 0
	WORD  wPlanes;      // Color planes
	WORD  wBitCount;    // Bits per pixel
	DWORD dwBytesInRes; // Resource size in bytes
	WORD  nID;          // Resource identifier where you can find the icon resource this structure describes

} IconResource;

// The structure at the start of a RT_GROUP_ICON resource that describes all the icons there
typedef struct {

	WORD         Reserved; // Must be 0
	WORD         ResType;  // Resource type, either icon 1 or cursor 2
	WORD         ResCount; // Number of icons
	IconResource entry[1]; // After that, there is a structure for each icon

} IconResourceCount;

// Return to the previous packing alignment that we saved
#pragma pack(pop)

// Give string and text types better names
typedef CString string;  // Instead of typing CString, we can just type string
typedef LPCTSTR text;    // A pointer to characters and a null terminator that can only be read
#define length  lstrlen  // The number of characters in a string or text literal, not including the null terminator

// A buffer object holds some memory we can write data to, and frees it when it goes out of scope
class buffer {
public:

	// Memory pointer and byte counts
	byte* Memory;           // Pointer to the allocated block of memory
	DWORD Size, MemorySize; // The number of bytes written in the memory block, and its allocated size

	// Make a new buffer object, then let it go out of scope to have it free its memory
	buffer() { Memory = NULL; Size = MemorySize = 0; } // Set member variables to null and 0
	~buffer() { free(Memory); }                        // Free the allocated memory block

	// Takes a pointer to data in memory, and the number of bytes we can read there
	// Adds it to the end of the memory in this buffer object
	void Add(byte* memory, DWORD size) {

		// Make space at the end of the buffer, and copy the new memory there
		Prepare(size);                     // Have the buffer prepare this much space at the end
		CopyMemory(Write(), memory, size); // Copy the given memory into the new empty space
		Wrote(size);                       // Record that it was filled
	}

	// Takes the number of bytes you need to write into the end of the memory block
	// Allocates enough space for them
	void Prepare(DWORD size) {

		// If there isn't enough empty space at the end of the buffer for this many more bytes
		if (MemorySize < Size + size) {

			// Make the allocated block bigger
			MemorySize = Size + size;                    // Record that the block size is larger
			Memory = (byte*)realloc(Memory, MemorySize); // Reallocate the memory block to make it this size
		}
	}

	// After calling Prepare, call Write and then Wrote
	byte* Write() { return Memory + Size; }  // Get a pointer to the start of the empty space where you can write
	void Wrote(DWORD size) { Size += size; } // Tell the buffer object how much you wrote there
};

// Functions
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show);
BOOL CALLBACK DialogProcedure(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam);
void Assemble();
void Create(text icon, text folder, text run, text save);
void Compress(buffer* b, byte* memory, DWORD size);
void ResourceRead(buffer* b, text name);
void ResourceWrite(text path, text name, byte* memory, DWORD size);
void ResourceIcon(text save, text icon);
void FileRead(buffer* b, text path);
void FileWrite(text path, byte* memory, DWORD size, FILETIME* date);
bool FileList(text folder, HANDLE* find, WIN32_FIND_DATA* info);
bool FileListAll(text folder, HANDLE* find, WIN32_FIND_DATA* info);
HKEY RegistryOpen(HKEY root, text path);
string RegistryRead(HKEY root, text path, text name);
void RegistryWrite(HKEY root, text path, text name, text value);
string DialogOpen(text filter);
string DialogSave(text extension);
string DialogBrowse(text display);
string DialogText(HWND dialog, int item);
