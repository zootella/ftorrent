
// Copyright (c) Zootella Development Team, 2005. This file is part of Zootella, http://www.zootella.com
// Zootella is free software; you can redistribute it and modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or, at your option, any later version.
// Zootella is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with Zootella; if not, go to http://www.gnu.org/copyleft/gpl.html or write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.

// When compiling this .cpp file, paste the contents of the corresponding .h file into the top of this one
#include "Create.h"

// The global handle to the main dialog box
HWND Dialog;

// Program main function
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show) {

	// Display the Setup Creator dialog box to let the user make a self extracting setup file
	DialogBox(instance, "DIALOG_CREATE", NULL, DialogProcedure);

	// Returning will end this running process
	return 0;
}

// Dialog box procedure
// Shows the Setup Creator dialog box, letting the user make a self extracting setup file
BOOL CALLBACK DialogProcedure(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {

	// Create local string objects outside the switch statement
	string icon, folder, run, save;

	// The dialog box is about to be displayed
	switch (message) {
	case WM_INITDIALOG:

		// Save the dialog box handle in the global variable
		Dialog = dialog;

		// Get text from the registry for the edit boxes
		SetDlgItemText(dialog, EDIT_ICON,   RegistryRead(HKEY_CURRENT_USER, "Software\\Zootella Setup Creator", "icon"));
		SetDlgItemText(dialog, EDIT_FOLDER, RegistryRead(HKEY_CURRENT_USER, "Software\\Zootella Setup Creator", "folder"));
		SetDlgItemText(dialog, EDIT_RUN,    RegistryRead(HKEY_CURRENT_USER, "Software\\Zootella Setup Creator", "run"));

		// Let the system place the focus
		return true;

	// The message is a command
	break;
	case WM_COMMAND:

		// The user clicked the button with the Web address
		switch (LOWORD(wparam)) {
		case BUTTON_HELP:

			// The developer wrote a secret code word in the icon box and clicked the Web address button
			if (DialogText(dialog, EDIT_ICON) == "assemble") {

				// Assemble the small extracting program into a copy of this one
				Assemble();

			// The user just clicked the button
			} else {

				// Open the Web page with help, documentation, and source code
				ShellExecute(NULL, NULL, "http://www.zootella.com/setupcreator/", "", "", SW_SHOWNORMAL);
			}

		// The user clicked Open for the icon
		break;
		case BUTTON_ICON:

			// Show the Open dialog box, and write the path the user chooses into the edit box
			icon = DialogOpen("Icons (*.ico)\0*.ico\0\0"); // Pass a pointer to a string literal here with multiple nulls
			if (icon != "") SetDlgItemText(dialog, EDIT_ICON, icon);

		// The user clicked Browse for the files
		break;
		case BUTTON_FOLDER:

			// Show the Browse for Folder dialog box, and write the path the user chooses into the edit box
			folder = DialogBrowse("Choose the folder that contains the setup files");
			if (folder != "") SetDlgItemText(dialog, EDIT_FOLDER, folder);

		// The user clicked the Create button
		break;
		case IDOK: // The OK button is titled "Create" in the dialog box

			// Get the values the user set in the dialog box
			icon   = DialogText(dialog, EDIT_ICON);   // Path to the icon to put in the self extracting executable file
			folder = DialogText(dialog, EDIT_FOLDER); // Folder where the files to compress and include are located
			run    = DialogText(dialog, EDIT_RUN);    // File name, like "setup.exe" to run once the files are decompressed

			// Save the values in the registry
			RegistryWrite(HKEY_CURRENT_USER, "Software\\Zootella Setup Creator", "icon",   DialogText(dialog, EDIT_ICON));
			RegistryWrite(HKEY_CURRENT_USER, "Software\\Zootella Setup Creator", "folder", DialogText(dialog, EDIT_FOLDER));
			RegistryWrite(HKEY_CURRENT_USER, "Software\\Zootella Setup Creator", "run",    DialogText(dialog, EDIT_RUN));

			// Show the Save As dialog box to find out where the user wants to write the file
			save = DialogSave("exe"); // Have the dialog box append ".exe" if the user doesn't type it

			// Show the Save As dialog box, and create the self extracting executable there
			if (save != "") Create(icon, folder, run, save); // If the user cancelled the Save As dialog box, save will be blank

		// The user clicked the Close button, or the X button in the title bar
		break;
		case IDCANCEL: // The cancel button is titled "Close" in the dialog box

			// Close the dialog box
			EndDialog(dialog, 0);
			return true;

		break;
		}

	break;
	}

	// Have Windows process messages that make it down here
	return false;
}

// Copies this running instance of the create program to disk, and injects the setup program into it as the resource "DATA_PROGRAM"
void Assemble() {

	// Have the user find the setup program, and choose where to save the create program with the setup program inside it
	string program = DialogOpen("Programs (*.exe)\0*.exe\0\0");
	string save = DialogSave("exe");

	// Get the path of this running instance of the create program, and copy it to the save location
	char running[MAX_PATH];
	GetModuleFileName(NULL, running, MAX_PATH);
	CopyFile(running, save, false); // False to overwrite a file already there

	// Read all the bytes of the setup program into a buffer, and inject them into the disk copy of the create program
	buffer resource;
	FileRead(&resource, program);
	ResourceWrite(save, "DATA_PROGRAM", resource.Memory, resource.Size);
}

// Takes the text the user entered into the Setup Creator dialog box
// Generates a self extracting setup program
void Create(text icon, text folder, text run, text save) {

	// Make a buffer for the not yet compressed data, and write the name to run at the start as null terminated ASCII text
	buffer data;
	data.Add((byte*)run, length(run) + 1); // Plus 1 to add the null terminator also

	// Loop through the files in the folder, adding the name, size, date, and data of each one to the buffer
	HANDLE find = NULL;
	WIN32_FIND_DATA info;
	while (FileList(folder, &find, &info)) {

		// Write the file name null terminated, size 4 bytes, date 8 bytes, and data size bytes
		data.Add((byte*)(info.cFileName), length(info.cFileName) + 1);          // Plus 1 to add the null terminator also
		data.Add((byte*)&(info.nFileSizeLow), sizeof(info.nFileSizeLow));       // Size, a 4 byte DWORD
		data.Add((byte*)&(info.ftLastWriteTime), sizeof(info.ftLastWriteTime)); // Date modified, an 8 byte FILETIME structure
		FileRead(&data, string(folder) + "\\" + info.cFileName);                // Add the file's data, size is the number of bytes
	}

	// Compose the compressed data for the resource
	buffer resource;
	resource.Add((byte*)&data.Size, sizeof(data.Size)); // Begin with the size of the data uncompressed, a 4 byte DWORD
	Compress(&resource, data.Memory, data.Size);        // Follow that with the data of the first buffer compressed

	// Pull the setup program out of a resource in this running .exe file
	buffer program;
	ResourceRead(&program, "DATA_PROGRAM");

	// Write it to disk
	DeleteFile(save); // Delete a file that's already there
	FileWrite(save, program.Memory, program.Size, NULL); // Don't change the date modified

	// If the user specified a path to a .ico file, add it as a resource to the setup program
	if (string(icon) != "") ResourceIcon(save, icon);

	// Inject the compressed data into the setup program as a raw data resource
	ResourceWrite(save, "DATA_SETUP", resource.Memory, resource.Size);
}

// Takes a pointer to memory and how many bytes are there
// Uses the ZLib library to compress the data
// Writes the compressed data into the given buffer
void Compress(buffer* b, byte* memory, DWORD size) {

	// Have ZLib guess how big this data will be compressed
	DWORD after = compressBound(size); // It just uses a mathamatical expression to guess the size

	// Prepare the given amount of space for the compressed bytes
	b->Prepare(after);

	// Have ZLib compress the data
	compress( 
		b->Write(),    // Tell ZLib where it can write
		&after,        // And how much space it has there, it changes after to report the number of bytes it wrote
		memory, size); // The data to compress

	// Tell the buffer how many bytes ZLib wrote into it
	b->Wrote(after);
}

// Takes the name of a raw data resource in this .exe file, like "DATA_SETUP"
// Reads the data into the given buffer
void ResourceRead(buffer* b, text name) {

	// Find the data resource with the given name in this .exe file
	HRSRC resource = FindResource(NULL, name, RT_RCDATA); // Look for a data resource with that name in this running .exe
	HGLOBAL global = LoadResource(NULL, resource);        // Get a pointer to its memory and find out its size
	LPVOID lock = LockResource(global);                   // It's not necessary to unlock or unload what we're getting here
	DWORD size = SizeofResource(NULL, resource);

	// Write the bytes of the resource into the given buffer
	b->Add((byte*)lock, size);
}

// Takes a path to a .exe file, a name for the new resource like "DATA_SETUP", and the data
// Writes a new raw data resource with that name and data in the file on the disk
void ResourceWrite(text path, text name, byte* memory, DWORD size) {

	// Open the .exe, write the memory in as a new raw data resource with the given name, and close it
	HANDLE update = BeginUpdateResource(path, false); // False to not clear the resources from the file
	UpdateResource(update, RT_RCDATA, name, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), memory, size);
	EndUpdateResource(update, false); // False to apply the changes to the file
}

