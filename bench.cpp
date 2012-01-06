
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

	*effect = DROPEFFECT_COPY;
	return S_OK;
}

HRESULT __stdcall Target::DragOver(DWORD key, POINTL point, DWORD *effect) {

	*effect = DROPEFFECT_COPY;
	return S_OK;
}

HRESULT __stdcall Target::DragLeave(void) {

	return S_OK;
}

HRESULT __stdcall Target::Drop(IDataObject *data, DWORD key, POINTL point, DWORD *effect) {

	/*
	EnumData(window, data);
	*/
	*effect = DROPEFFECT_NONE;
	return S_OK;
}


