#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

// Resolves loaded shared objects of the host process (our own, since this
// runs injected via LD_PRELOAD) by reading /proc/self/maps - the Linux
// equivalent of GetModuleHandle + walking PE sections.
namespace modules {

struct Region {
    uintptr_t   base = 0;    // lowest mapped address for the module
    uintptr_t   start = 0;   // start of its executable (r-xp) mapping
    size_t      size = 0;    // size of that executable mapping
    std::string path;        // full path as seen in /proc/self/maps
};

// `soName` is matched against the basename of each mapped file, e.g.
// "libclient.so". Returns an empty Region (base == 0) when not mapped yet -
// not an error, since CS2 loads its modules lazily at startup.
Region Find(const char* soName);

} // namespace modules