// Takes a path to an .exe file, and a path to a .ico file
// Turns the icon into a resource, and adds it to the setup program
void ResourceIcon(text save, text icon) {

	// Read the contents of the .ico file into memory
	buffer b1;
	FileRead(&b1, icon);
	IconFileCount* file = (IconFileCount*)(b1.Memory); // It starts with the structure that tells how many icons there are

	// Make a buffer big enough to compose the data of the RT_GROUP_ICON resource
	DWORD size = sizeof(IconResourceCount) + (sizeof(IconResource) * (file->ResCount - 1)); // Minus 1 because IconResourceCount contains 1 IconResource already
	buffer b2;
	b2.Prepare(size);
	IconResourceCount* resource = (IconResourceCount*)(b2.Memory); // Look at the buffer as the resource structure

	// Copy the count information from the the file to the resource
	resource->Reserved = 0;              // Must be 0
	resource->ResType  = file->ResType;  // Resource type, 1 for icon
	resource->ResCount = file->ResCount; // The number of icons we're copying across

	// Loop through each structure that describes an icon in the .ico file, and add it to the resource buffer
	for (int i = 0; i < file->ResCount; i++) {

		// Copy the parts of the structure across
		resource->entry[i].bWidth       = file->entry[i].bWidth;
		resource->entry[i].bHeight      = file->entry[i].bHeight;
		resource->entry[i].bColorCount  = file->entry[i].bColorCount;
		resource->entry[i].bReserved    = file->entry[i].bReserved;
		resource->entry[i].wPlanes      = file->entry[i].wPlanes;
		resource->entry[i].wBitCount    = file->entry[i].wBitCount;
		resource->entry[i].dwBytesInRes = file->entry[i].dwBytesInRes;
		resource->entry[i].nID          = i; // Make the icon indices like 0, 1, 2 or 0 through 8
	}

	// Add the RT_GROUP_ICON resource with a resource identifier of 100
	HANDLE update = BeginUpdateResource(save, true);
	UpdateResource(
		update,                                    // Handle from BeginUpdateResource
		RT_GROUP_ICON,                             // Resource type, headers for a group of icons
		MAKEINTRESOURCE(100),                      // Use 100 as the resource identifier number for this icon in the .exe
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), // No language specified
		resource,                                  // Bytes of the all the headers
		size);                                     // Size of all the headers

	// Loop through each image in the .ico file and add it to the .exe as an RT_ICON resource
	for (int i = 0; i < file->ResCount; i++) {

		// Add the RT_ICON resource for this image
		UpdateResource(
			update,                                                   // Handle from BeginUpdateResource
			RT_ICON,                                                  // Resource type, icon data
			MAKEINTRESOURCE(resource->entry[i].nID),                  // Resource identifier number for this icon
			MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),                // No language specified
			(LPVOID)((DWORD_PTR)file + file->entry[i].dwImageOffset), // Calculate where in the file the data for this icon starts
			file->entry[i].dwBytesInRes);                             // How big the data for this icon is
	}

	// Done editing resources
	EndUpdateResource(update, false); // False to apply the changes to the file
}

