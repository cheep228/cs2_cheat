#include <Windows.h>
#include <cctype>
#include <cstring>

#include "sdk/entity.h"
#include "sdk/sigscan.h"

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

// Accepts "48 89 0D ? ? ? ? E9" and the compact "4889 0D??" form alike.
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
            if (*p == '?') ++p;         // "??" is the same as "?"
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

Region Executable(const char* moduleName) {
    Region r;

    const HMODULE mod = GetModuleHandleA(moduleName);
    if (!mod)
        return r;

    const auto base = reinterpret_cast<uintptr_t>(mod);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return r;

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return r;

    const auto* section = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
        if (!(section->Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;
        r.base = base;
        r.start = base + section->VirtualAddress;
        r.size = section->Misc.VirtualSize;
        return r;   // first executable section is .text
    }

    return r;
}

uintptr_t Find(const Region& region, const char* pattern) {
    if (!region.start || region.size == 0)
        return 0;

    Pattern pat;
    if (!Parse(pattern, pat))
        return 0;

    const auto* data = reinterpret_cast<const uint8_t*>(region.start);
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
