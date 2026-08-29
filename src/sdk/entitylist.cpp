#include <Windows.h>

#include "core/log.h"
#include "sdk/entity.h"
#include "sdk/entitylist.h"

namespace {

// Search space. The defaults sit first so a still-correct layout is confirmed
// on the first probe instead of being "rediscovered" as something exotic.
constexpr ptrdiff_t kChunkArrayCandidates[] = { 0x10, 0x08, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40 };
constexpr ptrdiff_t kStrideCandidates[]     = { 0x78, 0x80, 0x88, 0x70, 0x68, 0x90, 0x60, 0x98 };
constexpr ptrdiff_t kInstanceCandidates[]   = { 0x00, 0x08 };

constexpr int kPlayerSlots = 64;
constexpr int kAllSlots    = 2048;

uintptr_t Resolve(uintptr_t entityList, int index, const entlist::Layout& l) {
    const uintptr_t chunk =
        mem::Read<uintptr_t>(entityList + l.chunkArray + 0x8 * ((index & 0x7FFF) >> 9));
    if (!mem::Valid(chunk))
        return 0;
    return mem::Read<uintptr_t>(chunk + l.stride * (index & 0x1FF) + l.instance);
}

// Scans slots for `needle`; returns the slot it was found at, or -1.
int FindSlot(uintptr_t entityList, const entlist::Layout& l, uintptr_t needle, int maxSlot) {
    for (int i = 1; i <= maxSlot; ++i) {
        if (Resolve(entityList, i, l) == needle)
            return i;
    }
    return -1;
}

} // namespace

namespace entlist {

Layout layout;

uintptr_t Get(uintptr_t entityList, int index) {
    if (!mem::Valid(entityList) || index < 0)
        return 0;
    return Resolve(entityList, index, layout);
}

bool Calibrate(uintptr_t entityList, uintptr_t localController, uintptr_t localPawn) {
    if (layout.calibrated)
        return true;

    if (!mem::Valid(entityList) || !mem::Valid(localController)) {
        DBG_EVERY(5000, dbg::Level::Trace, "entlist",
                  "cannot calibrate yet - need a live local controller");
        return false;
    }

    DBG_I("entlist", "calibrating against local controller 0x%llX",
          static_cast<unsigned long long>(localController));

    for (ptrdiff_t chunkArray : kChunkArrayCandidates) {
        for (ptrdiff_t stride : kStrideCandidates) {
            for (ptrdiff_t instance : kInstanceCandidates) {
                Layout probe;
                probe.chunkArray = chunkArray;
                probe.stride = stride;
                probe.instance = instance;

                const int ctrlSlot = FindSlot(entityList, probe, localController, kPlayerSlots);
                if (ctrlSlot < 0)
                    continue;

                // A single hit could be luck. Requiring the pawn as well makes a
                // false positive essentially impossible.
                const int pawnSlot = mem::Valid(localPawn)
                                         ? FindSlot(entityList, probe, localPawn, kAllSlots)
                                         : -1;
                if (mem::Valid(localPawn) && pawnSlot < 0) {
                    DBG_T("entlist", "chunk=0x%llX stride=0x%llX inst=0x%llX matched the "
                                     "controller but not the pawn - rejected",
                          static_cast<unsigned long long>(chunkArray),
                          static_cast<unsigned long long>(stride),
                          static_cast<unsigned long long>(instance));
                    continue;
                }

                layout = probe;
                layout.calibrated = true;

                DBG_OK("entlist", "layout found: chunkArray=0x%llX stride=0x%llX instance=0x%llX",
                       static_cast<unsigned long long>(chunkArray),
                       static_cast<unsigned long long>(stride),
                       static_cast<unsigned long long>(instance));
                DBG_OK("entlist", "local controller at slot %d, local pawn at slot %d",
                       ctrlSlot, pawnSlot);
                return true;
            }
        }
    }

    DBG_EVERY(10000, dbg::Level::Error, "entlist",
              "no layout in the search space produced the local controller - "
              "the entity system itself moved");
    return false;
}

} // namespace entlist
