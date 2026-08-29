/* Stub that only exists so the payload's GetModuleHandleW(L"client.dll")
   succeeds inside the test host. It exports nothing and does nothing. */
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE m, DWORD reason, LPVOID reserved) {
    (void)m; (void)reason; (void)reserved;
    return TRUE;
}
