#include <msctf.h>
#include <windows.h>

// Helper functions for COM and TSF Registration
BOOL RegisterServer();
void UnregisterServer();
BOOL RegisterProfiles();
void UnregisterProfiles();
BOOL RegisterCategories();
void UnregisterCategories();

// Define CLSID and Profile GUIDs
extern const CLSID c_clsidMcBopomofoTIP;
extern const GUID c_guidMcBopomofoProfile;