// Takes a path to a file
// Opens it and reads all its bytes
// Writes them into the given buffer object
void FileRead(buffer* b, text path) {

	// Open the file for reading, and find out how big it is
	HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	DWORD size = GetFileSize(file, NULL);

	// Read all its contents into the given buffer
	b->Prepare(size);                              // Have the buffer object prepare this much more space at its end
	ReadFile(file, b->Write(), size, &size, NULL); // Read the bytes of the file into the new space
	b->Wrote(size);                                // Tell the buffer object how many bytes we wrote

	// Close the file
	CloseHandle(file);
}

// Takes memory, a file path, and a date
// Makes a new file at the path, writes in the memory, and sets the date modified
void FileWrite(text path, byte* memory, DWORD size, FILETIME* date) {

	// Create and open a new empty file at the given path, write its contents, and close it
	HANDLE file = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	WriteFile(file, memory, size, &size, NULL);

	// If the caller passed a pointer to a date and time, use it to set the modified date of the file
	if (date) SetFileTime(file, NULL, NULL, date); // Only set Date Modified, leave Created and Accessed unchanged and set to right now

	// Close the file
	CloseHandle(file);
}

// Takes a path to a folder without a trailing slash, and a pointer to a handle set to null
// Lists all the files in the folder, writing find data information for each one
// Returns false when there are no more files to list, and the caller should exit the loop that calls this
bool FileList(text folder, HANDLE* find, WIN32_FIND_DATA* info) {

	// Find the next item in the folder
	while (FileListAll(folder, find, info)) {

		// If it isn't a directory navigation code or a folder, return true so the caller can read the info structure about this file
		if (info->cFileName != "." && info->cFileName != ".." && (!(info->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))) return true;
	}

	// Everything in the folder has been listed
	return false;
}

