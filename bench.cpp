
#include "include.h" // Include headers and definitions
extern app App; // Access global object

// Run a snippet of test code
void Test() {

}





Target::Target(HWND window) {

	references = 1;
}

Target::~Target() {

}

HRESULT __stdcall Target::QueryInterface(REFIID id, void **object) {

	if (id == IID_IDropTarget || id == IID_IUnknown) {

		AddRef();
		*object = this;
		return S_OK;

	} else {

		*object = 0;
		return E_NOINTERFACE;
	}
}

ULONG __stdcall Target::AddRef(void) {

	return InterlockedIncrement(&references);
}

ULONG __stdcall Target::Release(void) {

	LONG count = InterlockedDecrement(&references);
	if (count == 0) {

		delete this;
		return 0;

	} else {

		return count;
	}
}

HRESULT __stdcall Target::DragEnter(IDataObject *data, DWORD key, POINTL point, DWORD *effect) {

	*effect = DROPEFFECT_COPY; //TODO say what the result of a drop would be
	return S_OK;
}

HRESULT __stdcall Target::DragOver(DWORD key, POINTL point, DWORD *effect) {

	*effect = DROPEFFECT_COPY; //TODO say what the result of a drop would be
	return S_OK;
}

HRESULT __stdcall Target::DragLeave(void) {

	return S_OK;
}

HRESULT __stdcall Target::Drop(IDataObject *data, DWORD key, POINTL point, DWORD *effect) {

	log(L"drop");

	FORMATETC format;
	STGMEDIUM storage;
	ZeroMemory(&format, sizeof(format));
	ZeroMemory(&storage, sizeof(storage));

	storage.tymed = TYMED_HGLOBAL;

	IEnumFORMATETC *formats;

	if (data->EnumFormatEtc(DATADIR_GET, &formats) != S_OK) return FALSE;

	ULONG n = 0;
	while (formats->Next(DATADIR_GET, &format, &n) == S_OK) {


		WCHAR name[MAX_PATH];
		lstrcpy(name, L"");


		// Get textual name of format
		if (GetClipboardFormatName(format.cfFormat, name, MAX_PATH)) {

			CString s1 = name;

			if (s1 == L"FileNameW" || s1 == L"UniformResourceLocatorW") {


				if (data->GetData(&format, &storage) == S_OK) {

					HGLOBAL h = storage.hGlobal;

					WCHAR *w = (WCHAR *)GlobalLock(h);

					CString s2 = w;

					GlobalUnlock(h);

					log(L"got it: ", s1, L", ", s2);

				



				}


			}
		}

	}

	formats->Release();








	*effect = DROPEFFECT_COPY; //TODO say what the result of a drop would be
	return S_OK;
}


