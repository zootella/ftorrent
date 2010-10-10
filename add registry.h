
// Wraps a registry key, taking care of closing it
class CRegistry {
public:

	// The handle to the registry key
	HKEY Key;

	// Open a registry key and store its handle in this object
	bool Open(HKEY root, read path, bool write);
	void Close() { if (Key) RegCloseKey(Key); Key = NULL; }

	// Make a new local CRegistry object, and delete it when it goes out of scope
	CRegistry() { Key = NULL; }
	~CRegistry() { Close(); }
};

// Functions in Registry.cpp
int RegistryReadNumber(JNIEnv *e, HKEY root, read path, read name);
CString RegistryReadText(JNIEnv *e, HKEY root, read path, read name);
bool RegistryWriteNumber(HKEY root, read path, read name, int value);
bool RegistryWriteText(HKEY root, read path, read name, read value);
bool RegistryDelete(HKEY base, read path);