// Takes a path to a folder without a trailing slash, and a pointer to a handle set to null
// Lists the contents of the folder, writing find data information for each one
// Returns false when there is nothing more to list, and the caller should exit the loop that calls this
bool FileListAll(text folder, HANDLE* find, WIN32_FIND_DATA* info) {

	// Begin, continue, or end the file find operation
	if (!*find) { *find = FindFirstFile(string(folder) + "\\*.*", info); return true; } // Begin the find
	else if (FindNextFile(*find, info)) { return true; } // Continue the find
	else { FindClose(*find); return false; } // Nothing more to list, close the find and return false
}

// Takes a root registry key handle, and a path from it
// Opens that registry key
// Returns a handle to the key
HKEY RegistryOpen(HKEY root, text path) {

	// Open or create and open the registry key, and return the open handle
	HKEY key;
	DWORD info;
	RegCreateKeyEx(root, path, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &info);
	return key;
}

// Takes a root registry key handle, a path from it, and a variable name
// Reads the text value of that variable from the registry
// Returns it
string RegistryRead(HKEY root, text path, text name) {

	// Open the registry key
	HKEY key = RegistryOpen(root, path);

	// Find out the total size of the bytes of ASCII text and null terminator in the registry
	DWORD size; // If the text is "hello", size will be 6
	RegQueryValueEx(key, name, 0, NULL, NULL, &size);

	// Open a string, read the text bytes, and close it
	string s;
	LPTSTR sbuffer = s.GetBuffer(size);
	RegQueryValueEx(key, name, 0, NULL, (LPBYTE)sbuffer, &size);
	s.ReleaseBuffer(); // The null terminator was in the registry, and copied here

	// Close the registry key and return the string
	RegCloseKey(key);
	return s;
}

// Takes a root registry key handle, a path from it, a variable name, and a text value
// Sets the registry variable to the text value
void RegistryWrite(HKEY root, text path, text name, text value) {

	// Open the registry key, set or make and set the text value, and close it
	HKEY key = RegistryOpen(root, path);
	RegSetValueEx(key, name, 0, REG_SZ, (byte*)value, length(value) + 1); // Add 1 to include the null terminator
	RegCloseKey(key);
}

