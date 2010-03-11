
// String
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

// Utility
void Report(read r);
void CloseHandleSafely(HANDLE *handle);
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
string DialogBrowse(read display);
COLORREF ColorMix(COLORREF color1, int amount1, COLORREF color2, int amount2);
brushitem CreateBrush(COLORREF color);
void PaintMessage(HWND window);
void TaskbarIconAdd();
void TaskbarIconRemove();
HCURSOR LoadSharedCursor(read name);
HICON LoadIconResource(read name, int size);
void CursorSet(HCURSOR cursor);
HMENU MenuLoad(read name);
HMENU MenuClip(HMENU menus, int index);
void MenuSet(HMENU menu, UINT command, UINT state, HBITMAP bitmap);
UINT MenuShow(HMENU menu, bool taskbar, sizeitem *size);
void MouseCapture(HWND window = NULL);
void MouseRelease(HWND window = NULL);
bool MouseInside();
areaitem *MouseOver();
sizeitem MouseArea(areaitem *a);
sizeitem MouseClient(HWND window = NULL);
sizeitem MouseScreen();
brushitem BrushSystem(int color);
brushitem BrushColor(COLORREF color);
COLORREF MixColors(COLORREF color1, int amount1, COLORREF color2, int amount2);
sizeitem SizeClient(HWND window = NULL);
sizeitem SizeWindow(HWND window = NULL);
void TipAdd(sizeitem size, read r);
sizeitem SizeText(deviceitem *device, read r);
void PaintText(deviceitem *device, read r, sizeitem size, bool horizontal = false, bool vertical = false, bool left = false, bool right = false, int adjust = 0, HFONT font = NULL, brushitem *color = NULL, brushitem *background = NULL);
void PaintFill(deviceitem *device, sizeitem size, HBRUSH brush = NULL);
void PaintBorder(deviceitem *device, sizeitem size, HBRUSH brush1, HBRUSH brush2);
void PaintIcon(deviceitem *device, sizeitem position, HICON icon);
void WindowSize(HWND window, int x, int y);
void WindowMove(HWND window, sizeitem size, bool paint);
HFONT FontMenu(boolean underline);
HFONT FontName(read face, int points);
int Greatest(int i1 = 0, int i2 = 0, int i3 = 0, int i4 = 0, int i5 = 0, int i6 = 0, int i7 = 0, int i8 = 0);

// Paint
void PaintCreate();
void Paint();
void PaintLoad();
void PaintWindow(deviceitem *device);
void PaintArea(deviceitem *device, areaitem *a);
bool PaintCustom(LPNMLVCUSTOMDRAW draw);
void PaintProgress(deviceitem *device, sizeitem bound, COLORREF foreground, COLORREF background, read r);

// Window
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR command, int show);
LRESULT CALLBACK MainWinProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
void MenuTaskbar();

// Area
void AreaCreate();
void AreaPulse();
void AreaPopUp();
void AreaPopDown();
void SizeColumns(int *width1, int *width2, int *width3, int *width4, int *width5, int *width6);
void Size(int move);

// Test
void Test();
