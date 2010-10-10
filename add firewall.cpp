
// Include libtorrent
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/alert_types.hpp"

// Include platform
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>
#include <netfw.h>

// Include program
#include "resource.h"
#include "define.h"
#include "object.h"
#include "library.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;

// Determines if this copy of Windows has Windows Firewall
// Returns true if it does, false if not or there was an error
bool FirewallPresent() {

	// Make a Windows Firewall object and have it access the COM interfaces of Windows Firewall
	firewallitem firewall;
	if (!firewall.Access()) return false;
	return true;
}

// Determines if Windows Firewall is off or on
// Returns true if the firewall is on, false if it's off or there was an error
bool FirewallEnabled() {

	// Make a Windows Firewall object and have it access the COM interfaces of Windows Firewall
	firewallitem firewall;
	if (!firewall.Access()) return false;

	// Determine if Windows Firewall is off or on
	bool enabled = false;
	if (!firewall.FirewallEnabled(&enabled)) return false;
	return enabled;
}

// Determines if the Exceptions not allowed check box in Windows Firewall is checked
// Returns true if the exceptions not allowed box is checked, false if it's not checked or there was an error
bool FirewallExceptionsNotAllowed() {

	// Make a Windows Firewall object and have it access the COM interfaces of Windows Firewall
	firewallitem firewall;
	if (!firewall.Access()) return false;

	// Determine if the Exceptions not allowed box is checked
	bool notallowed = false;
	if (!firewall.ExceptionsNotAllowed(&notallowed)) return false;
	return notallowed;
}

// Takes a program path and file name, like "C:\Folder\Program.exe"
// Determines if it's listed in Windows Firewall
// Returns true if is listed, false if it's not or there was an error
bool FirewallProgramListed(read path) {

	// Make a Windows Firewall object and have it access the COM interfaces of Windows Firewall
	firewallitem firewall;
	if (!firewall.Access()) return false;

	// Determine if the program has a listing on the Exceptions tab
	bool listed = false;
	if (!firewall.ProgramListed(path, &listed)) return false;
	return listed;
}

// Takes a program path and file name like "C:\Folder\Program.exe"
// Determines if the listing for that program in Windows Firewall is checked or unchecked
// Returns true if it is enabled, false if it's not or there was an error
bool FirewallProgramEnabled(read path) {

	// Make a Windows Firewall object and have it access the COM interfaces of Windows Firewall
	firewallitem firewall;
	if (!firewall.Access()) return false;

	// Determine if the program has a listing on the Exceptions tab, and that listing is checked
	bool enabled = false;
	if (!firewall.ProgramEnabled(path, &enabled)) return false;
	return enabled;
}

// Takes a path like "C:\Folder\Program.exe" and a name like "My Program"
// Adds the program's listing in Windows Firewall to make sure it is listed and checked
// Returns false on error
bool FirewallAdd(read path, read name) {

	// Make a Windows Firewall object and have it access the COM interfaces of Windows Firewall
	firewallitem firewall;
	if (!firewall.Access()) return false;

	// Add the program's listing
	if (!firewall.AddProgram(path, name)) return false;
	return true;
}

// Takes a path and file name like "C:\Folder\Program.exe"
// Removes the program's listing from the Windows Firewall exceptions list
// Returns false on error
bool FirewallRemove(read path) {

	// Make a Windows Firewall object and have it access the COM interfaces of Windows Firewall
	firewallitem firewall;
	if (!firewall.Access()) return false;

	// Remove the program's listing
	if (!firewall.RemoveProgram(path)) return false;
	return true;
}

// Get access to the COM objects
// Returns true if it works, false if there was an error
bool firewallitem::Access() {

	// Initialize COM itself so this thread can use it
	HRESULT result = CoInitialize(NULL); // Must be NULL
	if (FAILED(result)) return false;

	// Create an instance of the firewall settings manager
	result = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void **)&manager);
	if (FAILED(result) || !manager) return false;

	// Retrieve the local firewall policy
	result = manager->get_LocalPolicy(&policy);
	if (FAILED(result) || !policy) return false;

	// Retrieve the firewall profile currently in effect
	result = policy->get_CurrentProfile(&profile);
	if (FAILED(result) || !profile) return false;

	// Retrieve the authorized application collection
	result = profile->get_AuthorizedApplications(&list);
	if (FAILED(result) || !list) return false;

	// Everything worked
	return true;
}

