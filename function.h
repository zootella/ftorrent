
// String
string make(read r1 = L"", read r2 = L"", read r3 = L"", read r4 = L"", read r5 = L"", read r6 = L"", read r7 = L"", read r8 = L"", read r9 = L""); // Has defaults
string upper(read r);
string lower(read r);
int number(read r);
string numerals(int number);
bool is(read r);
bool isblank(read r);
bool same(read r1, read r2, matching m = Different); // Has defaults
int compare(read r1, read r2, matching m = Different); // Has defaults
bool starts(read r, read t, matching m = Different); // Has defaults
bool trails(read r, read t, matching m = Different); // Has defaults
bool has(read r, read t, matching m = Different); // Has defaults
int find(read r, read t, direction d = Forward, matching m = Different); // Has defaults
string parse(read r, read t1, read t2, matching m = Different); // Has defaults
string before(read r, read t, direction d = Forward, matching m = Different); // Has defaults
string after(read r, read t, direction d = Forward, matching m = Different); // Has defaults
void split(read r, read t, string *b, string *a, direction d = Forward, matching m = Different); // Has defaults
string replace(read r, read t1, read t2, matching m = Different); // Has defaults
string clip(read r, int startindex, int characters = -1); // Has defaults
string on(read r, read t, direction d = Forward, matching m = Different); // Has defaults
string off(read r, read t, direction d = Forward, matching m = Different); // Has defaults
string trim(read r, read t1 = L"", read t2 = L"", read t3 = L""); // Has defaults
string saynumber(int number, read name);
string insertcommas(read r);
string saytime(DWORD time);

// Window
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR command, int show);
void WindowPulse();
LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
void MenuTaskbar();

