#include "modules.h"

#include <cstdio>
#include <cstring>

namespace modules {

Region Find(const char* soName) {
    Region out;

    FILE* f = std::fopen("/proc/self/maps", "r");
    if (!f)
        return out;

    char line[512];
    std::string fullPath;
    uintptr_t moduleBase = 0;
    bool found = false;

    while (std::fgets(line, sizeof(line), f)) {
        unsigned long long start = 0, end = 0, offset = 0;
        char perms[8] = {};
        char dev[16] = {};
        unsigned long inode = 0;
        char path[400] = {};

        const int n = std::sscanf(line, "%llx-%llx %7s %llx %15s %lu %399s",
                                   &start, &end, perms, &offset, dev, &inode, path);
        if (n < 7 || path[0] != '/')
            continue;

        const char* base = std::strrchr(path, '/');
        base = base ? base + 1 : path;
        if (std::strcmp(base, soName) != 0)
            continue;

        found = true;
        fullPath = path;
        if (moduleBase == 0 || start < moduleBase)
            moduleBase = start;

        // First executable segment is the one a pattern scan wants - same
        // "first .text-like section" rule as the Windows sigscan.
        if (!out.start && std::strchr(perms, 'x')) {
            out.start = start;
            out.size = static_cast<size_t>(end - start);
        }
    }
    std::fclose(f);

    if (found) {
        out.base = moduleBase;
        out.path = fullPath;
    }
    return out;
}

} // namespace modules
