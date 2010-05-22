
// Program build and name
#define PROGRAM_TEST true
#define PROGRAM_NAME string(L"ltorrent")
#define PROGRAM_HELP string(L"http://wiki.github.com/zootella/ltorrent/")

// Compose display text
#define PROGRAM_ABOUT1 string(L"Version 0.1")
#define PROGRAM_ABOUT2 string(L"Released under the GNU General Public License")
#define PROGRAM_ABOUT3 string(L"Built on Rasterbar libtorrent 0.15.0, Boost 1.43.0, and OpenSSL 0.9.8n")
#define PROGRAM_ABOUT4 string(L"http://github.com/zootella/ltorrent")

// Windows
#define WINDOW_LIST 1
#define WINDOW_TABS 2

// Our only icon in the taskbar notification area
#define TASKBAR_ICON 0

// Pulse
#define PULSE 100
#define TIMER_PULSE 1

// Image list
#define ICON_CAPACITY 128 // The program image list can hold 128 icons

// Messages
#define MESSAGE_TASKBAR (WM_APP + 1)

// Number types
typedef unsigned __int64 big;

// Text define
#define length lstrlenW // Rename function

// Text types
typedef CString string;    // string is a CString object
typedef LPCWSTR read;      // read is a pointer to characters and a null terminator that will only be read
typedef LPWSTR  write;     // write is a pointer to a character buffer
typedef WCHAR   character; // character is a single character or characters in a buffer

// Text options
enum direction {Forward,   Reverse};  // Default forward and the start, or reverse and the end
enum matching  {Different, Matching}; // Default case sensitive, or case insensitive matching
