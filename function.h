
// Functions in platform paint.cpp
COLORREF ColorMix(COLORREF color1, int amount1, COLORREF color2, int amount2);
brushitem CreateBrush(COLORREF color);
HFONT CreateFont(read face, int points);
void PaintMessage(HWND window);
void PaintFill(deviceitem *device, sizeitem size, HBRUSH brush);
void PaintBorder(deviceitem *device, sizeitem size, HBRUSH brush);
void PaintText(deviceitem *device, read r, sizeitem size);

// Functions in platform string.cpp
string make(read r1 = L"", read r2 = L"", read r3 = L"", read r4 = L"", read r5 = L"", read r6 = L"", read r7 = L"", read r8 = L"", read r9 = L"");
string upper(read r);
string lower(read r);
int number(read r);
string numerals(int number);
bool is(read r);
bool isblank(read r);
bool same(read r1, read r2, matching m = Different);
int compare(read r1, read r2, matching m = Different);
bool starts(read r, read t, matching m = Different);
bool trails(read r, read t, matching m = Different);
bool has(read r, read t, matching m = Different);
int find(read r, read t, direction d = Forward, matching m = Different);
string parse(read r, read t1, read t2, matching m = Different);
string before(read r, read t, direction d = Forward, matching m = Different);
string after(read r, read t, direction d = Forward, matching m = Different);
void split(read r, read t, string *b, string *a, direction d = Forward, matching m = Different);
string replace(read r, read t1, read t2, matching m = Different);
string clip(read r, int startindex, int characters = -1);
string on(read r, read t, direction d = Forward, matching m = Different);
string off(read r, read t, direction d = Forward, matching m = Different);
string trim(read r, read t1 = L"", read t2 = L"", read t3 = L"");
string saynumber(int number, read name);
string insertcommas(read r);
string saytime(DWORD time);

// Functions in platform window.cpp
void Report(read r);
void BeginThread(LPVOID function);
void WindowTextSet(HWND window, read r);
string WindowTextGet(HWND window);
void EditAppend(HWND window, read r);
void EditScroll(HWND window);
HWND WindowCreateEdit(bool scrollbars, bool capacity);
HWND WindowCreateButton(read r);
HWND WindowCreate(read name, read title, DWORD style, int size, HWND parent, HMENU menu);
void WindowSize(HWND window, sizeitem size);
void WindowEdit(HWND window, boolean edit);
HMENU MenuLoad(read name, int index);
UINT MenuShow(HMENU menu, int x, int y);
string DialogBrowse(read display);

// Functions in user paint.cpp
void PaintCreate();
void Paint();

// Functions in user window.cpp
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR command, int show);
LRESULT CALLBACK MainWinProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

// Functions in (test).cpp
void Test();
