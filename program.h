
// Program build and name
#define PROGRAM_TEST true
#define PROGRAM_NAME string(L"ltorrent")

// Program settings
#define PULSE 100

// Timer
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