// Utility
void Report(read r);
void Error(read r);
void Log(read r);
//void Exception(std::exception &e);
void CloseHandleSafely(HANDLE *handle);
void BeginThread(LPVOID function);
void TextDialogSet(HWND dialog, int control, read r);
string TextDialog(HWND dialog, int control);
void TextWindowSet(HWND window, read r);
string TextWindow(HWND window);
void EditAppend(HWND window, read r);
void EditScroll(HWND window);
HWND WindowCreateEdit(bool scrollbars, bool capacity);
HWND WindowCreateButton(read r);
HWND WindowCreate(read name, read title, DWORD style, int size, HWND parent, HMENU menu);
void WindowSize(HWND window, int x, int y);
void WindowMove(HWND window, sizeitem size, bool paint = false); // Has defaults
void WindowEdit(HWND window, boolean edit);
string DialogBrowse(read display);
COLORREF ColorMix(COLORREF color1, int amount1, COLORREF color2, int amount2);
brushitem CreateBrush(COLORREF color);
void PaintMessage(HWND window = NULL); // Has defaults
void TaskbarIconAdd();
void TaskbarIconUpdate();
void TaskbarIconRemove();
HCURSOR LoadSharedCursor(read name);
HICON LoadIconResource(read name, int w, int h);
void CursorSet(HCURSOR cursor);
HMENU MenuLoad(read name);
HMENU MenuClip(HMENU menus, int index);
void MenuSet(HMENU menu, UINT command, UINT state, HBITMAP bitmap);
UINT MenuShow(HMENU menu, bool taskbar, sizeitem *size);
void TipAdd(sizeitem size, read r);
void MouseCapture(HWND window = NULL); // Has defaults
void MouseRelease(HWND window = NULL); // Has defaults
bool MouseInside();
areaitem *MouseOver();
sizeitem MouseArea(areaitem *a);
sizeitem MouseClient(HWND window = NULL); // Has defaults
sizeitem MouseScreen();
brushitem BrushSystem(int color);
brushitem BrushColor(COLORREF color);
COLORREF MixColors(COLORREF color1, int amount1, COLORREF color2, int amount2);
sizeitem SizeClient(HWND window = NULL); // Has defaults
sizeitem SizeWindow(HWND window = NULL); // Has defaults
sizeitem SizeText(deviceitem *device, read r);
void PaintLabel(deviceitem *device, read r, sizeitem size);
void PaintText(deviceitem *device, read r, sizeitem size, bool horizontal = false, bool vertical = false, bool left = false, bool right = false, int adjust = 0, HFONT font = NULL, brushitem *color = NULL, brushitem *background = NULL); // Has defaults
void PaintFill(deviceitem *device, sizeitem size, HBRUSH brush = NULL); // Has defaults
void PaintBorder(deviceitem *device, sizeitem size, HBRUSH brush1, HBRUSH brush2);
void PaintIcon(deviceitem *device, sizeitem position, HICON icon, HBRUSH background = NULL); // Has defaults
HFONT FontMenu(boolean underline);
HFONT FontName(read face, int points);
int Greatest(int i1 = 0, int i2 = 0, int i3 = 0, int i4 = 0, int i5 = 0, int i6 = 0, int i7 = 0, int i8 = 0); // Has defaults
void InitializeCommonControls();
void KillTimerSafely(UINT_PTR timer, HWND window = NULL);
void TimerSet(UINT_PTR timer, UINT time = 0, HWND window = NULL);
void FileRun(read path, read parameters = L""); // Has defaults
int Dialog(LPCTSTR resource, DLGPROC procedure = NULL, LPARAM lparam = 0); // Has defaults
BOOL CALLBACK DialogProcedure(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam);
void AddTab(HWND window, int index, read title);
void SetIcon(HWND window, HICON icon16, HICON icon32);
int Icon(read ext, string *type);
int IconGet(read ext, string *type);
int IconAddResource(read resource);
int IconAdd(HICON icon, int systemindex);
void ColumnIcon(HWND window, int column, int icon);
void ColumnSelect(HWND window, int column);
void ListColumnInsert(HWND window, int column, int format, int image, read r, int width);
void ListColumnDelete(HWND window, int column);
LPARAM ListGet(HWND window, int row);
LPARAM ListMark(HWND window);
LPARAM ListMouse(HWND window);
sizeitem ListCell(HWND window, int row, int column);
int ListRows(HWND window);
int ListSelectedRows(HWND window);
bool ListSelected(HWND window, int row);
void ListSelectAll(HWND window);
void ListScroll(HWND window);
void ListRemove(HWND window, int row);
void ListRemoveAll(HWND window);
void ListAddStart(HWND window, int rows);
void ListAddDone(HWND window, int rows);
void ListAdd(HWND window, int columns, LPARAM p, int icon1, read r1, int icon2, read r2, read r3, read r4, read r5, read r6);
void ListEdit(HWND window, int columns, LPARAM p, int icon1, read r1, int icon2, read r2, read r3, read r4, read r5, read r6);
int ListFind(HWND window, LPARAM p);
string ListText(HWND window, int row, int column);
bool ShellInfo(read ext, int *systemindex, string *type);
bool ShellIcon(read ext, HICON *icon);
void DestroyIconSafely(HICON icon);
string TextGuid();

// Paint
void PaintWindow(deviceitem *device);
void PaintArea(deviceitem *device, areaitem *a);

// Area
void AreaCreate();
void AreaPulse();
void AreaPopUp();
void AreaPopDown();
void SizeColumns(int *width1, int *width2, int *width3, int *width4, int *width5, int *width6);
void Size(int move = 0); // Has defaults

// Command
bool AreaCommand(areaitem *area);

// Options
void DialogOptions();
BOOL APIENTRY DialogOptionsDownload(HWND dialog, UINT message, UINT wparam, LPARAM lparam);
BOOL APIENTRY DialogOptionsConnections(HWND dialog, UINT message, UINT wparam, LPARAM lparam);
BOOL APIENTRY DialogOptionsPasswords(HWND sheet, UINT message, UINT wparam, LPARAM lparam);
BOOL CALLBACK DialogAbout(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam);

// Start
void StartIcon();

// Test
void Test();

void mytest();






