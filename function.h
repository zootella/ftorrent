
// String
CString make(read r1 = L"", read r2 = L"", read r3 = L"", read r4 = L"", read r5 = L"", read r6 = L"", read r7 = L"", read r8 = L"", read r9 = L""); // Has defaults
CString upper(read r);
CString lower(read r);
int number(read r);
CString numerals(int number, int width = 0); // Has defaults
bool is(read r);
bool isblank(read r);
bool same(read r1, read r2, matching m = Different); // Has defaults
int compare(read r1, read r2, matching m = Different); // Has defaults
bool starts(read r, read t, matching m = Different); // Has defaults
bool trails(read r, read t, matching m = Different); // Has defaults
bool has(read r, read t, matching m = Different); // Has defaults
int find(read r, read t, direction d = Forward, matching m = Different); // Has defaults
CString parse(read r, read t1, read t2, matching m = Different); // Has defaults
CString before(read r, read t, direction d = Forward, matching m = Different); // Has defaults
CString after(read r, read t, direction d = Forward, matching m = Different); // Has defaults
void split(read r, read t, CString *b, CString *a, direction d = Forward, matching m = Different); // Has defaults
CString replace(read r, read t1, read t2, matching m = Different); // Has defaults
CString clip(read r, int startindex, int characters = -1); // Has defaults
CString on(read r, read t, direction d = Forward, matching m = Different); // Has defaults
CString off(read r, read t, direction d = Forward, matching m = Different); // Has defaults
CString trim(read r, read t1 = L"", read t2 = L"", read t3 = L""); // Has defaults
CString saynumber(int number, read name);
CString insertcommas(read r);
CString saytime(DWORD time);
CString saynow();

// Window
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, PSTR command, int show);
void WindowPulse();
LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
void MenuTaskbar();

// Utility
void error(read r1 = L"", read r2 = L"", read r3 = L"", read r4 = L"", read r5 = L"", read r6 = L"", read r7 = L"", read r8 = L"", read r9 = L""); // Has defaults
void log(read r1 = L"", read r2 = L"", read r3 = L"", read r4 = L"", read r5 = L"", read r6 = L"", read r7 = L"", read r8 = L"", read r9 = L""); // Has defaults
void report(read r);
void CloseHandleSafely(HANDLE *handle);
void BeginThread(LPVOID function);
void TextDialogSet(HWND dialog, int control, read r);
CString TextDialog(HWND dialog, int control);
void TextWindowSet(HWND window, read r);
CString TextWindow(HWND window);
void EditAppend(HWND window, read r);
void EditScroll(HWND window);
HWND WindowCreateEdit(bool scrollbars, bool capacity);
HWND WindowCreateButton(read r);
HWND WindowCreate(read name, read title, DWORD style, int size, HWND parent, HMENU menu);
void WindowSize(HWND window, int x, int y);
void WindowMove(HWND window, sizeitem size, bool paint = false); // Has defaults
void WindowEdit(HWND window, boolean edit);
CString DialogBrowse(read display);
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
int Icon(read ext, CString *type);
int IconGet(read ext, CString *type);
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
CString ListText(HWND window, int row, int column);
bool ShellInfo(read ext, int *systemindex, CString *type);
bool ShellIcon(read ext, HICON *icon);
void DestroyIconSafely(HICON icon);
CString TextGuid();
CString PathStartPrograms();
CString PathQuickLaunch();
CString PathDesktop();
CString PathDocuments();
CString PathApplicationData();
CString PathId(int id);
CString PathRunningFolder();
CString PathRunning();
CString PathPortable();
CString PathStore();
CString PathOption();
CString PathTorrentMeta(libtorrent::sha1_hash hash);
CString PathTorrentStore(libtorrent::sha1_hash hash);
CString PathTorrentOption(libtorrent::sha1_hash hash);

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

// Library
std::string convertPtoS(const char *p);
std::wstring convertRtoW(const wchar_t *r);
CString convertRtoC(const wchar_t *r);
CString convertWtoC(std::wstring w);
CString widenPtoC(const char *p);
CString widenStoC(std::string s);
std::wstring widenPtoW(const char *p);
std::wstring widenStoW(std::string s);
std::string narrowRtoS(const wchar_t *r);
std::string narrowWtoS(std::wstring w);
libtorrent::big_number StringToHash(const char *s);
CString HashToString(const libtorrent::sha1_hash &hash);
CString PeerToString(const libtorrent::peer_id &id);
libtorrent::torrent_handle FindTorrentHandle(const char *id);
bool SaveEntry(read path, const libtorrent::entry &e);
bool LoadEntry(read path, libtorrent::entry &e);

// Wrapper

// Start
void InitializeLibtorrent(settings_structure *info);
void UpdateSettings(settings_structure *info);

// Close
void FreezeAndSaveAllFastResumeData();
void AbortTorrents();
void SignalFastResumeDataRequest(const char *id);
void SaveFastResumeData(alert_structure *info, wchar_t *path);
void SaveDhtState(const wchar_t *path);

// Update
void GetAlerts();
void ProcessAlert(const libtorrent::alert *alert, alert_structure *info);

// Torrent
void AddTorrent(char *infohash, char *trackerurl, wchar_t *torrentpath, wchar_t *savepath, wchar_t *resumepath);
void RemoveTorrent(const char *id);

// Change
void PauseTorrent(const char *id);
void ResumeTorrent(const char *id);
void MoveTorrent(const char *id, wchar_t *path);
void ForceReannounce(const char *id);
void ClearErrorAndRetry(const char *id);
void SetAutoManagedTorrent(const char *id, bool auto_managed);
void SetSeedRatio(const char *id, float seed_ratio);

// Look
void IsValid(const char *id, int &is_valid);
void HasMetadata(const char *id, int &has_metadata);
void GetTorrentInfo(const char *id, torrent_structure *info);
void GetTorrentStatus(const char *id, status_structure *info);

// Tracker
void AddTracker(const char *id, char *url, int tier);
void RemoveTracker(const char *id, char *url, int tier);
void GetNumTrackers(const char *id, int &n);
void GetTrackers(const char *id, announce_structure **torrent_trackers, int n);
void ScrapeTracker(const char *id);

// Tabs
void GetNumFiles(const char *id, int &num_files);
void GetFiles(const char *id, file_structure **file_entries);
void GetNumPeers(const char *id, int &num_peers);
void GetPeers(const char *id, std::vector<peer_structure> *v);
void GetPiecesStatus(const char *id, pieces_structure *info);
void SetFilePriority(const char *id, int index, int priority);
void SetFilePriorities(const char *id, int *priorities, int n);

// DHT
void AddDhtNode(const char *address, int port);
void AddDhtRouter(const char *address, int port);
void StartDht(const wchar_t *path);
void StopDht();

// Service
void StartLsd();
void StopLsd();
void StartUpnp();
void StopUpnp();
void StartNatpmp();
void StopNatpmp();





// Test
void Test();


void LibraryStart();
void LibraryClose();
void AddTorrent();
void LibraryPulse();
void AlertLoop();
void AlertLook(const libtorrent::alert *a);














