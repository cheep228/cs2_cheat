#pragma once

// Minimal stderr logger for the Linux build. There is no console window to
// allocate like the Windows project's dbg:: - whoever launches the game from
// a terminal (or checks its log file) sees these lines instead.
namespace dbg {

enum class Level { Trace = 0, Info, Ok, Warn, Error };

void SetMinLevel(Level level);
void Print(Level level, const char* tag, const char* fmt, ...);

} // namespace dbg

#define DBG_T(tag, ...)  ::dbg::Print(::dbg::Level::Trace, tag, __VA_ARGS__)
#define DBG_I(tag, ...)  ::dbg::Print(::dbg::Level::Info,  tag, __VA_ARGS__)
#define DBG_OK(tag, ...) ::dbg::Print(::dbg::Level::Ok,    tag, __VA_ARGS__)
#define DBG_W(tag, ...)  ::dbg::Print(::dbg::Level::Warn,  tag, __VA_ARGS__)
#define DBG_E(tag, ...)  ::dbg::Print(::dbg::Level::Error, tag, __VA_ARGS__)
