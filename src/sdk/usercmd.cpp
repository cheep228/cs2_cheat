#include "core/log.h"
#include "sdk/entity.h"
#include "sdk/usercmd.h"

namespace usercmd {
namespace cmdmgr {

Layout layout;

namespace {

// One candidate under observation.
struct Probe {
    int  last    = -1;
    int  advances = 0;   // times it moved forward by a sane step
    bool dead    = false;
};

Probe g_probe[2];
int   g_samples = 0;

// A real sequence counter is non-negative, nowhere near INT_MAX, and steps
// forward a tick at a time. A float reinterpreted as an int fails all three.
bool ValueLooksLikeSequence(int v) {
    return v >= 0 && v < 1'000'000;
}

} // namespace

bool Calibrate(uintptr_t manager) {
    if (layout.calibrated)
        return true;
    if (!mem::Valid(manager))
        return false;

    ++g_samples;

    for (int i = 0; i < 2; ++i) {
        Probe& p = g_probe[i];
        if (p.dead)
            continue;

        const int v = mem::Read<int>(manager + kSeqCandidates[i]);

        if (!ValueLooksLikeSequence(v)) {
            p.dead = true;
            DBG_I("cmd", "sequence candidate +0x%llX ruled out (value %d)",
                  static_cast<unsigned long long>(kSeqCandidates[i]), v);
            continue;
        }

        // Steps larger than a handful of ticks mean we are watching something
        // that merely happens to be a small integer.
        if (p.last >= 0) {
            const int step = v - p.last;
            if (step < 0 || step > 8) {
                p.dead = true;
                DBG_I("cmd", "sequence candidate +0x%llX ruled out (step %d)",
                      static_cast<unsigned long long>(kSeqCandidates[i]), step);
                continue;
            }
            if (step > 0)
                ++p.advances;
        }
        p.last = v;
    }

    // Settle only on a candidate that actually moved: a stale value that never
    // changes would otherwise win by simply never being disqualified.
    for (int i = 0; i < 2; ++i) {
        if (g_probe[i].dead || g_probe[i].advances < 4)
            continue;

        const bool otherStillAlive = !g_probe[1 - i].dead && g_probe[1 - i].advances >= 4;
        if (otherStillAlive && g_samples < 120)
            continue;   // both plausible so far; keep watching

        layout.sequence = kSeqCandidates[i];
        layout.calibrated = true;
        DBG_OK("cmd", "command ring calibrated: sequence at +0x%llX (%d advances)",
               static_cast<unsigned long long>(layout.sequence), g_probe[i].advances);
        return true;
    }

    if (g_samples == 300)
        DBG_E("cmd", "command ring NOT calibrated after 300 ticks - "
                     "the manager layout moved; aim writes stay off");

    return false;
}

UserCmd* Current(uintptr_t manager) {
    if (!layout.calibrated || !mem::Valid(manager))
        return nullptr;

    const int sequence = mem::Read<int>(manager + layout.sequence);
    if (!ValueLooksLikeSequence(sequence))
        return nullptr;

    const uintptr_t cmd =
        manager + kCommands + static_cast<uintptr_t>(sequence % kRingSize) * sizeof(UserCmd);
    if (!mem::Valid(cmd))
        return nullptr;

    return reinterpret_cast<UserCmd*>(cmd);
}

} // namespace cmdmgr
} // namespace usercmd
