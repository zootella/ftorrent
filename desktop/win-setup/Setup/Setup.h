
// Copyright (c) Zootella Development Team, 2005. This file is part of Zootella, http://www.zootella.com
// Zootella is free software; you can redistribute it and modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or, at your option, any later version.
// Zootella is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with Zootella; if not, go to http://www.gnu.org/copyleft/gpl.html or write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.

// Include Windows and shell definitions, and the ZLib compression library
#include <windows.h>
#include <shlobj.h>
#include "..\ZLib\zlib.h"

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
void ShellPath(char* path, int folder);
void ResourceRead(buffer* b, char* name);
void Decompress(buffer *b, byte* memory, DWORD size, DWORD after);
void FileWrite(char* path, byte* memory, DWORD size, FILETIME* date);
