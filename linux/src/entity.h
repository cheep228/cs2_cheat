#pragma once
#include <cstddef>
#include <cstdint>

// Source 2's entity list is a two-level table: a fixed number of chunks, each
// holding a run of entries with the entity pointer at a fixed offset inside
// the entry. The exact {chunk array offset, entry stride, pointer offset}
// triple is not something either the Windows or the Linux build hardcodes
// here - Calibrate() brute-forces it against a small candidate space and
// verifies the result against a pointer we already trust (the local
// controller/pawn from offsets.h), the same approach the Windows project's
// sdk/entitylist.cpp uses. That is what makes this file portable: it has no
// platform- or build-specific numbers of its own.
namespace entlist {

struct Layout {
    ptrdiff_t chunkArray = 0;
    ptrdiff_t stride     = 0;
    ptrdiff_t instance   = 0;
    bool      calibrated = false;
};

extern Layout layout;

uintptr_t Get(uintptr_t entityList, int index);

inline uintptr_t GetByHandle(uintptr_t entityList, uint32_t handle) {
    if (handle == 0xFFFFFFFF || handle == 0)
        return 0;
    return Get(entityList, static_cast<int>(handle & 0x7FFF));
}

// Tries every {chunkArray, stride, instance} combination in the search space
// until one of them produces localController at some player slot (and, if
// given, localPawn at some entity slot too). No-op once already calibrated.
bool Calibrate(uintptr_t entityList, uintptr_t localController, uintptr_t localPawn);

} // namespace entlist
