
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
void error(read r);
void log(read r);
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

std::wstring convertTtoW(wchar_t *t);
CString convertTtoC(wchar_t *t);
std::string narrowTtoS(wchar_t *t);
std::wstring widenPtoW(char *p);
CString widenPtoC(char *p);
std::string convertPtoS(char *p);
CString convertWtoC(std::wstring w);
std::string narrowWtoS(std::wstring w);
std::wstring widenStoW(std::string s);
CString widenStoC(std::string s);

CString HashToCString(const libtorrent::sha1_hash &hash);
CString PeerIdToCString(const libtorrent::peer_id &id);
const char *HashToString(const libtorrent::sha1_hash &hash);
const char *PeerIdToString(const libtorrent::peer_id &id);
const char *CopyStringFromStream(const std::stringstream &stream);
char *CopyString(const char *s);
wchar_t *CopyWideString(const wchar_t *s);
boost::filesystem::path WideToPath(wchar_t *w);
libtorrent::torrent_handle FindTorrentHandle(const char *id);
libtorrent::big_number StringToHash(const char *s);
void ProcessSaveResumeDataAlert(libtorrent::torrent_handle handle, libtorrent::save_resume_data_alert const *alert, alert_structure *alertInfo);
void ProcessAlert(libtorrent::alert const *alert, alert_structure *alertInfo);



// libtorrentwrapper.cpp

/*
*/

void SaveFastResumeData(alert_structure *alert, wchar_t *filePath);
void FreezeAndSaveAllFastResumeData(void(*alertCallback)(void*));
void UpdateSettings(settings_structure *settings);
void InitializeLibtorrent(settings_structure *setting);
void AbortTorrents();
void MoveTorrent(const char *id, wchar_t *path);
void AddTorrent(char *sha1String, char *trackerURI, wchar_t *torrentPath, wchar_t *savePath, wchar_t *fastResumePath);
void PauseTorrent(const char *id);
void SetAutoManagedTorrent(const char *id, bool auto_managed);
void RemoveTorrent(const char *id);
void ResumeTorrent(const char *id);
void ForceReannounce(const char *id);
void ScrapeTracker(const char *id);
void GetTorrentStatus(const char *id, status_structure *stats);
void GetTorrentInfo(const char *id, torrent_structure *info);
void SignalFastResumeDataRequest(const char *id);
void ClearErrorAndRetry(const char *id);
void GetNumPeers(const char *id, int &num_peers);
void HasMetadata(const char *id, int &has_metadata);
void IsValid(const char *id, int &is_valid);
void GetPeers(const char *id, std::vector<peer_structure> *v);
void GetAlerts(void(*alertCallback)(void*));
void SetSeedRatio(const char *id, float seed_ratio);
void GetNumFiles(const char *id, int &num_files);
void GetFiles(const char *id, file_structure **file_entries);
void SetFilePriorities(const char *id, int *priorities, int num_priorities);
void SetFilePriority(const char *id, int index, int priority);

void StartDht(const wchar_t *dht_state_file_path);
void AddDhtRouter(const char *address, int port);
void AddDhtNode(const char *address, int port);
void SaveDhtState(const wchar_t *dht_state_file_path);
void StopDht();

void StartUpnp();
void StopUpnp();
void StartLsd();
void StopLsd();
void StartNatpmp();
void StopNatpmp();

void GetPiecesStatus(const char *id, pieces_structure *info);
void AddTracker(const char *id, char *url, int tier);
void RemoveTracker(const char *id, char *url, int tier);
void GetNumTrackers(const char *id, int &num_trackers);
void GetTrackers(const char *id, announce_structure **torrent_trackers, int numTrackers);
void FreeTrackers(announce_structure **torrent_trackers, int numTrackers);






// Test
void Test();

















