// Minimal, dependency-free x64 LoadLibrary injector with real diagnostics.
//
// Extreme Injector's stealth methods (manual map, erase/randomize PE header)
// break on modern Windows x64 and report "the injection method used returned
// NULL". This does the plain, reliable thing instead: open the target, write
// the DLL path into it, run LoadLibraryW there, then verify by enumerating the
// target's modules rather than trusting the thread exit code.
//
// This is NOT an anti-cheat bypass. On a VAC-secured CS2 session VAC will block
// or detect the load regardless of the injector used; run the game with the
// -insecure launch option (or offline with bots) so VAC is not active. Injecting
// into VAC-secured CS2 violates the Valve Subscriber Agreement and leads to a
// permanent ban - there is no injector setting that changes that.
//
// Usage:
//   inject.exe                       -> cs2.exe + cs2_overlay.dll next to this exe
//   inject.exe cs2.exe payload.dll   -> explicit process name and DLL path
//   inject.exe --selftest [dll]      -> load the DLL locally to prove its
//                                       dependencies resolve; no target needed

#include <Windows.h>
#include <TlHelp32.h>

#include <cstdio>
#include <string>

namespace {

void Fail(const char* what) {
    const DWORD err = GetLastError();
    printf("[!] %s (GetLastError = %lu)\n", what, err);

    // The three that actually happen, spelled out instead of left as a number.
    switch (err) {
    case ERROR_ACCESS_DENIED:
        printf("    -> access denied: run this exe as administrator.\n");
        break;
    case ERROR_INVALID_PARAMETER:
        printf("    -> usually an architecture mismatch (x86 tool vs x64 game).\n");
        break;
    case ERROR_PARTIAL_COPY:
        printf("    -> the target refused the write; VAC or another tool is guarding it.\n");
        break;
    default:
        break;
    }
}

// Lets OpenProcess reach a process the current token could not otherwise touch.
bool EnableDebugPrivilege() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return false;

    LUID luid{};
    bool ok = LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &luid) != 0;
    if (ok) {
        TOKEN_PRIVILEGES tp{};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr);
        ok = GetLastError() == ERROR_SUCCESS;
    }
    CloseHandle(token);
    return ok;
}

bool IsElevated() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    TOKEN_ELEVATION elev{};
    DWORD size = sizeof(elev);
    const bool ok = GetTokenInformation(token, TokenElevation, &elev, size, &size) &&
                    elev.TokenIsElevated;
    CloseHandle(token);
    return ok;
}

DWORD FindProcess(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    DWORD pid = 0;

    if (Process32FirstW(snap, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, name) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);
    return pid;
}

// The authoritative success check. The remote thread's exit code is only the
// low 32 bits of the returned HMODULE, and on x64 those can legitimately be
// zero for a module that loaded fine - so ask the target what it has mapped.
bool ModuleLoadedIn(DWORD pid, const wchar_t* moduleName) {
    HANDLE snap = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 8; ++attempt) {
        snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap != INVALID_HANDLE_VALUE)
            break;
        if (GetLastError() != ERROR_BAD_LENGTH)   // documented "retry" case
            return false;
        Sleep(50);
    }
    if (snap == INVALID_HANDLE_VALUE)
        return false;

    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    bool found = false;

    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, moduleName) == 0) {
                wprintf(L"    module base 0x%llX  size %lu\n",
                        reinterpret_cast<unsigned long long>(me.modBaseAddr),
                        me.modBaseSize);
                found = true;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return found;
}

// An x64 tool can only inject into an x64 process. Catch the mismatch here
// instead of letting WriteProcessMemory fail with a useless code.
bool TargetIsSameArch(HANDLE proc) {
    USHORT procMachine = 0, nativeMachine = 0;
    if (!IsWow64Process2(proc, &procMachine, &nativeMachine))
        return true;                                  // pre-1709, assume fine
    if (procMachine == IMAGE_FILE_MACHINE_UNKNOWN)    // not WOW64 -> native x64
        return true;
    printf("[!] target is a 32-bit (WOW64) process; this injector and the "
           "payload are x64.\n");
    return false;
}

// Absolute path of the DLL, so the remote LoadLibraryW never depends on the
// target's working directory. Empty on failure.
std::wstring FullDllPath(const wchar_t* dll) {
    wchar_t buf[MAX_PATH]{};
    if (!GetFullPathNameW(dll, MAX_PATH, buf, nullptr))
        return {};
    if (GetFileAttributesW(buf) == INVALID_FILE_ATTRIBUTES)
        return {};
    return buf;
}

const wchar_t* FileName(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? path.c_str() : path.c_str() + slash + 1;
}

