#pragma once
#include <cstdint>

#include "sdk/math.h"

// Line-of-sight queries.
//
// HONEST STATE: this project has no ray trace. A present-hook overlay reads
// memory; it never calls into the engine, and CS2's trace entry points
// (CGameTraceManager / the physics query interface) are not resolved anywhere
// in this codebase. So there is nothing here that can tell you whether a wall
// stands between two points.
//
// Rather than return a made-up boolean that would silently corrupt the
// ragebot's point scoring, every query answers Unknown and Available() is
// false. Callers must branch on that: the ragebot marks such points
// "visibility unknown" and does not award them a visibility bonus.
//
// Wiring this up for real needs a resolved trace interface and a filter that
// skips the local pawn; the shape below is what that implementation would fill
// in, so nothing above this layer has to change when it arrives.
namespace trace {

enum class Result {
    Unknown = 0,   // no trace backend - the only value returned today
    Visible,
    Blocked
};

// False until a real backend exists. Check it before trusting a Result.
bool Available();

// Would a bullet from `from` reach `to` without hitting world geometry?
// `skipPawn` is the shooter, which a real implementation must exclude.
Result LineOfSight(const Vector3& from, const Vector3& to, uintptr_t skipPawn);

} // namespace trace
