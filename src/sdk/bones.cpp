#include <Windows.h>
#include <cmath>

#include "core/log.h"
#include "sdk/bones.h"
#include "sdk/entity.h"
#include "sdk/offsets.h"

namespace {

constexpr ptrdiff_t kBoneArrayCandidates[] = { 0x80, 0x78, 0x88, 0x90, 0x70, 0xA0, 0x68, 0x98 };

// A pawn is roughly 72 units tall, so every bone must sit within this of its
// origin. Anything further means we are reading the wrong array.
constexpr float kMaxBoneDistance = 200.f;

uintptr_t BoneArray(uintptr_t pawn, ptrdiff_t arrayOffset) {
    const uintptr_t node = mem::Read<uintptr_t>(pawn + schema::m_pGameSceneNode);
    if (!mem::Valid(node))
        return 0;

    const uintptr_t modelState = node + schema::m_modelState;
    return mem::Read<uintptr_t>(modelState + arrayOffset);
}

Vector3 ReadBone(uintptr_t array, int boneId, ptrdiff_t stride) {
    if (!mem::Valid(array) || boneId < 0)
        return {};
    return mem::Read<Vector3>(array + static_cast<ptrdiff_t>(boneId) * stride);
}

bool Plausible(const Vector3& v, const Vector3& origin) {
    if (v.IsZero())
        return false;
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z))
        return false;
    return (v - origin).Length() < kMaxBoneDistance;
}

} // namespace

namespace bones {

Layout layout;

bool Calibrate(uintptr_t pawn, const Vector3& originHint) {
    if (layout.calibrated)
        return true;
    if (!mem::Valid(pawn) || originHint.IsZero())
        return false;

    for (ptrdiff_t candidate : kBoneArrayCandidates) {
        const uintptr_t array = BoneArray(pawn, candidate);
        if (!mem::Valid(array))
            continue;

        const Vector3 pelvis = ReadBone(array, Pelvis, 0x20);
        const Vector3 head   = ReadBone(array, Head, 0x20);

        if (!Plausible(pelvis, originHint) || !Plausible(head, originHint))
            continue;

        // The head must actually be above the pelvis, by about the height of a
        // torso. This is what separates the real array from a lookalike.
        const float rise = head.z - pelvis.z;
        if (rise < 25.f || rise > 90.f) {
            DBG_T("bones", "offset 0x%llX: head-pelvis rise %.1f is wrong - rejected",
                  static_cast<unsigned long long>(candidate), rise);
            continue;
        }

        layout.boneArray = candidate;
        layout.stride = 0x20;
        layout.calibrated = true;

        DBG_OK("bones", "bone array at modelState+0x%llX, stride 0x%llX",
               static_cast<unsigned long long>(candidate),
               static_cast<unsigned long long>(layout.stride));
        DBG_OK("bones", "pelvis z=%.1f head z=%.1f (rise %.1f)", pelvis.z, head.z, rise);
        return true;
    }

    DBG_EVERY(10000, dbg::Level::Warn, "bones",
              "no bone array offset produced a sane skeleton - skeleton/aimbot disabled");
    return false;
}

Vector3 Position(uintptr_t pawn, int boneId) {
    if (!layout.calibrated)
        return {};
    const uintptr_t array = BoneArray(pawn, layout.boneArray);
    return ReadBone(array, boneId, layout.stride);
}

void DumpAll(uintptr_t pawn, int count) {
    if (!layout.calibrated) {
        DBG_W("bones", "not calibrated - nothing to dump");
        return;
    }

    const uintptr_t array = BoneArray(pawn, layout.boneArray);
    DBG_I("bones", "bone array 0x%llX", static_cast<unsigned long long>(array));

    // Everything is printed relative to bone 0, because that is what makes a
    // bone identifiable: the head is the highest, ankles the lowest, hands the
    // furthest out sideways.
    const Vector3 root = ReadBone(array, 0, layout.stride);
    DBG_I("bones", "idx |    dz  |  dist | world x, y, z");

    for (int i = 0; i < count; ++i) {
        const Vector3 p = ReadBone(array, i, layout.stride);
        if (p.IsZero())
            continue;
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
            continue;

        const Vector3 d = p - root;
        DBG_I("bones", "%3d | %+6.1f | %5.1f | %8.1f %8.1f %8.1f",
              i, d.z, d.Length(), p.x, p.y, p.z);
    }
}

} // namespace bones
