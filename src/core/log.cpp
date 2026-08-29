#include "core/log.h"

#include <cstdarg>
#include <cstdio>

namespace {

FILE*  g_stdout  = nullptr;
FILE*  g_stderr  = nullptr;
FILE*  g_file    = nullptr;   // plain-text mirror, no ANSI codes
char   g_filePath[MAX_PATH]{};
HWND   g_console = nullptr;
bool   g_open       = false;
bool   g_visible    = true;
bool   g_owned      = false;   // we called AllocConsole and must FreeConsole
bool   g_hasConsole = false;

dbg::Level g_minLevel = dbg::Level::Trace;
int        g_counts[5]{};

CRITICAL_SECTION g_lock;
bool             g_lockReady = false;

const char* TagOf(dbg::Level l) {
    switch (l) {
    case dbg::Level::Trace: return "trace";
    case dbg::Level::Info:  return "info ";
    case dbg::Level::Ok:    return "ok   ";
    case dbg::Level::Warn:  return "warn ";
    case dbg::Level::Error: return "ERROR";
    }
    return "?    ";
}

const char* ColorOf(dbg::Level l) {
    switch (l) {
    case dbg::Level::Trace: return "\x1b[90m";   // grey
    case dbg::Level::Info:  return "\x1b[37m";   // white
    case dbg::Level::Ok:    return "\x1b[92m";   // green
    case dbg::Level::Warn:  return "\x1b[93m";   // yellow
    case dbg::Level::Error: return "\x1b[91m";   // red
    }
    return "\x1b[0m";
}

void EnableAnsi() {
    const HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

} // namespace

namespace dbg {

void Open(const wchar_t* title) {
    if (g_open)
        return;

    // The file mirror is opened first: it survives even when no console can be
    // allocated, so a silent failure still leaves something to read afterwards.
    char temp[MAX_PATH]{};
    if (GetTempPathA(MAX_PATH, temp)) {
        snprintf(g_filePath, sizeof(g_filePath), "%scs2_overlay.log", temp);
        fopen_s(&g_file, g_filePath, "w");
    }

    // A process launched from Explorer has no console, so AllocConsole normally
    // wins. If the host already owns one (a test harness), attach to that.
    if (AllocConsole()) {
        g_owned = true;
        g_hasConsole = true;
    } else if (AttachConsole(ATTACH_PARENT_PROCESS) || GetConsoleWindow()) {
        g_hasConsole = true;
    }

    if (g_hasConsole) {
        freopen_s(&g_stdout, "CONOUT$", "w", stdout);
        freopen_s(&g_stderr, "CONOUT$", "w", stderr);
        setvbuf(stdout, nullptr, _IOLBF, 4096);

        SetConsoleOutputCP(CP_UTF8);
        SetConsoleTitleW(title ? title : L"debug");
        EnableAnsi();

        const HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD bufferSize{ 140, 3000 };   // generous scrollback
        SetConsoleScreenBufferSize(h, bufferSize);

        g_console = GetConsoleWindow();
        if (g_console) {
            // Closing a console with the X button terminates the whole process,
            // and that process is the game. Take the option away.
            if (HMENU sys = GetSystemMenu(g_console, FALSE))
                DeleteMenu(sys, SC_CLOSE, MF_BYCOMMAND);
        }
    }

    InitializeCriticalSection(&g_lock);
    g_lockReady = true;
    g_open = true;
    g_visible = g_hasConsole;

    if (g_hasConsole)
        printf("\x1b[96m"
               "-------------------------------------------------------------\n"
               " cs2_overlay debug console\n"
               " this window belongs to the game process - do not close it\n"
               " toggle it from Misc -> Debug console\n"
               "-------------------------------------------------------------"
               "\x1b[0m\n");

    if (g_file)
        fprintf(g_file, "cs2_overlay log - console=%d\n", g_hasConsole ? 1 : 0);

    Print(Level::Info, "log", "writing to %s", g_filePath[0] ? g_filePath : "(no file)");
}

void Close() {
    if (!g_open)
        return;

    g_open = false;
    if (g_lockReady) {
        DeleteCriticalSection(&g_lock);
        g_lockReady = false;
    }
    if (g_file) {
        fclose(g_file);
        g_file = nullptr;
    }
    if (g_owned)
        FreeConsole();
    g_console = nullptr;
    g_owned = false;
    g_hasConsole = false;
}

void SetVisible(bool visible) {
    if (!g_console)
        return;
    g_visible = visible;
    ShowWindow(g_console, visible ? SW_SHOW : SW_HIDE);
}

bool IsVisible() { return g_visible; }
bool IsOpen()    { return g_open; }

void  SetMinLevel(Level level) { g_minLevel = level; }
Level MinLevel()               { return g_minLevel; }

const char* FilePath() { return g_filePath; }

int Count(Level level) {
    const int i = static_cast<int>(level);
    return (i >= 0 && i < 5) ? g_counts[i] : 0;
}

void Print(Level level, const char* tag, const char* fmt, ...) {
    if (!g_open || level < g_minLevel)
        return;

    char body[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);

    SYSTEMTIME t{};
    GetLocalTime(&t);

    if (g_lockReady)
        EnterCriticalSection(&g_lock);

    ++g_counts[static_cast<int>(level)];

    if (g_hasConsole)
        printf("\x1b[90m%02d:%02d:%02d.%03d\x1b[0m %s%s\x1b[0m \x1b[36m%-7s\x1b[0m %s%s\x1b[0m\n",
               t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
               ColorOf(level), TagOf(level),
               tag ? tag : "-",
               level == Level::Error || level == Level::Warn ? ColorOf(level) : "",
               body);

    if (g_file) {
        fprintf(g_file, "%02d:%02d:%02d.%03d %s %-7s %s\n",
                t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
                TagOf(level), tag ? tag : "-", body);
        fflush(g_file);   // the process may be killed at any moment
    }

    if (g_lockReady)
        LeaveCriticalSection(&g_lock);
}

} // namespace dbg