// Takes a pointer to a string literal with text separated by nulls, like "Icons (*.ico)\0*.ico\0\0"
// Shows the Open dialog box to let the user find and choose a file
// Returns its path, or blank if the user clicked Cancel
string DialogOpen(text filter) {

	// Prepare the structure the call needs
	char name[MAX_PATH];
	lstrcpy(name, "");                    // The call will read this, so it must start out blank
	OPENFILENAME info;
	ZeroMemory(&info, sizeof(info));      // Zero all the bytes to not have to set things to NULL or 0
	info.lStructSize = sizeof(info);      // Size of this structure
	info.hwndOwner   = Dialog;            // Use the main dialog as the parent window
	info.lpstrFilter = filter;            // Filter text, like "Icons (*.ico)\0*.ico\0\0"
	info.Flags       = OFN_HIDEREADONLY   // Hide the Read Only check box
	                 | OFN_PATHMUSTEXIST; // Only let the user type names of paths and files that exist
	info.lpstrFile   = name;              // This is where the call writes the full path of the file the user selects
	info.nMaxFile    = MAX_PATH;

	// Show the Open system dialog box, read the path if the user clicks OK, and return it
	string s;
	if (GetOpenFileName(&info)) s = info.lpstrFile;
	return s;
}

// Takes the default extension to save without a period, like "exe"
// Shows the Save As dialog box to let the user choose where to save a file
// Returns the path, or blank if the user clicked Cancel
string DialogSave(text extension) {

	// Prepare the structure the call needs
	char name[MAX_PATH];
	lstrcpy(name, "");                      // The call will read this, so it must start out blank
	OPENFILENAME info;
	ZeroMemory(&info, sizeof(info));        // Zero all the bytes to not have to set things to NULL or 0
	info.lStructSize = sizeof(info);        // Size of this structure
	info.hwndOwner   = Dialog;              // Use the main dialog as the parent window
	info.lpstrDefExt = extension;           // Have the dialog append the extension if the user doesn't type it
	info.Flags       = OFN_HIDEREADONLY     // Hide the Read Only check box
	                 | OFN_OVERWRITEPROMPT; // Show the user the "Do you want to replace it?" message box if the file already exists
	info.lpstrFile   = name;                // This is where the call writes the full path of the file the user selects
	info.nMaxFile    = MAX_PATH;

	// Show the Save As system dialog box, read the path if the user clicks OK, and return it
	string s;
	if (GetSaveFileName(&info)) s = info.lpstrFile;
	return s;
}

// Takes text to show in the dialog box
// Displays the Browse for Folder dialog box
// Returns the path the user chose without a trailing slash, or blank if the user clicked Cancel
string DialogBrowse(text display) {

	// Prepare the structure the call needs
	char name[MAX_PATH];
	BROWSEINFO info;
	ZeroMemory(&info, sizeof(info));            // Zero all the bytes to not have to set things to NULL or 0
	info.hwndOwner      = Dialog;               // Use the main dialog as the parent window
	info.pszDisplayName = name;                 // Write the display name of the chosen folder here
	info.lpszTitle      = display;              // Text to show in the dialog box
	info.ulFlags        = BIF_RETURNONLYFSDIRS; // Only let the user choose from amongst file system folders

	// Display the Browse for Folder system dialog box
	LPITEMIDLIST result = SHBrowseForFolder(&info);
	if (!result) return string(""); // The user clicked cancel in the browse dialog

	// Get the folder path, make a new string from the text, and return it
	char bay[MAX_PATH];
	SHGetPathFromIDList(result, bay);
	return string(bay);
}

// Takes a handle to a dialog box, and a dialog box item identifier
// Gets the text from the item
// Returns it
string DialogText(HWND dialog, int item) {

	// Each dialog box item is a window itself, get a handle to this one
	HWND window = GetDlgItem(dialog, item);

	// Find out how many bytes we need to hold the ASCII text characters and a null terminator
	int size = (int)SendMessage(window, WM_GETTEXTLENGTH, 0, 0) + 1; // Add one for the null terminator

	// Open a string, read in the text, and close and return it
	string s;
	LPTSTR sbuffer = s.GetBuffer(size);
	GetWindowText(window, sbuffer, size); // Writes a null terminator at the end of the buffer
	s.ReleaseBuffer();
	return s;
}
