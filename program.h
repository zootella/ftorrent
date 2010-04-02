
// Program build and name
#define PROGRAM_TEST true
#define PROGRAM_NAME string(L"ltorrent")

// Compose display text
#define PROGRAM_ABOUT1 string(L"ltorrent 0.1")
#define PROGRAM_ABOUT2 string(L"Released under the GNU General Public License")
#define PROGRAM_ABOUT3 string(L"Rasterbar libtorrent 0.14.8")
#define PROGRAM_ABOUT4 string(L"Boost 1.42.0")
#define PROGRAM_ABOUT5 string(L"OpenSSL 0.9.8l")
#define PROGRAM_ABOUT6 string(L"http://github.com/zootella/ltorrent")

// Pulse
#define PULSE 100
#define TIMER_PULSE 1

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
