#include "sdk/trace.h"

namespace trace {

bool Available() {
    // Flip this the day a real backend lands, and nothing above has to change.
    return false;
}

Result LineOfSight(const Vector3& from, const Vector3& to, uintptr_t skipPawn) {
    (void)from;
    (void)to;
    (void)skipPawn;

    // Deliberately not a guess. Returning Visible here would make every
    // occluded point look shootable and would quietly poison the ragebot's
    // scoring with data nobody measured.
    return Result::Unknown;
}

} // namespace trace
