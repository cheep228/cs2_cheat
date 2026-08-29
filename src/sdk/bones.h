#pragma once
#include <cstdint>

#include "sdk/math.h"

namespace bones {

// Bone indices for the CS2 player skeleton, read off a live F10 dump rather
// than assumed: the heights below are the measured dz from the pelvis, and they
// matched across two different pawns.
//
//   0 pelvis   1..4 spine (+38.6 .. +48.1)   5 neck (+54.2)   6 head (+59.5)
//   9/13 shoulders   10/14 elbows   11/15 hands
//   17/20 hips       18/21 knees    19/22 ankles
//
// Bone 27 is a weapon attachment that sits ~1000 units away; the link-length
// filter in skeleton.cpp is what keeps it out of the drawing.
enum Id : int {
    Pelvis     = 0,
    Spine0     = 1,
    Spine1     = 2,
    Spine2     = 3,
    Chest      = 4,   // the "torso" aim point
    Neck       = 5,
    Head       = 6,
    ClavicleL  = 8,
    ArmUpperL  = 9,
    ArmLowerL  = 10,
    HandL      = 11,
    ClavicleR  = 12,
    ArmUpperR  = 13,
    ArmLowerR  = 14,
    HandR      = 15,
    LegUpperL  = 17,
    LegLowerL  = 18,
    AnkleL     = 19,
    LegUpperR  = 20,
    LegLowerR  = 21,
    AnkleR     = 22,
    Count      = 23,
};

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

// Logs every bone index and position for one pawn, for verifying the table.
void DumpAll(uintptr_t pawn, int count = 64);

} // namespace bones
