#pragma once
#include <Windows.h>

// Namespace is `dbg`, not `log` — <cmath> already declares ::log at global
// scope and a namespace cannot share that name.
namespace dbg {

enum class Level { Trace = 0, Info, Ok, Warn, Error };

// Allocates the console and redirects stdout/stderr to it. No-op if already open.
void Open(const wchar_t* title);
void Close();

void SetVisible(bool visible);
bool IsVisible();
bool IsOpen();

void  SetMinLevel(Level level);
Level MinLevel();

// Lines emitted so far, per level — surfaced in the Misc tab.
int Count(Level level);

// Path of the plain-text mirror the console also writes to.
const char* FilePath();

void Print(Level level, const char* tag, const char* fmt, ...);

} // namespace dbg

#define DBG_T(tag, ...)  ::dbg::Print(::dbg::Level::Trace, tag, __VA_ARGS__)
#define DBG_I(tag, ...)  ::dbg::Print(::dbg::Level::Info,  tag, __VA_ARGS__)
#define DBG_OK(tag, ...) ::dbg::Print(::dbg::Level::Ok,    tag, __VA_ARGS__)
#define DBG_W(tag, ...)  ::dbg::Print(::dbg::Level::Warn,  tag, __VA_ARGS__)
#define DBG_E(tag, ...)  ::dbg::Print(::dbg::Level::Error, tag, __VA_ARGS__)

// Rate limit for anything inside the render loop: at most one line per `ms`
// from this call site, so a per-frame failure cannot flood the console.
#define DBG_EVERY(ms, lvl, tag, ...)                                       \
    do {                                                                   \
        static DWORD _dbgLast = 0;                                         \
        const DWORD _dbgNow = GetTickCount();                              \
        if (_dbgNow - _dbgLast >= static_cast<DWORD>(ms)) {                \
            _dbgLast = _dbgNow;                                            \
            ::dbg::Print(lvl, tag, __VA_ARGS__);                           \
        }                                                                  \
    } while (0)

// Fires exactly once per process.
#define DBG_ONCE(lvl, tag, ...)                                            \
    do {                                                                   \
        static bool _dbgDone = false;                                      \
        if (!_dbgDone) {                                                   \
            _dbgDone = true;                                               \
            ::dbg::Print(lvl, tag, __VA_ARGS__);                           \
        }                                                                  \
    } while (0)
