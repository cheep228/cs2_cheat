#include "sigscan.h"

#include "mem.h"

namespace {

constexpr int kMaxPatternBytes = 64;

struct Pattern {
    uint8_t bytes[kMaxPatternBytes]{};
    bool    wild[kMaxPatternBytes]{};
    int     length = 0;
};

int HexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool Parse(const char* text, Pattern& out) {
    out.length = 0;

    for (const char* p = text; *p; ) {
        if (*p == ' ') { ++p; continue; }
        if (out.length >= kMaxPatternBytes)
            return false;

        if (*p == '?') {
            out.wild[out.length] = true;
            out.bytes[out.length] = 0;
            ++out.length;
            ++p;
            if (*p == '?') ++p;
            continue;
        }

        const int hi = HexVal(p[0]);
        const int lo = p[1] ? HexVal(p[1]) : -1;
        if (hi < 0 || lo < 0)
            return false;

        out.wild[out.length] = false;
        out.bytes[out.length] = static_cast<uint8_t>(hi * 16 + lo);
        ++out.length;
        p += 2;
    }

    return out.length > 0;
}

} // namespace

namespace sig {

uintptr_t Find(const Region& region, const char* pattern) {
    if (!region.start || region.size == 0)
        return 0;

    Pattern pat;
    if (!Parse(pattern, pat))
        return 0;

    const auto* data = reinterpret_cast<const uint8_t*>(region.start);
    if (static_cast<size_t>(pat.length) > region.size)
        return 0;
    const size_t last = region.size - static_cast<size_t>(pat.length);

    for (size_t i = 0; i <= last; ++i) {
        int k = 0;
        for (; k < pat.length; ++k) {
            if (!pat.wild[k] && data[i + k] != pat.bytes[k])
                break;
        }
        if (k == pat.length)
            return region.start + i;
    }

    return 0;
}

uintptr_t RipTarget(uintptr_t at, int relOffset, int instrLen) {
    if (!at)
        return 0;
    const int32_t rel = mem::Read<int32_t>(at + relOffset);
    return at + instrLen + rel;
}

uint32_t Imm32(uintptr_t at, int offset) {
    if (!at)
        return 0;
    return mem::Read<uint32_t>(at + offset);
}

} // namespace sig
