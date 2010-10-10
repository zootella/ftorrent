
// Windows Firewall, only available in Windows XP Service Pack 2 or later
#include <netfw.h>

// Wraps a BSTR, a COM string, taking care of memory allocation
class CBstr {
public:

	// The BSTR
	BSTR B;

	// Make a new CBstr object
	CBstr()       { B = NULL; }         // With no BSTR allocated
	CBstr(read r) { B = NULL; Set(r); } // From the given text
	~CBstr()      { Clear(); }          // It frees its memory when you delete it

	// Use AllocSysString and SysFreeString to allocate and free the BSTR
	void Set(CString s) { Clear(); B = s.AllocSysString(); }
	void Clear() { if (B) { SysFreeString(B); B = NULL; } }
};

// Add an application to the Windows Firewall exceptions list
class CWindowsFirewall {
public:

	// COM interfaces
	INetFwMgr*                    Manager;
	INetFwPolicy*                 Policy;
	INetFwProfile*                Profile;
	INetFwAuthorizedApplications* ProgramList;
	INetFwAuthorizedApplication*  Program;

	// Make a new CWindowsFirewall object
	CWindowsFirewall() {

		// Set the COM interface pointers to NULL so we'll know if we've initialized them
		Manager     = NULL;
		Policy      = NULL;
		Profile     = NULL;
		ProgramList = NULL;
		Program     = NULL;
	}

	// Delete the CWindowsFirewall object
	~CWindowsFirewall() {

		// Release the COM interfaces that we got access to
		if (Program)     { Program->Release();     Program     = NULL; } // Release them in reverse order
		if (ProgramList) { ProgramList->Release(); ProgramList = NULL; }
		if (Profile)     { Profile->Release();     Profile     = NULL; }
		if (Policy)      { Policy->Release();      Policy      = NULL; }
		if (Manager)     { Manager->Release();     Manager     = NULL; }
	}

	// Methods to adjust the settings of Windows Firewall
	bool Access();
	bool FirewallEnabled(bool *enabled);
	bool ExceptionsNotAllowed(bool *notallowed);
	bool IsProgramListed(read path, bool *listed);
	bool IsProgramEnabled(read path, bool *enabled);
	bool AddProgram(read path, read name);
	bool EnableProgram(read path);
	bool RemoveProgram(read path);
};

// Functions in Firewall.cpp
bool WindowsFirewallPresent();
bool WindowsFirewallEnabled();
bool WindowsFirewallExceptionsNotAllowed();
bool WindowsFirewallIsProgramListed(read path);
bool WindowsFirewallIsProgramEnabled(read path);
bool WindowsFirewallAdd(read path, read name);
bool WindowsFirewallRemove(read path);