// Determines if Windows Firewall is off or on
// Returns true if it works, and writes the answer in enabled
bool firewallitem::FirewallEnabled(bool *enabled) {

	// Find out if the firewall is enabled
	VARIANT_BOOL v;
	HRESULT result = profile->get_FirewallEnabled(&v);
	if (FAILED(result)) return false;
	if (v == VARIANT_FALSE) {

		// The Windows Firewall setting is "Off (not recommended)"
		*enabled = false;
		return true;

	} else {

		// The Windows Firewall setting is "On (recommended)"
		*enabled = true;
		return true;
	}
}

// Determines if the Exceptions not allowed check box is checked
// Returns true if it works, and writes the answer in enabled
bool firewallitem::ExceptionsNotAllowed(bool *notallowed) {

	// Find out if the exceptions box is checked
	VARIANT_BOOL v;
	HRESULT result = profile->get_ExceptionsNotAllowed(&v);
	if (FAILED(result)) return false;
	if (v == VARIANT_FALSE) {

		// The "Don't allow exceptions" box is checked
		*notallowed = false;
		return true;

	} else {

		// The "Don't allow exceptions" box is not checked
		*notallowed = true;
		return true;
	}
}

// Takes a program path and file name, like "C:\Folder\Program.exe"
// Determines if it's listed in Windows Firewall
// Returns true if it works, and writes the answer in listed
bool firewallitem::ProgramListed(read path, bool *listed) {

	// Look for the program in the list
	if (program) { program->Release(); program = NULL; }
	bstritem p(path); // Express the name as a BSTR
	HRESULT result = list->Item(p.bstr, &program); // Try to get the interface for the program with the given name
	if (SUCCEEDED(result)) {

		// The program is in the list
		*listed = true;
		return true;

	// The list->Item call failed
	} else {

		// The error is not found
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {

			// The program is not in the list
			*listed = false;
			return true;

		// Some other error occurred
		} else {

			// Report it
			return false;
		}
	}
}

// Takes a program path and file name like "C:\Folder\Program.exe"
// Determines if the listing for that program in Windows Firewall is checked or unchecked
// Returns true if it works, and writes the answer in enabled
bool firewallitem::ProgramEnabled(read path, bool *enabled) {

	// First, make sure the program is listed
	bool listed;
	if (!ProgramListed(path, &listed)) return false; // This sets the program interface we can use here
	if (!listed) return false; // The program isn't in the list at all

	// Find out if the program is enabled
	VARIANT_BOOL v;
	HRESULT result = program->get_Enabled(&v);
	if (FAILED(result)) return false;
	if (v == VARIANT_FALSE) {

		// The program is on the list, but the checkbox next to it is cleared
		*enabled = false;
		return true;

	} else {

		// The program is on the list and the checkbox is checked
		*enabled = true;
		return true;
	}
}

// Takes a path and file name like "C:\Folder\Program.exe" and a name like "My Program"
// Lists and checks the program on Windows Firewall, so now it can listed on a socket without a warning popping up
// Returns false on error
bool firewallitem::AddProgram(read path, read name) {

	// Create an instance of an authorized application, we'll use this to add our new application
	if (program) { program->Release(); program = NULL; }
	HRESULT result = CoCreateInstance(__uuidof(NetFwAuthorizedApplication), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwAuthorizedApplication), (void **)&program);
	if (FAILED(result)) return false;

	// Set the text
	bstritem p(path);                                   // Express the text as BSTRs
	result = program->put_ProcessImageFileName(p.bstr); // Set the process image file name
	if (FAILED(result)) return false;
	bstritem n(name);
	result = program->put_Name(n.bstr);                 // Set the program name
	if (FAILED(result)) return false;

	// Get the program on the Windows Firewall exceptions list
	result = list->Add(program); // Add the application to the collection
	if (FAILED(result)) return false;
	return true;
}

// Takes a program path and file name like "C:\Folder\Program.exe"
// Checks the checkbox next to its listing in Windows Firewall
// Returns false on error
bool firewallitem::EnableProgram(read path) {

	// First, make sure the program is listed
	bool listed;
	if (!ProgramListed(path, &listed)) return false; // This sets the program interface we can use here
	if (!listed) return false; // The program isn't on the list at all

	// Check the box next to the program
	VARIANT_BOOL v = true;
	HRESULT result = program->put_Enabled(v);
	if (FAILED(result)) return false;
	return true;
}

// Takes a path like "C:\Folder\Program.exe"
// Removes the program from Windows Firewall
// Returns false on error
bool firewallitem::RemoveProgram(read path) {

	// Remove the program from the Windows Firewall exceptions list
	bstritem p(path); // Express the text as a BSTR
	HRESULT result = list->Remove(p.bstr);
	if (FAILED(result)) return false;
	return true;
}
