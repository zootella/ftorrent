
// Program build and name
#define PROGRAM_TEST    true
#define PROGRAM_NAME    CString(L"ftorrent")
#define PROGRAM_HELP    CString(L"http://ftorrent.com/help")
#define PROGRAM_VERSION 0, 1, 0, 0

// Compose display text
#define PROGRAM_ABOUT1 CString(L"Version 0.1")
#define PROGRAM_ABOUT2 CString(L"Released under the GNU General Public License")
#define PROGRAM_ABOUT3 CString(L"Built on Rasterbar libtorrent 0.15.4, Boost 1.45.0, and OpenSSL 1.0.0c")
#define PROGRAM_ABOUT4 CString(L"http://ftorrent.com")

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
typedef          __int64       sbig; // Signed 8 byte integer, same as "long long"
typedef unsigned __int64       ubig; // Unsigned 8 byte integer, same as "ULONGLONG"
typedef libtorrent::big_number hbig; // 20 byte SHA1 hash value, same as libtorrent::big_number, sha1_hash and peer_id

// Text define
#define length lstrlenW // Rename function

// Text types
typedef const wchar_t *read; // read is a pointer to wide characters and a null terminator that will only be read

// Text options
enum direction {Forward,   Reverse};  // Default forward and the start, or reverse and the end
enum matching  {Different, Matching}; // Default case sensitive, or case insensitive matching

// Exit times
#define EXIT_TIME_LIMIT 2 // Wait this many seconds for trackers to respond to our goodbye before exiting the process

// Web download limits
#define WEB_BAY_SIZE          1 * 1024 // 1 KB download bay, in bytes
#define WEB_SIZE_LIMIT 4 * 1024 * 1024 // 4 MB download size limit, in bytes
#define WEB_TIME_LIMIT        4 * 1000 // 4 second download time limit, in milliseconds