// Loads the DLL in this process to prove the file is intact and every import
// resolves. DONT_RESOLVE_DLL_REFERENCES is deliberately not used: the point is
// to exercise the dependency chain, not to skip it. DllMain runs, so the
// payload's own thread starts and then fails to find client.dll - expected.
int SelfTest(const wchar_t* dllArg) {
    const std::wstring dllPath = FullDllPath(dllArg);
    if (dllPath.empty()) {
        wprintf(L"[!] DLL not found: %ls\n", dllArg);
        wprintf(L"    if the file was there a moment ago, Defender quarantined it.\n");
        return 1;
    }
    wprintf(L"[*] self-test loading %ls\n", dllPath.c_str());

    HMODULE m = LoadLibraryW(dllPath.c_str());
    if (!m) {
        Fail("LoadLibraryW failed locally - the payload itself is the problem");
        printf("    193 = not a valid x64 image, 126 = a dependency is missing,\n"
               "    5 = blocked by Defender / an ACL on the file.\n");
        return 1;
    }
    wprintf(L"[+] loads cleanly, base 0x%llX - imports and image are fine.\n",
            reinterpret_cast<unsigned long long>(m));
    wprintf(L"    so any injection failure is the injector or the target, "
            L"not the DLL.\n");
    return 0;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc > 1 && _wcsicmp(argv[1], L"--selftest") == 0)
        return SelfTest(argc > 2 ? argv[2] : L"cs2_overlay.dll");

    const wchar_t* procName = argc > 1 ? argv[1] : L"cs2.exe";
    const wchar_t* dllArg   = argc > 2 ? argv[2] : L"cs2_overlay.dll";

    wprintf(L"[*] target : %ls\n", procName);
    printf("[*] elevated: %s\n", IsElevated() ? "yes" : "NO - run as administrator");

    const std::wstring dllPath = FullDllPath(dllArg);
    if (dllPath.empty()) {
        wprintf(L"[!] DLL not found: %ls\n", dllArg);
        wprintf(L"    if it was there a moment ago, Defender quarantined it.\n");
        return 1;
    }
    wprintf(L"[*] dll    : %ls\n", dllPath.c_str());

    EnableDebugPrivilege();

    const DWORD pid = FindProcess(procName);
    if (!pid) {
        wprintf(L"[!] %ls is not running\n", procName);
        return 1;
    }
    wprintf(L"[*] pid    : %lu\n", pid);

    if (ModuleLoadedIn(pid, FileName(dllPath))) {
        wprintf(L"[!] %ls is already loaded in the target - press END in game to "
                L"unload it first.\n", FileName(dllPath));
        return 1;
    }

    HANDLE proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                  PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                                  PROCESS_VM_READ,
                              FALSE, pid);
    if (!proc) {
        Fail("OpenProcess failed");
        printf("    on CS2 this is the handle being denied or stripped: run as "
               "admin, and with VAC off (-insecure).\n");
        return 1;
    }

    if (!TargetIsSameArch(proc)) {
        CloseHandle(proc);
        return 1;
    }

    const SIZE_T bytes = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE,
                                   PAGE_READWRITE);
    if (!remote) {
        Fail("VirtualAllocEx failed");
        CloseHandle(proc);
        return 1;
    }

    if (!WriteProcessMemory(proc, remote, dllPath.c_str(), bytes, nullptr)) {
        Fail("WriteProcessMemory failed");
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }

    // kernel32 is mapped at the same base in every process this session, so the
    // local address of LoadLibraryW is valid in the target.
    auto loadLib = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
    if (!loadLib) {
        Fail("GetProcAddress(LoadLibraryW) failed");
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }

    HANDLE thread = CreateRemoteThread(proc, nullptr, 0, loadLib, remote, 0, nullptr);
    if (!thread) {
        Fail("CreateRemoteThread failed");
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }

    if (WaitForSingleObject(thread, 15000) == WAIT_TIMEOUT)
        printf("[!] the remote LoadLibraryW did not return in 15 s - the loader "
               "lock is held; checking the module list anyway.\n");

    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);

    // Verify against the target's module list, not the exit code: the code is
    // only the low 32 bits of the HMODULE and can be zero on a successful load.
    const bool loaded = ModuleLoadedIn(pid, FileName(dllPath));

    if (loaded) {
        wprintf(L"[+] %ls is mapped in %ls - injection succeeded.\n",
                FileName(dllPath), procName);
        wprintf(L"    the payload's own console reports what happens next.\n");
    } else {
        wprintf(L"[!] remote LoadLibraryW returned NULL (exit code 0x%08lX) and the "
                L"module is not mapped.\n", exitCode);
        printf("    run 'inject.exe --selftest' first: if the DLL loads locally,\n"
               "    the target is refusing it - VAC is active, or Defender is\n"
               "    blocking the load. Test with -insecure / offline vs bots.\n");
    }

    CloseHandle(thread);
    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    CloseHandle(proc);
    return loaded ? 0 : 1;
}
