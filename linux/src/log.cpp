#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace dbg {
namespace {

Level g_minLevel = Level::Info;

const char* Tag(Level l) {
    switch (l) {
    case Level::Trace: return "TRC";
    case Level::Info:  return "INF";
    case Level::Ok:    return "OK ";
    case Level::Warn:  return "WRN";
    case Level::Error: return "ERR";
    }
    return "?";
}

} // namespace

void SetMinLevel(Level level) { g_minLevel = level; }

void Print(Level level, const char* tag, const char* fmt, ...) {
    if (static_cast<int>(level) < static_cast<int>(g_minLevel))
        return;

    std::timespec ts{};
    std::timespec_get(&ts, TIME_UTC);
    std::tm tmv{};
    gmtime_r(&ts.tv_sec, &tmv);

    char line[1024];
    int n = std::snprintf(line, sizeof(line), "[%02d:%02d:%02d] [%s] [%s] ",
                           tmv.tm_hour, tmv.tm_min, tmv.tm_sec, Tag(level), tag);
    if (n < 0 || static_cast<size_t>(n) >= sizeof(line))
        return;

    va_list args;
    va_start(args, fmt);
    std::vsnprintf(line + n, sizeof(line) - n, fmt, args);
    va_end(args);

    std::fprintf(stderr, "[skinchanger] %s\n", line);
    std::fflush(stderr);
}

} // namespace dbg
