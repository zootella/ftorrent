
// Copyright (c) Zootella Development Team, 2005. This file is part of Zootella, http://www.zootella.com
// Zootella is free software; you can redistribute it and modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or, at your option, any later version.
// Zootella is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with Zootella; if not, go to http://www.gnu.org/copyleft/gpl.html or write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.

// When compiling this .cpp file, paste the contents of the corresponding .h file into the top of this one
#include "Setup.h"

// Program main function
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show) {

	// Compose the path to the new empty folder where we will extract the files
	char folder[MAX_PATH], unique[MAX_PATH];
	ShellPath(folder, CSIDL_APPDATA);       // Make folder like "C:\Documents and Settings\User\Application Data", or blank if not found
	wsprintf(unique, "%x", GetTickCount()); // Make unique like "92424a9"
	if (!lstrlen(folder)) { lstrcpy(folder, "C:\\"); lstrcat(folder, unique); } // If the application data folder was not found, make folder like "C:\92424a9"
	else                  { lstrcat(folder, "\\");   lstrcat(folder, unique); } // Make the folder like "C:\Documents and Settings\User\Application Data\92424a9"

	// Create the new empty folder
	CreateDirectory(folder, NULL);

	// Extract the compressed setup data from the "DATA_SETUP" resource in this running instance of the setup program
	buffer resource;
	ResourceRead(&resource, "DATA_SETUP");

	// Make a pointer and byte count we can move across the contents of buffers
	byte* p = resource.Memory;
	DWORD n = resource.Size;

	// Read how many bytes the data will be decompressed
	DWORD after = *((DWORD*)p); // The first 4 bytes of the resource data tell how big the rest will be decompressed
	p += sizeof(after);         // Move past that DWORD to the start of the compressed data
	n -= sizeof(after);

	// Decompress the data
	buffer data;
	Decompress(&data, p, n, after);
	p = data.Memory; // Now, point p and set n on the start of the decompressed data
	n = data.Size;

	// Read the name of the setup program we will run at the end
	char run[MAX_PATH];
	lstrcpy(run, (char*)p);
	p += (lstrlen(run) + 1); // Move past the ASCII text and null terminator
	n -= (lstrlen(run) + 1);

	// Loop to get information about each file, and create it
	char name[MAX_PATH]; // File name
	DWORD size;          // Size of the file in bytes
	FILETIME date;       // Date modified
	char path[MAX_PATH]; // Complete path for the file
	while (n) {          // Loop until there are no more bytes to process

		// Read the file name, size, date, and create it with its data
		lstrcpy(name, (char*)p); p += (lstrlen(name) + 1); n -= (lstrlen(name) + 1); // Move past the ASCII text and null terminator
		size = *((DWORD*)p);     p += sizeof(size);        n -= sizeof(size);        // Move past the 4 byte DWORD
		date = *((FILETIME*)p);  p += sizeof(date);        n -= sizeof(date);        // Move past the 8 byte FILETIME structure

		// Compose the path for the file, and write it there
		lstrcpy(path, folder);
		lstrcat(path, "\\");
		lstrcat(path, name);
		FileWrite(path, p, size, &date);
		p += size; // Move past the bytes of the file
		n -= size;
	}

	// Compose the path to the setup program, and run it
	lstrcat(folder, "\\");
	lstrcat(folder, run);
	ShellExecute(NULL, NULL, folder, "", "", SW_SHOWNORMAL);

	// Returning will end this running process
	return 0;
}

// Takes a special folder id, like CSIDL_APPDATA
// Finds the path of that folder on this computer
// Writes the path, which does not have a trailing slash
void ShellPath(char* path, int folder) {

	// Get memory
	LPMALLOC memory;
	if (SHGetMalloc(&memory) == NOERROR) {

		// Get list
		LPITEMIDLIST list;
		if (SHGetSpecialFolderLocation(NULL, folder, &list) == NOERROR) {

			// Get path
			if (!SHGetPathFromIDList(list, path)) lstrcpy(path, ""); // Only keep the text if the call works

			// Free list
			memory->Free(list);
		}

		// Free memory
		memory->Release();
	}
}

// Takes the name of a raw data resource in this .exe file, like "DATA_SETUP"
// Reads the data into the given buffer
void ResourceRead(buffer* b, char* name) {

	// Find the data resource with the given name in this .exe file
	HRSRC resource = FindResource(NULL, name, RT_RCDATA); // Look for a data resource with that name in this running .exe
	HGLOBAL global = LoadResource(NULL, resource);        // Get a pointer to its memory and find out its size
	LPVOID lock = LockResource(global);                   // It's not necessary to unlock or unload what we're getting here
	DWORD size = SizeofResource(NULL, resource);

	// Write the bytes of the resource into the given buffer
	b->Add((byte*)lock, size);
}

// Takes a pointer and the number of bytes we can read there, and how big it will be after decompression
// Uses the ZLib library to decompress the data
// Writes the decompressed data into the given buffer
void Decompress(buffer *b, byte* memory, DWORD size, DWORD after) {

	// Prepare the given amount of space for the decompressed bytes
	b->Prepare(after);

	// Have ZLib decompress the data
	uncompress(
		b->Write(),    // Tell ZLib where it can write
		&after,        // And how much space it has there, it changes after to report the number of bytes it wrote
		memory, size); // The data to decompress

	// Tell the buffer how many bytes ZLib wrote into it
	b->Wrote(after);
}

// Takes memory, a file path, and a date
// Makes a new file at the path, writes in the memory, and sets the date modified
void FileWrite(char* path, byte* memory, DWORD size, FILETIME* date) {

	// Create and open a new empty file at the given path, write its contents, and close it
	HANDLE file = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	WriteFile(file, memory, size, &size, NULL);

	// If the caller passed a pointer to a date and time, use it to set the modified date of the file
	if (date) SetFileTime(file, NULL, NULL, date); // Only set Date Modified, leave Created and Accessed unchanged and set to right now

	// Close the file
	CloseHandle(file);
}
