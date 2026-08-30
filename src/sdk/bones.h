#pragma once
#include <cstdint>

#include "sdk/math.h"

namespace bones {

// Bone indices for the CS2 player skeleton.
//
// These are the real ones, taken from the published CS2 bone table rather than
// guessed from a dump. Two earlier attempts got this wrong and both are worth
// remembering:
//
//   - The legs were 17..22. Those indices belong to entirely different bones,
//     which is why the leg "skeleton" drew as lines converging near the feet.
//     The legs are 22..27.
//   - Trying to identify roles geometrically at runtime was worse still: it
//     depends on the pose, failed on the local pawn, and silently fell back to
//     wrong defaults. Removed - the table is stable, so a table is correct.
//
// Note the spine numbering is genuinely irregular in this model: spine_1 is 4
// and spine_2 is 2. That is the game's own ordering, not a typo here.
enum Id : int {
    Pelvis     = 0,    // pelvis
    Spine0     = 1,
    Spine2     = 2,    // spine_2
    Spine3     = 3,
    Chest      = 4,    // spine_1, the "torso" aim point
    Neck       = 5,    // neck_0
    Head       = 6,    // head_0 - the JOINT, see kHeadCentre below

    ArmUpperL  = 8,
    ArmLowerL  = 9,
    HandL      = 10,
    ArmUpperR  = 13,
    ArmLowerR  = 14,
    HandR      = 15,

    LegUpperL  = 22,
    LegLowerL  = 23,
    AnkleL     = 24,
    LegUpperR  = 25,
    LegLowerR  = 26,
    AnkleR     = 27,

    Count      = 32,
};

// head_0 sits at the base of the skull, where the neck joins it - not in the
// middle of the head. Aiming straight at the bone therefore lands on the neck
// and upper chest, which is exactly what the head aim looked like. The aim
// point is lifted by this much to reach the centre of the head hitbox.
//
// Fixed on purpose: this and the head bone index are dialled in values, not
// preferences, so they are not in the config and cannot be changed from the
// menu. A saved preset can therefore never restore a wrong one.
inline constexpr float kHeadCentre = 4.1f;

// The bone array pointer is a raw member of CModelState, not a schema field,
// so its offset is probed at runtime the same way the entity list is.
struct Layout {
    ptrdiff_t boneArray  = 0x80;   // pointer to the bone data inside CModelState
    ptrdiff_t stride     = 0x20;   // position (Vector3) + quaternion (Vector4)
    bool      calibrated = false;
};

extern Layout layout;

// Probes candidate offsets until the bones sit plausibly around `originHint`
// (the pawn's own origin). Needs one live pawn; a no-op once calibrated.
bool Calibrate(uintptr_t pawn, const Vector3& originHint);

// World-space position of a bone, or a zero vector when unavailable.
Vector3 Position(uintptr_t pawn, int boneId);

// Position to actually aim at for a bone. Identical to Position() except for
// the head, which gets lifted to the middle of the hitbox.
Vector3 AimPoint(uintptr_t pawn, int boneId);

// Logs every bone index and position for one pawn, for verifying the table.
void DumpAll(uintptr_t pawn, int count = 64);

} // namespace bones
